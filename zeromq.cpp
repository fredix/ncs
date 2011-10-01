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


Zreceive::Zreceive()
{
    qDebug() << "Zreceive::construct";
    m_context = new zmq::context_t(1);
    //zmq::context_t context (1);

    // zmq::socket_t sender (*z_context, ZMQ_XREP);
    // sender.bind("tcp://*:5555");
}


void Zreceive::init_payload()
{
    qDebug() << "Zreceive::init_payload";
    z_sender = new zmq::socket_t(*m_context, ZMQ_PULL);
    z_sender->connect("tcp://*:5555");



    zmq::socket_t z_workers (*m_context, ZMQ_PUSH);
    z_workers.bind ("inproc://workers");


    //  Connect work threads to client threads via a queue
    std::cout << "Zreceive::init_payload BEFORE ZMQ_STREAMER" << std::endl;
    zmq::device (ZMQ_STREAMER, *z_sender, z_workers);
    std::cout << "Zreceive::init_payload AFTER ZMQ_STREAMER" << std::endl;
}

Zreceive::~Zreceive()
{}




Zdispatch::Zdispatch()
{}

Zdispatch::~Zdispatch()
{}


Zdispatch::Zdispatch(zmq::context_t *a_context)
{        
    std::cout << "Zdispatch::Zdispatch constructeur" << std::endl;

    m_context = a_context;
}

void Zdispatch::receive_payload()
{
    std::cout << "Zdispatch::receive_payload" << std::endl;
    zmq::socket_t socket (*m_context, ZMQ_PULL);
    socket.connect ("inproc://workers");

    std::cout << "Zdispatch::receive_payload AFTER inproc" << std::endl;


    while (true) {
        //  Wait for next request from client
        zmq::message_t request;
        socket.recv (&request);

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

        emit forward_payload(data);

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

    std::cout << "worker_uptime AFTER PUSH INIT" << std::endl;

    z_message = new zmq::message_t(2);
}


Zworker_push::Zworker_push()
{}


Zworker_push::~Zworker_push()
{}


void Zworker_push::send_payload_load(QString payload)
{
    std::cout << "Zworker_push::send_payload_load slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_load Sending tasks to workers...\n" << std::endl;
    //memcpy(message.data(), "0", 1);
    //sender.send(message, ZMQ_NOBLOCK);


    z_message->rebuild(payload.size());
    memcpy(z_message->data(), (char*)payload.toAscii().data(), payload.size());

    z_load_sender->send(*z_message, ZMQ_NOBLOCK);
    //sleep (1);              //  Give 0MQ time to deliver
}


void Zworker_push::send_payload_cpu(QString payload)
{
    std::cout << "Zworker_push::send_payload_cpu slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_cpu Sending tasks to workers...\n" << std::endl;

    z_message->rebuild(payload.size());
    memcpy(z_message->data(), (char*)payload.toAscii().data(), payload.size());

    z_cpu_sender->send(*z_message, ZMQ_NOBLOCK);
    //sleep (1);              //  Give 0MQ time to deliver
}

void Zworker_push::send_payload_network(QString payload)
{
    std::cout << "Zworker_push::send_payload_network slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_network Sending tasks to workers...\n" << std::endl;

    z_message->rebuild(payload.size());
    memcpy(z_message->data(), (char*)payload.toAscii().data(), payload.size());

    z_network_sender->send(*z_message, ZMQ_NOBLOCK);
    //sleep (1);              //  Give 0MQ time to deliver
}

void Zworker_push::send_payload_memory(QString payload)
{
    std::cout << "Zworker_push::send_payload_memory slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_memory Sending tasks to workers...\n" << std::endl;

    z_message->rebuild(payload.size());
    memcpy(z_message->data(), (char*)payload.toAscii().data(), payload.size());

    z_memory_sender->send(*z_message, ZMQ_NOBLOCK);
    //sleep (1);              //  Give 0MQ time to deliver
}


void Zworker_push::send_payload_uptime(QString payload)
{
    std::cout << "Zworker_push::send_payload_uptime slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_uptime Sending tasks to workers...\n" << std::endl;

    z_message->rebuild(payload.size());
    memcpy(z_message->data(), (char*)payload.toAscii().data(), payload.size());

    z_uptime_sender->send(*z_message, ZMQ_NOBLOCK);
    //sleep (1);              //  Give 0MQ time to deliver
}


void Zworker_push::send_payload_process(QString payload)
{
    std::cout << "Zworker_push::send_payload_process slot" << std::endl;

    /***************** PUSH ********************/
    std::cout << "Zworker_push::send_payload_process Sending tasks to workers...\n" << std::endl;


    //std::cout << "PROCESS BSON TO STRING : " << payload.toStdString() << std::endl;

    z_message->rebuild(payload.size());
    memcpy(z_message->data(), (char*)payload.toAscii().data() , payload.size());
    //memcpy(z_message->data(), (char*)payload.data() , payload.size());

    z_process_sender->send(*z_message, ZMQ_NOBLOCK);
    //sleep (1);              //  Give 0MQ time to deliver
}







Zeromq::Zeromq()
{}

Zeromq::Zeromq(QString host, int port)
{
    qDebug() << "Zeromq::construct";

    /******* QTHREAD ACCORDING TO
    http://developer.qt.nokia.com/doc/qt-4.7/qthread.html ->
                        Subclassing no longer recommended way of using QThread
    http://labs.qt.nokia.com/2010/06/17/youre-doing-it-wrong/
    ******************************/


    QThread *thread_receive = new QThread;
    receive = new Zreceive();
    connect(thread_receive, SIGNAL(started()), receive, SLOT(init_payload()));
    receive->moveToThread(thread_receive);
    thread_receive->start();
//    QMetaObject::invokeMethod(receive, "init_payload", Qt::QueuedConnection);


    std::cout << "Zeromq::Zeromq AFTER thread_receive" << std::endl;


    sleep(1);

    QThread *thread_dispatch = new QThread;
    dispatch = new Zdispatch(receive->m_context);
    connect(thread_dispatch, SIGNAL(started()), dispatch, SLOT(receive_payload()));

    dispatch->moveToThread(thread_dispatch);
    thread_dispatch->start();


    sleep(1);


    QThread *thread_worker_push = new QThread;
    worker_push = new Zworker_push(receive->m_context);
    //connect(thread_worker_load, SIGNAL(started()), worker_load, SLOT(send_payload()));

    worker_push->moveToThread(thread_worker_push);
    thread_worker_push->start();
}


Zeromq::~Zeromq()
{}




