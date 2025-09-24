
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
    resize(800, 500);
    mainLayout = new QHBoxLayout(this);

    listWidgetTxtFiles = new QListWidget(this);
    listWidgetTxtFiles->setMinimumWidth(220);
    mainLayout->addWidget(listWidgetTxtFiles);

    chartContainer = new QWidget(this);
    QHBoxLayout *chartLayout = new QHBoxLayout(chartContainer);
    chartView = new QChartView(chartContainer);
    chartLayout->addWidget(chartView, 2);
    // Tạo layout dọc cho phần bên phải
    QVBoxLayout *rightLayout = new QVBoxLayout();
    trendLabel = new QLabel(chartContainer);
    trendLabel->setWordWrap(true);
    trendLabel->setMinimumWidth(220);
    trendLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    rightLayout->addWidget(trendLabel);
    QPushButton *btnTachFile = new QPushButton("Tách file", chartContainer);
    rightLayout->addWidget(btnTachFile);
    rightLayout->addStretch();
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
    bool allSame = true;
    for (int i = 1; i < n; ++i)
    {
        if (dirList[i] != dirList[0])
        {
            allSame = false;
            break;
        }
    }
    if (allSame)
    {
        QMessageBox::warning(this, "Lỗi", "Không có đoạn tăng/giảm để vẽ!");
        return;
    }
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
    QString result = analyzeDepthTrend(txtPath);
    QMessageBox::information(this, "Kết quả tách xu hướng Depth", result);
    // Hàm phân tích xu hướng tăng/giảm Depth
}

QString frmXuLyFileDauVao::analyzeDepthTrend(const QString &txtPath)
{
    QFile txtFile(txtPath);
    if (!txtFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return "Không mở được file TXT!";
    }
    QTextStream in(&txtFile);
    QList<double> depthList;
    int lineIdx = 0;
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        if (lineIdx < 2)
        {
            ++lineIdx;
            continue;
        }
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 2)
        {
            bool ok = false;
            double depth = parts[1].toDouble(&ok);
            if (ok)
                depthList.append(depth);
        }
        ++lineIdx;
    }
    txtFile.close();
    if (depthList.size() < 2)
    {
        return "Không đủ dữ liệu Depth để phân tích!";
    }
    // Phân tích xu hướng tăng/giảm
    QList<QPair<int, int>> tangList, giamList;
    int start = 0;
    bool isTang = depthList[1] > depthList[0];
    for (int i = 1; i < depthList.size(); ++i)
    {
        bool currTang = depthList[i] > depthList[i - 1];
        if (currTang != isTang)
        {
            if (isTang)
                tangList.append(qMakePair(start, i - 1));
            else
                giamList.append(qMakePair(start, i - 1));
            start = i - 1;
            isTang = currTang;
        }
    }
    // Đoạn cuối cùng
    if (isTang)
        tangList.append(qMakePair(start, depthList.size() - 1));
    else
        giamList.append(qMakePair(start, depthList.size() - 1));

    // Kết quả
    QString result;
    result += "Đoạn xu hướng TĂNG Depth:\n";
    for (const auto &seg : tangList)
        result += QString("  Dòng %1 đến %2\n").arg(seg.first + 3).arg(seg.second + 3); // +3 vì bỏ qua 2 dòng đầu
    result += "\nĐoạn xu hướng GIẢM Depth:\n";
    for (const auto &seg : giamList)
        result += QString("  Dòng %1 đến %2\n").arg(seg.first + 3).arg(seg.second + 3);
    return result;
}
