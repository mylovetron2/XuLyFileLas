#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QDesktopServices>
#include <QUrl>
#include <QProgressDialog>
#include <QDebug>
#include "LisFile2.h"
#include <QLabel>
#include "frmXuLyFileDauVao.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::on_pushButton_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString inputPath1 = QFileDialog::getOpenFileName(this, "Chọn file LAS đầu vào", "", "LAS Files (*.las)");
    if (inputPath1.isEmpty())
        return;
    QString inputPath2 = QFileDialog::getOpenFileName(this, "Chọn file TXT (lấy dữ liệu DoSau)", "", "Text Files (*.txt)");
    if (inputPath2.isEmpty())
        return;
    QString outputPath = QFileDialog::getSaveFileName(this, "Lưu file LAS mới", "", "LAS Files (*.las)");
    if (outputPath.isEmpty())
        return;

    QFile inFile1(inputPath1);
    QFile inFile2(inputPath2);
    QFile outFile(outputPath);
    if (!inFile1.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Lỗi", "Không mở được file đầu vào!");
        return;
    }
    if (!inFile2.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Lỗi", "Không mở được file TXT!");
        return;
    }
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Lỗi", "Không tạo được file đầu ra!");
        return;
    }

    QTextStream in1(&inFile1);
    QTextStream in2(&inFile2);
    QTextStream out(&outFile);
    bool inCurve = false, inAscii = false;
    QList<QString> curveLines;
    QMap<QString, QString> timeToDosau; // Map thời gian (hh:mm:ss) -> DoSau
    QList<QString> mergedTimes;         // Danh sách các thời gian đã merger
    double currentTimeMs = 0;
    // Hiển thị progress khi đọc file TXT
    QProgressDialog progressTxt("Đang đọc file TXT...", "Hủy", 0, 0, this);
    progressTxt.setWindowModality(Qt::WindowModal);
    progressTxt.setMinimumDuration(0);
    progressTxt.setAutoClose(true);
    progressTxt.setAutoReset(true);
    progressTxt.setValue(0);
    progressTxt.setWindowTitle("Đang đọc dữ liệu TXT");
    progressTxt.setLabelText("Vui lòng chờ trong khi dữ liệu TXT được đọc...");
    progressTxt.setCancelButtonText("Hủy bỏ");
    progressTxt.show();
    QList<double> dosauList; // Lưu toàn bộ giá trị độ sâu để phân tích xu hướng
    while (!in2.atEnd())
    {
        QString line2 = in2.readLine().trimmed();
        if (line2.isEmpty() || line2.toUpper().startsWith("DEPTH"))
            continue;
        QStringList parts2 = line2.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts2.size() >= 2)
        {
            QString timeStr = parts2[0];
            int idx = timeStr.indexOf(":");
            if (idx != -1)
            {
                // timeStr = timeStr.mid(idx + 1); // Bỏ, giữ nguyên chuỗi để chuyển sang giây
            }
            qint64 timeSec = MainWindow::timeStringToSeconds(timeStr);
            bool ok = false;
            double dosauVal = parts2[1].toDouble(&ok);
            QString dosauStr;
            if (ok)
            {
                double dosauFloor = std::floor(dosauVal * 10.0) / 10.0;
                dosauStr = QString::number(dosauFloor, 'f', 3);
                dosauList.append(dosauFloor);
            }
            else
            {
                dosauStr = parts2[1];
            }
            timeToDosau[QString::number(timeSec)] = dosauStr;
        }
        QApplication::processEvents();
        if (progressTxt.wasCanceled())
        {
            progressTxt.close();
            return;
        }
    }

    // Phân tích xu hướng bằng hồi quy tuyến tính
    QList<double> regularDepths;
    QList<double> interpolatedValues;
    if (dosauList.size() > 1)
    {
        int n = dosauList.size();
        double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
        for (int i = 0; i < n; ++i)
        {
            sumX += i;
            sumY += dosauList[i];
            sumXY += i * dosauList[i];
            sumXX += i * i;
        }
        double slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
        if (slope > 0.0)
            qDebug() << "Xu hướng độ sâu: TĂNG (slope=" << slope << ")";
        else if (slope < 0.0)
            qDebug() << "Xu hướng độ sâu: GIẢM (slope=" << slope << ")";
        else
            qDebug() << "Xu hướng độ sâu: KHÔNG ĐỔI (slope=0)";

        // --- Tạo dãy độ sâu đều và nội suy giá trị nếu thiếu ---
        double minDepth = *std::min_element(dosauList.begin(), dosauList.end());
        double maxDepth = *std::max_element(dosauList.begin(), dosauList.end());
        if (minDepth > maxDepth)
            std::swap(minDepth, maxDepth);
        for (double d = minDepth; d <= maxDepth + 1e-6; d += 0.1)
        {
            double dRound = std::round(d * 1000.0) / 1000.0;
            regularDepths.append(dRound);
        }
        // Tạo map độ sâu gốc -> giá trị
        QMap<double, double> depthToValue;
        for (double v : dosauList)
        {
            depthToValue[v] = v;
        }
        // Nội suy tuyến tính cho các độ sâu đều nếu thiếu
        for (double d : regularDepths)
        {
            if (depthToValue.contains(d))
            {
                interpolatedValues.append(depthToValue[d]);
            }
            else
            {
                // Tìm 2 điểm gần nhất để nội suy
                double d1 = minDepth, d2 = maxDepth;
                for (double v : dosauList)
                {
                    if (v < d && v > d1)
                        d1 = v;
                    if (v > d && v < d2)
                        d2 = v;
                }
                if (d1 == d2)
                {
                    interpolatedValues.append(d1);
                }
                else
                {
                    // Nội suy tuyến tính
                    double v1 = d1, v2 = d2;
                    double interp = v1 + (v2 - v1) * (d - d1) / (d2 - d1);
                    interpolatedValues.append(interp);
                }
            }
        }
        // Debug: In ra dãy độ sâu đều và giá trị nội suy
        qDebug() << "Dãy độ sâu đều (step 0.1):" << regularDepths;
        qDebug() << "Giá trị nội suy tương ứng:" << interpolatedValues;
    }
    progressTxt.setValue(1);
    progressTxt.close();

    // Hiển thị progress khi đọc file LAS đầu vào
    QProgressDialog progressLas("Đang đọc file LAS đầu vào...", "Hủy", 0, 0, this);
    progressLas.setWindowModality(Qt::WindowModal);
    progressLas.setMinimumDuration(0);
    progressLas.setAutoClose(false);
    progressLas.setAutoReset(false);
    progressLas.setWindowTitle("Đang xử lý dữ liệu");
    progressLas.setLabelText("Vui lòng chờ trong khi dữ liệu LAS được xử lý...");
    progressLas.setCancelButtonText("Hủy bỏ");
    progressLas.setRange(0, 0); // Indeterminate mode
    progressLas.show();

    blockList.clear();
    BlockData currentBlock;
    bool hasCurrentBlock = false;
    bool inAsciiSection = false;
    while (!in1.atEnd())
    {
        QString line = in1.readLine();
        QString trimmed = line.trimmed();
        if (trimmed.toUpper().startsWith("~ASCII"))
        {
            inAsciiSection = true;
            out << line << "\n";
            continue;
        }
        if (!inAsciiSection)
        {
            out << line << "\n";
            continue;
        }
        // Bỏ qua dòng trống hoặc comment
        if (trimmed.isEmpty() || trimmed.startsWith("#"))
            continue;

        // Block header: dòng chỉ có 1 số, không có khoảng trắng đầu dòng
        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        bool isBlockHeader = (parts.size() == 1 && !line.isEmpty() && !line.at(0).isSpace());
        if (isBlockHeader)
        {
            if (hasCurrentBlock)
            {
                blockList.append(currentBlock);
                currentBlock = BlockData();
            }
            bool ok = false;
            double depthVal = parts[0].toDouble(&ok);
            if (ok)
            {
                currentBlock.depth = depthVal;
                hasCurrentBlock = true;
            }
            else
            {
                hasCurrentBlock = false;
            }
            continue;
        }
        // Nếu không phải block header, là dòng dữ liệu: luôn thêm vào block hiện tại nếu đang có block
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
        QApplication::processEvents();
        if (progressLas.wasCanceled())
            break;
    }
    // Lưu block cuối cùng nếu có
    if (hasCurrentBlock && !currentBlock.data.isEmpty())
    {
        blockList.append(currentBlock);
    }
    progressLas.setValue(100);
    progressLas.close();
    // Debug: In ra số lượng block và thông tin block đầu tiên
    qDebug() << "BlockList size:" << blockList.size();
    if (!blockList.isEmpty())
    {
        qDebug() << "First block depth:" << blockList.first().depth;
        qDebug() << "First block data size:" << blockList.first().data.size();
        if (!blockList.first().data.isEmpty())
            qDebug() << "First block first data row:" << blockList.first().data.first();
    }
    inFile1.close();
    inFile2.close();
    outFile.close();
    QMessageBox::information(this, "Thành công", "Đã xử lý xong file LAS.");
    QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));
}

void MainWindow::on_exportLisToTextButton_clicked()
{
    QString lisPath = QFileDialog::getOpenFileName(this, "Chọn file LIS", "", "LIS Files (*.lis)");
    if (lisPath.isEmpty())
        return;
    QString lasPath = QFileDialog::getSaveFileName(this, "Lưu file LAS", "", "LAS Files (*.las)");
    if (lasPath.isEmpty())
        return;

    LisFile2 lis;
    if (!lis.open(lisPath))
    {
        QMessageBox::warning(this, "Lỗi", "Không mở được file LIS!");
        return;
    }
    lis.exportToLas(lasPath);
    QMessageBox::information(this, "Thành công", "Đã chuyển file LIS thành file LAS!");
}

QString MainWindow::secondsToTimeString(qint64 totalSeconds)
{
    totalSeconds = totalSeconds / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

qint64 MainWindow::timeStringToSeconds(const QString &str)
{
    QStringList parts = str.split(":");
    if (parts.size() != 4)
        return 0; // hoặc xử lý lỗi

    int days = parts[0].toInt();
    int hours = parts[1].toInt();
    int minutes = parts[2].toInt();
    int seconds = parts[3].toInt();

    return (days * 86400 + hours * 3600 + minutes * 60 + seconds) * 1000;
}

void MainWindow::on_btnTaoFile_clicked()
{
    frmXuLyFileDauVao form(this);
    form.setWindowModality(Qt::ApplicationModal);
    form.exec();
}

void MainWindow::readTXT(const QString &txtPath)
{
    headerList.clear();
    unitList.clear();
    dataRows.clear();

    // Đọc file TXT với cấu trúc: dòng 1 tiêu đề, dòng 2 đơn vị, dòng 3 trở đi là dữ liệu
    QFile txtFile(txtPath);
    if (!txtFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Lỗi", "Không mở được file TXT!");
        return;
    }
    QTextStream txtIn(&txtFile);

    headerList.clear();
    unitList.clear();
    dataRows.clear();
    bool foundHeader = false;
    bool foundUnit = false;
    while (!txtIn.atEnd())
    {
        QString line = txtIn.readLine().trimmed();
        if (line.isEmpty())
            continue;
        // Bỏ qua dòng ngày tháng hoặc dòng tiêu đề phụ
        if (line.contains("- TIME - RECORDER", Qt::CaseInsensitive) ||
            line.contains("DEPTH", Qt::CaseInsensitive) ||
            line.contains("LOGGING", Qt::CaseInsensitive) ||
            line.contains("VIETSOVPETRO", Qt::CaseInsensitive) ||
            line.contains("RECORDER", Qt::CaseInsensitive) ||
            QRegularExpression("^\\d{1,2}/\\d{1,2}/\\d{4}").match(line).hasMatch())
            continue;

        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        // Nhận diện dòng header (TIME DEPT ...)
        if (!foundHeader && !parts.isEmpty() && (parts[0].toUpper() == "TIME" || parts[0].toUpper() == "DEPTH"))
        {
            headerList = parts;
            foundHeader = true;
            continue;
        }
        // Nhận diện dòng đơn vị (nếu có)
        if (foundHeader && !foundUnit && !parts.isEmpty() && (parts[0].contains(":") || parts[0].contains("M") || parts[0].contains("S")))
        {
            unitList = parts;
            foundUnit = true;
            continue;
        }
        // Nhận diện dòng dữ liệu bắt đầu bằng TIME hợp lệ (0:00:00:01 ...)
        if (!parts.isEmpty() && QRegularExpression("^\\d+:\\d{2}:\\d{2}:\\d{2}$").match(parts[0]).hasMatch())
        {
            dataRows.append(parts);
        }
    }
    txtFile.close();

    // Chuyển đổi TIME sang giây cho từng dòng dữ liệu
    for (QStringList &row : dataRows)
    {
        if (!row.isEmpty())
        {
            QString timeStr = row[0];
            QStringList timeParts = timeStr.split(":");
            qint64 totalSeconds = 0;
            if (timeParts.size() == 4)
            {
                int days = timeParts[0].toInt();
                int hours = timeParts[1].toInt();
                int minutes = timeParts[2].toInt();
                int seconds = timeParts[3].toInt();
                totalSeconds = days * 86400 + hours * 3600 + minutes * 60 + seconds;
            }
            else if (timeParts.size() == 3)
            {
                int hours = timeParts[0].toInt();
                int minutes = timeParts[1].toInt();
                int seconds = timeParts[2].toInt();
                totalSeconds = hours * 3600 + minutes * 60 + seconds;
            }
            row[0] = QString::number(totalSeconds);
            qDebug() << "TIME TXT chuyển sang giây:" << timeStr << "->" << totalSeconds;
            // xử lý giá trị DEPTH
            if (row.size() > 1)
            {
                bool ok = false;
                double depthVal = row[1].toDouble(&ok);
                if (ok)
                {
                    double depthFloor = std::floor(depthVal * 10.0) / 10.0;
                    row[1] = QString::number(depthFloor, 'f', 3);
                }
            }
        }
    }

    qDebug() << " Debug: In ra header, unit, và 5 dòng dữ liệu đầu tiên";
    qDebug() << "Header:" << headerList;
    qDebug() << "Unit:" << unitList;
    for (int i = 0; i < qMin(5, dataRows.size()); ++i)
    {
        qDebug() << "Data row" << i << ":" << dataRows[i];
    }
}

void MainWindow::mergeTxtLas(const QString &lasPath)
{
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
}

// Ghi blockList ra file LAS mới, giữ nguyên header và format LAS
bool MainWindow::writeBlockListToLas(const QString &outputPath, const QList<BlockData> &blocks, const QStringList &headerLines)
{
    QFile outFile(outputPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&outFile);

    // Sửa thông tin Step
    for (CurveInfo &info : wellInfoList)
    {
        if (info.mnemonic.toUpper() == "STEP")
        {
            if (isDepthIncreasing)
            {
                info.value = "0.100"; // Cố định step 0.1
            }
            else
            {
                info.value = "-0.100"; // Cố định step -0.1
            }
            // info.value = "0.100"; // Cố định step 0.1
            info.unit = "M"; // Đơn vị mét
            break;
        }
    }

    // Sửa thông tin curve TIME thành DEPTH
    if (!curveInfoList.isEmpty())
    {
        curveInfoList[0].mnemonic = "DEPT";
        curveInfoList[0].unit = "M";
        curveInfoList[0].description = "M";
    }

    // Ghi section ~Version information
    out << "~Version information\n";
    out << "  VERS.                         2.00 : CWLS LOG ASCII STANDARD - VERSION 2.00              \n";
    out << "  WRAP.                          YES : MULTIPLE LINES PER DEPTH STEP             \n";

    writeCurveInfo(out, wellInfoList, "~Well information");
    writeCurveInfo(out, curveInfoList, "~Curve information");

    out << "~ASCII\n";
    // Ghi từng block
    for (const BlockData &block : blocks)
    {
        // Ghi depth với 3 số thập phân (vd: 12.300)
        out << QString::number(block.depth, 'f', 3) << "\n";
        for (const QVector<double> &row : block.data)
        {
            QStringList rowStrs;
            for (double v : row)
                rowStrs << QString::number(v, 'f', 3);
            out << rowStrs.join("    ") << "\n";
        }
    }
    outFile.close();
    return true;
}

void MainWindow::writeCurveInfo(QTextStream &out, const QList<CurveInfo> &curveList, const QString &sectionName)
{
    out << sectionName << "\n";
    // Dòng tiêu đề mẫu giống new37.las
    out << "# ====.==============================:=====================================================\n";
    for (const CurveInfo &info : curveList)
    {
        // Format: mnemonic.unit (left, 15), value (right, 15, 3 decimals), colon, description (left, 40)
        // Ví dụ: STRT.MS         86283000.000 : I         a      /I          d
        QString mnemonicUnit = QString("%1.%2").arg(info.mnemonic).arg(info.unit);
        mnemonicUnit = mnemonicUnit.leftJustified(15, ' ');
        QString value = info.value.isEmpty() ? "" : QString::number(info.value.toDouble(), 'f', 3);
        value = value.rightJustified(15, ' ');
        QString desc = info.description.leftJustified(40, ' ');
        QString line = QString("%1%2 : %3").arg(mnemonicUnit).arg(value).arg(desc);
        out << line << "\n";
    }
}

void MainWindow::on_btnTachFile_clicked()
{
    QString txtPath = QFileDialog::getOpenFileName(this, "Chọn file TXT", "", "Text Files (*.txt)");
    if (txtPath.isEmpty())
        return;
    QString lasPath = QFileDialog::getOpenFileName(this, "Chọn file LAS", "", "LAS Files (*.las)");
    if (lasPath.isEmpty())
        return;

    QString savePath = QFileDialog::getSaveFileName(this, "Lưu file LAS đã tách", "", "LAS Files (*.las)");
    if (savePath.isEmpty())
        return;
    // Progress cho readTXT
    QProgressDialog progressReadTxt("Đang đọc file TXT...", "Hủy", 0, 0, this);
    progressReadTxt.setWindowModality(Qt::WindowModal);
    progressReadTxt.setMinimumDuration(0);
    progressReadTxt.setAutoClose(true);
    progressReadTxt.setAutoReset(true);
    progressReadTxt.setRange(0, 0); // Indeterminate mode
    progressReadTxt.setWindowTitle("Đang đọc dữ liệu TXT");
    progressReadTxt.setLabelText("Vui lòng chờ trong khi dữ liệu TXT được đọc...");
    progressReadTxt.setCancelButtonText("Hủy bỏ");
    progressReadTxt.show();
    readTXT(txtPath);
    progressReadTxt.setValue(1);
    progressReadTxt.close();

    // Progress cho mergeTxtLas
    QProgressDialog progressMergeLas("Đang merge TXT vào LAS...", "Hủy", 0, 0, this);
    progressMergeLas.setWindowModality(Qt::WindowModal);
    progressMergeLas.setMinimumDuration(0);
    progressMergeLas.setAutoClose(true);
    progressMergeLas.setAutoReset(true);
    progressMergeLas.setRange(0, 0); // Indeterminate mode
    progressMergeLas.setWindowTitle("Đang merge dữ liệu");
    progressMergeLas.setLabelText("Vui lòng chờ trong khi dữ liệu LAS được xử lý...");
    progressMergeLas.setCancelButtonText("Hủy bỏ");
    progressMergeLas.show();
    mergeTxtLas(lasPath);
    progressMergeLas.setValue(1);
    progressMergeLas.close();

    QStringList headerLines;
    // Đọc lại header từ file LAS gốc
    QFile lasFile(lasPath);
    if (lasFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&lasFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            if (line.trimmed().toUpper().startsWith("~ASCII"))
                break;
            headerLines.append(line);
        }
        lasFile.close();
    }
    // Xử lý blockList nếu cần (lọc, gộp, v.v.)
    xuLyBlocklist(blockList);

    // Progress cho ghi file
    QProgressDialog progressWriteFile("Đang ghi file LAS mới...", "Hủy", 0, 0, this);
    progressWriteFile.setWindowModality(Qt::WindowModal);
    progressWriteFile.setMinimumDuration(0);
    progressWriteFile.setAutoClose(true);
    progressWriteFile.setAutoReset(true);
    progressWriteFile.setRange(0, 0); // Indeterminate mode
    progressWriteFile.setWindowTitle("Đang ghi file LAS");
    progressWriteFile.setLabelText("Vui lòng chờ trong khi file LAS được ghi...");
    progressWriteFile.setCancelButtonText("Hủy bỏ");
    progressWriteFile.show();
    bool writeOk = writeBlockListToLas(savePath, blockList, headerLines);
    progressWriteFile.setValue(1);
    progressWriteFile.close();
    if (writeOk)
    {
        QMessageBox::information(this, "Thành công", "Đã Lưu file LAS mới thành công!");
        QDesktopServices::openUrl(QUrl::fromLocalFile(savePath));
    }
    else
    {
        QMessageBox::warning(this, "Lỗi", "Không ghi được file LAS mới!");
    }
}

// Hàm xử lý blockList trước khi ghi file (có thể lọc, chỉnh sửa, v.v.)
void MainWindow::xuLyBlocklist(QList<BlockData> &blocks)
{
    // Gom nhóm các block theo depth bằng QList
    QList<QPair<double, QList<BlockData>>> groupedBlocks;
    for (const BlockData &block : blocks)
    {
        bool found = false;
        for (auto &pair : groupedBlocks)
        {
            if (qFuzzyCompare(pair.first + 1, block.depth + 1))
            { // So sánh double an toàn
                pair.second.append(block);
                found = true;
                break;
            }
        }
        if (!found)
        {
            groupedBlocks.append(qMakePair(block.depth, QList<BlockData>{block}));
        }
    }
    // Sắp xếp theo depth tăng dần
    // std::sort(groupedBlocks.begin(), groupedBlocks.end(), [](const QPair<double, QList<BlockData>> &a, const QPair<double, QList<BlockData>> &b)
    //           { return a.first < b.first; });
    QList<BlockData> result;
    for (const auto &pair : groupedBlocks)
    {
        const QList<BlockData> &blockGroup = pair.second;
        if (blockGroup.isEmpty())
            continue;
        // Nếu chỉ có 1 block, giữ nguyên
        if (blockGroup.size() == 1)
        {
            result.append(blockGroup.first());
            continue;
        }
        // Nếu có nhiều block cùng depth, tính trung bình từng cột giữa các block (theo vị trí cột)
        int maxRow = 0;
        int maxCol = 0;
        for (const BlockData &b : blockGroup)
        {
            if (b.data.size() > maxRow)
                maxRow = b.data.size();
            for (const QVector<double> &row : b.data)
                if (row.size() > maxCol)
                    maxCol = row.size();
        }
        // Tính trung bình từng cột cho từng dòng (theo index dòng và index cột)
        QList<QVector<double>> avgRows;
        for (int rowIdx = 0; rowIdx < maxRow; ++rowIdx)
        {
            QVector<double> sumCol(maxCol, 0.0);
            int countCol[maxCol];
            std::fill(countCol, countCol + maxCol, 0);
            for (const BlockData &b : blockGroup)
            {
                if (b.data.size() > rowIdx)
                {
                    const QVector<double> &row = b.data[rowIdx];
                    for (int col = 0; col < row.size(); ++col)
                    {
                        sumCol[col] += row[col];
                        countCol[col]++;
                    }
                }
            }
            QVector<double> avgCol(maxCol, 0.0);
            for (int col = 0; col < maxCol; ++col)
            {
                if (countCol[col] > 0)
                    avgCol[col] = sumCol[col] / countCol[col];
                else
                    avgCol[col] = 0.0;
            }
            avgRows.append(avgCol);
        }
        BlockData newBlock;
        newBlock.depth = pair.first;
        newBlock.data = avgRows;
        result.append(newBlock);
    }
    // Debug: In ra 10 giá trị đầu tiên của result
    qDebug() << "Debug 10 giá trị đầu tiên của result:";
    int debugCount = 0;
    for (const BlockData &block : result)
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
    //  Xác định xu hướng độ sâu (tăng/giảm/không đổi) dựa trên slope hồi quy tuyến tính
    // bool isIncreasing = true; // default
    if (!result.isEmpty())
    {
        QList<double> depthList;
        for (const BlockData &b : result)
        {
            depthList.append(b.depth);
        }
        if (depthList.size() > 1)
        {
            int n = depthList.size();
            double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
            for (int i = 0; i < n; ++i)
            {
                sumX += i;
                sumY += depthList[i];
                sumXY += i * depthList[i];
                sumXX += i * i;
            }
            double slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
            if (slope > 0.0)
            {
                qDebug() << "Xu hướng độ sâu: TĂNG (slope=" << slope << ")";
                isDepthIncreasing = true;
            }
            else if (slope < 0.0)
            {
                qDebug() << "Xu hướng độ sâu: GIẢM (slope=" << slope << ")";
                isDepthIncreasing = false;
            }
            else
            {
                qDebug() << "Xu hướng độ sâu: KHÔNG ĐỔI (slope=0)";
                isDepthIncreasing = true; // hoặc false đều được, vì không đổi
            }
        }
    }

    // Lọc các block chỉ giữ theo xu hướng chính, loại bỏ block có độ sâu âm
    if (!result.isEmpty())
    {
        QList<BlockData> filtered;
        // Chỉ thêm block đầu tiên nếu depth >= 0
        if (result.first().depth >= 0)
            filtered.append(result.first());
        for (int i = 1; i < result.size(); ++i)
        {
            // Loại bỏ block có độ sâu âm
            if (result[i].depth < 0)
                continue;
            if (filtered.isEmpty())
            {
                filtered.append(result[i]);
                continue;
            }
            if (isDepthIncreasing)
            {
                if (result[i].depth >= filtered.last().depth)
                {
                    filtered.append(result[i]);
                }
            }
            else
            {
                if (result[i].depth <= filtered.last().depth)
                {
                    filtered.append(result[i]);
                }
            }
        }
        result = filtered;
    }

    // Nội suy độ sâu nếu có khoảng trống lớn hơn 0.1 (step = 0.1)
    if (result.size() > 1)
    {
        QList<BlockData> interpolated;
        for (int i = 0; i < result.size() - 1; ++i)
        {
            const BlockData &b1 = result[i];
            const BlockData &b2 = result[i + 1];
            interpolated.append(b1);

            double d1 = b1.depth;
            double d2 = b2.depth;
            double step = (d2 > d1) ? 0.1 : -0.1;
            double nextDepth = d1 + step;
            // Chèn tất cả các giá trị nội suy giữa d1 và d2 (không bao gồm d1, d2)
            while ((step > 0 && nextDepth < d2 - 1e-6) || (step < 0 && nextDepth > d2 + 1e-6))
            {
                BlockData interpBlock;
                interpBlock.depth = std::round(nextDepth * 1000.0) / 1000.0;

                // Nội suy từng giá trị trong data
                int rowCount = std::min(b1.data.size(), b2.data.size());
                for (int row = 0; row < rowCount; ++row)
                {
                    const QVector<double> &row1 = b1.data[row];
                    const QVector<double> &row2 = b2.data[row];
                    int colCount = std::min(row1.size(), row2.size());
                    QVector<double> interpRow;
                    for (int col = 0; col < colCount; ++col)
                    {
                        double v1 = row1[col];
                        double v2 = row2[col];
                        double t = (interpBlock.depth - d1) / (d2 - d1);
                        double vInterp = v1 + (v2 - v1) * t;
                        interpRow.append(vInterp);
                    }
                    interpBlock.data.append(interpRow);
                }
                interpolated.append(interpBlock);
                nextDepth += step;
            }
        }
        interpolated.append(result.last());
        result = interpolated;
    }
    blocks = result;
}

// Vẽ đồ thị dữ liệu trong file TXT
void MainWindow::on_btnDrawTxtChart_clicked()
{
    QString txtPath = QFileDialog::getOpenFileName(this, "Chọn file TXT để vẽ biểu đồ", "", "Text Files (*.txt)");
    if (txtPath.isEmpty())
        return;
    QFile txtFile(txtPath);
    if (!txtFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Lỗi", "Không mở được file TXT!");
        return;
    }
    QTextStream in(&txtFile);
    QList<double> xList, yList;
    int lineIdx = 0;
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        // Bỏ qua dòng tiêu đề và dòng đơn vị (2 dòng đầu)
        if (lineIdx < 2)
        {
            ++lineIdx;
            continue;
        }
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 2)
        {
            bool ok2 = false;
            double x = 0;
            // Thử parse cột 1 là số
            bool ok1 = false;
            x = parts[0].toDouble(&ok1);
            if (!ok1)
            {
                // Nếu không phải số, thử parse dạng thời gian d:hh:mm:ss hoặc hh:mm:ss
                QStringList timeParts = parts[0].split(":");
                qint64 totalSeconds = 0;
                if (timeParts.size() == 4)
                {
                    int days = timeParts[0].toInt();
                    int hours = timeParts[1].toInt();
                    int minutes = timeParts[2].toInt();
                    int seconds = timeParts[3].toInt();
                    totalSeconds = days * 86400 + hours * 3600 + minutes * 60 + seconds;
                }
                else if (timeParts.size() == 3)
                {
                    int hours = timeParts[0].toInt();
                    int minutes = timeParts[1].toInt();
                    int seconds = timeParts[2].toInt();
                    totalSeconds = hours * 3600 + minutes * 60 + seconds;
                }
                else
                {
                    qDebug() << "Bỏ qua dòng không nhận diện được thời gian:" << line;
                    ++lineIdx;
                    continue;
                }
                x = static_cast<double>(totalSeconds);
                ok1 = true;
            }
            double y = parts[1].toDouble(&ok2);
            if (ok1 && ok2)
            {
                xList.append(x);
                yList.append(y);
            }
            else
            {
                qDebug() << "Bỏ qua dòng không hợp lệ (không parse được số):" << line;
            }
        }
        else
        {
            qDebug() << "Bỏ qua dòng không đủ cột dữ liệu:" << line;
        }
        ++lineIdx;
    }
    txtFile.close();
    if (xList.isEmpty() || yList.isEmpty())
    {
        QMessageBox::warning(this, "Lỗi", "Không có dữ liệu hợp lệ để vẽ!");
        return;
    }
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < xList.size(); ++i)
        series->append(yList[i], xList[i]); // Đổi trục: giá trị là X, TIME/DEPTH là Y
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Biểu đồ dữ liệu TXT (Giá trị - TIME/DEPTH)");
    chart->createDefaultAxes();
    chart->axisX()->setTitleText("Cột 2 (Giá trị)");
    chart->axisY()->setTitleText("Cột 1 (TIME hoặc DEPTH)");
    // Đảo chiều trục Y: giá trị lớn ở trên, nhỏ ở dưới
    QValueAxis *axisY = qobject_cast<QValueAxis *>(chart->axisY());
    if (axisY)
    {
        axisY->setReverse(true);
    }
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setWindowTitle("Biểu đồ TXT");
    chartView->resize(800, 600);
    // Bật chức năng scale (zoom/pan) với nút chuyển đổi
    chartView->setInteractive(true);
    chartView->setDragMode(QGraphicsView::ScrollHandDrag); // Mặc định là pan

    // Thêm nút chuyển đổi chế độ zoom/pan
    QPushButton *btnToggle = new QPushButton("Chuyển Zoom/Pan", chartView);
    btnToggle->setGeometry(10, 10, 130, 30);
    btnToggle->setStyleSheet("background: white; border: 1px solid gray;");
    btnToggle->raise();
    // Biến trạng thái
    bool *isZoomMode = new bool(false);
    QObject::connect(btnToggle, &QPushButton::clicked, [chartView, btnToggle, isZoomMode]()
                     {
        *isZoomMode = !*isZoomMode;
        if (*isZoomMode) {
            chartView->setRubberBand(QChartView::RectangleRubberBand);
            chartView->setDragMode(QGraphicsView::NoDrag);
            btnToggle->setText("Chuyển Pan");
        } else {
            chartView->setRubberBand(QChartView::NoRubberBand);
            chartView->setDragMode(QGraphicsView::ScrollHandDrag);
            btnToggle->setText("Chuyển Zoom");
        } });
    btnToggle->setText("Chuyển Zoom");

    // Thêm nút Reset Zoom
    QPushButton *btnReset = new QPushButton("Reset Zoom", chartView);
    btnReset->setGeometry(150, 10, 110, 30);
    btnReset->setStyleSheet("background: white; border: 1px solid gray;");
    btnReset->raise();
    QObject::connect(btnReset, &QPushButton::clicked, [chart, chartView]()
                     {
        chart->zoomReset();
        chartView->setRubberBand(QChartView::NoRubberBand);
        chartView->setDragMode(QGraphicsView::ScrollHandDrag); });

    chartView->show();
}
