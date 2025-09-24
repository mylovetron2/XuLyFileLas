/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *btnTaoFile;
    QPushButton *btnTachFile;
    QPushButton *btnDrawTxtChart;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(400, 200);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        btnTaoFile = new QPushButton(centralwidget);
        btnTaoFile->setObjectName("btnTaoFile");
        btnTaoFile->setGeometry(QRect(20, 70, 161, 41));
        btnTachFile = new QPushButton(centralwidget);
        btnTachFile->setObjectName("btnTachFile");
        btnTachFile->setGeometry(QRect(20, 20, 161, 40));
        btnDrawTxtChart = new QPushButton(centralwidget);
        btnDrawTxtChart->setObjectName("btnDrawTxtChart");
        btnDrawTxtChart->setGeometry(QRect(200, 20, 180, 40));
        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "X\341\273\255 l\303\275 file LAS", nullptr));
        btnTaoFile->setText(QCoreApplication::translate("MainWindow", "X\341\273\255 l\303\275 file \304\221\341\272\247u v\303\240o", nullptr));
        btnTachFile->setText(QCoreApplication::translate("MainWindow", "Covert LAS file", nullptr));
        btnDrawTxtChart->setText(QCoreApplication::translate("MainWindow", "V\341\272\275 \304\221\341\273\223 th\341\273\213 TXT", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
