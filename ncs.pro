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
    http_api.cpp
LIBS += /usr/local/lib/libmongoclient.a \
        -lboost_system \
        -lboost_filesystem-mt \
        -lboost_thread-mt \
        -lmemcached \
        -lQxtCore \
        -lQxtWeb \
        -lzmq \
        -lqxmpp

INCLUDEPATH += /usr/include/
INCLUDEPATH += /usr/include/qxt/
INCLUDEPATH += /usr/include/qxt/QxtCore
INCLUDEPATH += /usr/include/qxt/QxtWeb
INCLUDEPATH += ./externals/mongodb-src-r2.0.3
INCLUDEPATH += ./externals/qxmpp-0.3.0/src/

HEADERS += main.h \
    nosql.h \
    zeromq.h \
    xmpp_server.h \
    xmpp_client.h \
    api.h \
    http_api.h
