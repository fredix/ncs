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

#include <QtCore/QCoreApplication>
#include "main.h"


int Dispatcher::sighupFd[2]={};
int Dispatcher::sigtermFd[2]={};

QSettings settings("nodecast", "ncs");


// http://doc.qt.nokia.com/4.7/unix-signals.html
static void setup_unix_signal_handlers()
{
    struct sigaction hup, term;

    hup.sa_handler = Dispatcher::hupSignalHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    /*if (sigaction(SIGHUP, &hup, 0) > 0)
       return 1;*/

    term.sa_handler = Dispatcher::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    /*if (sigaction(SIGTERM, &term, 0) > 0)
       return 2;

    return 0;*/

    sigaction (SIGINT, &hup, NULL);
    sigaction (SIGTERM, &term, NULL);
}





Dispatcher::Dispatcher(params ncs_params)
{
    qDebug() << "Dispatcher construct";



    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
        qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("Couldn't create TERM socketpair");
    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));




    nosql_front = new Nosql("front", ncs_params.mongodb_ip, ncs_params.mongodb_base);
    nosql_back = new Nosql("back", ncs_params.mongodb_ip, ncs_params.mongodb_base);
    zeromq = new Zeromq();

    api = new Api();
    api->Http_init();
    api->Tracker_init();
    api->Xmpp_init(ncs_params.domain_name, ncs_params.xmpp_client_port, ncs_params.xmpp_server_port);
    api->Worker_init();

    zeromq->init();

    connect(zeromq->dispatch, SIGNAL(emit_pubsub(bson::bo)), api->worker_api, SLOT(pubsub_payload(bson::bo)), Qt::QueuedConnection);

    QThread *thread_alert = new QThread;
    alert = new Alert(ncs_params.alert_email);
    alert->moveToThread(thread_alert);
    thread_alert->start();
    connect(zeromq->ztracker, SIGNAL(sendAlert(QString)), alert, SLOT(sendEmail(QString)), Qt::QueuedConnection);
}

Dispatcher::~Dispatcher()
{}





void Dispatcher::hupSignalHandler(int)
{
    char a = 1;
    ::write(sighupFd[0], &a, sizeof(a));
}

void Dispatcher::termSignalHandler(int)
{
    char a = 1;
    ::write(sigtermFd[0], &a, sizeof(a));
}

void Dispatcher::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    std::cout << "Received SIGTERM" << std::endl;
    snTerm->setEnabled(true);
}

void Dispatcher::handleSigHup()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sighupFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    std::cout << "Received SIGHUP" << std::endl;

    delete(alert);
    delete(api);
    delete(zeromq);
    nosql_front->kill_front ();
    nosql_back->kill_back ();

    snHup->setEnabled(true);

    std::cout << "ncs shutdown successfull" << std::endl;
    qApp->exit();
}



int main(int argc, char *argv[])
{
    bool debug;
    bool verbose;

    params ncs_params;

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

    options.add("smtp-hostname", "set the smtp hostname", QxtCommandOptions::Required);
    options.alias("smtp-hostname", "sph");

    options.add("smtp-username", "set the smtp username", QxtCommandOptions::Required);
    options.alias("smtp-username", "spu");

    options.add("smtp-password", "set the smtp password", QxtCommandOptions::Required);
    options.alias("smtp-password", "spp");


    options.add("smtp-sender", "set the smtp sender", QxtCommandOptions::Required);
    options.alias("smtp-sender", "sps");


    options.add("smtp-recipient", "set the smtp recipient", QxtCommandOptions::Required);
    options.alias("smtp-recipient", "spr");


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
        ncs_params.mongodb_base = options.value("mongodb-base").toString();
        settings.setValue("mongodb-base", ncs_params.mongodb_base);
    }
    else if(settings.contains("mongodb-base"))
    {
        ncs_params.mongodb_base = settings.value("mongodb-base").toString();
    }
    else {
        std::cout << "ncs: --mongodb-base requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }



    if(options.count("mongodb-ip")) {
        ncs_params.mongodb_ip = options.value("mongodb-ip").toString();                
        settings.setValue("mongodb-ip", ncs_params.mongodb_ip);
    }
    else if(settings.contains("mongodb-ip"))
    {
        ncs_params.mongodb_ip = settings.value("mongodb-ip").toString();
    }
    else {
        std::cout << "ncs: --mongodb-ip requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("domain-name")) {
        ncs_params.domain_name = options.value("domain-name").toString();                
        settings.setValue("domain-name", ncs_params.domain_name);
    }
    else if(settings.contains("domain-name"))
    {
        ncs_params.domain_name = settings.value("domain-name").toString();
    }
    else {
        std::cout << "ncs: --domain-name requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("xmpp-client-port")) {
        ncs_params.xmpp_client_port = options.value("xmpp-client-port").toInt();
        settings.setValue("xmpp-client-port", ncs_params.xmpp_client_port);
    }
    else if(settings.contains("xmpp-client-port"))
    {
        ncs_params.xmpp_client_port = settings.value("xmpp-client-port").toInt();
    }
    else {
        std::cout << "ncs: --xmpp-client-port requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }

    if(options.count("xmpp-server-port")) {
        ncs_params.xmpp_server_port = options.value("xmpp-server-port").toInt();
        settings.setValue("xmpp-server-port", ncs_params.xmpp_server_port);
    }
    else if(settings.contains("xmpp-server-port"))
    {
        ncs_params.xmpp_server_port = settings.value("xmpp-server-port").toInt();
    }
    else {
        std::cout << "ncs: --xmpp-server-port requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("smtp-hostname")) {
        ncs_params.alert_email.smtp_hostname = options.value("smtp-hostname").toString();
        settings.setValue("smtp-hostname", ncs_params.alert_email.smtp_hostname);
    }
    else if(settings.contains("smtp-hostname"))
    {
        ncs_params.alert_email.smtp_hostname = settings.value("smtp-hostname").toString();
    }
    else {
        std::cout << "ncs: --smtp-hostname requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("smtp-username")) {
        ncs_params.alert_email.smtp_username = options.value("smtp-username").toString();
        settings.setValue("smtp-username", ncs_params.alert_email.smtp_username);
    }
    else if(settings.contains("smtp-username"))
    {
        ncs_params.alert_email.smtp_username = settings.value("smtp-username").toString();
    }
    else {
        std::cout << "ncs: --smtp-username requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("smtp-password")) {
        ncs_params.alert_email.smtp_password = options.value("smtp-password").toString();
        settings.setValue("smtp-password", ncs_params.alert_email.smtp_password);
    }
    else if(settings.contains("smtp-password"))
    {
        ncs_params.alert_email.smtp_password = settings.value("smtp-password").toString();
    }
    else {
        std::cout << "ncs: --smtp-password requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }



    if(options.count("smtp-sender")) {
        ncs_params.alert_email.smtp_sender = options.value("smtp-sender").toString();
        settings.setValue("smtp-sender", ncs_params.alert_email.smtp_sender);
    }
    else if(settings.contains("smtp-sender"))
    {
        ncs_params.alert_email.smtp_sender = settings.value("smtp-sender").toString();
    }
    else {
        std::cout << "ncs: --smtp-sender requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("smtp-recipient")) {
        ncs_params.alert_email.smtp_recipient = options.value("smtp-recipient").toString();
        settings.setValue("smtp-recipient", ncs_params.alert_email.smtp_recipient);
    }
    else if(settings.contains("smtp-recipient"))
    {
        ncs_params.alert_email.smtp_recipient = settings.value("smtp-recipient").toString();
    }
    else {
        std::cout << "ncs: --smtp-recipient requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }

    if (!QDir("/tmp/nodecast").exists()) QDir().mkdir("/tmp/nodecast");


    setup_unix_signal_handlers();


    Dispatcher dispatcher(ncs_params);


    qDebug() << "end";

    return ncs.exec();
}


