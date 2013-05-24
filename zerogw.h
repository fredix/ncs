/****************************************************************************
**   ncs is the backend's server of nodecast
**   Copyright (C) 2010-2013  Frédéric Logier <frederic@logier.org>
**
**   https://github.com/nodecast/ncs
**
**   This program is free software: you can redistribute it and/or modify
**   it under the terms of the GNU Affero General Public License as
**   published by the Free Software Foundation, either version 3 of the
**   License, or (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**   GNU Affero General Public License for more details.
**
**   You should have received a copy of the GNU Affero General Public License
**   along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#ifndef ZEROGW_H
#define ZEROGW_H

#include <QObject>
#include <QStateMachine>
#include <QDateTime>
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QTimer>
#include <QUuid>
#include <QSocketNotifier>
#include <QxtCore/QxtCommandOptions>
#include <QxtJSON>
#include <QCryptographicHash>
#include <QRunnable>
#include <QThreadPool>
#include <QCryptographicHash>

#include <zmq.hpp>
#include "mongodb.h"
#include "mongo/client/gridfs.h"
#include "zeromq.h"



enum ZerogwHeader {
    HTTP_METHOD,
    URI,
    X_user_token,
    X_user_email,
    X_user_password,
    X_node_uuid,
    X_node_password,
    X_workflow_uuid,
    X_payload_filename,
    X_payload_type,
    X_payload_mime,
    X_payload_action,
    X_session_uuid,
    BODY
};




typedef QMap<int, ZerogwHeader> IntToZerogwHeaderPayload;
typedef QMap<int, ZerogwHeader> IntToZerogwHeaderSession;
typedef QMap<int, ZerogwHeader> IntToZerogwHeaderApp;
typedef QMap<int, ZerogwHeader> IntToZerogwHeaderFtpauth;



class GridfsTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    GridfsTask(QHash <QString, QString> a_zerogw);
    void run();
    QByteArray *m_requestContent;

private:
    Mongodb *mongodb_;
    QHash <QString, QString> m_zerogw;


signals:
    void forward_payload(BSONObj data);
};



class Zerogw : public QObject
{
    Q_OBJECT
public:
    Zerogw(QString basedirectory, int port, QString ipc_name, QObject *parent = 0);
    ~Zerogw();

protected:
    QString m_ipc_name;
    QBool checkAuth(QString token, BSONObjBuilder &payload_builder, BSONObj &a_user);
    QBool checkAuth(QString token, QHash <QString, QString> &hash_builder, BSONObj &a_user);
    QString buildResponse(QString action, QString data1, QString data2="");
    QString m_basedirectory;
    int m_port;
    Mongodb *mongodb_;
    Zeromq *zeromq_;
    zmq::context_t *m_context;

    QSocketNotifier *check_http_data;
    zmq::socket_t *m_socket_zerogw;
    QMutex *m_mutex_http;
    zmq::socket_t *z_push_api;

private slots:
    void init();
    virtual void receive_http_payload()=0;
    void forward_payload_to_zpull(BSONObj payload);    
};



class Api_payload : public Zerogw
{
    Q_OBJECT
public:
    Api_payload(QString basedirectory, int port, QString ipc_name);


private:
    IntToZerogwHeaderPayload enumToZerogwHeaderPayload;

private slots:
    void receive_http_payload();
};



class Api_node : public Zerogw
{
    Q_OBJECT
public:
    Api_node(QString basedirectory, int port);

private slots:
    void receive_http_payload();
};



class Api_workflow : public Zerogw
{
    Q_OBJECT
public:
    Api_workflow(QString basedirectory, int port);

private slots:
    void receive_http_payload();
};


class Api_user : public Zerogw
{
    Q_OBJECT
public:
    Api_user(QString basedirectory, int port);

signals:
    void create_ftp_user(QString, QString);

private slots:
    void receive_http_payload();
};



class Api_session : public Zerogw
{
    Q_OBJECT
public:
    Api_session(QString basedirectory, int port);

private:
    IntToZerogwHeaderSession enumToZerogwHeaderSession;

private slots:
    void receive_http_payload();
};


class Api_app : public Zerogw
{
    Q_OBJECT
public:
    Api_app(QString basedirectory, int port);


private:
    IntToZerogwHeaderApp enumToZerogwHeaderApp;


private slots:
    void receive_http_payload();
};


class Api_ftpauth : public Zerogw
{
    Q_OBJECT
public:
    Api_ftpauth(QString basedirectory, int port);

private:
    IntToZerogwHeaderFtpauth enumToZerogwHeaderFtpauth;

private slots:
    void receive_http_payload();
};


#endif // ZEROGW_H
