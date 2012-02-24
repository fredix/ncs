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



Ztracker::Ztracker(Nosql& a, zmq::context_t *a_context) : nosql_(a), m_context(a_context)
{
    std::cout << "Ztracker::Ztracker construct" << std::endl;

    m_socket = new zmq::socket_t (*m_context, ZMQ_REP);
    m_socket->bind("tcp://*:5569");
}


void Ztracker::init()
{
    while (true) {
    zmq::message_t request;

    // Wait for next request from client
    m_socket->recv (&request);
    std::cout << "Received Hello : " << (char*) request.data() << std::endl;

    // Do some 'work'
    sleep (1);

    // Send reply back to client
    zmq::message_t reply (5);
    memcpy ((void *) reply.data (), "World", 5);
    m_socket->send (reply);
    }
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





Zdispatch::Zdispatch(Nosql& a, zmq::context_t *a_context) : nosql_(a), m_context(a_context)
{        
    std::cout << "Zdispatch::Zdispatch constructeur" << std::endl;
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
        be uuid = data.getField("uuid");
        be qname = data.getField("action");


        std::cout << "uuid : " << uuid.toString() << std::endl;
        std::cout << "action : " << qname.toString() << std::endl;



        //  Send reply back to client
        //zmq::message_t reply (6);
        //memcpy ((void *) reply.data (), "PROCESS", 9);
        //socket.send (reply);


        /***************** PUSH ********************/
        std::cout << "Zdispatch BEFORE emit forward_payload" << std::endl;

        //emit forward_payload(data);



        std::cout << "DATA : " <<  data << std::endl;

        be gfs_id = data.getField("_id");

        std::cout << "GFS ID : " << gfs_id << std::endl;


        bo l_json_datas = nosql_.ExtractJSON(gfs_id);



        qDebug() << "Serializing datas and send to workers";
        qDebug() << "SEND CPU SIGNAL";

        if (l_json_datas["cpu_usage"]["activated"].boolean())
        {
            qDebug() << "emit payload_cpu(l_payload)";

            bo payload = BSON("headers" << data << "cpu_usage" << l_json_datas["cpu_usage"]);
            emit payload_cpu(payload);
        }

        qDebug() << "SEND LOAD SIGNAL";


        if (l_json_datas["load"]["activated"].boolean())
        {
            std::cout << "load : " << l_json_datas["load"]["activated"] << std::endl;
            qDebug() << "emit payload_load(l_payload)";

            bo payload = BSON("headers" << data << "load" << l_json_datas["load"]);
            emit payload_load(payload);
        }

        if (l_json_datas["network"]["activated"].boolean())
        {
            qDebug() << "emit payload_network(l_payload)";

            bo payload = BSON("headers" << data << "network" << l_json_datas["network"]);
            emit payload_network(payload);
        }

        if (l_json_datas["memory"]["activated"].boolean())
        {
            qDebug() << "emit payload_memory(l_payload)";

            bo payload = BSON("headers" << data << "memory" << l_json_datas["memory"]);
            emit payload_memory(payload);
        }

        if (l_json_datas["uptime"]["activated"].boolean())
        {
            qDebug() << "emit payload_uptime(l_payload)";

            bo payload = BSON("headers" << data << "uptime" << l_json_datas["uptime"]);
            emit payload_uptime(payload);
        }


        if (l_json_datas["process"]["activated"].boolean())
        {
            qDebug() << "emit payload_process(l_payload)";

            bo payload = BSON("headers" << data << "process" << l_json_datas["process"]);
            emit payload_process(payload);
        }


        std::cout << "FILE SYSTEM : " << l_json_datas["filesystems"] << "\n" << std::endl;


        if (l_json_datas["filesystems"]["activated"].boolean())
        {
            qDebug() << "emit payload_filesystem(l_payload)";

            bo payload = BSON("headers" << data << "filesystems" << l_json_datas["filesystems"]);
            emit payload_filesystem(payload);
        }






        std::cout << "Zdispatch AFTER emit forward_payload" << std::endl;

        //sleep (1);              //  Give 0MQ time to deliver
    }    
}






Zworker_push::Zworker_push(zmq::context_t *a_context)
{
    std::cout << "Zworker_load::Zworker_load constructeur" << std::endl;


    z_load_sender = new zmq::socket_t(*a_context, ZMQ_PUSH);
    z_load_sender->bind("tcp://*:5560");

    z_cpu_sender = new zmq::socket_t(*a_context, ZMQ_PUSH);
    z_cpu_sender->bind("tcp://*:5561");

    z_network_sender = new zmq::socket_t(*a_context, ZMQ_PUSH);
    z_network_sender->bind("tcp://*:5562");

    z_memory_sender = new zmq::socket_t(*a_context, ZMQ_PUSH);
    z_memory_sender->bind("tcp://*:5563");

    z_uptime_sender = new zmq::socket_t(*a_context, ZMQ_PUSH);
    z_uptime_sender->bind("tcp://*:5564");

    z_process_sender = new zmq::socket_t(*a_context, ZMQ_PUSH);
    z_process_sender->bind("tcp://*:5565");

    z_filesystem_sender = new zmq::socket_t(*a_context, ZMQ_PUSH);
    z_filesystem_sender->bind("tcp://*:5566");

    std::cout << "worker_uptime AFTER PUSH INIT" << std::endl;

    z_message = new zmq::message_t(2);
}


Zworker_push::Zworker_push()
{}


Zworker_push::~Zworker_push()
{}


void Zworker_push::send_payload_load(bson::bo payload)
{
    std::cout << "Zworker_push::send_payload_load slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_load Sending tasks to workers...\n" << std::endl;
    //memcpy(message.data(), "0", 1);
    //sender.send(message, ZMQ_NOBLOCK);


    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
    z_load_sender->send(*z_message, ZMQ_NOBLOCK);
}


void Zworker_push::send_payload_cpu(bson::bo payload)
{
    std::cout << "Zworker_push::send_payload_cpu slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_cpu Sending tasks to workers...\n" << std::endl;

    std::cout << "PAYLOAD : " << payload << std::endl;


    /*
    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
    z_cpu_sender->send(*z_message, ZMQ_NOBLOCK);*/
}

void Zworker_push::send_payload_network(bson::bo payload)
{
    std::cout << "Zworker_push::send_payload_network slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_network Sending tasks to workers...\n" << std::endl;

    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
    z_network_sender->send(*z_message, ZMQ_NOBLOCK);
}

void Zworker_push::send_payload_memory(bson::bo payload)
{
    std::cout << "Zworker_push::send_payload_memory slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_memory Sending tasks to workers...\n" << std::endl;

    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
    z_memory_sender->send(*z_message, ZMQ_NOBLOCK);
}


void Zworker_push::send_payload_uptime(bson::bo payload)
{
    std::cout << "Zworker_push::send_payload_uptime slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_uptime Sending tasks to workers...\n" << std::endl;

    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
    z_uptime_sender->send(*z_message, ZMQ_NOBLOCK);
}


void Zworker_push::send_payload_process(bson::bo payload)
{
    std::cout << "Zworker_push::send_payload_process slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_process Sending tasks to workers...\n" << std::endl;


    //std::cout << "PROCESS BSON TO STRING : " << payload.toStdString() << std::endl;

    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata() , payload.objsize());
    z_process_sender->send(*z_message, ZMQ_NOBLOCK);
}


void Zworker_push::send_payload_filesystem(bson::bo payload)
{
    std::cout << "Zworker_push::send_payload_filesystem slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_filesystem Sending tasks to workers...\n" << std::endl;


    //std::cout << "PROCESS BSON TO STRING : " << payload.toStdString() << std::endl;

    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata() , payload.objsize());
    z_filesystem_sender->send(*z_message, ZMQ_NOBLOCK);
}





Zeromq::Zeromq(Nosql& a, QString host, int port) : nosql_(a)
{
    qDebug() << "Zeromq::construct";

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


}


Zeromq::~Zeromq()
{}





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

    std::cout << "Zeromq::Zeromq AFTER thread_receive" << std::endl;




    QThread *thread_dispatch = new QThread;
    dispatch = new Zdispatch(nosql_, m_context);
    connect(thread_dispatch, SIGNAL(started()), dispatch, SLOT(receive_payload()));

    dispatch->moveToThread(thread_dispatch);
    thread_dispatch->start();



    QThread *thread_tracker = new QThread;
    ztracker = new Ztracker(nosql_, m_context);
    connect(thread_tracker, SIGNAL(started()), ztracker, SLOT(init()));

    ztracker->moveToThread(thread_tracker);
    thread_tracker->start();






/*

    QThread *thread_dispatch_http = new QThread;
    dispatch_http = new Zdispatch(m_context, "http", http_mutex, xmpp_mutex);
    connect(thread_dispatch_http, SIGNAL(started()), dispatch_http, SLOT(receive_payload()));

    dispatch_http->moveToThread(thread_dispatch_http);
    thread_dispatch_http->start();



    QThread *thread_dispatch_xmpp = new QThread;
    dispatch_xmpp = new Zdispatch(m_context, "xmpp", http_mutex, xmpp_mutex);
    connect(thread_dispatch_xmpp, SIGNAL(started()), dispatch_xmpp, SLOT(receive_payload()));

    dispatch_xmpp->moveToThread(thread_dispatch_xmpp);
    thread_dispatch_xmpp->start();
*/


    QThread *thread_worker_push = new QThread;
    worker_push = new Zworker_push(m_context);
    //connect(thread_worker_load, SIGNAL(started()), worker_load, SLOT(send_payload()));

    worker_push->moveToThread(thread_worker_push);
    thread_worker_push->start();

}
