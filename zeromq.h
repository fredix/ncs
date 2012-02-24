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
#include <QThread>
#include <QDebug>
#include <QMutex>
#include <QxtCore/QxtCommandOptions>

#include <zmq.hpp>
#include "client/gridfs.h"
#include "nosql.h"

using namespace mongo;
using namespace bson;



class Ztracker : public QObject
{
    Q_OBJECT
public:
    Ztracker(Nosql &a, zmq::context_t *a_context);
    ~Ztracker();

private:
    zmq::context_t *m_context;
    zmq::socket_t *m_socket;
    Nosql &nosql_;

public slots:
    void init();
};



class Zdispatch : public QObject
{
    Q_OBJECT
public:
    Zdispatch(Nosql &a, zmq::context_t *a_context);
    ~Zdispatch();


private:
    zmq::context_t *m_context;
    zmq::socket_t *m_socket;
    Nosql &nosql_;


signals:
    void forward_payload(bson::bo data);
    void payload_cpu(bson::bo data);
    void payload_load(bson::bo data);
    void payload_network(bson::bo data);
    void payload_memory(bson::bo data);
    void payload_uptime(bson::bo data);
    void payload_process(bson::bo data);
    void payload_filesystem(bson::bo data);

public slots:
    void receive_payload();
};



class Zreceive : public QObject
{
    Q_OBJECT
public:
    Zreceive(zmq::context_t *a_context, QString a_port, QString inproc);
    ~Zreceive();
    zmq::context_t *m_context;

private:
    QMutex *m_mutex;
    QString m_inproc;
    QString m_port;
    zmq::socket_t *z_workers;
    zmq::socket_t *z_sender;
    QString m_host;


signals:
    void send_payload();

public slots:
    void init_payload();
};






class Zworker_push : public QObject
{
    Q_OBJECT
public:
    Zworker_push();
    Zworker_push(zmq::context_t *a_context);
    ~Zworker_push();

private:
    zmq::socket_t *z_load_sender;
    zmq::socket_t *z_cpu_sender;
    zmq::socket_t *z_network_sender;
    zmq::socket_t *z_memory_sender;
    zmq::socket_t *z_uptime_sender;
    zmq::socket_t *z_process_sender;
    zmq::socket_t *z_filesystem_sender;

    zmq::message_t *z_message;

    QString m_host;
    int m_port;


public slots:
    void send_payload_load(bson::bo payload);
    void send_payload_cpu(bson::bo payload);
    void send_payload_network(bson::bo payload);
    void send_payload_memory(bson::bo payload);
    void send_payload_uptime(bson::bo payload);
    void send_payload_process(bson::bo payload);
    void send_payload_filesystem(bson::bo payload);
};





class Zeromq : public QObject
{    
    Q_OBJECT
public:
    Zeromq(Nosql &a, QString host, int port);
    ~Zeromq();
    void init();

    zmq::context_t *m_context;

    Zdispatch *dispatch;
    Ztracker *ztracker;

    Zreceive *receive_http;
    Zreceive *receive_xmpp;
    Zworker_push *worker_push;

private:
    QMutex *m_http_mutex;
    QMutex *m_xmpp_mutex;
    Nosql &nosql_;


signals:
    void payload(bo data);
};

#endif // ZEROMQ_H
