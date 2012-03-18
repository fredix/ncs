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

    nosql_ = Nosql::getInstance_back();


    m_socket = new zmq::socket_t (*m_context, ZMQ_REP);
    m_socket->bind("tcp://*:5569");
}


void Ztracker::init()
{
    while (true) {
    zmq::message_t request;

    // Wait for next request from client
    m_socket->recv (&request);
    std::cout << "Received : " << (char*) request.data() << std::endl;

    // Do some 'work'
    sleep (1);

    // Send reply back to client
    zmq::message_t reply (5);
    memcpy ((void *) reply.data (), "ACK", 5);
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

    std::cout << "Zeromq::Zeromq AFTER thread_receive" << std::endl;




    QThread *thread_dispatch = new QThread;
    dispatch = new Zdispatch(m_context);
    connect(thread_dispatch, SIGNAL(started()), dispatch, SLOT(receive_payload()));

    dispatch->moveToThread(thread_dispatch);
    thread_dispatch->start();



    QThread *thread_tracker = new QThread;
    ztracker = new Ztracker(m_context);
    connect(thread_tracker, SIGNAL(started()), ztracker, SLOT(init()));

    ztracker->moveToThread(thread_tracker);
    thread_tracker->start();

}
