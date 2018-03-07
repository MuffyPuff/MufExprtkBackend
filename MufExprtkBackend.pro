#-------------------------------------------------
#
# Project created by QtCreator 2017-12-30T23:52:38
#
#-------------------------------------------------

QT       += core gui widgets

DESTDIR = ../../../bin

TARGET = MufExprtkBackend
TEMPLATE = lib

SOURCES += \
        src/mufexprtkbackend.cpp \
        lib/exprtk.hpp

HEADERS += \
        src/mufexprtkbackend.h

INCLUDEPATH += $$PWD/lib

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
