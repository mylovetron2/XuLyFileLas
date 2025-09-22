#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

// using namespace QtCharts;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

struct BlockData
{
    double depth = 0.0;
    QList<QVector<double>> data;
};

// Cấu trúc lưu thông tin từng curve trong ~Curve information
struct CurveInfo
{
    QString mnemonic;    // Tên viết tắt (ví dụ: DEPT.M)
    QString unit;        // Đơn vị (ví dụ: M, MS, V, ...)
    QString value;       // Giá trị (ví dụ: 86283000.000)
    QString description; // Mô tả hoặc comment
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static qint64 timeStringToSeconds(const QString &str);
    // Ghi blockList ra file LAS mới, giữ nguyên header và format LAS
    bool writeBlockListToLas(const QString &outputPath, const QList<BlockData> &blocks, const QStringList &headerLines);
    void xuLyBlocklist(QList<BlockData> &blocks);

private slots:
    void on_pushButton_clicked();
    void on_drawChartButton_clicked();
    void on_exportLisToTextButton_clicked();
    void on_btnTaoFile_clicked();

    void on_btnTachFile_clicked();

    void readTXT(const QString &txtPath);
    void mergeTxtLas(const QString &lasPath);
    void writeCurveInfo(QTextStream &out, const QList<CurveInfo> &curveList, const QString &sectionName);

private:
    Ui::MainWindow *ui;
    QChartView *chartView = nullptr;
    static QString secondsToTimeString(qint64 totalSeconds);

    QList<BlockData> blockList; // Danh sách các block đã parse
    QStringList headerList;
    QStringList unitList;
    QList<QStringList> dataRows;
    QList<CurveInfo> curveInfoList;
    QList<CurveInfo> wellInfoList; // Lưu toàn bộ header của file LAS gốc
};

#endif // MAINWINDOW_H
