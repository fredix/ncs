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



Ztracker::Ztracker(zmq::context_t *a_context) : m_context(a_context)
{
    std::cout << "Ztracker::Ztracker construct" << std::endl;

    m_mutex = new QMutex();


    nosql_ = Nosql::getInstance_back();

    /***************** SERVICE SOCKET *************/
    m_message = new zmq::message_t(2);
    m_socket = new zmq::socket_t (*m_context, ZMQ_REP);

    uint64_t hwm = 50000;
    m_socket->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    m_socket->bind("tcp://*:5569");
    /**********************************************/



    int socket_tracker_fd;
    size_t socket_size;
    m_socket->getsockopt(ZMQ_FD, &socket_tracker_fd, &socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << socket_tracker_fd << " errno : " << zmq_strerror (errno);

    check_tracker = new QSocketNotifier(socket_tracker_fd, QSocketNotifier::Read, this);
    connect(check_tracker, SIGNAL(activated(int)), this, SLOT(receive_payload()), Qt::DirectConnection);




    /**************** DATA SOCKET *****************/
    m_data_message = new zmq::message_t(2);
    m_data_socket = new zmq::socket_t(*m_context, ZMQ_PUSH);
    m_data_socket->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    //m_data_socket->bind("inproc://workers");
    m_data_socket->bind("ipc:///tmp/nodecast/workers");
    /**********************************************/

    worker_timer = new QTimer();
    connect(worker_timer, SIGNAL(timeout()), this, SLOT(worker_update_ticker ()), Qt::DirectConnection);
    worker_timer->start (5000);

    service_timer = new QTimer();
    connect(service_timer, SIGNAL(timeout()), this, SLOT(service_update_ticker ()), Qt::DirectConnection);
    service_timer->start (5000);
}

int Ztracker::get_available_port()
{
    BSONObj empty;
    QList <BSONObj> workers_list = nosql_->FindAll("workers", empty);

    int port_counter = 5559;

    foreach (BSONObj worker, workers_list)
    {
        std::cout << "worker : " << worker.getField("port").numberInt() << std::endl;

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
            zmq::message_t request;

            // Wait for next request from client
            bool res = m_socket->recv (&request, ZMQ_NOBLOCK);
            if (!res && zmq_errno () == EAGAIN) break;


            std::cout << "Ztracker::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

            char *plop = (char*) request.data();
            if (strlen(plop) == 0) {
                std::cout << "Zpull::worker_response STRLEN received request 0" << std::endl;
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
                break;
            }


            if (l_payload.getFieldDotted("payload.type").str() == "worker")
            {
                if (l_payload.getFieldDotted("payload.action").str() == "register")
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

                        worker = BSON(GENOID << "name" << l_payload.getFieldDotted("payload.name") << "port" << worker_port);
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
                    worker_builder.append(l_payload.getFieldDotted("payload.timestamp"));
                    worker_builder.append("status", "up");


                    BSONObj node = BSON("nodes" << worker_builder.obj());

                    be worker_id = worker.getField("_id");
                    nosql_->Addtoarray("workers", worker_id.wrap(), node);


                    m_message->rebuild(payload.objsize());
                    memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
                }
                else if (l_payload.getFieldDotted("payload.action").str() == "watchdog")
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
                else if (l_payload.getFieldDotted("payload.action").str() == "terminate")
                {
                    qDebug() << "RECEIVE TERMINATE !!!!";
                    BSONObj payload = l_payload.getField("payload").Obj();


                    /****** PUSH API PAYLOAD *******/
                    qDebug() << "FORWARD NEXT PAYLOAD";
                    m_data_message->rebuild(payload.objsize());
                    memcpy(m_data_message->data(), (char*)payload.objdata(), payload.objsize());
                    m_data_socket->send(*m_data_message);
                    /************************/
                   }

            }

            else if (l_payload.getFieldDotted("payload.type").str() == "service")
            {
                if (l_payload.getFieldDotted("payload.action").str() == "register")
                {
                    QUuid session_uuid = QUuid::createUuid();
                    QString str_session_uuid = session_uuid.toString().mid(1,36);

                    qDebug() << "REGISTER ID : " << str_session_uuid;
                    std::cout << "PAYLOAD : " << l_payload << std::endl;


                    // Send reply back to client
                    bo payload = BSON("uuid" << str_session_uuid.toStdString());

                    // workers_builder.append("name", l_payload.getFieldDotted("payload.worker"));


                    bob services_builder;
                    services_builder.genOID();
                    services_builder.append(payload.getField("uuid"));
                    services_builder.append(l_payload.getFieldDotted("payload.pid"));
                    services_builder.append(l_payload.getFieldDotted("payload.timestamp"));
                    services_builder.append("status", "up");


                    BSONObj node = BSON("nodes" << services_builder.obj());

                    be service_name = l_payload.getFieldDotted("payload.name");
                    BSONObj service = nosql_->Find("services", service_name.wrap());

                    if (service.nFields() == 0)
                    {
                        service = BSON(GENOID << "name" << l_payload.getFieldDotted("payload.name"));
                        nosql_->Insert("services", service);
                    }

                    be service_id = service.getField("_id");

                    nosql_->Addtoarray("services", service_id.wrap(), node);



                    m_message->rebuild(payload.objsize());
                    memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
                }
                else if (l_payload.getFieldDotted("payload.action").str() == "watchdog")
                   {
                       be timestamp = l_payload.getFieldDotted("payload.timestamp");
                      // be child_pid = l_payload.getFieldDotted("payload.child_pid");

                       be uuid = l_payload.getField("uuid");

                       bo bo_node_uuid = BSON("nodes.uuid" << uuid);
                       //bo service_node = BSON("nodes.$.timestamp" << timestamp << "nodes.$.child_pid" << child_pid);
                       bo service_node = BSON("nodes.$.timestamp" << timestamp);

                       qDebug() << "UPDATE NODE's TIMESTAMP";
                       nosql_->Update("services", bo_node_uuid, service_node);

                       BSONObj payload = BSON("status" << "ACK");
                       m_message->rebuild(payload.objsize());
                       memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
                   }
            }
            else if (l_payload.getFieldDotted("payload.type").str() == "init")
            {
                qDebug() << "RECEIVE INIT SOCKET";
                BSONObj payload = BSON("status" << "pong");
                m_message->rebuild(payload.objsize());
                memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
            }

            m_socket->send(*m_message);
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

            be node_id;
            l_node.getObjectID (node_id);

            be node_timestamp = l_node.getField("timestamp");
            be status = l_node.getField("status");
            be uuid = l_node.getField("uuid");

            t_timestamp.setTime_t(node_timestamp.number());


            //qDebug() << "QT TIMESTAMP : " << t_timestamp.toString("dd MMMM yyyy hh:mm:ss");
            //qDebug() << "SECONDES DIFF : " << t_timestamp.secsTo(l_timestamp);
            //std::cout << "STATUS : " <<  status.str() << std::endl;


            if (t_timestamp.secsTo(l_timestamp) > 30 && status.str() == "up")
            {
                qDebug() << "SEND ALERT !!!!!!!";

                BSONObj bo_node_id = BSON("nodes._id" << node_id.OID());
                BSONObj worker_status = BSON("nodes.$.status" << "down");
                nosql_->Update("workers", bo_node_id, worker_status);


                QString l_worker = "WORKER ";
                l_worker.append(QString::fromStdString(worker.getField("name").str()));
                l_worker.append (", uuid : ").append (uuid.valuestr()).append (", at : ").append (t_timestamp.toString("dd MMMM yyyy hh:mm:ss"));
                qDebug() << "WORKER ALERT ! " << l_worker;
                emit sendAlert(l_worker);
            }
        }
    }

    m_mutex->unlock();
}




void Ztracker::service_update_ticker()
{
    std::cout << "Ztracker::update_ticker" << std::endl;

    m_mutex->lock();

    QDateTime l_timestamp = QDateTime::currentDateTime();
    qDebug() << l_timestamp.toTime_t ();

    QDateTime t_timestamp;

    BSONObj empty;
    QList <BSONObj> services_list = nosql_->FindAll("services", empty);
    QList<BSONObj>::iterator i;


    foreach (BSONObj service, services_list)
    {    
        //std::cout << "TICKER !!!  workers : " << workers << std::endl;

        BSONObj nodes = service.getField("nodes").Obj();


        list<be> list_nodes;
        nodes.elems(list_nodes);
        list<be>::iterator i;

        /********   Iterate over each worker's nodes   *******/
        /********  find node with uuid and set the node id to payload collection *******/
        for(i = list_nodes.begin(); i != list_nodes.end(); ++i) {
            BSONObj l_node = (*i).embeddedObject ();

            //std::cout << "L_NODE : " << l_node << std::endl;

            be node_id;
            l_node.getObjectID (node_id);

            be node_timestamp = l_node.getField("timestamp");
            be status = l_node.getField("status");
            be uuid = l_node.getField("uuid");

            t_timestamp.setTime_t(node_timestamp.number());


            qDebug() << "QT TIMESTAMP : " << t_timestamp.toString("dd MMMM yyyy hh:mm:ss");
            qDebug() << "SECONDES DIFF : " << t_timestamp.secsTo(l_timestamp);
            std::cout << "STATUS : " <<  status.str() << std::endl;


            if (t_timestamp.secsTo(l_timestamp) > 30 && status.str() == "up")
            {
                qDebug() << "SEND ALERT !!!!!!!";

                BSONObj bo_node_id = BSON("nodes._id" << node_id.OID());
                BSONObj service_status = BSON("nodes.$.status" << "down");
                nosql_->Update("services", bo_node_id, service_status);


                QString l_service = "SERVICE ";
                l_service.append(QString::fromStdString(service.getField("name").str()));
                l_service.append (", uuid : ").append (uuid.valuestr()).append (", at : ").append (t_timestamp.toString("dd MMMM yyyy hh:mm:ss"));
                qDebug() << "SERVICE ALERT ! " << l_service;
                emit sendAlert(l_service);
            }
        }
    }

    m_mutex->unlock();
}



Ztracker::~Ztracker()
{}




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
{}





Zpull::Zpull(zmq::context_t *a_context) : m_context(a_context)
{        
    std::cout << "Zpull::Zpull constructeur" << std::endl;

    m_socket_http = new zmq::socket_t (*m_context, ZMQ_PULL);
    uint64_t hwm = 50000;
    m_socket_http->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    m_socket_http->connect("ipc:///tmp/nodecast/http");

    m_socket_workers = new zmq::socket_t (*m_context, ZMQ_PULL);
    m_socket_workers->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    m_socket_workers->connect("ipc:///tmp/nodecast/workers");



    int http_socket_fd;
    size_t socket_size;
    m_socket_http->getsockopt(ZMQ_FD, &http_socket_fd, &socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << http_socket_fd << " errno : " << zmq_strerror (errno);

    check_http_data = new QSocketNotifier(http_socket_fd, QSocketNotifier::Read, this);
    connect(check_http_data, SIGNAL(activated(int)), this, SLOT(receive_payload()));



    int worker_socket_fd;
    size_t worker_socket_size;
    m_socket_workers->getsockopt(ZMQ_FD, &worker_socket_fd, &worker_socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << worker_socket_fd << " errno : " << zmq_strerror (errno);

    check_worker_response = new QSocketNotifier(worker_socket_fd, QSocketNotifier::Read, this);
    connect(check_worker_response, SIGNAL(activated(int)), this, SLOT(worker_response()));
}


Zpull::~Zpull()
{}



void Zpull::receive_payload()
{
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

            char *plop = (char*) request.data();
            if (strlen(plop) == 0) {
                std::cout << "Zpull::worker_response STRLEN received request 0" << std::endl;
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
                    emit forward_payload(data);
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
                break;
            }
        }

    }
    check_http_data->setEnabled(true);
}





void Zpull::worker_response()
{
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

            std::cout << "Zpull::worker_response received request: [" << (char*) request.data() << "]" << std::endl;

            char * plop = (char*) request.data();
            if (strlen(plop) == 0) {
                std::cout << "Zpull::worker_response STRLEN received request 0" << std::endl;
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
                    emit forward_payload(data);
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
                break;
            }

        }
    }
    check_worker_response->setEnabled(true);
}




Zdispatch::Zdispatch(zmq::context_t *a_context) : m_context(a_context)
{
    m_mutex = new QMutex();

    std::cout << "Zdispatch::Zdispatch constructeur" << std::endl;
    //nosql_ = Nosql::getInstance_back();
    nosql_ = Nosql::getInstance_front();


    BSONObj empty;
    workflow_list = nosql_->FindAll("workflows", empty);
    worker_list = nosql_->FindAll("workers", empty);

    // read workers collection and instance workers's server
    foreach (BSONObj worker, worker_list)
    {
        std::cout << "LIST SIZE : " << worker_list.size() << std::endl;

        for (int i = 0; i < worker_list.size(); ++i) {
            std::cout << "LIST WORKERS : " << worker_list.at(i) << std::endl;
         }

        std::cout << "WORKER : " << worker << std::endl;

        int port = worker.getField("port").number();
        std::cout << "INT worker port : " << port << std::endl;

        QString w_name = QString::fromStdString(worker.getField("name").str());
        QString w_port = QString::number(worker.getField("port").Int());
        std::cout << "worker name : " << w_name.toStdString() << std::endl;
        std::cout << "worker port : " << w_port.toStdString() << std::endl;

        bind_server(w_name, w_port);
    }
}


Zdispatch::~Zdispatch()
{}

void Zdispatch::bind_server(QString name, QString port)
{
    qDebug() << "BEFORE BIND SERVER SLOT !";
    workers_push[name] =  QSharedPointer<Zworker_push> (new Zworker_push(m_context, name.toStdString(), port.toStdString()));

    qDebug() << "AFTER BIND SERVER SLOT !";

    BSONObj empty;
    worker_list.clear();
    worker_list = nosql_->FindAll("workers", empty);
}




void Zdispatch::push_payload(BSONObj data)
{
        std::cout << "Zdispatch received data : " << data << std::endl;


        //BSONElement session_uuid = data.getField("session_uuid");
        BSONObj session_uuid = BSON("uuid" << data.getField("session_uuid").str());
        BSONElement b_session_uuid = session_uuid.getField("uuid");

        BSONElement b_action = data.getField("action");


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




        BSONElement timestamp = session.getField("start_timestamp");

        //std::cout << "ACTION : " << data.getField("action") << std::endl;
        std::cout << "start_timestamp : " << timestamp << std::endl;


        //string action = data.getField("action").valuestr();
        string action = b_action.valuestr();

        std::cout << "ACTION : " << action << std::endl;

        BSONElement gfs_id = payload.getField("gfs_id");


        BSONElement datas;
        if (data.hasField("datas")) datas = data.getField("datas");


        if (action.compare("create") == 0 && w_workers.nFields() !=0)
        {
            // create step
            QString worker_name;
            QString worker_port;


            std::cout << "Zdispatch::receive_payload : ACTION CREATE : " << action << std::endl;

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


                /* EXTRACT PAYLOAD */
                QString path = "/tmp/";


                std::cout << "BINARY && COPY : " << gfs_id << std::endl;
                QString filename;

                if (nosql_->ExtractBinary(gfs_id, path.toStdString(), filename))
                {
                    qDebug() << "EXTRACT PAYLOAD, FILENAME : " << filename;

                    /********** RECORD PAYLOAD STEP *********/
                    BSONObjBuilder step_builder;
                    step_builder.genOID();
                    step_builder.append("name", worker_name.toStdString());
                    step_builder.append("action", "create");
                    step_builder.append("order", 1);
                    step_builder.append("payload", path.append(filename).toStdString());
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


                    BSONObj payload = BSON("datas" << path.toStdString() << "session_uuid" << b_session_uuid.str());

                    qDebug() << "push_payload : " << worker_name;
                    workers_push[worker_name]->push_payload(payload);


                }
            }
            else qDebug() << "WORKER NERVER CONNECTED : LOST PAYLOAD";

        }
        else if (action.compare("terminate") == 0)
        {
            std::cout << "Zdispatch::receive_payload : ACTION TERMINATE : " << action << std::endl;
            // add step
            //be payload_step_id = session.getField("step_id");
            //data.getObjectID (payload_step_id);

            //BSONObj payload_id = BSON("_id" << session.getField("payload_id").OID());


            BSONObj search = BSON("steps._id" << session.getField("step_id").OID());

            std::cout << "search : " << search << std::endl;

            BSONObj field = BSON("_id" << 0 << "steps.order" << 1);

            //BSONObj old_step = nosql_->Find("payloads", search);

            BSONObj step_order = nosql_->Find("payloads", search, &field);


            std::cout << "step_order: " << step_order << std::endl;


            /* Gruik ........................... !!*/
            BSONElement order = step_order.getField("steps").embeddedObject().getFieldDotted("0.order");
            /**************************************/

            int workflow_order = order.numberInt() + 1;
            std::cout << "ORDER YO : " << workflow_order << std::endl;


            std::cout << "search : " << search << "STEP ORDER : " << step_order << std::endl;
            std::cout << "STEP ORDER : " << order.Number() << std::endl;


            be worker_name = data.getField("name");
            be exitcode = data.getField("exitcode");
            be exitstatus = data.getField("exitstatus");
            be datas = data.getField("datas");


            /********** RECORD PAYLOAD STEP *********/
            BSONObjBuilder step_builder;
            BSONObj step_id = BSON("steps._id" << session.getField("step_id").OID());

            std::cout << "STEP ID " << step_id << std::endl;

            step_builder << "steps.$.name" << worker_name.str();
            step_builder << "steps.$.action" << "terminate";
            step_builder << "steps.$.datas" << datas.str();
            step_builder << "steps.$.exitcode" << exitcode;
            step_builder << "steps.$.exitstatus" << exitstatus;
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
                BSONObj payload = BSON("datas" << datas.str() << "session_uuid" << b_session_uuid.str());
                qDebug() << "push_payload : " << w_name;
                workers_push[w_name]->push_payload(payload);
            }
            else qDebug() << "WORKER NERVER CONNECTED : LOST PAYLOAD";

        }


        std::cout << "!!!!!!! PAYLOAD SEND !!!!" << std::endl;
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
{}


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
        std::cout << "!!!!!!!!!!!!!!!!!!!   PAYLOAD BACKUPED !!!!!!!!!!!!!!!!!!" << std::endl;
        std::cout << "EVENTS : " << events << " ERROR" << zmq_strerror(zmq_errno()) << std::endl;

        QDateTime timestamp = QDateTime::currentDateTime();

        BSONObj payload = BSON("worker" << m_worker << "timestamp" << timestamp.toTime_t() << "data" << a_payload);
        nosql_->Insert("lost_payloads", payload);
    }

    std::cout << "Zworker_push::push_payload after send" << std::endl;
    m_mutex->unlock();
}


Zeromq *Zeromq::_singleton = NULL;


Zeromq::Zeromq()
{
    qDebug() << "Zeromq::construct";

    nosql_ = Nosql::getInstance_back();


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
{}



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

    zmq::socket_t *z_workers = new zmq::socket_t (*m_context, ZMQ_PUSH);

    uint64_t hwm = 50000;
    zmq_setsockopt (z_workers, ZMQ_HWM, &hwm, sizeof (hwm));

    z_workers->bind("inproc://payload");


    qRegisterMetaType<BSONObj>("BSONObj");


    QThread *thread_dispatch = new QThread;
    dispatch = new Zdispatch(m_context);
    dispatch->moveToThread(thread_dispatch);
    thread_dispatch->start();


    QThread *thread_tracker = new QThread;
    ztracker = new Ztracker(m_context);
    ztracker->moveToThread(thread_tracker);
    thread_tracker->start();

    connect(ztracker, SIGNAL(create_server(QString,QString)), dispatch, SLOT(bind_server(QString,QString)), Qt::QueuedConnection);



    QThread *thread_pull = new QThread;
    pull = new Zpull(m_context);
    pull->moveToThread(thread_pull);
    thread_pull->start();
    connect(pull, SIGNAL(forward_payload(BSONObj)), dispatch, SLOT(push_payload(BSONObj)), Qt::QueuedConnection);



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
