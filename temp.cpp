curveInfoList.clear();
wellInfoList.clear();

QFile lasFileCurve(lasPath);

if (lasFileCurve.open(QIODevice::ReadOnly | QIODevice::Text))
{
    QTextStream inCurve(&lasFileCurve);
    bool inCurveSection = false;
    bool inWellSection = false;
    while (!inCurve.atEnd())
    {
        QString line = inCurve.readLine();
        QString trimmed = line.trimmed();

        // Xác định section ~WELL
        if (trimmed.toUpper().startsWith("~WELL"))
        {
            inWellSection = true;
            inCurveSection = false;
            continue;
        }
        // Xác định section ~CURVE
        if (trimmed.toUpper().startsWith("~CURVE"))
        {
            inCurveSection = true;
            inWellSection = false;
            continue;
        }
        // Nếu gặp section mới khác ~WELL/~CURVE thì thoát section hiện tại
        if (trimmed.startsWith("~") && !trimmed.toUpper().startsWith("~WELL") && !trimmed.toUpper().startsWith("~CURVE"))
        {
            inWellSection = false;
            if (inCurveSection)
                break; // Kết thúc section ~CURVE
        }

        // Đọc thông tin WELL

        if (inWellSection)
        {
            if (trimmed.isEmpty() || trimmed.startsWith("#"))
                continue;
            // Định dạng: mnemonic.unit value : description
            // Ví dụ: STRT.MS         86283000.000 : I         a      /I          d
            QRegularExpression re(R"(^\s*(\S+)\s*\.\s*(\S*)\s+([\d\.-]+)?\s*:\s*(.*)$)");
            QRegularExpressionMatch match = re.match(trimmed);
            if (match.hasMatch())
            {
                CurveInfo info;
                info.mnemonic = match.captured(1);
                info.unit = match.captured(2);
                info.value = match.captured(3).trimmed();
                info.description = match.captured(4).trimmed();
                wellInfoList.append(info);
            }
            else
            {
                // Nếu không match format mới, thử format cũ (không có value)
                QRegularExpression re2(R"(^\s*(\S+)\s*\.\s*(\S*)\s+(.*)$)");
                QRegularExpressionMatch match2 = re2.match(trimmed);
                if (match2.hasMatch())
                {
                    CurveInfo info;
                    info.mnemonic = match2.captured(1);
                    info.unit = match2.captured(2);
                    info.value = "";
                    info.description = match2.captured(3).trimmed();
                    wellInfoList.append(info);
                }
            }
        }

        // Đọc thông tin CURVE
        if (inCurveSection)
        {
            if (trimmed.isEmpty() || trimmed.startsWith("#"))
                continue;
            // Định dạng: mnemonic.unit value : description
            // Ví dụ: STRT.MS         86283000.000 : I         a      /I          d
            QRegularExpression re(R"(^\s*(\S+)\s*\.\s*(\S*)\s+([\d\.-]+)?\s*:\s*(.*)$)");
            QRegularExpressionMatch match = re.match(trimmed);
            if (match.hasMatch())
            {
                CurveInfo info;
                info.mnemonic = match.captured(1);
                info.unit = match.captured(2);
                info.value = match.captured(3).trimmed();
                info.description = match.captured(4).trimmed();
                curveInfoList.append(info);
            }
            else
            {
                // Nếu không match format mới, thử format cũ (không có value)
                QRegularExpression re2(R"(^\s*(\S+)\s*\.\s*(\S*)\s+(.*)$)");
                QRegularExpressionMatch match2 = re2.match(trimmed);
                if (match2.hasMatch())
                {
                    CurveInfo info;
                    info.mnemonic = match2.captured(1);
                    info.unit = match2.captured(2);
                    info.value = "";
                    info.description = match2.captured(3).trimmed();
                    curveInfoList.append(info);
                }
            }
        }
    }
    lasFileCurve.close();
}

// Debug:
// In ra nội dung curveInfoList và wellInfoList
// qDebug()
//     << "curveInfoList:";
// for (const CurveInfo &info : curveInfoList)
// {
//     qDebug() << "  mnemonic:" << info.mnemonic << ", unit:" << info.unit << ", description:" << info.description;
// }
// qDebug() << "wellInfoList:";
// for (const CurveInfo &info : wellInfoList)
// {
//     qDebug() << "  mnemonic:" << info.mnemonic << ", unit:" << info.unit << ", description:" << info.description;
// }

// Bắt đầu merge dữ liệu TXT vào LAS
blockList.clear();
QSet<qint64> txtTimeSet;
QMap<qint64, double> timeToDepthMap;
qint64 minTime = LLONG_MAX, maxTime = LLONG_MIN;
for (const QStringList &row : dataRows)
{
    if (row.isEmpty() || row.size() < 2)
        continue;
    bool ok1 = false, ok2 = false;
    qint64 t = row[0].toLongLong(&ok1);
    double depthVal = row[1].toDouble(&ok2);
    if (ok1 && ok2)
    {
        txtTimeSet.insert(t);
        // Làm tròn xuống 1 số thập phân (floor về 0.1)
        double depthValFloor = std::floor(depthVal * 10.0) / 10.0;
        // Định dạng depth: 1 số thập phân + 2 số 0 phía sau (vd: 12.300)
        QString depthStr = QString::number(depthValFloor, 'f', 1) + "00";
        timeToDepthMap[t] = depthStr.toDouble();
        if (t < minTime)
            minTime = t;
        if (t > maxTime)
            maxTime = t;
    }
}

if (minTime == LLONG_MAX || maxTime == LLONG_MIN)
{
    QMessageBox::warning(this, "Lỗi", "Không tìm thấy TIME hợp lệ trong TXT!");
    return;
}

QFile lasFile(lasPath);
if (!lasFile.open(QIODevice::ReadOnly | QIODevice::Text))
{
    QMessageBox::warning(this, "Lỗi", "Không mở được file LAS!");
    return;
}
QTextStream in(&lasFile);
bool inAscii = false;
BlockData currentBlock;
bool hasCurrentBlock = false;
blockList.clear();
qint64 lastBlockTime = -1;
bool stopReading = false;
while (!in.atEnd() && !stopReading)
{
    QString line = in.readLine();
    QString trimmed = line.trimmed();
    if (trimmed.toUpper().startsWith("~ASCII"))
    {
        inAscii = true;
        continue;
    }
    if (!inAscii)
        continue;
    if (trimmed.isEmpty() || trimmed.startsWith("#"))
        continue;
    QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    bool isBlockHeader = (parts.size() == 1 && !line.isEmpty() && !line.at(0).isSpace());
    if (isBlockHeader)
    {
        // Nếu đang có block, kiểm tra block đó có TIME trong TXT không, nếu có thì giữ lại
        if (hasCurrentBlock)
        {
            if (lastBlockTime >= minTime && lastBlockTime <= maxTime && txtTimeSet.contains(lastBlockTime) && !currentBlock.data.isEmpty())
            {
                // Gán lại depth từ TXT khi merge
                if (timeToDepthMap.contains(lastBlockTime))
                    currentBlock.depth = timeToDepthMap[lastBlockTime];
                blockList.append(currentBlock);
            }
            // Nếu block vừa duyệt là maxTime thì dừng luôn
            if (lastBlockTime == maxTime)
            {
                stopReading = true;
                break;
            }
            currentBlock = BlockData();
        }
        bool ok = false;
        double depthVal = parts[0].toDouble(&ok);
        if (ok)
        {
            qint64 t = static_cast<qint64>(depthVal);
            t = t / 1000;
            lastBlockTime = t;
            if (t < minTime || t > maxTime)
            {
                hasCurrentBlock = false;
                continue;
            }
            // Không gán depth ở đây, sẽ gán lại từ TXT khi merge
            hasCurrentBlock = true;
        }
        else
        {
            hasCurrentBlock = false;
        }
        continue;
    }
    // Dữ liệu trong block
    if (hasCurrentBlock)
    {
        QVector<double> dataVec;
        bool allOk = true;
        for (const QString &val : parts)
        {
            bool ok = false;
            double d = val.toDouble(&ok);
            if (!ok)
            {
                allOk = false;
                break;
            }
            dataVec.append(d);
        }
        if (allOk && !dataVec.isEmpty())
        {
            currentBlock.data.append(dataVec);
        }
    }
}
// Xử lý block cuối cùng
if (hasCurrentBlock && !stopReading)
{
    if (lastBlockTime >= minTime && lastBlockTime <= maxTime && txtTimeSet.contains(lastBlockTime) && !currentBlock.data.isEmpty())
    {
        if (timeToDepthMap.contains(lastBlockTime))
            currentBlock.depth = timeToDepthMap[lastBlockTime];
        blockList.append(currentBlock);
    }
}
lasFile.close();
qDebug() << "Đã merge xong TXT vào LAS. blockList size:" << blockList.size();
qDebug() << " Debug: In ra 10 giá trị đầu tiên của blockList sau khi merge";
int debugCount = 0;
for (const BlockData &block : blockList)
{
    qDebug() << "Block depth:" << block.depth;
    for (const QVector<double> &row : block.data)
    {
        qDebug() << "  Data row:" << row;
        debugCount++;
        if (debugCount >= 10)
            break;
    }
    if (debugCount >= 10)
        break;
}