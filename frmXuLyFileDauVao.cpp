
#include <QTimer>

#include "frmXuLyFileDauVao.h"
#include <QDir>
#include <QHBoxLayout>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QDebug>
#include <QFileDialog>

// using namespace QtCharts;

frmXuLyFileDauVao::frmXuLyFileDauVao(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Phân tích file TXT");
    resize(1200, 600);
    setMinimumSize(900, 500);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    mainLayout = new QHBoxLayout(this);

    listWidgetTxtFiles = new QListWidget(this);
    listWidgetTxtFiles->setMinimumWidth(220);
    mainLayout->addWidget(listWidgetTxtFiles);

    chartContainer = new QWidget(this);
    QHBoxLayout *chartLayout = new QHBoxLayout(chartContainer);
    chartView = new QChartView(chartContainer);
    chartView->setStyleSheet("background: #f8f9fa; border-radius: 12px; border: 1px solid #d1d5db;");
    chartView->setRubberBand(QChartView::RectangleRubberBand);
    chartLayout->addWidget(chartView, 4);
    // Tạo layout dọc cho phần bên phải
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QLabel *trendTitle = new QLabel("Kết quả xu hướng Depth", chartContainer);
    trendTitle->setStyleSheet("font-weight: bold; font-size: 15px; color: #2d3748; margin-bottom: 8px;");
    rightLayout->addWidget(trendTitle);
    trendLabel = new QLabel(chartContainer);
    trendLabel->setWordWrap(true);
    trendLabel->setMinimumWidth(140);
    trendLabel->setMaximumWidth(180);
    trendLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    trendLabel->setStyleSheet("background: #fff; border-radius: 8px; border: 1px solid #cbd5e1; padding: 8px; font-size: 13px; color: #222; margin-bottom: 12px;");
    rightLayout->addWidget(trendLabel);
    QPushButton *btnTachFile = new QPushButton("Tách file", chartContainer);
    btnTachFile->setStyleSheet("background: #3182ce; color: #fff; border-radius: 6px; padding: 6px 18px; font-weight: bold; font-size: 14px;");
    btnTachFile->setIcon(QIcon(":/icons/split.png"));
    rightLayout->addWidget(btnTachFile);
    // Thêm nút Reset zoom
    QPushButton *btnResetZoom = new QPushButton("Reset", chartContainer);
    btnResetZoom->setStyleSheet("background: #e53e3e; color: #fff; border-radius: 6px; padding: 6px 18px; font-weight: bold; font-size: 14px; margin-top: 8px;");
    btnResetZoom->setIcon(QIcon(":/icons/reset.png"));
    rightLayout->addWidget(btnResetZoom);
    rightLayout->addStretch();
    connect(btnResetZoom, &QPushButton::clicked, this, [this]()
            {
        if (chartView && chartView->chart())
            chartView->chart()->zoomReset(); });
    chartLayout->addLayout(rightLayout, 1);
    chartContainer->setLayout(chartLayout);
    mainLayout->addWidget(chartContainer);
    connect(btnTachFile, &QPushButton::clicked, this, &frmXuLyFileDauVao::onBtnTachFileClicked);

    setLayout(mainLayout);

    // Chọn thư mục chứa file TXT
    QString selectedDir = QFileDialog::getExistingDirectory(this, "Chọn thư mục chứa file TXT", qApp->applicationDirPath());
    if (selectedDir.isEmpty())
    {
        // Nếu không chọn, đóng form
        QTimer::singleShot(0, this, SLOT(close()));
        return;
    }
    listTxtFiles(selectedDir);
    connect(listWidgetTxtFiles, &QListWidget::itemClicked, this, &frmXuLyFileDauVao::onTxtFileSelected);
}

frmXuLyFileDauVao::~frmXuLyFileDauVao() {}

void frmXuLyFileDauVao::listTxtFiles(const QString &dirPath)
{
    QDir dir(dirPath);
    QStringList txtFiles = dir.entryList(QStringList() << "*.txt", QDir::Files);
    listWidgetTxtFiles->clear();
    QLabel *listTitle = new QLabel("Danh sách file TXT", listWidgetTxtFiles);
    listTitle->setStyleSheet("font-weight: bold; font-size: 15px; color: #2d3748; margin-bottom: 8px;");
    listWidgetTxtFiles->setStyleSheet("background: #f1f5f9; border-radius: 10px; font-size: 13px; color: #222; padding: 6px;");
    listWidgetTxtFiles->setFont(QFont("Segoe UI", 11));
    for (const QString &file : txtFiles)
        listWidgetTxtFiles->addItem(dir.filePath(file));
}

void frmXuLyFileDauVao::onTxtFileSelected()
{
    QListWidgetItem *item = listWidgetTxtFiles->currentItem();
    if (!item)
        return;
    QString txtPath = item->text();
    drawChartForTxt(txtPath);
}

void frmXuLyFileDauVao::drawChartForTxt(const QString &txtPath)
{
    QFile txtFile(txtPath);
    if (!txtFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Lỗi", "Không mở được file TXT!");
        return;
    }
    QTextStream in(&txtFile);
    QList<double> xList, yList;
    QList<int> dirList;
    int dirColIdx = -1;
    bool foundHeader = false;
    int dataStartLine = 0;
    int lineIdx = 0;
    QStringList headers;
    QList<QStringList> dataLines;
    // Tìm dòng tiêu đề chứa DIR
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
        {
            ++lineIdx;
            continue;
        }
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (!foundHeader)
        {
            for (int i = 0; i < parts.size(); ++i)
            {
                if (parts[i].toUpper() == "DIR")
                {
                    dirColIdx = i;
                    foundHeader = true;
                    headers = parts;
                    dataStartLine = lineIdx + 1;
                    break;
                }
            }
            ++lineIdx;
            continue;
        }
        if (foundHeader)
        {
            dataLines.append(parts);
        }
        ++lineIdx;
    }
    txtFile.close();
    // Xử lý dữ liệu từ dataLines
    for (int i = 0; i < dataLines.size(); ++i)
    {
        const QStringList &parts = dataLines[i];
        if (dirColIdx == -1 || parts.size() <= dirColIdx)
            continue;
        bool ok1 = false, ok2 = false, ok3 = false;
        double x = parts[0].toDouble(&ok1);
        double y = parts[1].toDouble(&ok2);
        int dir = parts[dirColIdx].toInt(&ok3);
        if (!ok1)
        {
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
                continue;
            }
            x = static_cast<double>(totalSeconds);
            ok1 = true;
        }
        if (ok1 && ok2 && ok3)
        {
            xList.append(x);
            yList.append(y);
            dirList.append(dir);
            qDebug() << "DIR value at line" << (dataStartLine + i + 1) << ":" << dir;
        }
    }
    int n = xList.size();
    if (xList.isEmpty() || yList.isEmpty() || dirList.isEmpty())
    {
        QMessageBox::warning(this, "Lỗi", "Không có dữ liệu hợp lệ để vẽ!");
        return;
    }
    if (n < 2)
    {
        QMessageBox::warning(this, "Lỗi", "Không đủ dữ liệu để vẽ xu hướng!");
        return;
    }
    // Kiểm tra nếu tất cả giá trị dir đều giống nhau (không có đoạn để vẽ)
    QChart *chart = new QChart();
    int start = 0;
    int currDir = dirList[0];
    for (int i = 1; i < n; ++i)
    {
        if (dirList[i] != currDir || i == n - 1)
        {
            int end = (dirList[i] != currDir) ? i - 1 : i;
            QLineSeries *series = new QLineSeries();
            for (int j = start; j <= end; ++j)
                series->append(yList[j], xList[j]);
            if (currDir == 0)
            {
                series->setColor(Qt::red);
                series->setName("down");
            }
            else
            {
                series->setName("up");
            }
            chart->addSeries(series);
            start = end;
            currDir = dirList[i];
        }
    }
    chart->setTitle("Biểu đồ dữ liệu TXT (Giá trị - TIME/DEPTH)");
    chart->createDefaultAxes();
    chart->axisX()->setTitleText("Cột 2 (Giá trị)");
    chart->axisY()->setTitleText("Cột 1 (TIME hoặc DEPTH)");
    QValueAxis *axisY = qobject_cast<QValueAxis *>(chart->axisY());
    if (axisY)
        axisY->setReverse(true);
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Hiển thị kết quả phân tích xu hướng cố định trên form
    QString trendResult = analyzeDepthTrend(txtPath);
    trendLabel->setText(trendResult);
}

void frmXuLyFileDauVao::onBtnTachFileClicked()
{
    QListWidgetItem *item = listWidgetTxtFiles->currentItem();
    if (!item)
    {
        QMessageBox::warning(this, "Lỗi", "Vui lòng chọn một file TXT trong danh sách!");
        return;
    }
    QString txtPath = item->text();
    QFile txtFile(txtPath);
    if (!txtFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Lỗi", "Không mở được file TXT!");
        return;
    }
    QTextStream in(&txtFile);
    QStringList lines;
    while (!in.atEnd())
    {
        lines << in.readLine();
    }
    txtFile.close();
    // Tìm dòng tiêu đề chứa DIR
    int dirColIdx = -1;
    int headerLineIdx = -1;
    for (int i = 0; i < lines.size(); ++i)
    {
        QString line = lines[i].trimmed();
        if (line.isEmpty())
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        for (int j = 0; j < parts.size(); ++j)
        {
            if (parts[j].toUpper() == "DIR")
            {
                dirColIdx = j;
                headerLineIdx = i;
                break;
            }
        }
        if (dirColIdx != -1)
            break;
    }
    if (dirColIdx == -1 || headerLineIdx == -1)
    {
        QMessageBox::warning(this, "Lỗi", "Không tìm thấy cột DIR trong file!");
        return;
    }
    // Tách dữ liệu up/down
    QStringList upLines, downLines;
    // Giữ nguyên cấu trúc header
    for (int i = 0; i <= headerLineIdx; ++i)
    {
        upLines << lines[i];
        downLines << lines[i];
    }
    // Duyệt dữ liệu sau header
    for (int i = headerLineIdx + 1; i < lines.size(); ++i)
    {
        QString line = lines[i].trimmed();
        if (line.isEmpty())
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() <= dirColIdx)
            continue;
        bool ok = false;
        int dir = parts[dirColIdx].toInt(&ok);
        if (!ok)
            continue;
        if (dir == 0)
            downLines << lines[i];
        else
            upLines << lines[i];
    }
    // Tạo file mới
    QFileInfo fi(txtPath);
    QString baseName = fi.completeBaseName();
    QString ext = fi.suffix();
    QString upFile = fi.path() + "/" + baseName + "_up." + ext;
    QString downFile = fi.path() + "/" + baseName + "_down." + ext;
    QFile fUp(upFile), fDown(downFile);
    if (fUp.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&fUp);
        for (const QString &l : upLines)
            out << l << "\n";
        fUp.close();
    }
    if (fDown.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&fDown);
        for (const QString &l : downLines)
            out << l << "\n";
        fDown.close();
    }
    QMessageBox::information(this, "Tách file", QString("Đã tạo file:\n%1\n%2").arg(upFile).arg(downFile));
    // Refresh lại danh sách file TXT trong thư mục hiện tại
    QString currentDir = fi.path();
    listTxtFiles(currentDir);
}

QString frmXuLyFileDauVao::analyzeDepthTrend(const QString &txtPath)
{
    QFile txtFile(txtPath);
    if (!txtFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return "Không mở được file TXT!";
    QTextStream in(&txtFile);
    int dirColIdx = -1;
    bool foundHeader = false;
    int lineIdx = 0;
    QList<int> dirList;
    // Tìm dòng tiêu đề chứa DIR
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
        {
            ++lineIdx;
            continue;
        }
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (!foundHeader)
        {
            for (int i = 0; i < parts.size(); ++i)
            {
                if (parts[i].toUpper() == "DIR")
                {
                    dirColIdx = i;
                    foundHeader = true;
                    break;
                }
            }
            ++lineIdx;
            continue;
        }
        if (foundHeader)
        {
            if (dirColIdx != -1 && parts.size() > dirColIdx)
            {
                bool ok = false;
                int dir = parts[dirColIdx].toInt(&ok);
                if (ok)
                    dirList.append(dir);
            }
        }
        ++lineIdx;
    }
    txtFile.close();
    if (dirList.size() < 2)
        return "Không đủ dữ liệu DIR để phân tích!";
    // Phân tích xu hướng dựa vào DIR
    QList<QPair<int, int>> upList, downList;
    int start = 0;
    int currDir = dirList[0];
    for (int i = 1; i < dirList.size(); ++i)
    {
        if (dirList[i] != currDir)
        {
            if (currDir == 0)
                downList.append(qMakePair(start, i - 1));
            else
                upList.append(qMakePair(start, i - 1));
            start = i;
            currDir = dirList[i];
        }
    }
    // Đoạn cuối cùng
    if (currDir == 0)
        downList.append(qMakePair(start, dirList.size() - 1));
    else
        upList.append(qMakePair(start, dirList.size() - 1));
    // Kết quả
    QString result;
    result += "Đoạn xu hướng DOWN (màu đỏ):\n";
    for (const auto &seg : downList)
        result += QString("  Dòng %1 đến %2\n").arg(seg.first + 1).arg(seg.second + 1);
    result += "\nĐoạn xu hướng UP:\n";
    for (const auto &seg : upList)
        result += QString("  Dòng %1 đến %2\n").arg(seg.first + 1).arg(seg.second + 1);
    return result;
}
