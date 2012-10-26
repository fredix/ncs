/****************************************************************************
**   ncs is the backend's server of nodecast
**   Copyright (C) 2010-2012  Frédéric Logier <frederic@logier.org>
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

#include "zeromq.h"
#include <fstream>


Ztracker::Ztracker(zmq::context_t *a_context) : m_context(a_context)
{
    std::cout << "Ztracker::Ztracker construct" << std::endl;

    m_mutex = new QMutex();


    nosql_ = Nosql::getInstance_front();

    /***************** SERVICE SOCKET *************/
    m_message = new zmq::message_t(2);
    m_socket = new zmq::socket_t (*m_context, ZMQ_REP);

    uint64_t hwm = 50000;
    m_socket->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    m_socket->bind("tcp://*:5569");
    /**********************************************/



    int socket_tracker_fd;
    size_t socket_size = sizeof(socket_tracker_fd);
    m_socket->getsockopt(ZMQ_FD, &socket_tracker_fd, &socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << socket_tracker_fd << " errno : " << zmq_strerror (errno);

    check_tracker = new QSocketNotifier(socket_tracker_fd, QSocketNotifier::Read, this);
    connect(check_tracker, SIGNAL(activated(int)), this, SLOT(receive_payload()), Qt::DirectConnection);


    /**************** DATA SOCKET *****************
    m_data_message = new zmq::message_t(2);
    m_data_socket = new zmq::socket_t(*m_context, ZMQ_PUSH);
    m_data_socket->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    m_data_socket->bind("ipc:///tmp/nodecast/workers");
    **********************************************/

    worker_timer = new QTimer();
    connect(worker_timer, SIGNAL(timeout()), this, SLOT(worker_update_ticker ()), Qt::DirectConnection);
    worker_timer->start (5000);
}

int Ztracker::get_available_port()
{
    BSONObj empty;
    QList <BSONObj> workers_list = nosql_->FindAll("workers", empty);

    int port_counter = 5559;

    foreach (BSONObj worker, workers_list)
    {
        std::cout << "worker : " << worker.getField("port").numberInt() << std::endl;

        if (worker.getField("port").numberInt() > port_counter)
            port_counter = worker.getField("port").numberInt();
    }

    port_counter++;
    return port_counter;
}



void Ztracker::receive_payload()
{   
    check_tracker->setEnabled(false);
    qDebug() << "Ztracker::receive_payload !!!!!!!!!";

    qint32 events = 0;
    std::size_t eventsSize = sizeof(events);
    m_socket->getsockopt(ZMQ_EVENTS, &events, &eventsSize);

    std::cout << "ZMQ_EVENTS : " <<  events << std::endl;


    if (events & ZMQ_POLLIN)
    {
        std::cout << "Ztracker::receive_payload ZMQ_POLLIN" <<  std::endl;


        while (true) {

            flush_socket:
            zmq::message_t request;

            bool res = m_socket->recv (&request, ZMQ_NOBLOCK);
            if (!res && zmq_errno () == EAGAIN) break;


            std::cout << "Ztracker::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

            if (request.size() == 0) {
                std::cout << "Zpull::worker_response received request 0" << std::endl;
                break;
            }


            BSONObj l_payload;
            try {
                l_payload = bo((char*)request.data());
                std::cout << "Ztracker Received payload : " << l_payload << std::endl;
            }
            catch (mongo::MsgAssertionException &e)
            {
                std::cout << "error on data : " << l_payload << std::endl;
                std::cout << "error on data BSON : " << e.what() << std::endl;
                goto flush_socket;
            }

            QString worker_type = QString::fromStdString(l_payload.getFieldDotted("payload.type").str());
            QString payload_action = QString::fromStdString(l_payload.getFieldDotted("payload.action").str());

            if (payload_action == "register")
            {
                QUuid session_uuid = QUuid::createUuid();
                QString str_session_uuid = session_uuid.toString().mid(1,36);

                qDebug() << "REGISTER ID : " << str_session_uuid;
                std::cout << "PAYLOAD : " << l_payload << std::endl;


                be worker_name = l_payload.getFieldDotted("payload.name");
                bo worker = nosql_->Find("workers", worker_name.wrap());

                int worker_port;
                if (worker.nFields() == 0)
                {
                    worker_port = get_available_port();

                    worker = BSON(GENOID
                                  << "type" << worker_type.toStdString()
                                  << "name" << l_payload.getFieldDotted("payload.name")
                                  << "port" << worker_port);
                    nosql_->Insert("workers", worker);

                    // create server
                    qDebug() << "BEFORE EMIT CREATE SERVER";
                    emit create_server(QString::fromStdString(worker_name.str()), QString::number(worker_port));
                    qDebug() << "AFTER EMIT CREATE SERVER";
                }
                else
                {
                    worker_port = worker.getField("port").numberInt();
                }


                // Send reply back to client
                BSONObj payload = BSON("uuid" << str_session_uuid.toStdString() << "port" << worker_port);


                bob worker_builder;
                worker_builder.genOID();
                worker_builder.append(payload.getField("uuid"));
                worker_builder.append(l_payload.getFieldDotted("payload.pid"));
                worker_builder.append(l_payload.getFieldDotted("payload.node_uuid"));
                worker_builder.append(l_payload.getFieldDotted("payload.timestamp"));
                worker_builder.append("status", "up");


                BSONObj node = BSON("nodes" << worker_builder.obj());

                be worker_id = worker.getField("_id");
                nosql_->Addtoarray("workers", worker_id.wrap(), node);


                m_message->rebuild(payload.objsize());
                memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
            }
            else if (payload_action == "watchdog")
               {
                   be timestamp = l_payload.getFieldDotted("payload.timestamp");
                   be uuid = l_payload.getField("uuid");

                   BSONObj bo_node_uuid = BSON("nodes.uuid" << uuid);
                   BSONObj workers_node = BSON("nodes.$.timestamp" << timestamp);

                   qDebug() << "UPDATE NODE's TIMESTAMP";
                   nosql_->Update("workers", bo_node_uuid, workers_node);

                   BSONObj payload = BSON("status" << "ACK");
                   m_message->rebuild(payload.objsize());
                   memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
               }
            else if (payload_action == "ping")
            {
                qDebug() << "RECEIVE INIT SOCKET";
                BSONObj payload = BSON("status" << "pong");
                m_message->rebuild(payload.objsize());
                memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
            }
            /*else
            {
                qDebug() << "RECEIVE ACTION : " << payload_action;

                BSONObjBuilder b_payload;
                b_payload.append(l_payload.getFieldDotted("payload.name"));
                b_payload.append(l_payload.getFieldDotted("payload.action"));
                b_payload.append(l_payload.getFieldDotted("payload.timestamp"));
                b_payload.append(l_payload.getFieldDotted("payload.session_uuid"));

                *** ACTION TERMINATE ***
                BSONObj tmp = l_payload.getField("payload").Obj();

                if (tmp.hasField("datas")) b_payload.append(l_payload.getFieldDotted("payload.datas"));
                if (tmp.hasField("exitcode")) b_payload.append(l_payload.getFieldDotted("payload.exitcode"));
                if (tmp.hasField("exitstatus")) b_payload.append(l_payload.getFieldDotted("payload.exitstatus"));

                BSONObj payload = b_payload.obj();

                std::cout << "PAYLOAD FORWARD : " << payload << std::endl;

                ****** PUSH API PAYLOAD *******
                qDebug() << "FORWARD NEXT PAYLOAD";
                m_data_message->rebuild(payload.objsize());
                memcpy(m_data_message->data(), (char*)payload.objdata(), payload.objsize());
                m_data_socket->send(*m_data_message);
                ************************

                payload = BSON("status" << "ACK");
                m_message->rebuild(payload.objsize());
                memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
               }*/


            m_socket->send(*m_message);
            //delete(m_message);
        }
    }
    check_tracker->setEnabled(true);
}



void Ztracker::worker_update_ticker()
{
    std::cout << "Ztracker::update_ticker" << std::endl;

    m_mutex->lock();


    QDateTime l_timestamp = QDateTime::currentDateTime();
    qDebug() << l_timestamp.toTime_t ();

    QDateTime t_timestamp;

    BSONObj empty;
    QList <BSONObj> workers_list = nosql_->FindAll("workers", empty);

    foreach (BSONObj worker, workers_list)
    {
        //std::cout << "TICKER !!!  workers : " << workers << std::endl;

        BSONObj nodes = worker.getField("nodes").Obj();


        list<be> list_nodes;
        nodes.elems(list_nodes);
        list<be>::iterator i;

        /********   Iterate over each worker's nodes   *******/
        /********  find node with uuid and set the node id to payload collection *******/
        for(i = list_nodes.begin(); i != list_nodes.end(); ++i) {
            BSONObj l_node = (*i).embeddedObject ();

            //std::cout << "L_NODE : " << l_node << std::endl;

            BSONElement node_id;
            l_node.getObjectID (node_id);

            BSONElement node_timestamp = l_node.getField("timestamp");
            BSONElement status = l_node.getField("status");
            BSONElement uuid = l_node.getField("uuid");

            t_timestamp.setTime_t(node_timestamp.number());


            //qDebug() << "QT TIMESTAMP : " << t_timestamp.toString("dd MMMM yyyy hh:mm:ss");
            //qDebug() << "SECONDES DIFF : " << t_timestamp.secsTo(l_timestamp);
            //std::cout << "STATUS : " <<  status.str() << std::endl;


            if (t_timestamp.secsTo(l_timestamp) > 30 && status.str() == "up")
            {
                qDebug() << "SEND ALERT !!!!!!!";


                std::cout << "node_id : " << node_id << std::endl;
                std::cout << "node_id.OID() : " << node_id.OID() << std::endl;

                BSONElement node_uuid = l_node.getField("node_uuid");
                BSONObj node = nosql_->Find("nodes", node_uuid.wrap());

                BSONObj bo_node_id = BSON("nodes._id" << node_id.OID());

                std::cout << "node name : " << node << std::endl;

                BSONObj worker_status = BSON("nodes.$.status" << "down");
                nosql_->Update("workers", bo_node_id, worker_status);


                QString l_worker = "WORKER ";
                l_worker.append(QString::fromStdString(worker.getField("name").str()));
                l_worker.append(" NODE NAME : ");
                l_worker.append(QString::fromStdString(node.getField("nodename").str()));
                l_worker.append(" TYPE : ");
                l_worker.append(QString::fromStdString(worker.getField("type").str()));
                l_worker.append (", UUID : ").append (uuid.valuestr()).append (", AT : ").append (t_timestamp.toString("dd MMMM yyyy hh:mm:ss"));
                qDebug() << "WORKER ALERT ! " << l_worker;
                emit sendAlert(l_worker);
            }
        }
    }

    m_mutex->unlock();
}





Ztracker::~Ztracker()
{
    qDebug() << "Ztracker deleted socket";
    delete(m_socket);
}


void Ztracker::destructor()
{
    m_mutex->lock();

    qDebug() << "Ztracker destructor";
    check_tracker->setEnabled(false);

    qDebug() << "Ztracker worker_timer stop";
    worker_timer->stop ();

    qDebug() << "Ztracker close socket";
    m_socket->close ();
}



Zreceive::Zreceive(zmq::context_t *a_context, zmq::socket_t *a_workers, QString a_inproc) : m_context(a_context), z_workers(a_workers), m_inproc(a_inproc)
{
    qDebug() << "Zreceive::construct";
}


void Zreceive::init_payload()
{
    qDebug() << "Zreceive::init_payload, proc ; " << m_inproc;
    z_sender = new zmq::socket_t(*m_context, ZMQ_PULL);
    qDebug() << " INPROC : " << m_inproc;

    uint64_t hwm = 50000;
    zmq_setsockopt (z_sender, ZMQ_HWM, &hwm, sizeof (hwm));

    z_sender->connect("inproc://" + m_inproc.toAscii());

    //  Connect work threads to client threads via a queue
    std::cout << "Zreceive::init_payload BEFORE ZMQ_STREAMER" << std::endl;
    zmq::device (ZMQ_STREAMER, *z_sender, *z_workers);
    std::cout << "Zreceive::init_payload AFTER ZMQ_STREAMER" << std::endl;
}

Zreceive::~Zreceive()
{
    qDebug() << "Zreceive::~Zreceive";
    delete(z_sender);
}

void Zreceive::destructor()
{
    qDebug() << "Zreceive destructor";
    z_sender->close ();
}




Zpull::Zpull(zmq::context_t *a_context) : m_context(a_context)
{        
    std::cout << "Zpull::Zpull constructeur" << std::endl;

    m_mutex_zeromq = new QMutex();
    m_mutex_http = new QMutex();
    m_mutex_worker_response = new QMutex();


    m_socket_http = new zmq::socket_t (*m_context, ZMQ_PULL);
    uint64_t hwm = 50000;
    m_socket_http->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    m_socket_http->connect("ipc:///tmp/nodecast/http");

    m_socket_workers = new zmq::socket_t (*m_context, ZMQ_PULL);
    m_socket_workers->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    m_socket_workers->connect("ipc:///tmp/nodecast/workers");


    m_socket_zeromq = new zmq::socket_t (*m_context, ZMQ_PULL);
    m_socket_zeromq->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    m_socket_zeromq->connect("ipc:///tmp/nodecast/zeromq");



    int http_socket_fd;
    size_t socket_size = sizeof(http_socket_fd);
    m_socket_http->getsockopt(ZMQ_FD, &http_socket_fd, &socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << http_socket_fd << " errno : " << zmq_strerror (errno);

    check_http_data = new QSocketNotifier(http_socket_fd, QSocketNotifier::Read, this);
    connect(check_http_data, SIGNAL(activated(int)), this, SLOT(receive_http_payload()));



    int zeromq_socket_fd;
    size_t zeromq_socket_size = sizeof(zeromq_socket_fd);
    m_socket_zeromq->getsockopt(ZMQ_FD, &zeromq_socket_fd, &zeromq_socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << zeromq_socket_fd << " errno : " << zmq_strerror (errno);

    check_zeromq_data = new QSocketNotifier(zeromq_socket_fd, QSocketNotifier::Read, this);
    connect(check_zeromq_data, SIGNAL(activated(int)), this, SLOT(receive_zeromq_payload()));



    int worker_socket_fd;
    size_t worker_socket_size = sizeof(worker_socket_fd);
    m_socket_workers->getsockopt(ZMQ_FD, &worker_socket_fd, &worker_socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << worker_socket_fd << " errno : " << zmq_strerror (errno);

    check_worker_response = new QSocketNotifier(worker_socket_fd, QSocketNotifier::Read, this);
    connect(check_worker_response, SIGNAL(activated(int)), this, SLOT(worker_response()));
}


Zpull::~Zpull()
{
    qDebug() << "Zpull::~Zpull";
    delete(m_socket_http);
    delete(m_socket_workers);
    delete(m_socket_zeromq);
}

void Zpull::destructor()
{
    qDebug() << "Zpull destructor";

    m_mutex_http->lock();
    check_http_data->setEnabled(false);
    m_socket_http->close ();

    m_mutex_zeromq->lock();
    check_zeromq_data->setEnabled(false);
    m_socket_zeromq->close ();

    m_mutex_worker_response->lock();
    check_worker_response->setEnabled(false);
    m_socket_workers->close ();

}


void Zpull::receive_http_payload()
{
    m_mutex_http->lock();

    check_http_data->setEnabled(false);

    std::cout << "Zpull::receive_payload" << std::endl;

    qint32 events = 0;
    std::size_t eventsSize = sizeof(events);

    m_socket_http->getsockopt(ZMQ_EVENTS, &events, &eventsSize);


    std::cout << "Zpull::receive_payload ZMQ_EVENTS : " <<  events << std::endl;


    if (events & ZMQ_POLLIN)
    {
        std::cout << "Zpull::receive_payload ZMQ_POLLIN" <<  std::endl;


        while (true)
        {
            flush_socket:


    //  Initialize poll set
//    zmq::pollitem_t items = { *m_socket, 0, ZMQ_POLLIN, 0 };


    //while (true) {
        //  Wait for next request from client
        zmq::message_t request;

  //      zmq::poll (&items, 1, 5000);


    //    if (items.revents & ZMQ_POLLIN)
//        {
            bool res = m_socket_http->recv(&request, ZMQ_NOBLOCK);
            if (!res && zmq_errno () == EAGAIN) break;

            std::cout << "Zpull::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

            if (request.size() == 0) {
                std::cout << "Zpull::worker_response received request 0" << std::endl;
                break;
            }


        //m_socket->recv (&request, ZMQ_NOBLOCK);


            BSONObj data;

            try {
                data = BSONObj((char*)request.data());


                if (data.isValid() && !data.isEmpty())
                {
                    std::cout << "Zpull received : " << res << " data : " << data  << std::endl;

                    std::cout << "!!!!!!! BEFORE FORWARD PAYLOAD !!!!" << std::endl;
                    emit forward_payload(data.copy());
                    std::cout << "!!!!!!! AFTER FORWARD PAYLOAD !!!!" << std::endl;
                }
                else
                {
                    std::cout << "DATA NO VALID !" << std::endl;
                }

            }
            catch (mongo::MsgAssertionException &e)
            {
                std::cout << "error on data : " << data << std::endl;
                std::cout << "error on data BSON : " << e.what() << std::endl;                                    
                goto flush_socket;
            }
        }

    }
    check_http_data->setEnabled(true);
    m_mutex_http->unlock();
}




void Zpull::receive_zeromq_payload()
{
    m_mutex_zeromq->lock();
    check_zeromq_data->setEnabled(false);

    std::cout << "Zpull::receive_zeromq_payload" << std::endl;

    qint32 events = 0;
    std::size_t eventsSize = sizeof(events);

    m_socket_zeromq->getsockopt(ZMQ_EVENTS, &events, &eventsSize);


    std::cout << "Zpull::receive_payload ZMQ_EVENTS : " <<  events << std::endl;


    if (events & ZMQ_POLLIN)
    {
        std::cout << "Zpull::receive_zeromq_payload ZMQ_POLLIN" <<  std::endl;


        while (true)
        {
            flush_socket:


    //  Initialize poll set
//    zmq::pollitem_t items = { *m_socket, 0, ZMQ_POLLIN, 0 };


    //while (true) {
        //  Wait for next request from client
        zmq::message_t request;

  //      zmq::poll (&items, 1, 5000);


    //    if (items.revents & ZMQ_POLLIN)
//        {
            bool res = m_socket_zeromq->recv(&request, ZMQ_NOBLOCK);
            if (!res && zmq_errno () == EAGAIN) break;

            std::cout << "Zpull::receive_zeromq_payload received request: [" << (char*) request.data() << "]" << std::endl;

            if (request.size() == 0) {
                std::cout << "Zpull::receive_zeromq_payload received request 0" << std::endl;
                break;
            }


        //m_socket->recv (&request, ZMQ_NOBLOCK);


            BSONObj data;

            try {
                data = BSONObj((char*)request.data());


                if (data.isValid() && !data.isEmpty())
                {
                    std::cout << "Zpull received : " << res << " data : " << data  << std::endl;

                    std::cout << "!!!!!!! BEFORE FORWARD PAYLOAD !!!!" << std::endl;
                    emit forward_payload(data.copy());
                    std::cout << "!!!!!!! AFTER FORWARD PAYLOAD !!!!" << std::endl;
                }
                else
                {
                    std::cout << "DATA NO VALID !" << std::endl;
                }

            }
            catch (mongo::MsgAssertionException &e)
            {
                std::cout << "error on data : " << data << std::endl;
                std::cout << "error on data BSON : " << e.what() << std::endl;
                goto flush_socket;
            }
        }

    }
    check_zeromq_data->setEnabled(true);
    m_mutex_zeromq->unlock();
}



void Zpull::worker_response()
{
    m_mutex_worker_response->lock();

    check_worker_response->setEnabled(false);

    std::cout << "Zpull::worker_response" << std::endl;

    qint32 events = 0;
    std::size_t eventsSize = sizeof(events);

    m_socket_workers->getsockopt(ZMQ_EVENTS, &events, &eventsSize);


    std::cout << "Zpull::worker_response ZMQ_EVENTS : " <<  events << std::endl;


    if (events & ZMQ_POLLIN)
    {

        std::cout << "Zpull::worker_response ZMQ_POLLIN" <<  std::endl;


        while (true)
        {
            flush_socket:


    //  Initialize poll set
//    zmq::pollitem_t items = { *m_socket, 0, ZMQ_POLLIN, 0 };


    //while (true) {
        //  Wait for next request from client
        zmq::message_t request;

  //      zmq::poll (&items, 1, 5000);


    //    if (items.revents & ZMQ_POLLIN)
//        {
            int res = m_socket_workers->recv(&request, ZMQ_NOBLOCK);
            if (res == -1 && zmq_errno () == EAGAIN) break;


            if (request.size() == 0) {
                std::cout << "Zpull::worker_response received request 0" << std::endl;
                break;
            }


        //m_socket->recv (&request, ZMQ_NOBLOCK);

        //std::cout << "Zdispatch received request: [" << (char*) request.data() << "]" << std::endl;


            BSONObj data;
            try {
                data = BSONObj((char*)request.data());

                if (data.isValid() && !data.isEmpty())
                {
                    std::cout << "Zpull received : " << res << " data : " << data  << std::endl;

                    std::cout << "!!!!!!! BEFORE FORWARD PAYLOAD !!!!" << std::endl;
                    emit forward_payload(data.copy());
                    std::cout << "!!!!!!! AFTER FORWARD PAYLOAD !!!!" << std::endl;
                }
                else
                {
                    std::cout << "DATA NO VALID !" << std::endl;
                }

            }
            catch (mongo::MsgAssertionException &e)
            {
                std::cout << "error on data : " << data << std::endl;
                std::cout << "error on data BSON : " << e.what() << std::endl;
                goto flush_socket;
            }

        }
    }
    check_worker_response->setEnabled(true);
    m_mutex_worker_response->unlock();
}




Zdispatch::Zdispatch(zmq::context_t *a_context) : m_context(a_context)
{
    m_mutex_replay_payload = new QMutex();
    m_mutex_push_payload = new QMutex();

    std::cout << "Zdispatch::Zdispatch constructeur" << std::endl;
    //nosql_ = Nosql::getInstance_back();
    nosql_ = Nosql::getInstance_front();


    BSONObj empty;
    workflow_list = nosql_->FindAll("workflows", empty);
    worker_list = nosql_->FindAll("workers", empty);

    // read workers collection and instance workers's server
    foreach (BSONObj worker, worker_list)
    {
        /*std::cout << "LIST SIZE : " << worker_list.size() << std::endl;

        for (int i = 0; i < worker_list.size(); ++i) {
            std::cout << "LIST WORKERS : " << worker_list.at(i) << std::endl;
         }

        std::cout << "WORKER : " << worker << std::endl;
        */

        int port = worker.getField("port").number();
        std::cout << "INT worker port : " << port << std::endl;

        QString w_name = QString::fromStdString(worker.getField("name").str());
        QString w_port = QString::number(worker.getField("port").Int());
        std::cout << "worker name : " << w_name.toStdString() << std::endl;
        std::cout << "worker port : " << w_port.toStdString() << std::endl;

        bind_server(w_name, w_port);
    }



    replay_payload_timer = new QTimer();
    connect(replay_payload_timer, SIGNAL(timeout()), this, SLOT(replay_payload ()), Qt::DirectConnection);
    //replay_payload_timer->start (60000);
    replay_payload_timer->start (60000);
}


Zdispatch::~Zdispatch()
{}


void Zdispatch::destructor()
{
    qDebug() << "Zdispatch destructor";
    m_mutex_replay_payload->lock();
    m_mutex_push_payload->lock();

    BSONObj empty;
    worker_list = nosql_->FindAll("workers", empty);

    // read workers collection and instance workers's server
    foreach (BSONObj worker, worker_list)
    {


        /*for (int i = 0; i < worker_list.size(); ++i) {
            std::cout << "LIST WORKERS : " << worker_list.at(i) << std::endl;
         }
        */

        QString w_name = QString::fromStdString(worker.getField("name").str());
        std::cout << "worker name : " << w_name.toStdString() << std::endl;
        qDebug() << "delete worker push : " << w_name;
        if (workers_push.contains (w_name)) workers_push[w_name].clear ();
    }
}




void Zdispatch::replay_payload()
{
    qDebug() << "Zdispatch::replay_payload";
    m_mutex_replay_payload->lock();


    BSONObj pushed = BSON("pushed" << false);
    QList <BSONObj> lost_payload_list = nosql_->FindAll("lost_pushpull_payloads", pushed);
    //nosql_->Update("lost_pushpull_payloads", BSON("pushed" << false), BSON("pushed" << true), false, true);


    // read workers collection and instance workers's server
    foreach (BSONObj lost_payload, lost_payload_list)
    {

        nosql_->Update("lost_pushpull_payloads", BSON("_id" << lost_payload.getField("_id") << "pushed" << false), BSON("pushed" << true));


        std::cout << "LIST SIZE : " << lost_payload_list.size() << std::endl;

        /*
        for (int i = 0; i < lost_payload_list.size(); ++i) {
            std::cout << "LIST PAYLOADS : " << lost_payload_list.at(i) << std::endl;
         }


        std::cout << "LOST PAYLOAD : " << lost_payload << std::endl;
        */

        QString w_name = QString::fromStdString(lost_payload.getField("worker").str());
        BSONObj payload = BSON("payload" << lost_payload.getFieldDotted("data.payload"));


        if (workers_push.contains(w_name))
            workers_push[w_name]->push_payload(payload);
    }


    nosql_->Flush("lost_pushpull_payloads", BSON("pushed" << true));
    m_mutex_replay_payload->unlock();
}

void Zdispatch::bind_server(QString name, QString port)
{
    qDebug() << "BEFORE BIND SERVER SLOT !";
    workers_push[name] =  QSharedPointer<Zworker_push> (new Zworker_push(m_context, name.toStdString(), port.toStdString()));

    qDebug() << "AFTER BIND SERVER SLOT !";

    BSONObj empty;
    worker_list.clear();
    worker_list = nosql_->FindAll("workers", empty);
}




void Zdispatch::push_payload(BSONObj a_data)
{

    m_mutex_push_payload->lock();
    std::cout << "Zdispatch received data : " << a_data << std::endl;


    //BSONElement session_uuid = data.getField("session_uuid");
    BSONObj session_uuid = BSON("uuid" << a_data.getField("session_uuid").str());
    BSONElement b_session_uuid = session_uuid.getField("uuid");

    BSONElement b_action = a_data.getField("action");


    std::cout << "session_uuid : " << session_uuid << std::endl;
    std::cout << "b_session_uuid : " << b_session_uuid << std::endl;
    std::cout << "b_action : " << b_action << std::endl;


    BSONObj session = nosql_->Find("sessions", session_uuid);

    std::cout << "!!!!! session !!!!!! : " << session << std::endl;

    BSONObj session_id = BSON("_id" << session.getField("_id").OID());
    std::cout << "session_id : " << session_id << std::endl;

    BSONObj payload_id = BSON("_id" << session.getField("payload_id").OID());
    //session.getField("payload_id").Obj().getObjectID(payload_id);

    std::cout << "payload_id : " << payload_id << std::endl;

    BSONObj payload = nosql_->Find("payloads", payload_id);
    BSONElement l_payload_id = payload.getField("_id");

    BSONElement payload_type = payload.getField("payload_type");


    bool gridfs = payload.getField("gridfs").Bool();
    string filename;

    if (gridfs)
    {
        BSONElement gfs_id = payload.getField("gfs_id");

        std::cout << "EXTRACT GFSID : " << gfs_id << std::endl;
        BSONObj gfsid = BSON("_id" << gfs_id);

        filename = nosql_->GetFilename(gfsid.firstElement());
        std::cout << "EXTRACT PAYLOAD, FILENAME : " << filename << std::endl;
    }

    //std::cout << "payload : " << payload << std::endl;
    std::cout << "l_payload_id : " << l_payload_id << std::endl;

    BSONObj payload_steps;
    if (payload.hasField("steps")) payload_steps = payload.getField("steps").Obj();


    BSONObj workflow_id = BSON("_id" << session.getField("workflow_id").OID());
    BSONObj workflow = nosql_->Find("workflows", workflow_id);
    std::cout << "FIND WORKER'S WORKFLOW : " << workflow << std::endl;

    BSONObj w_workers = workflow.getField("workers").Obj();
    std::cout << "WORKER'S WORKFLOW : " << w_workers << std::endl;


    list <BSONElement> list_w_workers;
    w_workers.elems(list_w_workers);
    int workers_number = (int) list_w_workers.size();

    std::cout << "WORKER NUMBER : " << workers_number << std::endl;




    BSONElement timestamp = a_data.getField("timestamp");

    //std::cout << "ACTION : " << data.getField("action") << std::endl;
    std::cout << "timestamp : " << timestamp << std::endl;


    //string action = data.getField("action").valuestr();
    string action = b_action.valuestr();

    std::cout << "ACTION : " << action << std::endl;



    BSONElement datas;
    if (a_data.hasField("data")) datas = a_data.getField("data");


/*
    if (action.compare("publish") == 0 && w_workers.nFields() !=0)
    {
        std::cout << "Zdispatch::receive_payload RECEIVE PUBLISH : " << datas << std::endl;

        //emit emit_pubsub(datas.Obj().copy());

    }*/
    if ((action.compare("push") == 0 || action.compare("publish") == 0) && w_workers.nFields() !=0)
    {
        // create step
        QString worker_name;
        QString worker_port;

        std::cout << "Zdispatch::receive_payload : ACTION PUSH : " << action << std::endl;

        bool worker_never_connected = true;

        for (BSONObj::iterator i = w_workers.begin(); i.more();)
        {
            if (!worker_never_connected) break;

            BSONElement w_worker = i.next();

            std::cout << "W_WORKER : " << w_worker.Number() << std::endl;


            if (w_worker.Number() == 1)
            {
                worker_name = QString::fromStdString(w_worker.fieldName());

                qDebug() << "worker_name : " << worker_name << " WORKER LIST SIZE : " << worker_list.size();


                // find worker
                foreach (BSONObj worker, worker_list)
                {

                    std::cout << "worker name : " << worker.getField("name").str() << std::endl;
                    std::cout << "worker port : " << worker.getField("port").numberInt() << std::endl;

                    if (worker_name.contains(QString::fromStdString(worker.getField("name").str())))
                    {
                        worker_port = QString::number(worker.getField("port").numberInt());
                        qDebug() << "worker_port : " << worker_port;
                        worker_never_connected = false;
                        break;
                    }
                }
            }
        }

        if (!worker_never_connected)
        {


                //if (gridfs) nosql_->ExtractBinary(gfs_id, path.toStdString(), filename);



                /********** RECORD PAYLOAD STEP *********/
                BSONObjBuilder step_builder;
                step_builder.genOID();
                step_builder.append("name", worker_name.toStdString());
                step_builder.append("action", "push");
                step_builder.append("order", 1);


                if (gridfs)
                {
                    step_builder.append("data", filename);
                }
                else step_builder.append("data", payload.getField("data").str());

                step_builder.append("send_timestamp", timestamp.Number());

                BSONObj step = BSON("steps" << step_builder.obj());

                nosql_->Addtoarray("payloads", l_payload_id.wrap(), step);
                /*****************************************/

                /********* UPDATE SESSION **********/
                BSONObjBuilder session_builder;
                session_builder.append("step_id", step.getField("steps").Obj().getField("_id").OID());
                session_builder.append("counter", workers_number -1);
                session_builder.append("last_worker", worker_name.toStdString());
                BSONObj l_session = session_builder.obj();

                std::cout << "SESSION !!! " << l_session << std::endl;

                nosql_->Update("sessions", session_id, l_session);
                /***********************************/

                //be step_id;
                //step.getField("steps").Obj().getObjectID(step_id);

                /**** SEND PAYLOAD ****/
                //BSONObj payload = BSON("datas" << path.toStdString() << "workflow_uuid" << workflow_uuid.str() << "payload_uuid" << payload_uuid.str() << "step_id" << step.getField("steps").Obj().getField("_id"));

                /*if (worker_never_connected)
                {
                    qDebug() << "WORKER NERVER CONNECTED : LOST PAYLOAD";
                    return;
                }*/

                BSONObj s_payload;
                if (gridfs)
                {
                    //s_payload = BSON("data" << path.toStdString() << "session_uuid" << b_session_uuid.str());

                    BSONObj t_data = BSON("command" << "get_file" <<
                                          "filename" << filename <<
                                          payload_type <<
                                          "session_uuid" << b_session_uuid.str());
                    /*

                    BSONObj data = BSON("command" << "get_file" <<
                                        "filename" << filename <<
                                        payload_type <<
                                        "session_uuid" << b_session_uuid.str()) <<
                                        "action" << action <<
                                        "session_uuid" << b_session_uuid.str();
                    */


                    BSONObj data = BSON("data" << t_data.jsonString(Strict, false) <<
                                        "action" << action <<
                                        "session_uuid" << b_session_uuid.str());



                    s_payload = BSON("payload" << data);


/*                    s_payload = BSON("payload" << BSON("command" << "get_file" <<
                                                       "filename" << filename <<
                                                       "action" << action <<
                                                       payload_type <<
                                                       "session_uuid" << b_session_uuid.str()));
                                                       */


                    //std::cout << "!!!!!!!!!!!!!PUSH PAYLOAD : " << s_payload << std::endl;

                    //s_payload = BSON("data" << BSON("command" << "receive") << "session_uuid" << b_session_uuid.str());
                }
                else s_payload = BSON("payload" << BSON("data" << payload.getField("data").str() <<
                                                        payload_type <<
                                                        "session_uuid" << b_session_uuid.str()) <<
                                                        "action" << action <<
                                                        "session_uuid" << b_session_uuid.str());



                qDebug() << "push_payload : " << worker_name;
                workers_push[worker_name]->push_payload(s_payload);




        }
        else qDebug() << "WORKER NERVER CONNECTED : LOST PAYLOAD";

    }
    else if (action.compare("receive") == 0)
    {
        std::cout << "Zdispatch::receive_payload : ACTION receive : " << action << std::endl;
        // add step
        //be payload_step_id = session.getField("step_id");
        //data.getObjectID (payload_step_id);

        //BSONObj payload_id = BSON("_id" << session.getField("payload_id").OID());


        /********** RECORD PAYLOAD STEP *********/
        BSONObjBuilder step_builder;
        BSONObj step_id = BSON("steps._id" << session.getField("step_id").OID());

        std::cout << "STEP ID " << step_id << std::endl;
        step_builder << "steps.$.action" << "receive";
        step_builder << "steps.$.receive_timestamp" << timestamp.Number();

        BSONObj step = step_builder.obj();

        std::cout << "PAYLOAD RECEIVE !!!" << step << std::endl;


        nosql_->Update("payloads", step_id, step);
        /*****************************************/
    }
    else if (action.compare("terminate") == 0)
    {
        std::cout << "Zdispatch::receive_payload : ACTION TERMINATE : " << action << std::endl;
        // add step
        //be payload_step_id = session.getField("step_id");
        //data.getObjectID (payload_step_id);

        //BSONObj payload_id = BSON("_id" << session.getField("payload_id").OID());


        BSONObj step_id = BSON("steps._id" << session.getField("step_id").OID());

        std::cout << "step_id : " << step_id << std::endl;

        BSONObj field = BSON("_id" << 0 << "steps.order" << 1);

        //BSONObj old_step = nosql_->Find("payloads", search);

        BSONObj step_order = nosql_->Find("payloads", step_id, &field);


        std::cout << "step_order: " << step_order << std::endl;


        /* Gruik ........................... !!*/
        BSONElement order = step_order.getField("steps").embeddedObject().getFieldDotted("0.order");
        /**************************************/

        int workflow_order = order.numberInt() + 1;
        std::cout << "ORDER YO : " << workflow_order << std::endl;


        std::cout << "STEP ORDER : " << step_order << std::endl;
        std::cout << "STEP ORDER : " << order.Number() << std::endl;


        be worker_name = a_data.getField("name");

        be exitcode;
        be exitstatus;
        if (a_data.hasField("exitcode")) exitcode = a_data.getField("exitcode");
        if (a_data.hasField("exitstatus")) exitstatus = a_data.getField("exitstatus");


        be datas = a_data.getField("data");


        /********** RECORD PAYLOAD STEP *********/
        BSONObjBuilder step_builder;

        std::cout << "STEP ID " << step_id << std::endl;

        step_builder << "steps.$.name" << worker_name.str();
        step_builder << "steps.$.action" << "terminate";
        step_builder << "steps.$.data" << datas.str();
        if (a_data.hasField("exitcode")) step_builder << "steps.$.exitcode" << exitcode;
        if (a_data.hasField("exitstatus")) step_builder << "steps.$.exitstatus" << exitstatus;
        step_builder << "steps.$.terminate_timestamp" << timestamp.Number();

        BSONObj step = step_builder.obj();

        std::cout << "UPDATE PAYLOAD STEP !!!" << step << std::endl;


        nosql_->Update("payloads", step_id, step);
        /*****************************************/


        payload = nosql_->Find("payloads", payload.getField("_id").wrap());
        payload_steps = payload.getField("steps").Obj();

        bool worker_never_connected = true;
        QString w_name;
        //QString w_port;
        // Parse workflow to forward payload to the next worker
        for (BSONObj::iterator i = w_workers.begin(); i.more();)
        {
            if (!worker_never_connected) break;

            BSONElement w_worker = i.next();

            std::cout << "w_worker fieldname : " << w_worker.fieldName()  << " w_worker val : " << w_worker.numberInt() << " workflow order : " << workflow_order << std::endl;
            std::cout << "session number : " << session.getField("counter").numberInt() << std::endl;

            if (w_worker.numberInt() == workflow_order && session.getField("counter").numberInt() > 0)
            {
                w_name = QString::fromStdString(w_worker.fieldName());

                std::cout << "W NAME WORKER : " << w_name.toStdString() << std::endl;

                //QList<BSONObj>::iterator o;
                foreach (BSONObj worker, worker_list)
                {
                    std::cout << "WORKER : " << worker.getField("name").str() << std::endl;

                    if (worker.getField("name").str().compare(w_name.toStdString ()) == 0)
                    {
                        std::cout << "WORKER ENGAGE !!!! : " << worker << std::endl;
                        //w_port = QString::fromStdString(worker.getField("port").str());
                        worker_never_connected = false;
                        break;
                    }
                }
            }
        }

        if (!worker_never_connected)
        {
            std::cout << "worker name : " << w_name.toStdString() << std::endl;
            //std::cout << "worker port : " << w_port.toStdString() << std::endl;


            /********** RECORD PAYLOAD STEP *********/
            BSONObjBuilder stepnext_builder;
            stepnext_builder.genOID();
            stepnext_builder.append("name", w_name.toStdString());
            stepnext_builder.append("action", "next");
            stepnext_builder.append("order", workflow_order);
            stepnext_builder.append("send_timestamp", timestamp.Number());

            BSONObj stepnext = BSON("steps" << stepnext_builder.obj());
            nosql_->Addtoarray("payloads", payload.getField("_id").wrap(), stepnext);
            /*****************************************/

            qDebug() << "!!!!!!!!!!!!!!!!!  PAYLOAD UPDATE !!!!!!!!!!!!!!!!!!!";


            /********* UPDATE SESSION **********/
            BSONObjBuilder session_builder;
            session_builder.append("step_id", stepnext.getField("steps").Obj().getField("_id").OID());
            session_builder.append("last_worker", w_name.toStdString());
            BSONObj session_options = BSON("$inc" << BSON( "counter" << -1));

            qDebug() << "!!!!!!!!!!!!!!!!!  COUNTER -1 !!!!!!!!!!!!!!!!!!!";

            BSONObj l_session = session_builder.obj();
            nosql_->Update("sessions", session.getField("_id").wrap(), l_session, session_options);
            /***********************************/



            /**** SEND PAYLOAD ****/
            BSONObj payload = BSON("data" << datas.str() << "session_uuid" << b_session_uuid.str());
            qDebug() << "push_payload : " << w_name;
            workers_push[w_name]->push_payload(payload);
        }
        else qDebug() << "WORKER NERVER CONNECTED : LOST PAYLOAD";

    }


    std::cout << "!!!!!!! PAYLOAD SEND !!!!" << std::endl;
    m_mutex_push_payload->unlock();
}






Zworker_push::Zworker_push(zmq::context_t *a_context, string a_worker, string a_port) : m_context(a_context), m_worker(a_worker), m_port(a_port)
{

    std::cout << "Zworker_push::Zworker_push constructeur" << std::endl;
    std::cout << "m context : " << *m_context << std::endl;

    m_mutex = new QMutex();

    nosql_ = Nosql::getInstance_front();


    z_sender = new zmq::socket_t(*m_context, ZMQ_PUSH);
    //uint64_t hwm = 50000;
    uint64_t hwm = 0;
    z_sender->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));

    string addr = "tcp://*:" + m_port;
    std::cout << "ADDR : " << addr << std::endl;

    z_sender->bind(addr.data());
    z_message = new zmq::message_t(2);
}


Zworker_push::~Zworker_push()
{
    m_mutex->lock();
    qDebug() << "Zworker_push deleted";
    z_sender->close ();
    delete(z_sender);
}


void Zworker_push::push_payload(bson::bo a_payload)
{
    m_mutex->lock();
    /***************** PUSH ********************/
    std::cout << "Zworker_push::push_payload Sending tasks to workers..." << std::endl;

    z_message->rebuild(a_payload.objsize());
    memcpy(z_message->data(), (char*)a_payload.objdata(), a_payload.objsize());
    bool res = z_sender->send(*z_message, ZMQ_NOBLOCK);

    qint32 events = 0;
    std::size_t eventsSize = sizeof(events);
    z_sender->getsockopt(ZMQ_EVENTS, &events, &eventsSize);

    if (!res && zmq_errno () == EAGAIN)
    {
        std::cout << "Zworker_push::push_payload !!!!!!!!!!!!!!!!!!!   PAYLOAD BACKUPED !!!!!!!!!!!!!!!!!!" << std::endl;
        std::cout << "EVENTS : " << events << " ERROR" << zmq_strerror(zmq_errno()) << std::endl;

        QDateTime timestamp = QDateTime::currentDateTime();

        BSONObj payload = BSON("worker" << m_worker << "timestamp" << timestamp.toTime_t() << "data" << a_payload << "pushed" << false);
        //nosql_->Insert("lost_pushpull_payloads", payload);
        // UPDATE WITH UPSERT AT ON : INSERT IF NOT FOUND

        std::cout << "Zworker_push::push_payload a_payload : " << a_payload << std::endl;

        BSONObj payload_session = BSON("data.session_uuid" << a_payload.getField("session_uuid").str());


        std::cout << "Zworker_push::push_payload a_payload session : " << payload_session << std::endl;

        std::cout << "Zworker_push::push_payload payload_session_uuid : " << payload_session << std::endl;
        nosql_->Update("lost_pushpull_payloads", payload_session.copy(), payload, true);
    }

    std::cout << "Zworker_push::push_payload after send" << std::endl;

    //delete(z_message);
    m_mutex->unlock();
}



Zstream_push::Zstream_push(zmq::context_t *a_context) : m_context(a_context)
{

    std::cout << "Zstream_push::Zstream_push constructeur" << std::endl;
    std::cout << "m context : " << *m_context << std::endl;

    m_mutex = new QMutex();
    nosql_ = Nosql::getInstance_front();

    z_stream = new zmq::socket_t(*m_context, ZMQ_REP);
    uint64_t hwm = 50000;
    z_stream->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));

    z_stream->bind("tcp://*:5556");
    z_message = new zmq::message_t(2);


    int socket_stream_fd;
    size_t socket_size = sizeof(socket_stream_fd);
    z_stream->getsockopt(ZMQ_FD, &socket_stream_fd, &socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << socket_stream_fd << " errno : " << zmq_strerror (errno);

    check_stream = new QSocketNotifier(socket_stream_fd, QSocketNotifier::Read, this);
    connect(check_stream, SIGNAL(activated(int)), this, SLOT(stream_payload()), Qt::DirectConnection);

}


Zstream_push::~Zstream_push()
{}

void Zstream_push::destructor()
{
    qDebug() << "Zstream_push destructor";
    m_mutex->lock();

    check_stream->setEnabled(false);
    z_stream->close ();
    delete(z_stream);
}


void Zstream_push::stream_payload()
{
    m_mutex->lock();
    check_stream->setEnabled(false);

    std::cout << "Zstream_push::stream_payload" << std::endl;

    qint32 events = 0;
    std::size_t eventsSize = sizeof(events);
    z_stream->getsockopt(ZMQ_EVENTS, &events, &eventsSize);



    if (events & ZMQ_POLLIN)
    {
        std::cout << "Zstream_push::stream_payload ZMQ_POLLIN" <<  std::endl;


        while (true) {
            flush_socket:

            zmq::message_t request;

            bool res = z_stream->recv (&request, ZMQ_NOBLOCK);
            if (!res && zmq_errno () == EAGAIN) break;


            std::cout << "Zstream_push::stream_payload received request: [" << (char*) request.data() << "]" << std::endl;


            if (request.size() == 0) {
                std::cout << "Zstream_push::stream_payload received request 0" << std::endl;
                break;
            }



            BSONObj l_payload;
            try {
                l_payload = bo((char*)request.data());
                std::cout << "Zstream_push Received payload : " << l_payload << std::endl;
            }
            catch (mongo::MsgAssertionException &e)
            {
                std::cout << "error on data : " << l_payload << std::endl;
                std::cout << "error on data BSON : " << e.what() << std::endl;
                goto flush_socket;
            }

            QString payload_action = QString::fromStdString(l_payload.getField("action").str());

            if (payload_action == "get_filename")
            {
                std::cout << "!!!!!! get_filename RECEIVE !!!" << std::endl;

                BSONObj session_uuid = BSON("uuid" << l_payload.getFieldDotted("payload.session_uuid").str());
                BSONObj session = nosql_->Find("sessions", session_uuid);
                std::cout << "!!!!! session !!!!!! : " << session << std::endl;

                BSONObj payload_id = BSON("_id" << session.getField("payload_id").OID());
                std::cout << "payload_id : " << payload_id << std::endl;

                BSONObj payload = nosql_->Find("payloads", payload_id);

                BSONElement gfs_id = payload.getField("gfs_id");

                std::cout << "EXTRACT GFSID : " << gfs_id << std::endl;
                BSONObj gfsid = BSON("_id" << gfs_id);

                string filename = nosql_->GetFilename(gfsid.firstElement());
                std::cout << "filename : " << filename << std::endl;



                BSONObj b_filename = BSON("filename" << filename);
                z_message->rebuild(b_filename.objsize());
                memcpy ((void *) z_message->data(), (char*)b_filename.objdata(), b_filename.objsize());
                z_stream->send (*z_message, ZMQ_NOBLOCK);
                goto flush_socket;
            }

            else if (payload_action == "get_file")
            {
                BSONObj session_uuid = BSON("uuid" << l_payload.getFieldDotted("payload.session_uuid").str());
                BSONObj session = nosql_->Find("sessions", session_uuid);
                std::cout << "!!!!! session !!!!!! : " << session << std::endl;

                BSONObj payload_id = BSON("_id" << session.getField("payload_id").OID());
                std::cout << "payload_id : " << payload_id << std::endl;

                BSONObj payload = nosql_->Find("payloads", payload_id);

                BSONElement gfs_id = payload.getField("gfs_id");

                std::cout << "EXTRACT GFSID : " << gfs_id << std::endl;


                BSONObj gfsid = BSON("_id" << gfs_id);



                int num_chunck = nosql_->GetNumChunck(gfsid.firstElement());
                std::cout << "NUM CHUCK : " << num_chunck << std::endl;

                //std::fstream out;
                //out.open("/tmp/nodecast/dump_gridfile", ios::out);

                //ofstream out ("/tmp/nodecast/dump_gridfile",ofstream::binary);


                QFile out("/tmp/nodecast/dump_gridfile");
                out.open(QIODevice::WriteOnly);
                //out.write(requestContent);
                //out.close();

                QByteArray chunk_data;

                for ( int chunk_index = 0; chunk_index < num_chunck; chunk_index++ )
                {
                    std::cout << "BEFORE GET CHUNCK " << std::endl;

                    int chunk_length;

                    QBool res = nosql_->ExtractByChunck(gfsid.firstElement(), chunk_index, chunk_data, chunk_length);
                    //std::cout << "AFTER GET CHUNCK chunk_data : " << chunk_data << std::endl;
                    if (res)
                    {

                        std::cout << "Zstream_push::stream_payload CHUNK LEN : " << chunk_length << " size : " << chunk_data.size() << std::endl;


                        out.write( chunk_data.constData(), chunk_data.size());


                        //s_chunk_data = chunk_data.toBase64();
                        //chunk_data.clear();

                        //std::cout << "Zstream_push::stream_payload CHUNK toBase64 LEN : " << chunk_length << " size : " << s_chunk_data.size() << std::endl;



                        z_message->rebuild(chunk_data.size());
                        memcpy((void *) z_message->data(), chunk_data.constData(), chunk_data.size());


                        std::cout << "Zstream_push::stream_payload MESSAGE LEN : " << z_message->size() << std::endl;



                        bool l_res = z_stream->send(*z_message, (chunk_index+1<num_chunck)? ZMQ_SNDMORE | ZMQ_NOBLOCK: 0);
                        if (!l_res)
                        {
                            std::cout << "ERROR ON STREAMING DATA" << std::endl;
                            break;
                        }
                    }

                    chunk_data.clear();
                }
                out.close();
                qDebug() << "END OF STREAM CHUNCK";
            }
            else
            {
                BSONObj b_error = BSON("error" << "action unknown : " + payload_action.toStdString());
                z_message->rebuild(b_error.objsize());
                memcpy ((void *) z_message->data(), (char*)b_error.objdata(), b_error.objsize());
                z_stream->send (*z_message, ZMQ_NOBLOCK);
                goto flush_socket;
            }
        }
    }
    check_stream->setEnabled(true);
    m_mutex->unlock();
}





Zeromq *Zeromq::_singleton = NULL;


Zeromq::Zeromq()
{
    qDebug() << "Zeromq::construct";


    qRegisterMetaType<bson::bo>("bson::bo");
    qRegisterMetaType<BSONObj>("BSONObj");

    nosql_ = Nosql::getInstance_front();


    /******* QTHREAD ACCORDING TO
    http://developer.qt.nokia.com/doc/qt-4.7/qthread.html ->
                        Subclassing no longer recommended way of using QThread
    http://labs.qt.nokia.com/2010/06/17/youre-doing-it-wrong/
    ******************************/

    m_http_mutex = new QMutex();
    m_xmpp_mutex = new QMutex();

    m_context = new zmq::context_t(1);

    _singleton = this;       
    pull_timer = new QTimer();

}


Zeromq::~Zeromq()
{
    qDebug() << "pull_timer stop";
    pull_timer->stop ();

    qDebug() << "BEFORE SHUTDOWN";
    emit shutdown();
    qDebug() << "AFTER SHUTDOWN";

    qDebug() << "Zeromq delete ztracker";
    qDebug() << "stop ztracker thread";
    thread_tracker->quit ();
    while(!thread_tracker->isFinished ()){};
    delete(ztracker);

    qDebug() << "Zeromq delete pull";
    qDebug() << "stop pull thread";
    thread_pull->quit ();
    while(!thread_pull->isFinished ()){};
    delete(pull);

    qDebug() << "Zeromq delete dispatch";
    qDebug() << "stop dispatch thread";
    thread_dispatch->quit ();
    while(!thread_dispatch->isFinished ()){};
    delete(dispatch);

    qDebug() << "Zeromq delete stream_push";
    qDebug() << "stop stream_push thread";
    thread_stream->quit ();
    while(!thread_stream->isFinished ()){};
    delete(stream_push);

    qDebug() << "Zeromq close zworkers socket";
    z_workers->close ();
    qDebug() << "Zeromq delete zworkers";
    delete(z_workers);

    qDebug() << "Zeromq delete m_context";
    // delete zmq context block the exit ...
    delete(m_context);
    qDebug() << "Zeromq after delete m_context";
}



Zeromq* Zeromq::getInstance() {
    if (NULL == _singleton)
        {
          qDebug() << "creating singleton.";
          _singleton =  new Zeromq();
        }
      else
        {
          qDebug() << "singleton already created!";
        }
      return _singleton;
}



void Zeromq::init()
{
    qDebug() << "Zeromq::init";

    z_workers = new zmq::socket_t (*m_context, ZMQ_PUSH);

    //uint64_t hwm = 50000;
    //zmq_setsockopt (z_workers, ZMQ_HWM, &hwm, sizeof (hwm));

    z_workers->bind("inproc://payload");

    thread_dispatch = new QThread;
    dispatch = new Zdispatch(m_context);
    dispatch->moveToThread(thread_dispatch);
    thread_dispatch->start();
    connect(this, SIGNAL(shutdown()), dispatch, SLOT(destructor()), Qt::BlockingQueuedConnection);



    thread_tracker = new QThread;
    ztracker = new Ztracker(m_context);
    ztracker->moveToThread(thread_tracker);
    thread_tracker->start();

    connect(this, SIGNAL(shutdown()), ztracker, SLOT(destructor()), Qt::BlockingQueuedConnection);
    connect(ztracker, SIGNAL(create_server(QString,QString)), dispatch, SLOT(bind_server(QString,QString)), Qt::QueuedConnection);



    thread_pull = new QThread;
    pull = new Zpull(m_context);
    pull->moveToThread(thread_pull);    
    thread_pull->start();
    connect(this, SIGNAL(shutdown()), pull, SLOT(destructor()), Qt::BlockingQueuedConnection);
    connect(pull, SIGNAL(forward_payload(BSONObj)), dispatch, SLOT(push_payload(BSONObj)), Qt::QueuedConnection);





    thread_stream = new QThread;
    stream_push = new Zstream_push(m_context);
    stream_push->moveToThread(thread_stream);
    thread_stream->start();
    connect(this, SIGNAL(shutdown()), stream_push, SLOT(destructor()), Qt::BlockingQueuedConnection);

    //connect(pull, SIGNAL(forward_payload(BSONObj)), dispatch, SLOT(push_payload(BSONObj)), Qt::QueuedConnection);



    /**** PULL DATA ON HTTP API ****
    QThread *thread_http_receive = new QThread;
    receive_http = new Zreceive(m_context, z_workers, "http");
    connect(thread_http_receive, SIGNAL(started()), receive_http, SLOT(init_payload()));
    receive_http->moveToThread(thread_http_receive);
    thread_http_receive->start();*/
//    QMetaObject::invokeMethod(receive, "init_payload", Qt::QueuedConnection);



    /**** PULL DATA ON XMPP API ****
    QThread *thread_xmpp_receive = new QThread;
    receive_xmpp = new Zreceive(m_context, z_workers, "xmpp");
    connect(thread_xmpp_receive, SIGNAL(started()), receive_xmpp, SLOT(init_payload()));
    receive_xmpp->moveToThread(thread_xmpp_receive);
    thread_xmpp_receive->start();*/



    /**** PULL DATA FROM WORKERS ****
    QThread *thread_zeromq_receive = new QThread;
    receive_zeromq = new Zreceive(m_context, z_workers, "workers");
    connect(thread_zeromq_receive, SIGNAL(started()), receive_zeromq, SLOT(init_payload()));
    receive_zeromq->moveToThread(thread_zeromq_receive);
    thread_zeromq_receive->start();*/

    std::cout << "Zeromq::Zeromq AFTER thread_receive" << std::endl;

}
