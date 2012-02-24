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

#include <QtCore/QCoreApplication>
#include "main.h"




Dispatcher::Dispatcher(QString mongodb_ip, QString mongodb_base, QString domain_name, int xmpp_client_port, int xmpp_server_port)
{
    qDebug() << "Dispatcher construct";


    nosql = new Nosql(mongodb_ip, mongodb_base);
    zeromq = new Zeromq(*nosql, "127.0.0.1", 12345);


    /*********** HTTP API *************/
    api = new Api(*nosql, *zeromq);
    api->Http_init();
    api->Xmpp_init(domain_name, xmpp_client_port, xmpp_server_port);


    zeromq->init();
}

Dispatcher::~Dispatcher()
{}


/*
 *
 *  ncs --mongodb-ip 127.0.0.1
 *  --mongodb-base=nodecast_prod --qpid-ip=127.0.0.1
 * --qpid-port=5672
 *
*/

int main(int argc, char *argv[])
{
    bool debug;
    bool verbose;
    QString mongodb_ip;
    QString mongodb_base; 
    QString domain_name;
    int xmpp_client_port;
    int xmpp_server_port;

    /*
    openlog("NODECAST",LOG_PID,LOG_USER);
    syslog(LOG_INFO, "init dispatcher");
    closelog();
    */

    QCoreApplication ncs(argc, argv);


    QxtCommandOptions options;
    options.add("debug", "show debug informations");
    options.alias("debug", "d");
    options.add("mongodb-ip", "set the mongodb ip", QxtCommandOptions::Required);
    options.alias("mongodb-ip", "mdip");
    options.add("mongodb-base", "set the mongodb base", QxtCommandOptions::Required);
    options.alias("mongodb-base", "mdp");
    options.add("domain-name", "set the domain name", QxtCommandOptions::Required);
    options.alias("domain-name", "dn");
    options.add("xmpp-client-port", "set the xmpp client port", QxtCommandOptions::Required);
    options.alias("xmpp-client-port", "xcp");
    options.add("xmpp-server-port", "set the xmpp server port", QxtCommandOptions::Required);
    options.alias("xmpp-server-port", "xsp");


    options.add("verbose", "show more information about the process; specify twice for more detail", QxtCommandOptions::AllowMultiple);
    options.alias("verbose", "v");
    options.add("help", "show this help text");
    options.alias("help", "h");
    options.parse(QCoreApplication::arguments());
    if(options.count("help") || options.showUnrecognizedWarning()) {
        options.showUsage();
        return -1;
    }
    verbose = options.count("verbose");
    if(options.count("mongodb-base")) {
        mongodb_base = options.value("mongodb-base").toString();
    }
    else {
        std::cout << "ncs: --mongodb-base requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }



    if(options.count("mongodb-ip")) {
        mongodb_ip = options.value("mongodb-ip").toString();
    }
    else {
        std::cout << "ncs: --mongodb-ip requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("domain-name")) {
        domain_name = options.value("domain-name").toString();
    }
    else {
        std::cout << "ncs: --domain-name requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("xmpp-client-port")) {
        xmpp_client_port = options.value("xmpp-client-port").toInt();
    }
    else {
        std::cout << "ncs: --xmpp-client-port requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }

    if(options.count("xmpp-server-port")) {
        xmpp_server_port = options.value("xmpp-server-port").toInt();
    }
    else {
        std::cout << "ncs: --xmpp-server-port requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    Dispatcher dispatcher(mongodb_ip, mongodb_base, domain_name, xmpp_client_port, xmpp_server_port);



    qRegisterMetaType<bson::bo>("bson::bo");

    //QObject::connect(dispatcher.zeromq->dispatch, SIGNAL(forward_payload(bson::bo)), dispatcher.payload, SLOT(s_job_receive(bson::bo)), Qt::BlockingQueuedConnection);
    //QObject::connect(dispatcher.zeromq->dispatch_xmpp, SIGNAL(forward_payload(bson::bo)), dispatcher.payload, SLOT(s_job_receive(bson::bo)), Qt::BlockingQueuedConnection);

    QObject::connect(dispatcher.zeromq->dispatch, SIGNAL(payload_load(bson::bo)), dispatcher.zeromq->worker_push, SLOT(send_payload_load(bson::bo)), Qt::QueuedConnection);
    QObject::connect(dispatcher.zeromq->dispatch, SIGNAL(payload_cpu(bson::bo)), dispatcher.zeromq->worker_push, SLOT(send_payload_cpu(bson::bo)), Qt::QueuedConnection);
    QObject::connect(dispatcher.zeromq->dispatch, SIGNAL(payload_network(bson::bo)), dispatcher.zeromq->worker_push, SLOT(send_payload_network(bson::bo)), Qt::QueuedConnection);
    QObject::connect(dispatcher.zeromq->dispatch, SIGNAL(payload_memory(bson::bo)), dispatcher.zeromq->worker_push, SLOT(send_payload_memory(bson::bo)), Qt::QueuedConnection);
    QObject::connect(dispatcher.zeromq->dispatch, SIGNAL(payload_uptime(bson::bo)), dispatcher.zeromq->worker_push, SLOT(send_payload_uptime(bson::bo)), Qt::QueuedConnection);
    QObject::connect(dispatcher.zeromq->dispatch, SIGNAL(payload_process(bson::bo)), dispatcher.zeromq->worker_push, SLOT(send_payload_process(bson::bo)), Qt::QueuedConnection);
    QObject::connect(dispatcher.zeromq->dispatch, SIGNAL(payload_filesystem(bson::bo)), dispatcher.zeromq->worker_push, SLOT(send_payload_filesystem(bson::bo)), Qt::QueuedConnection);


    qDebug() << "end";

    return ncs.exec();
}


