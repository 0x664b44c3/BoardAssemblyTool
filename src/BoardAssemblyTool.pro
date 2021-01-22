#-------------------------------------------------
#
# Project created by QtCreator 2014-07-23T18:06:33
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ExtractMillingFromEagleBord
TEMPLATE = app


SOURCES += main.cpp\
        wndmain.cpp \
    board.cpp \
    layer.cpp \
    viewer.cpp \
    previewwidget.cpp \
    bommodel.cpp

HEADERS  += wndmain.h \
    board.h \
    eagle.h \
    dwgElements.h \
    layer.h \
    viewer.h \
    previewwidget.h \
    overviewviewer.h \
    bommodel.h

FORMS    += wndmain.ui

OTHER_FILES += \
    demo.xml
