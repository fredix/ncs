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

#ifndef XMPP_CLIENT_H
#define XMPP_CLIENT_H

#include <QObject>
#include <QDebug>
#include <QVariant>
#include <QUuid>
#include <QBuffer>
#include <QxtJSON>

#include "QXmppMessage.h"
#include "QXmppLogger.h"
#include "QXmppClient.h"
#include "QXmppIncomingClient.h"
#include <QXmppTransferManager.h>

#include "nosql.h"
#include "zeromq.h"


class Xmpp_client : public QXmppClient
{    
    Q_OBJECT

public:
    Xmpp_client(QString a_domain, int a_xmpp_client_port, QObject *parent = 0);
    ~Xmpp_client();

private:
    QString m_domain;
    int m_xmpp_client_port;
    QXmppLogger m_logger;
    QXmppPresence subscribe;
    QBuffer *m_buffer;
    //QXmppTransferJob *m_job;

    //zmq::context_t *m_context;
    zmq::socket_t *z_push_api;
    zmq::message_t *z_message;

    Nosql *nosql_;
    Zeromq *zeromq_;
    bool checkAuth(QString credentials, BSONObjBuilder &payload);
    QString buildResponse(QString action, QString data1, QString data2="");


public slots:
    void connectedToServer();
    void connectedError();
    void messageReceived(const QXmppMessage&);
    void presenceReceived(const QXmppPresence& presence);

private slots:
    void job_finished();
    void file_received(QXmppTransferJob *job);
    void job_error(QXmppTransferJob::Error error);
    void job_progress(qint64 done, qint64 total);
};

#endif // XMPP_CLIENT_H
