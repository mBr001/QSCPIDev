#-------------------------------------------------
#
# Project created by QtCreator 2011-10-13T22:23:25
#
#-------------------------------------------------

QT       -= gui

TARGET = QSCPIDev
TEMPLATE = lib
CONFIG += staticlib

SOURCES += qscpidev.cpp \
    qserial.cpp

HEADERS += qscpidev.h \
    qserial.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
