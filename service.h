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

#ifndef SERVICE_H
#define SERVICE_H

#include <QThread>
#include "nosql.h"
#include "zeromq.h"
#include "http_api.h"
#include "tracker.h"
#include "nodetrack/nodetrack.h"
#include "CFtpServer/nodeftp.h"
#include "worker_api.h"
#include <QxtHttpServerConnector>
#include <QxtHttpSessionManager>
#include "xmpp_server.h"
#include "xmpp_client.h"

class Service : public QObject
{
    Q_OBJECT
public:
    Service(QObject *parent = 0);
    ~Service();

    void Http_init();
    void Tracker_init();
    void Nodetrack_init();
    void Nodeftp_init(int port);
    void Xmpp_init(QString domain_name, int xmpp_client_port, int xmpp_server_port);
    void Worker_init();
    Worker_api *worker_api;


private:
    QxtHttpServerConnector m_connector;
    QxtHttpSessionManager m_session;
    Http_api *m_http_api;
    Xmpp_server *m_xmpp_server;
    Xmpp_client *m_xmpp_client;

    QxtHttpServerConnector m_tracker_connector;
    QxtHttpServerConnector m_nodetrack_connector;
    QxtHttpSessionManager m_tracker_session;
    QxtHttpSessionManager m_nodetrack_session;
    Tracker *m_tracker;
    Nodetrack *m_nodetrack;
    Nodeftp *m_nodeftp;


signals:
    void shutdown();
};

#endif // SERVICE_H
