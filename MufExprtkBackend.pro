#-------------------------------------------------
#
# Project created by QtCreator 2017-12-30T23:52:38
#
#-------------------------------------------------

QT       += core gui widgets

DESTDIR = ../StrahCalc/lib

TARGET = MufExprtkBackend
TEMPLATE = lib

SOURCES += \
        src/mufexprtkbackend.cpp \
        include/exprtk.hpp

HEADERS += \
        src/mufexprtkbackend.h

INCLUDEPATH += $$PWD/include

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
