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
    payload.cpp \
    zeromq.cpp \
    xmpp_server.cpp \
    xmpp_client.cpp \
    api.cpp \
    http_api.cpp
LIBS += ./externals/mongodb/libmongoclient.a \
    ./externals/qxmpp/lib/libqxmpp.a \
    -lboost_system \
    -lboost_filesystem \
    -lboost_thread-mt \
    -lmemcached \
    -lQxtCore \
    -lQxtWeb \
    -lzmq \
    -lqjson
INCLUDEPATH += /usr/include/qxt/
INCLUDEPATH += /usr/include/qxt/QxtCore
INCLUDEPATH += /usr/include/qxt/QxtWeb
INCLUDEPATH += ./externals/
INCLUDEPATH += ./externals/qxmpp/src/
HEADERS += main.h \
    nosql.h \
    payload.h \
    zeromq.h \
    xmpp_server.h \
    xmpp_client.h \
    api.h \
    http_api.h
