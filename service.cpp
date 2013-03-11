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

#include "service.h"

Service::Service(QObject *parent) : QObject(parent)
{
    m_nodetrack = NULL;
    m_nodeftp = NULL;
    m_xmpp_server = NULL;
    m_xmpp_client = NULL;
}

Service::~Service()
{
    qDebug() << "delete http server";
    delete(m_http_api);

    qDebug() << "delete tracker";
    if (m_nodetrack) delete(m_nodetrack);

    qDebug() << "delete ftp";
    if (m_nodeftp) delete(m_nodeftp);


    qDebug() << "delete xmpp server";
    if (m_xmpp_server) delete(m_xmpp_server);

    qDebug() << "delete xmpp client";
    if (m_xmpp_client) delete(m_xmpp_client);

    qDebug() << "delete worker api";
    emit shutdown();
    delete(worker_api);
}


void Service::Http_init()
{
    m_session.setPort(4567);
    m_session.setConnector(&m_connector);
    m_session.setAutoCreateSession(false);
    //Http_api s1(&session);

    m_http_api = new Http_api(&m_session);

    m_session.setStaticContentService(m_http_api);

    m_session.setSessionCookieName("nodecast");

    m_session.start();    
}




void Service::Nodetrack_init()
{
    m_nodetrack_session.setPort(6969);
    m_nodetrack_session.setConnector(&m_nodetrack_connector);

    m_nodetrack = new Nodetrack(&m_nodetrack_session);

    m_nodetrack_session.setStaticContentService(m_nodetrack);
    m_nodetrack_session.start();
}



void Service::Nodeftp_init(int port)
{      
   QThread *node_ftp = new QThread;
   m_nodeftp = new Nodeftp(port);
   m_nodeftp->moveToThread(node_ftp);
   node_ftp->start();

   m_nodeftp->connect(node_ftp, SIGNAL(started()), SLOT(ftp_init()));
}

void Service::Tracker_init()
{
    m_tracker_session.setPort(6060);
    m_tracker_session.setConnector(&m_tracker_connector);

    m_tracker = new Tracker(&m_tracker_session);

    m_tracker_session.setStaticContentService(m_tracker);
    m_tracker_session.start();
}


void Service::Xmpp_init(QString domain_name, int xmpp_client_port, int xmpp_server_port)
{
    qRegisterMetaType<QXmppLogger::MessageType>("QXmppLogger::MessageType");

    m_xmpp_server = new Xmpp_server(domain_name, xmpp_client_port, xmpp_server_port);
    m_xmpp_client = new Xmpp_client(domain_name, xmpp_client_port);
}



void Service::Worker_init()
{
    QThread *worker_pull = new QThread;
    worker_api = new Worker_api();
    worker_api->moveToThread(worker_pull);
    worker_pull->start();    
    connect(this, SIGNAL(shutdown()), worker_api, SLOT(destructor()), Qt::BlockingQueuedConnection);


}
