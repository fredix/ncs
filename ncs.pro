# -------------------------------------------------
# Project created by QtCreator 2010-08-29T14:47:00
# -------------------------------------------------


QMAKE_CXXFLAGS += -D_FILE_OFFSET_BITS=64

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
    zeromq.cpp \
    xmpp_server.cpp \
    xmpp_client.cpp \
    http_api.cpp \
    alert.cpp \
    worker_api.cpp \
    tracker.cpp \
    nodetrack/nodetrack.cpp \
    nodetrack/util.cpp \
    CFtpServer/CFtpServer.cpp \
    CFtpServer/nodeftp.cpp \
    service.cpp \
    mongodb.cpp \
    http_admin.cpp \
    zerogw.cpp \
    statemachine.cpp
LIBS += ./externals/mongo-cxx-driver/libmongoclient.a \
        -lz \
        -lboost_system \
        -lboost_filesystem-mt \
        -lboost_thread-mt \
        -lQxtCore \
        -lQxtWeb \
#        -lzmq \
        /usr/local/lib/libzmq.so \
        -lqxmpp

INCLUDEPATH += /usr/include/
INCLUDEPATH += ./externals/cppzmq/
INCLUDEPATH += ./externals/libqxt/
INCLUDEPATH += ./externals/libqxt/src/core
INCLUDEPATH += ./externals/libqxt/src/web
INCLUDEPATH += ./externals/mongo-cxx-driver/src
INCLUDEPATH += ./externals/qxmpp/src/base
INCLUDEPATH += ./externals/qxmpp/src/client
INCLUDEPATH += ./externals/qxmpp/src/server

HEADERS += main.h \
    zeromq.h \
    xmpp_server.h \
    xmpp_client.h \
    http_api.h \
    alert.h \
    worker_api.h \
    tracker.h \
    ncs_global.h \
    nodetrack/nodetrack.h \
    nodetrack/util.h \
    CFtpServer/CFtpServer.h \
    CFtpServer/CFtpServerGlobal.h \
    CFtpServer/nodeftp.h \
    CFtpServer/CFtpServerConfig.h \
    service.h \
    mongodb.h \
    http_admin.h \
    zerogw.h \
    statemachine.h
