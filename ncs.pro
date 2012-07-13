# -------------------------------------------------
# Project created by QtCreator 2010-08-29T14:47:00
# -------------------------------------------------
QT += network \
    xml \
    testlib
QT -= gui
TARGET = ncs
CONFIG += console
CONFIG -= app_bundle
CONFIG += qxt
QXT     += core web
TEMPLATE = app
SOURCES += main.cpp \
    nosql.cpp \
    zeromq.cpp \
    xmpp_server.cpp \
    xmpp_client.cpp \
    api.cpp \
    http_api.cpp \
    alert.cpp \
    zeromq_api.cpp
LIBS += /usr/local/lib/libmongoclient.a \
        -lboost_system \
        -lboost_filesystem-mt \
        -lboost_thread-mt \
        -lQxtCore \
        -lQxtWeb \
        -lzmq \
        -lqxmpp

INCLUDEPATH += /usr/include/
INCLUDEPATH += ./externals/libqxt/
INCLUDEPATH += ./externals/libqxt/core
INCLUDEPATH += ./externals/libqxt/web
INCLUDEPATH += ./externals/mongodb
INCLUDEPATH += ./externals/qxmpp/src/base
INCLUDEPATH += ./externals/qxmpp/src/client
INCLUDEPATH += ./externals/qxmpp/src/server

HEADERS += main.h \
    nosql.h \
    zeromq.h \
    xmpp_server.h \
    xmpp_client.h \
    api.h \
    http_api.h \
    alert.h \
    zeromq_api.h
