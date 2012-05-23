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

#include "zeromq.h"



Ztracker::Ztracker(zmq::context_t *a_context) : m_context(a_context)
{
    std::cout << "Ztracker::Ztracker construct" << std::endl;

    m_mutex = new QMutex();


    nosql_ = Nosql::getInstance_back();

    m_message = new zmq::message_t(2);
    m_socket = new zmq::socket_t (*m_context, ZMQ_REP);

    uint64_t hwm = 5000;
    zmq_setsockopt (m_socket, ZMQ_HWM, &hwm, sizeof (hwm));

    m_socket->bind("tcp://*:5569");

    worker_timer = new QTimer();
    connect(worker_timer, SIGNAL(timeout()), this, SLOT(worker_update_ticker ()), Qt::DirectConnection);
    worker_timer->start (5000);

    service_timer = new QTimer();
    connect(service_timer, SIGNAL(timeout()), this, SLOT(service_update_ticker ()), Qt::DirectConnection);
    service_timer->start (5000);
}


void Ztracker::init()
{   
    qDebug() << "Ztracker::init !!!!!!!!!";

    while (true) {
    zmq::message_t request;

    // Wait for next request from client
    m_socket->recv (&request);

    bo l_payload = bo((char*)request.data());

    std::cout << "Ztracker Received payload : " << l_payload << std::endl;



    if (l_payload.getFieldDotted("payload.type").str() == "worker")
    {
        if (l_payload.getFieldDotted("payload.action").str() == "register")
        {
            QUuid session_uuid = QUuid::createUuid();
            QString str_session_uuid = session_uuid.toString().mid(1,36);

            qDebug() << "REGISTER ID : " << str_session_uuid;
            std::cout << "PAYLOAD : " << l_payload << std::endl;


            // Send reply back to client
            bo payload = BSON("uuid" << str_session_uuid.toStdString());

            // worker_track_builder.append("name", l_payload.getFieldDotted("payload.worker"));


            bob worker_track_builder;
            worker_track_builder.genOID();
            worker_track_builder.append(payload.getField("uuid"));
            worker_track_builder.append(l_payload.getFieldDotted("payload.pid"));
            worker_track_builder.append(l_payload.getFieldDotted("payload.timestamp"));
            worker_track_builder.append("status", "up");


            bo node = BSON("nodes" << worker_track_builder.obj());

            be worker_name = l_payload.getFieldDotted("payload.name");
            bo worker_track = nosql_->Find("worker_track", worker_name.wrap());

            if (worker_track.nFields() == 0)
            {
                worker_track = BSON(GENOID << "name" << l_payload.getFieldDotted("payload.name"));
                nosql_->Insert("worker_track", worker_track);
            }

            be worker_track_id = worker_track.getField("_id");

            nosql_->Addtoarray("worker_track", worker_track_id.wrap(), node);



            m_message->rebuild(payload.objsize());
            memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
        }
        else if (l_payload.getFieldDotted("payload.action").str() == "watchdog")
           {
               be timestamp = l_payload.getFieldDotted("payload.timestamp");
               be uuid = l_payload.getField("uuid");

               bo bo_node_uuid = BSON("nodes.uuid" << uuid);
               bo worker_track_node = BSON("nodes.$.timestamp" << timestamp);

               qDebug() << "UPDATE NODE's TIMESTAMP";
               nosql_->Update("worker_track", bo_node_uuid, worker_track_node);

               bo payload = BSON("status" << "ACK");
               m_message->rebuild(payload.objsize());
               memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
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

            // worker_track_builder.append("name", l_payload.getFieldDotted("payload.worker"));


            bob service_track_builder;
            service_track_builder.genOID();
            service_track_builder.append(payload.getField("uuid"));
            service_track_builder.append(l_payload.getFieldDotted("payload.pid"));
            service_track_builder.append(l_payload.getFieldDotted("payload.timestamp"));
            service_track_builder.append("status", "up");


            bo node = BSON("nodes" << service_track_builder.obj());

            be service_name = l_payload.getFieldDotted("payload.name");
            bo service_track = nosql_->Find("service_track", service_name.wrap());

            if (service_track.nFields() == 0)
            {
                service_track = BSON(GENOID << "name" << l_payload.getFieldDotted("payload.name"));
                nosql_->Insert("service_track", service_track);
            }

            be service_track_id = service_track.getField("_id");

            nosql_->Addtoarray("service_track", service_track_id.wrap(), node);



            m_message->rebuild(payload.objsize());
            memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
        }
        else if (l_payload.getFieldDotted("payload.action").str() == "watchdog")
           {
               be timestamp = l_payload.getFieldDotted("payload.timestamp");
               be child_pid = l_payload.getFieldDotted("payload.child_pid");

               be uuid = l_payload.getField("uuid");

               bo bo_node_uuid = BSON("nodes.uuid" << uuid);
               bo service_track_node = BSON("nodes.$.timestamp" << timestamp << "nodes.$.child_pid" << child_pid);

               qDebug() << "UPDATE NODE's TIMESTAMP";
               nosql_->Update("service_track", bo_node_uuid, service_track_node);

               bo payload = BSON("status" << "ACK");
               m_message->rebuild(payload.objsize());
               memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
           }
    }
    else if (l_payload.getFieldDotted("payload.type").str() == "init")
    {
        qDebug() << "RECEIVE INIT SOCKET";
        bo payload = BSON("status" << "pong");
        m_message->rebuild(payload.objsize());
        memcpy(m_message->data(), (char*)payload.objdata(), payload.objsize());
    }


    m_socket->send(*m_message);
    }
}


void Ztracker::worker_update_ticker()
{
    //std::cout << "Ztracker::update_ticker" << std::endl;

    m_mutex->lock();


    QDateTime l_timestamp = QDateTime::currentDateTime();
    qDebug() << l_timestamp.toTime_t ();

    QDateTime t_timestamp;

    bo empty;
    list <bo> worker_track_list = nosql_->FindAll("worker_track", empty);
    list<bo>::iterator i;

    for(i = worker_track_list.begin(); i != worker_track_list.end(); ++i)
    {
        bo worker_track = *i;

        //std::cout << "TICKER !!!  WORKER_TRACK : " << worker_track << std::endl;

        bo nodes = worker_track.getField("nodes").Obj();


        list<be> list_nodes;
        nodes.elems(list_nodes);
        list<be>::iterator i;

        /********   Iterate over each worker's nodes   *******/
        /********  find node with uuid and set the node id to payload collection *******/
        for(i = list_nodes.begin(); i != list_nodes.end(); ++i) {
            bo l_node = (*i).embeddedObject ();

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

                bo bo_node_id = BSON("nodes._id" << node_id.OID());
                bo worker_track_status = BSON("nodes.$.status" << "down");
                nosql_->Update("worker_track", bo_node_id, worker_track_status);


                QString worker = "WORKER ";
                worker.append(QString::fromStdString(worker_track.getField("name").str()));
                worker.append (", uuid : ").append (uuid.valuestr()).append (", at : ").append (t_timestamp.toString("dd MMMM yyyy hh:mm:ss"));
                qDebug() << "WORKER ALERT ! " << worker;
                emit sendAlert(worker);
            }

            //mongo::Date_t ts = node_timestamp.timestampTime();


            //std::cout << "MONGO TIMESTAMP : " << ts.toString() << std::endl;


            //bo bo_node_id = BSON("nodes._id" << node_id.OID());

            //std::cout << "NODE ID => " << bo_node_id << std::endl;
            //std::cout << "NODE TIMESTAMP => " << node_timestamp << std::endl;

            // payload_builder.append("node_id", node_id.OID());
            //qDebug() << "REMOVE UPDATE NODE";

            //nosql_->Update("worker_track", bo_node_id, worker_track_node);

        }
    }

    m_mutex->unlock();
}




void Ztracker::service_update_ticker()
{
    //std::cout << "Ztracker::update_ticker" << std::endl;

    m_mutex->lock();


    QDateTime l_timestamp = QDateTime::currentDateTime();
    qDebug() << l_timestamp.toTime_t ();

    QDateTime t_timestamp;

    bo empty;
    list <bo> service_track_list = nosql_->FindAll("service_track", empty);
    list<bo>::iterator i;

    for(i = service_track_list.begin(); i != service_track_list.end(); ++i)
    {
        bo service_track = *i;

        //std::cout << "TICKER !!!  WORKER_TRACK : " << worker_track << std::endl;

        bo nodes = service_track.getField("nodes").Obj();


        list<be> list_nodes;
        nodes.elems(list_nodes);
        list<be>::iterator i;

        /********   Iterate over each worker's nodes   *******/
        /********  find node with uuid and set the node id to payload collection *******/
        for(i = list_nodes.begin(); i != list_nodes.end(); ++i) {
            bo l_node = (*i).embeddedObject ();

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

                bo bo_node_id = BSON("nodes._id" << node_id.OID());
                bo service_track_status = BSON("nodes.$.status" << "down");
                nosql_->Update("service_track", bo_node_id, service_track_status);


                QString service = "SERVICE ";
                service.append(QString::fromStdString(service_track.getField("name").str()));
                service.append (", uuid : ").append (uuid.valuestr()).append (", at : ").append (t_timestamp.toString("dd MMMM yyyy hh:mm:ss"));
                qDebug() << "SERVICE ALERT ! " << service;
                emit sendAlert(service);
            }

            //mongo::Date_t ts = node_timestamp.timestampTime();


            //std::cout << "MONGO TIMESTAMP : " << ts.toString() << std::endl;


            //bo bo_node_id = BSON("nodes._id" << node_id.OID());

            //std::cout << "NODE ID => " << bo_node_id << std::endl;
            //std::cout << "NODE TIMESTAMP => " << node_timestamp << std::endl;

            // payload_builder.append("node_id", node_id.OID());
            //qDebug() << "REMOVE UPDATE NODE";

            //nosql_->Update("worker_track", bo_node_id, worker_track_node);

        }
    }

    m_mutex->unlock();
}



Ztracker::~Ztracker()
{}




Zreceive::Zreceive(zmq::context_t *a_context, QString a_port, QString a_inproc) : m_context(a_context), m_port(a_port), m_inproc(a_inproc)
{
    qDebug() << "Zreceive::construct";

//    m_context = a_context;

    //m_context = new zmq::context_t(1);
    //zmq::context_t context (1);

    // zmq::socket_t sender (*z_context, ZMQ_XREP);
    // sender.bind("tcp://*:5555");

    z_workers = new zmq::socket_t (*m_context, ZMQ_PUSH);


    uint64_t hwm = 50000;
    zmq_setsockopt (z_workers, ZMQ_HWM, &hwm, sizeof (hwm));

    z_workers->bind("tcp://*:" + m_port.toAscii());
}


void Zreceive::init_payload()
{
    //m_mutex->lock();
    qDebug() << "Zreceive::init_payload, proc ; " << m_inproc <<" , port : " << m_port;
    z_sender = new zmq::socket_t(*m_context, ZMQ_PULL);
    //z_sender->connect("tcp://*:5555");
    qDebug() << "PORT : " << m_port << " INPROC : " << m_inproc;
    //z_sender->connect("tcp://*:" + m_port.toAscii());
    //sleep(1);

    uint64_t hwm = 5000;
    zmq_setsockopt (z_sender, ZMQ_HWM, &hwm, sizeof (hwm));

    z_sender->connect("inproc://" + m_inproc.toAscii());


    ///////!!!!!!!!!!!!!zmq::socket_t z_workers (*m_context, ZMQ_PUSH);
    //z_workers.bind ("inproc://" + m_inproc.toAscii());
    //!!!!!!!!z_workers.bind("tcp://*:" + m_port.toAscii());
    //z_workers.bind("tcp://*:5596");

    //m_mutex->unlock();

    //  Connect work threads to client threads via a queue
    std::cout << "Zreceive::init_payload BEFORE ZMQ_STREAMER" << std::endl;
    zmq::device (ZMQ_STREAMER, *z_sender, *z_workers);
    std::cout << "Zreceive::init_payload AFTER ZMQ_STREAMER" << std::endl;
}

Zreceive::~Zreceive()
{}





Zdispatch::Zdispatch(zmq::context_t *a_context) : m_context(a_context)
{        
    std::cout << "Zdispatch::Zdispatch constructeur" << std::endl;
    nosql_ = Nosql::getInstance_back();
}


Zdispatch::~Zdispatch()
{}



void Zdispatch::receive_payload()
{
/*    m_http_mutex->lock();
    m_xmpp_mutex->lock();
*/
    std::cout << "Zdispatch::receive_payload" << std::endl;
    //zmq::socket_t socket (*m_context, ZMQ_PULL);
    m_socket = new zmq::socket_t (*m_context, ZMQ_PULL);
    //m_socket->connect("inproc://" + m_inproc.toAscii());
    m_socket->bind("tcp://*:5559");



  /*  m_http_mutex->unlock();
    m_xmpp_mutex->unlock();
*/

    std::cout << "Zdispatch::receive_payload AFTER inproc" << std::endl;


    while (true) {
        //  Wait for next request from client
        zmq::message_t request;
        m_socket->recv (&request);

        std::cout << "Zdispatch::receive_payload WHILE TRUE" << std::endl;

        //std::cout << "Zdispatch received request: [" << (char*) request.data() << "]" << std::endl;
        bo data = bo((char*)request.data());
        be uuid = data.getFieldDotted("data.uuid");
        be qname = data.getFieldDotted("data.action");
        be gfs_id = data.getFieldDotted("data.gfs_id");
        be timestamp = data.getField("timestamp");


        be action = data.getField("action");

        /********** RECORD PAYLOAD STEP *********/
        BSONObjBuilder step_builder;
        step_builder.genOID();
        step_builder.append("counter", 1);
        step_builder.append("name", "dispatcher");
        step_builder.append("action", action.str());
        step_builder.append("timestamp", timestamp.str());


        bo step = BSON("steps" << step_builder.obj());
        nosql_->Addtoarray("payload_track", uuid.wrap(), step);
        /*****************************************/



        be format = data.getField("format");
        be path = data.getField("path");

        bo workers = data.getField("workers").Obj();

        list<be> w_list;
        workers.elems(w_list);


        if (workers_push.size() == 0)
        {
            list<be>::iterator i;
            for(i = w_list.begin(); i != w_list.end(); ++i) {
                QString w_name = (*i).fieldName();

                worker_name.push_front(w_name);
                string worker_port = (*i).str();

                //std::cout << "worker : " << worker_name << "port : " << worker_port << " " << std::endl;

                workers_push[w_name] = new Zworker_push(m_context, w_name.toStdString(), worker_port);

    /*
                QThread *thread_worker_push = new QThread;
                Zworker_push *worker_push = new Zworker_push(m_context, worker_name, worker_port);
                //connect(thread_worker_load, SIGNAL(started()), worker_load, SLOT(send_payload()));

                worker_push->moveToThread(thread_worker_push);
                thread_worker_push->start();

                l_workers_push.push_front(thread_worker_push);
    */

            }

        }

        std::cout << "workers : " << workers << std::endl;

        std::cout << "uuid : " << uuid.toString() << std::endl;
        std::cout << "action : " << qname.toString() << std::endl;



        /***************** PUSH ********************/
        std::cout << "Zdispatch BEFORE emit forward_payload" << std::endl;


        std::cout << "DATA : " <<  data << std::endl;


        std::cout << "GFS ID : " << gfs_id << std::endl;



        if (format.str() == "json" && action.str() == "split")
        {
            bo l_json_datas = nosql_->ExtractJSON(gfs_id);

            qDebug() << "SPLIT !!!!!!!!!!";



            for (int i = 0; i < worker_name.size(); ++i)
            {
                QString w_name = (QString) worker_name.at(i);
                qDebug() << "WNAME : " << w_name;


                //std::cout << "DATA : " << l_json_datas[w_name] << std::endl;

                bo payload = BSON("headers" << data["data"] << "worker" << w_name.toStdString() << "data" << l_json_datas[w_name.toStdString()]);
                workers_push[w_name]->push_payload(payload);
            }

        }
        else if (format.str() == "binary" && action.str() == "copy")
        {
            std::cout << "BINARY && COPY : " << gfs_id << std::endl;
            QString filename;
            if (nosql_->ExtractBinary(gfs_id, path.str(), filename))
            {
                qDebug() << "AFTER EXTRACT BINARY";
                for (int i = 0; i < worker_name.size(); ++i)
                {
                    QString w_name = (QString) worker_name.at(i);
                    qDebug() << "WNAME : " << w_name;


                    //std::cout << "DATA : " << l_json_datas[w_name] << std::endl;

                    bo payload = BSON("headers" << data["data"] << "worker" << w_name.toStdString() << "path" << path.str() << "data" << filename.toStdString());
                    workers_push[w_name]->push_payload(payload);
                }
            }

        }


        std::cout << "Zdispatch AFTER emit forward_payload" << std::endl;

        //sleep (1);              //  Give 0MQ time to deliver
    }    
}






Zworker_push::Zworker_push(zmq::context_t *a_context, string a_worker, string a_port) : m_context(a_context), m_worker(a_worker), m_port(a_port)
{
    std::cout << "Zworker_push::Zworker_push constructeur" << std::endl;

      z_sender = new zmq::socket_t(*m_context, ZMQ_PUSH);

      string addr = "tcp://*:" + m_port;

      std::cout << "ADDR : " << addr << std::endl;

      z_sender->bind(addr.data());

      z_message = new zmq::message_t(2);
}


Zworker_push::~Zworker_push()
{}


void Zworker_push::push_payload(bson::bo payload)
{
    std::cout << "Zworker_push::push_payload slot : " << payload << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::push_payload Sending tasks to workers...\n" << std::endl;
    //memcpy(message.data(), "0", 1);
    //sender.send(message, ZMQ_NOBLOCK);


    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
    z_sender->send(*z_message, ZMQ_NOBLOCK);
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

//    m_http_mutex->lock();
//    m_xmpp_mutex->lock();

    m_context = new zmq::context_t(1);


    _singleton = this;
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

    //zmq::socket_t *z_workers = new zmq::socket_t(*m_context, ZMQ_PUSH);
    //z_workers->bind("tcp://*:5556");

    /**** PULL DATA ON HTTP API ****/
    QThread *thread_http_receive = new QThread;
    receive_http = new Zreceive(m_context, "5556", "http");
    connect(thread_http_receive, SIGNAL(started()), receive_http, SLOT(init_payload()));
    receive_http->moveToThread(thread_http_receive);
    thread_http_receive->start();
//    QMetaObject::invokeMethod(receive, "init_payload", Qt::QueuedConnection);



    /**** PULL DATA ON XMPP API ****/
    QThread *thread_xmpp_receive = new QThread;
    receive_xmpp = new Zreceive(m_context, "5557", "xmpp");
    connect(thread_xmpp_receive, SIGNAL(started()), receive_xmpp, SLOT(init_payload()));
    receive_xmpp->moveToThread(thread_xmpp_receive);
    thread_xmpp_receive->start();



    /**** PULL DATA ON ZEROMQ API ****/
    QThread *thread_zeromq_receive = new QThread;
    receive_zeromq = new Zreceive(m_context, "5558", "zeromq");
    connect(thread_zeromq_receive, SIGNAL(started()), receive_zeromq, SLOT(init_payload()));
    receive_zeromq->moveToThread(thread_zeromq_receive);
    thread_zeromq_receive->start();

    std::cout << "Zeromq::Zeromq AFTER thread_receive" << std::endl;






    QThread *thread_dispatch = new QThread;
    dispatch = new Zdispatch(m_context);
    connect(thread_dispatch, SIGNAL(started()), dispatch, SLOT(receive_payload()));

    dispatch->moveToThread(thread_dispatch);
    thread_dispatch->start();



    QThread *thread_alert = new QThread;
    alert = new Alert();
    //connect(thread_alert, SIGNAL(started()), alert, SLOT(sendEmail(QString)));
    alert->moveToThread(thread_alert);
    thread_alert->start();


    QThread *thread_tracker = new QThread;
    ztracker = new Ztracker(m_context);
    connect(thread_tracker, SIGNAL(started()), ztracker, SLOT(init()));
    ztracker->moveToThread(thread_tracker);
    thread_tracker->start();


    connect(ztracker, SIGNAL(sendAlert(QString)), alert, SLOT(sendEmail(QString)));

}
