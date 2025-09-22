// LisFile2.cpp: implementation for LisFile2 class for Qt
#include "LisFile2.h"
#include <QDataStream>
#include <QTextStream>

LisFile2::LisFile2(QObject *parent)
    : QObject(parent)
{
}

LisFile2::~LisFile2()
{
    close();
}

bool LisFile2::open(const QString &fileName)
{
    close();
    file.setFileName(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        isOpen = false;
        return false;
    }
    fileData = file.readAll();
    // Example: convert raw bytes to float array (customize for LIS format)
    floatData.clear();
    QDataStream stream(fileData);
    stream.setByteOrder(QDataStream::LittleEndian);
    while (!stream.atEnd())
    {
        float value;
        stream >> value;
        floatData.append(value);
    }
    isOpen = true;
    return true;
}

void LisFile2::close()
{
    if (file.isOpen())
        file.close();
    isOpen = false;
    fileData.clear();
    floatData.clear();
}

int LisFile2::recordCount() const
{
    return floatData.size();
}

QVector<float> LisFile2::readAllData()
{
    return floatData;
}

void LisFile2::exportToText(const QString &outputFileName)
{
    QFile outFile(outputFileName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return;
    }
    QTextStream out(&outFile);
    for (float value : floatData)
    {
        out << QString::number(value, 'f', 6) << "\n";
    }
    outFile.close();
}

void LisFile2::exportToLas(const QString &outputFileName)
{
    QFile outFile(outputFileName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return;
    }
    QTextStream out(&outFile);
    // Ghi header LAS đơn giản
    out << "~Version Information\n";
    out << " VERS.                  2.0:   CWLS LOG ASCII STANDARD - VERSION 2.0\n";
    out << "~Well Information Block\n";
    out << " WELL.                  :   Converted from LIS\n";
    out << "~Curve Information\n";
    out << " DEPTH .M         :   Độ sâu\n";
    out << " VALUE .          :   Giá trị\n";
    out << "~ASCII\n";
    // Ghi dữ liệu từ LIS vào phần ~ASCII
    float depth = 0.0f;
    float step = 1.0f; // Nếu có thông tin step, có thể lấy từ header LIS
    for (float value : floatData)
    {
        out << QString::number(depth, 'f', 3) << " " << QString::number(value, 'f', 6) << "\n";
        depth += step;
    }
    outFile.close();
}
