QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += charts

TARGET = add_dosau_las
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           LisFile2.cpp

HEADERS += mainwindow.h \
           LisFile2.h

FORMS += mainwindow.ui
