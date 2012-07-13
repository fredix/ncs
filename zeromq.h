/****************************************************************************
**   ncs is the backend's server of nodecast
**   Copyright (C) 2010-2011  Frédéric Logier <frederic@logier.org>
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
#include <QDateTime>
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QTimer>
#include <QUuid>
#include <QxtCore/QxtCommandOptions>

#include <zmq.hpp>
#include "client/gridfs.h"
#include "nosql.h"

using namespace mongo;
using namespace bson;


class Zworker_push : public QObject
{
    Q_OBJECT
public:
    Zworker_push(zmq::context_t *a_context, string a_worker, string a_port);
    ~Zworker_push();
    void push_payload(bson::bo payload);

private:
    string m_worker;
    string m_port;
    zmq::context_t *m_context;
    zmq::socket_t *z_sender;
    zmq::message_t *z_message;

    QString m_host;

    //int m_port;
};



class Ztracker : public QObject
{
    Q_OBJECT
public:
    Ztracker(zmq::context_t *a_context);
    ~Ztracker();
    QTimer *worker_timer;
    QTimer *service_timer;

private:        
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
    void init();
    void worker_update_ticker();
    void service_update_ticker();
};





typedef QSharedPointer<Zworker_push> Zworker_pushPtr;

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
    QMutex *m_mutex;

public slots:
    void receive_payload();
    void bind_server(QString name, QString port);
};



class Zreceive : public QObject
{
    Q_OBJECT
public:
    Zreceive(zmq::context_t *a_context, zmq::socket_t *a_workers, QString inproc);
    ~Zreceive();

private:
    zmq::context_t *m_context;
    QMutex *m_mutex;
    QString m_inproc;
    zmq::socket_t *z_workers;
    zmq::socket_t *z_sender;


signals:
    void send_payload();

public slots:
    void init_payload();
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

    Zdispatch *dispatch;
    Ztracker *ztracker;

    Zreceive *receive_http;
    Zreceive *receive_xmpp;
    Zreceive *receive_zeromq;
    Zworker_push *worker_push;

private:
    static Zeromq *_singleton;
    QMutex *m_http_mutex;
    QMutex *m_xmpp_mutex;
    Nosql *nosql_;
    QString m_smtp_hostname;
    QString m_smtp_username;
    QString m_smtp_password;


signals:
    void payload(bo data);
};

#endif // ZEROMQ_H
