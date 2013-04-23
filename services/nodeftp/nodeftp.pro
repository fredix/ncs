#-------------------------------------------------
#
# Project created by QtCreator 2013-03-27T17:38:36
#
#-------------------------------------------------

QMAKE_CXXFLAGS += -D_FILE_OFFSET_BITS=64

QT       += core \
        network \
        xml \
        testlib

QT       -= gui

TARGET = nodeftp
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += qxt
QXT     += core
TEMPLATE = app


SOURCES += main.cpp \
    nodeftp.cpp \
    CFtpServer.cpp \
    service.cpp

HEADERS += \
    nodeftp.h \
    CFtpServerGlobal.h \
    CFtpServerConfig.h \
    CFtpServer.h \
    service.h \
    main.h


LIBS += -lz \
        -lboost_system \
        -lboost_filesystem-mt \
        -lboost_thread-mt \
        -lQxtCore

INCLUDEPATH += ../../externals/libqxt/
INCLUDEPATH += ../../externals/libqxt/src/core
