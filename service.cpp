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


ZerogwProxy::ZerogwProxy(params a_ncs_params, int port, QObject *parent) : m_ncs_params(a_ncs_params), m_port(port), QObject(parent)
{
    thread = new QThread;

    this->moveToThread(thread);
    thread->start();
    thread->connect(thread, SIGNAL(started()), this, SLOT(init()));

    qDebug() << "ZerogwProxy::ZerogwProxy";
    zeromq_ = Zeromq::getInstance ();
    qDebug() << "ZerogwProxy::getInstance";
}

ZerogwProxy::~ZerogwProxy()
{
    qDebug() << "!!!!!!! ZerogwProxy::CLOSE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!";

    //delete(api_payload);
    zerogw->close();
    worker_payload->close();
}

void ZerogwProxy::init()
{
    qDebug() << "ZerogwProxy::init";

    zerogw = new zmq::socket_t(*zeromq_->m_context, ZMQ_ROUTER);
    QString zgw_uri = "tcp://*:" + QString::number(m_port);
    zerogw->bind (zgw_uri.toLatin1());
    worker_payload = new zmq::socket_t(*zeromq_->m_context, ZMQ_DEALER);
    QString uri = "ipc://" + m_ncs_params.base_directory + "/payloads";

    worker_payload->bind (uri.toLatin1());

    //api_payload = new Api_payload(Api_payload, 0);
    //api_payload2 = new Api_payload(m_ncs_params.base_directory, 1);

    for(int i=0; i<2; i++)
    {
        api_payload_thread[i] = QSharedPointer<Api_payload> (new Api_payload(m_ncs_params.base_directory, 0));
    }

    qDebug() << "ZerogwProxy::init BEFORE PROXY";
    zmq::proxy (*zerogw, *worker_payload, NULL);
    qDebug() << "ZerogwProxy::init AFTER PROXY";

}


Service::Service(params a_ncs_params, QObject *parent) : m_ncs_params(a_ncs_params), QObject(parent)
{
    m_nodetrack = NULL;
    m_nodeftp = NULL;
    m_http_api = NULL;
    m_xmpp_server = NULL;
    m_xmpp_client = NULL;
    zerogwToPayload = NULL;
}

Service::~Service()
{
    qDebug() << "delete http api server";
    if (m_http_api) delete(m_http_api);

    qDebug() << "delete http admin server";
    delete(m_http_admin);

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
    //delete(api_payload);
    if (zerogwToPayload) delete(zerogwToPayload);

    delete(api_node);
    delete(api_workflow);
    delete(api_user);
}


/*
void Service::Http_api_init()
{
    int port;
    m_ncs_params.api_port == 0 ? port = 2502 : port = m_ncs_params.api_port;
    m_api_session.setPort(port);
    m_api_session.setConnector(&m_api_connector);
    m_api_session.setAutoCreateSession(false);
    //Http_api s1(&session);

    m_http_api = new Http_api(m_ncs_params.base_directory, &m_api_session);
    m_api_session.setStaticContentService(m_http_api);
    m_api_session.start();
}
*/


void Service::Http_api_init()
{
    qDebug() << "Service::Http_api_init";
    int port;
    m_ncs_params.api_port == 0 ? port = 2502 : port = m_ncs_params.api_port;


    zerogwToPayload = new ZerogwProxy(m_ncs_params, port);


/*
    zmq::socket_t zerogw (*zeromq_->m_context, ZMQ_ROUTER);
    zerogw.bind ("tcp://*:2503");
    zmq::socket_t worker_payload (*zeromq_->m_context, ZMQ_DEALER);
    worker_payload.bind ("inproc://workers");
    qDebug() << "Service::Http_api_init BEFORE PROXY";

    zmq::proxy (zerogw, worker_payload, NULL);
    qDebug() << "Service::Http_api_init AFTER PROXY";




    QThread *thread_api_payload = new QThread;
    api_payload = new Api_payload(m_ncs_params.base_directory, port);
    api_payload->moveToThread(thread_api_payload);
    thread_api_payload->start();

    QThread *thread_api_payload2 = new QThread;
    api_payload2 = new Api_payload(m_ncs_params.base_directory, port + 101);
    api_payload2->moveToThread(thread_api_payload2);
    thread_api_payload2->start();
*/

    //connect(this, SIGNAL(shutdown()), api_payload, SLOT(destructor()), Qt::BlockingQueuedConnection);
    //connect(api_payload, SIGNAL(forward_payload(BSONObj)), dispatch, SLOT(push_payload(BSONObj)), Qt::QueuedConnection);


    api_node = new Api_node(m_ncs_params.base_directory, port + 1);

    //connect(this, SIGNAL(shutdown()), api_node, SLOT(destructor()), Qt::BlockingQueuedConnection);
    //connect(api_node, SIGNAL(forward_payload(BSONObj)), dispatch, SLOT(push_payload(BSONObj)), Qt::QueuedConnection);

    api_workflow = new Api_workflow(m_ncs_params.base_directory, port + 2);
    //connect(this, SIGNAL(shutdown()), api_workflow, SLOT(destructor()), Qt::BlockingQueuedConnection);
    //connect(api_workflow, SIGNAL(forward_payload(BSONObj)), dispatch, SLOT(push_payload(BSONObj)), Qt::QueuedConnection);


    api_user = new Api_user(m_ncs_params.base_directory, port + 3);
}



void Service::Http_admin_init()
{
    int port;
    m_ncs_params.admin_port == 0 ? port = 2501 : port = m_ncs_params.admin_port;
    m_admin_session.setPort(port);
    m_admin_session.setConnector(&m_admin_connector);
    m_admin_session.setAutoCreateSession(false);
    //Http_api s1(&session);

    m_http_admin = new Http_admin(m_ncs_params.base_directory, &m_admin_session);
    m_admin_session.setStaticContentService(m_http_admin);
    m_admin_session.setSessionCookieName("nodecast");
    m_admin_session.start();
}


void Service::Nodetrack_init()
{
    int port;
    m_ncs_params.tracker_port == 0 ? port = 6969 : port = m_ncs_params.tracker_port;

    m_nodetrack_session.setPort(port);
    m_nodetrack_session.setConnector(&m_nodetrack_connector);

    m_nodetrack = new Nodetrack(m_ncs_params.base_directory, &m_nodetrack_session);

    m_nodetrack_session.setStaticContentService(m_nodetrack);
    m_nodetrack_session.start();
}



void Service::Nodeftp_init()
{      
    int port;
    m_ncs_params.ftp_server_port == 0 ? port = 2121 : port = m_ncs_params.ftp_server_port;

   QThread *node_ftp = new QThread;
   m_nodeftp = new Nodeftp(m_ncs_params.base_directory, port);
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


void Service::Xmpp_init()
{  
    qRegisterMetaType<QXmppLogger::MessageType>("QXmppLogger::MessageType");

    m_xmpp_server = new Xmpp_server(m_ncs_params.domain_name, m_ncs_params.xmpp_client_port, m_ncs_params.xmpp_server_port);
    m_xmpp_client = new Xmpp_client(m_ncs_params.base_directory, m_ncs_params.domain_name, m_ncs_params.xmpp_client_port);
}



void Service::Worker_init()
{
    QThread *worker_pull = new QThread;
    worker_api = new Worker_api(m_ncs_params.base_directory);
    worker_api->moveToThread(worker_pull);
    worker_pull->start();    
    connect(this, SIGNAL(shutdown()), worker_api, SLOT(destructor()), Qt::BlockingQueuedConnection);


}

void Service::link()
{
    if (m_nodeftp) connect(m_http_admin, SIGNAL(create_ftp_user(QString)), m_nodeftp, SLOT(add_ftp_user(QString)), Qt::DirectConnection);
    if (m_nodeftp) connect(api_user, SIGNAL(create_ftp_user(QString)), m_nodeftp, SLOT(add_ftp_user(QString)), Qt::DirectConnection);
}
