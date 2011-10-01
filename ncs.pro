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
LIBS += -L./externals/mongodb-2.0/ \
    -L./externals/qxmpp-0.3.0/lib/ \
    -lmongoclient \
    -lboost_system \
    -lboost_filesystem \
    -lboost_thread-mt \
    -lmemcached \
    -lQxtCore \
    -lzmq \
    -lqxmpp
#INCLUDEPATH += /usr/local/include
INCLUDEPATH += ./externals/qxmpp-0.3.0/src/
INCLUDEPATH += ./externals/mongodb-2.0/
HEADERS += main.h \
    nosql.h \
    payload.h \
    zeromq.h \
    xmpp_server.h \
    xmpp_client.h \
    api.h \
    http_api.h
