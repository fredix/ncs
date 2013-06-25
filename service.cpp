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
    qDebug() << "ZerogwProxy::ZerogwProxy";
    zeromq_ = Zeromq::getInstance ();
    qDebug() << "ZerogwProxy::getInstance";
}

ZerogwProxy::~ZerogwProxy()
{
    qDebug() << "!!!!!!! ZerogwProxy::CLOSE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    check_zerogw->setEnabled(false);
    check_reply->setEnabled(false);

    zerogw->close();
    worker_payload->close();

    qDebug() << "!!!!!!! ApiPayload clear !!!!!!!!!!!!!!!!!!!!!!!";
    api_payload_thread.clear();

    qDebug() << "!!!!!!! ApiPayload::wait 0 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    thread_payload[0]->wait();
    qDebug() << "!!!!!!! ApiPayload::wait 1 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    thread_payload[1]->wait();

    delete(check_zerogw);
    delete(check_reply);
}

void ZerogwProxy::init()
{
    qDebug() << "ZerogwProxy::init";


    zerogw = new zmq::socket_t(*zeromq_->m_context, ZMQ_ROUTER);
    QString zgw_uri = "tcp://*:" + QString::number(m_port);
    zerogw->bind (zgw_uri.toLatin1());


    int socket_zerogw_fd;
    size_t socket_size = sizeof(socket_zerogw_fd);
    zerogw->getsockopt(ZMQ_FD, &socket_zerogw_fd, &socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << socket_zerogw_fd << " errno : " << zmq_strerror (errno);

    check_zerogw = new QSocketNotifier(socket_zerogw_fd, QSocketNotifier::Read, this);
    connect(check_zerogw, SIGNAL(activated(int)), this, SLOT(receive_zerogw()), Qt::DirectConnection);



    worker_payload = new zmq::socket_t(*zeromq_->m_context, ZMQ_DEALER);
    QString uri = "ipc://" + m_ncs_params.base_directory + "/payloads_" + QString::number(m_port);
    worker_payload->bind (uri.toLatin1());


    int socket_reply_fd;
    size_t socket_reply_size = sizeof(socket_reply_fd);
    worker_payload->getsockopt(ZMQ_FD, &socket_reply_fd, &socket_reply_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << socket_reply_fd << " errno : " << zmq_strerror (errno);

    check_reply = new QSocketNotifier(socket_reply_fd, QSocketNotifier::Read, this);
    connect(check_reply, SIGNAL(activated(int)), this, SLOT(reply_payload()), Qt::DirectConnection);


    // 2 threads to receive HTTP payload from zerogw API
    // should be enough for everybody :)
    for(int i=0; i<2; i++)
    {
        thread_payload[i] = new QThread(this);
        api_payload_thread[i] = QSharedPointer<Api_payload> (new Api_payload(m_ncs_params.base_directory, 0, "/payloads_" + QString::number(m_port)), &QObject::deleteLater);

        connect(thread_payload[i], SIGNAL(started()), api_payload_thread[i].data(), SLOT(init()));
        connect(api_payload_thread[i].data(), SIGNAL(destroyed()), thread_payload[i], SLOT(quit()), Qt::DirectConnection);

        api_payload_thread[i]->moveToThread(thread_payload[i]);
        thread_payload[i]->start();
    }


   /* qDebug() << "ZerogwProxy::init BEFORE PROXY";
    * Do not use because of blocked zmq device.
    zmq::proxy (*zerogw, *worker_payload, NULL);
    qDebug() << "ZerogwProxy::init AFTER PROXY";
*/
}

void ZerogwProxy::receive_zerogw()
{
    check_zerogw->setEnabled(false);
  //  qDebug() << "ZerogwProxy::receive_zerogw !!!!!!!!!";
    //int counter=0;

    while (true)
    {
            zmq::message_t message;
            qint32 events = 0;
            std::size_t eventsSize = sizeof(events);

          //  qDebug() << "MULTIPART !!!!!!!!!! : " << counter;
            //  Process all parts of the message
            //bool res = zerogw->recv(&message, ZMQ_NOBLOCK);
            bool res = zerogw->recv(&message, ZMQ_NOBLOCK);
            if (!res && zmq_errno () == EAGAIN) break;


            zerogw->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);
            worker_payload->send(message, events? ZMQ_SNDMORE: 0);

            //std::cout << "ZMQ_EVENTS : " <<  events << std::endl;

            //QString tmp = QString::fromAscii((char*)message.data(), message.size());
            //qDebug() << "MESSAGE " << tmp;

        //    counter++;
    }
     check_zerogw->setEnabled(true);
}




void ZerogwProxy::reply_payload()
{
    check_reply->setEnabled(false);
    //qDebug() << "ZerogwProxy::reply_payload !!!!!!!!!";

    //int counter=0;

    while (true)
    {
        zmq::message_t message;
        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);

        bool res = worker_payload->recv(&message, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        worker_payload->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);
        zerogw->send(message, events? ZMQ_SNDMORE: 0);

      //  std::cout << "ZMQ_EVENTS : " <<  events << std::endl;


      //  QString tmp = QString::fromAscii((char*)message.data(), message.size());
      //  qDebug() << "MESSAGE " << tmp;

      //  counter++;
    }

    check_reply->setEnabled(true);
}




Service::Service(params a_ncs_params, QObject *parent) : m_ncs_params(a_ncs_params), QObject(parent)
{
    m_nodetrack = NULL;
    m_nodeftp = NULL;
   // m_http_api = NULL;
    m_xmpp_server = NULL;
    m_xmpp_client = NULL;
   // zerogwToPayload[0] = NULL;
   // zerogwToPayload[1] = NULL;
}

Service::~Service()
{
    //qDebug() << "delete http api server";
    //if (m_http_api) delete(m_http_api);

    qDebug() << "delete http admin server";
    m_admin_session.deleteLater();
    delete(m_http_admin);

    if (m_nodetrack)
    {
        qDebug() << "delete tracker";
        m_nodetrack_session.deleteLater();
        delete(m_nodetrack);
    }

    qDebug() << "delete ftp";
    if (m_nodeftp) {
        m_nodeftp->deleteLater();
        node_thread_ftp->wait();
    }

    qDebug() << "delete xmpp server";
    if (m_xmpp_server)
    {
        m_xmpp_server->deleteLater();
        thread_xmpp_server->wait();
    }

    qDebug() << "delete xmpp client";
    if (m_xmpp_client)
    {
        m_xmpp_client->deleteLater();
        thread_xmpp_client->wait();
    }


    qDebug() << "delete worker api";
    worker_api->deleteLater();
    worker_thread_api->wait();

/*
    if (zerogwToPayload[0])
    {
     //   delete(zerogwToPayload[0]);
       // zerogwToPayload[0]->thread->quit();

        //while(!zerogwToPayload[0]->thread->isFinished ()){};

        qDebug() << "zerogwToPayload[0]->deleteLater();";
        zerogwToPayload[0]->deleteLater();

    }
    if (zerogwToPayload[1])
    {
    //    delete(zerogwToPayload[1]);
    //    zerogwToPayload[1]->thread->quit();
    //    while(!zerogwToPayload[1]->thread->isFinished ()){};


        qDebug() << "zerogwToPayload[1]->deleteLater();";
        zerogwToPayload[1]->deleteLater();
    }
*/

    qDebug() << "zerogw_payload->deleteLater();";
    zerogw_payload->deleteLater();
    thread_ZerogwProxy->wait();

    qDebug() << "zerogw_payload2->deleteLater();";
    zerogw_payload2->deleteLater();
    thread_ZerogwProxy2->wait();


    qDebug() << "api_echo->deleteLater();";
    api_echo->deleteLater();
    thread_api_echo->wait();

    qDebug() << "api_node->deleteLater();";
    api_node->deleteLater();
    thread_api_node->wait();

    qDebug() << "api_workflow->deleteLater();";
    api_workflow->deleteLater();
    thread_api_workflow->wait();

    qDebug() << "api_user->deleteLater();";
    api_user->deleteLater();
    thread_api_user->wait();


    qDebug() << "api_session->deleteLater();";
    api_session->deleteLater();
    thread_api_session->wait();


    qDebug() << "api_app->deleteLater();";
    api_app->deleteLater();
    thread_api_app->wait();

    qDebug() << "api_ftp->deleteLater();";
    api_ftp->deleteLater();
    thread_api_ftp->wait();

   /* delete(api_node);
    delete(api_workflow);
    delete(api_user);*/
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


    //zerogwToPayload[0] = new ZerogwProxy(m_ncs_params, port);
   // zerogwToPayload[1] = new ZerogwProxy(m_ncs_params, port+1);


    thread_api_echo = new QThread(this);
    api_echo = new Api_echo(m_ncs_params.base_directory, 2500);

    connect(thread_api_echo, SIGNAL(started()), api_echo, SLOT(init()));
    connect(api_echo, SIGNAL(destroyed()), thread_api_echo, SLOT(quit()), Qt::DirectConnection);

    api_echo->moveToThread(thread_api_echo);
    thread_api_echo->start();


    thread_ZerogwProxy = new QThread(this);
    zerogw_payload = new ZerogwProxy(m_ncs_params, port);

    connect(thread_ZerogwProxy, SIGNAL(started()), zerogw_payload, SLOT(init()));
    connect(zerogw_payload, SIGNAL(destroyed()), thread_ZerogwProxy, SLOT(quit()), Qt::DirectConnection);

    zerogw_payload->moveToThread(thread_ZerogwProxy);
    thread_ZerogwProxy->start();



    thread_ZerogwProxy2 = new QThread(this);
    zerogw_payload2 = new ZerogwProxy(m_ncs_params, port+1);

    connect(thread_ZerogwProxy2, SIGNAL(started()), zerogw_payload2, SLOT(init()));
    connect(zerogw_payload2, SIGNAL(destroyed()), thread_ZerogwProxy2, SLOT(quit()), Qt::DirectConnection);

    zerogw_payload2->moveToThread(thread_ZerogwProxy2);
    thread_ZerogwProxy2->start();



    thread_api_node = new QThread(this);
    api_node = new Api_node(m_ncs_params.base_directory, port + 100);

    connect(thread_api_node, SIGNAL(started()), api_node, SLOT(init()));
    connect(api_node, SIGNAL(destroyed()), thread_api_node, SLOT(quit()), Qt::DirectConnection);

    api_node->moveToThread(thread_api_node);
    thread_api_node->start();



    thread_api_workflow = new QThread(this);
    api_workflow = new Api_workflow(m_ncs_params.base_directory, port + 101);

    connect(thread_api_workflow, SIGNAL(started()), api_workflow, SLOT(init()));
    connect(api_workflow, SIGNAL(destroyed()), thread_api_workflow, SLOT(quit()), Qt::DirectConnection);

    api_workflow->moveToThread(thread_api_workflow);
    thread_api_workflow->start();




    thread_api_user = new QThread(this);
    api_user = new Api_user(m_ncs_params.base_directory, port + 102);

    connect(thread_api_user, SIGNAL(started()), api_user, SLOT(init()));
    connect(api_user, SIGNAL(destroyed()), thread_api_user, SLOT(quit()), Qt::DirectConnection);

    api_user->moveToThread(thread_api_user);
    thread_api_user->start();


    thread_api_session = new QThread(this);
    api_session = new Api_session(m_ncs_params.base_directory, port + 103);

    connect(thread_api_session, SIGNAL(started()), api_session, SLOT(init()));
    connect(api_session, SIGNAL(destroyed()), thread_api_session, SLOT(quit()), Qt::DirectConnection);

    api_session->moveToThread(thread_api_session);
    thread_api_session->start();



    thread_api_app = new QThread(this);
    api_app = new Api_app(m_ncs_params.base_directory, port + 104);

    connect(thread_api_app, SIGNAL(started()), api_app, SLOT(init()));
    connect(api_app, SIGNAL(destroyed()), thread_api_app, SLOT(quit()), Qt::DirectConnection);

    api_app->moveToThread(thread_api_app);
    thread_api_app->start();


    thread_api_ftp = new QThread(this);
    api_ftp = new Api_ftp(m_ncs_params.base_directory, port + 105);

    connect(thread_api_ftp, SIGNAL(started()), api_ftp, SLOT(init()));
    connect(api_ftp, SIGNAL(destroyed()), thread_api_ftp, SLOT(quit()), Qt::DirectConnection);

    api_ftp->moveToThread(thread_api_ftp);
    thread_api_ftp->start();            
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

    node_thread_ftp = new QThread(this);
    m_nodeftp = new Nodeftp(m_ncs_params.base_directory, port);
    connect(m_nodeftp, SIGNAL(destroyed()), node_thread_ftp, SLOT(quit()), Qt::DirectConnection);
    m_nodeftp->connect(node_thread_ftp, SIGNAL(started()), SLOT(ftp_init()));

    m_nodeftp->moveToThread(node_thread_ftp);
    node_thread_ftp->start();
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

    thread_xmpp_server = new QThread(this);
    m_xmpp_server = new Xmpp_server(m_ncs_params.domain_name, m_ncs_params.xmpp_client_port, m_ncs_params.xmpp_server_port);
    connect(m_xmpp_server, SIGNAL(destroyed()), thread_xmpp_server, SLOT(quit()), Qt::DirectConnection);
    m_xmpp_server->moveToThread(thread_xmpp_server);
    thread_xmpp_server->start();


    thread_xmpp_client = new QThread(this);
    m_xmpp_client = new Xmpp_client(m_ncs_params.base_directory, m_ncs_params.domain_name, m_ncs_params.xmpp_client_port);
    connect(m_xmpp_client, SIGNAL(destroyed()), thread_xmpp_client, SLOT(quit()), Qt::DirectConnection);
    m_xmpp_client->moveToThread(thread_xmpp_client);
    thread_xmpp_client->start();
}



void Service::Worker_init()
{
    worker_thread_api = new QThread(this);;
    worker_api = new Worker_api(m_ncs_params.base_directory);

    connect(worker_api, SIGNAL(destroyed()), worker_thread_api, SLOT(quit()), Qt::DirectConnection);

    worker_api->moveToThread(worker_thread_api);
    worker_thread_api->start();
}

void Service::link()
{
    connect(m_http_admin, SIGNAL(create_ftp_user(QString,QString)), worker_api, SLOT(publish_command(QString,QString)), Qt::QueuedConnection);
    connect(api_user, SIGNAL(create_ftp_user(QString,QString)), worker_api, SLOT(publish_command(QString,QString)), Qt::QueuedConnection);
}
