// LisFile2.h: interface for the LisFile2 class for Qt
//
#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include <QFile>
#include <QByteArray>

class LisFile2 : public QObject
{
    Q_OBJECT
public:
    explicit LisFile2(QObject *parent = nullptr);
    ~LisFile2();

    bool open(const QString &fileName);
    void close();
    int recordCount() const;
    QVector<float> readAllData();
    void exportToText(const QString &outputFileName);
    void exportToLas(const QString &outputFileName);

private:
    QFile file;
    QByteArray fileData;
    QVector<float> floatData;
    bool isOpen = false;
};
