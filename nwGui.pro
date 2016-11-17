#-------------------------------------------------
#
# Project created by QtCreator 2015-06-01T12:44:57
#
#-------------------------------------------------

qnx {
 target.path = /tmp/$${TARGET}/bin
 INSTALLS += target
}

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH = /home/kan/qtserialport/serialport-build/include/QtSerialPort

TARGET = nwmachine
TEMPLATE = app
CONFIG += serialport

SOURCES += main.cpp\
        mainwindow.cpp \
    kama.pb.cc \
    framehandler.cpp \
    qcustomplot.cpp \
    settingsdialog.cpp \
    logwindow.cpp \
    guanoworkdialog.cpp

HEADERS  += mainwindow.h \
    kama.pb.h \
    convertation.h \
    framehandler.h \
    qcustomplot.h \
    settingsdialog.h \
    logwindow.h \
    guanoworkdialog.h

LIBS += -lprotobuf -lQtSerialPort

FORMS    += mainwindow.ui \
    settingsdialog.ui \
    logwindow.ui \
    guanoworkdialog.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    kama.proto
