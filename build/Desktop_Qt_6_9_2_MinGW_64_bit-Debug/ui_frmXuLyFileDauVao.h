/********************************************************************************
** Form generated from reading UI file 'frmXuLyFileDauVao.ui'
**
** Created by: Qt User Interface Compiler version 6.9.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FRMXULYFILEDAUVAO_H
#define UI_FRMXULYFILEDAUVAO_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Form
{
public:
    QHBoxLayout *horizontalLayout;
    QListWidget *listWidgetTxtFiles;
    QWidget *chartContainer;

    void setupUi(QWidget *Form)
    {
        if (Form->objectName().isEmpty())
            Form->setObjectName("Form");
        Form->resize(800, 500);
        horizontalLayout = new QHBoxLayout(Form);
        horizontalLayout->setObjectName("horizontalLayout");
        listWidgetTxtFiles = new QListWidget(Form);
        listWidgetTxtFiles->setObjectName("listWidgetTxtFiles");
        listWidgetTxtFiles->setMinimumWidth(220);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(listWidgetTxtFiles->sizePolicy().hasHeightForWidth());
        listWidgetTxtFiles->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(listWidgetTxtFiles);

        chartContainer = new QWidget(Form);
        chartContainer->setObjectName("chartContainer");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(chartContainer->sizePolicy().hasHeightForWidth());
        chartContainer->setSizePolicy(sizePolicy1);

        horizontalLayout->addWidget(chartContainer);


        retranslateUi(Form);

        QMetaObject::connectSlotsByName(Form);
    } // setupUi

    void retranslateUi(QWidget *Form)
    {
        Form->setWindowTitle(QCoreApplication::translate("Form", "Ph\303\242n t\303\255ch file TXT", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Form: public Ui_Form {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FRMXULYFILEDAUVAO_H
