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

#include <zmq.hpp>
#include "mongodb.h"
#include "mongo/client/gridfs.h"
#include "zeromq.h"

class Zerogw : public QObject
{
    Q_OBJECT
public:
    Zerogw(QString basedirectory, int port, QObject *parent = 0);
    ~Zerogw();

protected:
    QBool checkAuth(QString token, BSONObjBuilder &payload_builder, BSONObj &a_user);
    QString buildResponse(QString action, QString data1, QString data2="");
    QString m_basedirectory;

    Mongodb *mongodb_;
    Zeromq *zeromq_;
    zmq::context_t *m_context;

    QSocketNotifier *check_http_data;
    zmq::socket_t *m_socket_zerogw;
    QMutex *m_mutex_http;
    zmq::message_t *m_message;
    QMutex *m_mutex;

    zmq::socket_t *z_push_api;
    zmq::message_t *z_message;

signals:
    void forward_payload(BSONObj data);


private slots:
    virtual void receive_http_payload()=0;
    void forward_payload_to_zpull(BSONObj payload);
};



class Api_payload : public Zerogw
{
    Q_OBJECT
public:
    Api_payload(QString basedirectory, int port);


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

private slots:
    void receive_http_payload();
};



#endif // ZEROGW_H
