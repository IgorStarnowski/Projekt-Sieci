QT       += core gui widgets
QT += core network

greaterThan(QT_MAJOR_VERSION, 4): QT += printsupport
CONFIG += console
TARGET = UARSimulator
TEMPLATE = app

SOURCES += main.cpp \
           UARService.cpp \
           dialogarx.cpp \
           klienttcp.cpp \
           mainwindow.cpp \
           UAR.cpp \
           qcustomplot.cpp \
           dialogarx.cpp \
           serwertcp.cpp

HEADERS  += mainwindow.h \
            UAR.h \
            UARService.h \
            dialogarx.h \
            klienttcp.h \
            qcustomplot.h \
            dialogarx.h \
            serwertcp.h

FORMS    += mainwindow.ui \
    dialogarx.ui
