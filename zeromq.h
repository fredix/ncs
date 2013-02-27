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

#ifndef ZEROMQ_H
#define ZEROMQ_H

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

#include <zmq.hpp>
#include "nosql.h"
#include "mongo/client/gridfs.h"

using namespace mongo;
using namespace bson;


class Zworker_push : public QObject
{
    Q_OBJECT
public:
    Zworker_push(zmq::context_t *a_context, string a_worker, string a_port);
    ~Zworker_push();
    void push_payload(bson::bo a_payload);
    void stream_payload(bson::bo a_payload);

private:    
    Nosql *nosql_;
    string m_worker;
    string m_port;
    zmq::context_t *m_context;
    zmq::socket_t *z_sender;
    zmq::message_t *z_message;
    QList <BSONObj> m_payload_list;
    QMutex *m_mutex;

    QString m_host;

    //int m_port;
};


class Zstream_push : public QObject
{
    Q_OBJECT
public:
    Zstream_push(zmq::context_t *a_context);
    ~Zstream_push();

private:
    QSocketNotifier *check_stream;
    Nosql *nosql_;
    string m_worker;
    string m_port;
    zmq::context_t *m_context;
    zmq::socket_t *z_stream;
    zmq::message_t *z_message;
    QMutex *m_mutex;

public slots:
    void destructor();

private slots:
    void stream_payload();
};


class Ztracker : public QObject
{
    Q_OBJECT
public:
    Ztracker(zmq::context_t *a_context);
    ~Ztracker();
    QTimer *worker_timer;

private:        
    QSocketNotifier *check_tracker;
    zmq::context_t *m_context;
    QHash<QString, Zworker_push*> workers_push;
    int get_available_port();
    zmq::socket_t *m_socket;
    zmq::message_t *m_message;

    zmq::socket_t *m_data_socket;
    zmq::message_t *m_data_message;

    Nosql *nosql_;     
    QMutex *m_mutex;

    zmq::socket_t *z_workers;
    zmq::socket_t *z_sender;

signals:
    void sendAlert(QString worker);   
    void create_server(QString name, QString port);

public slots:
    void destructor();

private slots:
    void receive_payload();
    void worker_update_ticker();
};





typedef QSharedPointer<Zworker_push> Zworker_pushPtr;

class Zpull : public QThread
{
    Q_OBJECT
public:
    Zpull(zmq::context_t *a_context);
    ~Zpull();

private:                
    QSocketNotifier *check_http_data;
    QSocketNotifier *check_zeromq_data;
    QSocketNotifier *check_worker_response;
    zmq::context_t *m_context;
    zmq::socket_t *m_socket_http;
    zmq::socket_t *m_socket_workers;
    zmq::socket_t *m_socket_zeromq;
    QMutex *m_mutex_zeromq;
    QMutex *m_mutex_http;
    QMutex *m_mutex_worker_response;

signals:
    void forward_payload(BSONObj data);

public slots:
    void destructor();

private slots:
    void receive_http_payload();
    void receive_zeromq_payload();
    void worker_response();
};




class Zdispatch : public QObject
{
    Q_OBJECT
public:
    Zdispatch(zmq::context_t *a_context);
    ~Zdispatch();


private:
    zmq::context_t *m_context;
    zmq::socket_t *m_socket;
    Nosql *nosql_;
    //QHash<QString, Zworker_push*> workers_push;
    QHash<QString, Zworker_pushPtr> workers_push;
    QList <QString> worker_name;
    QList <BSONObj> workflow_list;
    QList <BSONObj> worker_list;
    QMutex *m_mutex_replay_payload;
    QMutex *m_mutex_push_payload;

    QTimer *replay_payload_timer;

signals:
    void emit_pubsub(bson::bo);


private slots:
    void replay_payload();

public slots:
    void bind_server(QString name, QString port);
    void push_payload(BSONObj data);
    void destructor();

};



class Zeromq : public QObject
{    
    Q_OBJECT
public:
    static Zeromq *getInstance();
    Zeromq();
    ~Zeromq();
    void init();

    zmq::context_t *m_context;

    Zpull *pull;
    Zdispatch *dispatch;
    Ztracker *ztracker;
    Zstream_push *stream_push;

    //Zreceive *receive_http;
    //Zreceive *receive_xmpp;
    //Zreceive *receive_zeromq;
    Zworker_push *worker_push;


private:
    QThread *thread_dispatch;
    QThread *thread_tracker;
    QThread *thread_pull;
    QThread *thread_stream;


    zmq::socket_t *z_workers;
    QTimer *pull_timer;
    static Zeromq *_singleton;
    QMutex *m_http_mutex;
    QMutex *m_xmpp_mutex;
    Nosql *nosql_;

signals:
    void payload(bo data);
    void shutdown();
};

#endif // ZEROMQ_H
