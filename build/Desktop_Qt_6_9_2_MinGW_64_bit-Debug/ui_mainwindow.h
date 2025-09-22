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
    QPushButton *pushButton;
    QPushButton *btnTaoFile;
    QPushButton *btnTachFile;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(400, 200);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        pushButton = new QPushButton(centralwidget);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(20, 20, 161, 40));
        btnTaoFile = new QPushButton(centralwidget);
        btnTaoFile->setObjectName("btnTaoFile");
        btnTaoFile->setGeometry(QRect(20, 70, 161, 41));
        btnTachFile = new QPushButton(centralwidget);
        btnTachFile->setObjectName("btnTachFile");
        btnTachFile->setGeometry(QRect(200, 20, 161, 40));
        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "X\341\273\255 l\303\275 file LAS", nullptr));
        pushButton->setText(QCoreApplication::translate("MainWindow", "Covert LAS file", nullptr));
        btnTaoFile->setText(QCoreApplication::translate("MainWindow", "T\341\272\241o file Test", nullptr));
        btnTachFile->setText(QCoreApplication::translate("MainWindow", "Covert LAS file", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
