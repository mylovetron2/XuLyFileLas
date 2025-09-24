QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += charts
QT += uitools

TARGET = add_dosau_las
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           LisFile2.cpp \
           frmXuLyFileDauVao.cpp

HEADERS += mainwindow.h \
           LisFile2.h \
           frmXuLyFileDauVao.h

FORMS += mainwindow.ui \
    frmXuLyFileDauVao.ui
