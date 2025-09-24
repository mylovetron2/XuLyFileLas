#ifndef FRMXULYFILEDAUVAO_H
#define FRMXULYFILEDAUVAO_H

#include <QDialog>
#include <QListWidget>
#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QList>
#include <QString>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

class frmXuLyFileDauVao : public QDialog

{
    Q_OBJECT
public:
    explicit frmXuLyFileDauVao(QWidget *parent = nullptr);
    ~frmXuLyFileDauVao();
    QString analyzeDepthTrend(const QString &txtPath);

private:
    QListWidget *listWidgetTxtFiles;
    QWidget *chartContainer;
    QHBoxLayout *mainLayout;
    QChartView *chartView;
    QLabel *trendLabel;

    void listTxtFiles(const QString &dirPath);
    void drawChartForTxt(const QString &txtPath);

private slots:
    void onBtnTachFileClicked();
    void onTxtFileSelected();
};

#endif // FRMXULYFILEDAUVAO_H
