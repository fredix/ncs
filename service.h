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
#include "mongodb.h"
#include "zeromq.h"
#include "http_api.h"
#include "http_admin.h"
#include "tracker.h"
#include "nodetrack/nodetrack.h"
#include "CFtpServer/nodeftp.h"
#include "worker_api.h"
#include <QxtHttpServerConnector>
#include <QxtHttpSessionManager>
#include "xmpp_server.h"
#include "xmpp_client.h"
#include "ncs_global.h"
#include "zerogw.h"

class Service : public QObject
{
    Q_OBJECT
public:
    Service(params a_ncs_params, QObject *parent = 0);
    ~Service();

    void Http_api_init();
    void Http_admin_init();
    void Tracker_init();
    void Nodetrack_init();
    void Nodeftp_init();
    void Xmpp_init();
    void Worker_init();
    void link();
    Worker_api *worker_api;
    Api_payload *api_payload;
    Api_node *api_node;
    Api_workflow *api_workflow;

private:
    Http_admin *m_http_admin;
    Nodeftp *m_nodeftp;
    Http_api *m_http_api;
    Xmpp_server *m_xmpp_server;
    Xmpp_client *m_xmpp_client;

    QxtHttpServerConnector m_tracker_connector;
    QxtHttpServerConnector m_nodetrack_connector;
    QxtHttpServerConnector m_admin_connector;
    QxtHttpServerConnector m_api_connector;

    QxtHttpSessionManager m_tracker_session;
    QxtHttpSessionManager m_nodetrack_session;
    QxtHttpSessionManager m_admin_session;
    QxtHttpSessionManager m_api_session;

    Tracker *m_tracker;
    Nodetrack *m_nodetrack;

    params m_ncs_params;

signals:
    void shutdown();
};

#endif // SERVICE_H
