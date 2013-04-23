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


Io::~Io()
{
    notifier->setEnabled(false);
    delete(input);
    io_log->close();
    qDebug() << "END IO";
}

Io::Io() : QObject()
{
    input  = new QTextStream( stdin,  QIODevice::ReadOnly );

    // Demand notification when there is data to be read from stdin
    notifier = new QSocketNotifier( STDIN_FILENO, QSocketNotifier::Read );
    connect(notifier, SIGNAL(activated(int)), this, SLOT(readStdin()), Qt::DirectConnection);


    io_log = new QFile("/tmp/ncw_nodeftp");
    if (!io_log->open(QIODevice::Append | QIODevice::Text))
            return;


    //*cout << prompt;
}

void Io::readStdin()
{
    notifier->setEnabled(false);


//    qDebug() << "READ STDIN";
    // Read the data
    QString line = input->readLine();

    io_log->write(line.toAscii());
    io_log->write("\n");
    io_log->flush();

    //writeStdout("LINE : " + line);
    //if (!line.isNull() && !line.isEmpty())
    //{
    // Parse received data

    emit parseData(line);


    //}

    notifier->setEnabled(true);
}


void Service::hupSignalHandler(int)
{
    char a = 1;
    ::write(sighupFd[0], &a, sizeof(a));
}

void Service::termSignalHandler(int)
{
    char a = 1;
    ::write(sigtermFd[0], &a, sizeof(a));
}

void Service::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    qDebug() << "Received SIGTERM";
    snTerm->setEnabled(true);
}

void Service::handleSigHup()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sighupFd[1], &tmp, sizeof(tmp));

    qDebug() << "Received SIGHUP";


    qDebug() << "Service::handleSigHup delete ncw";
    ncw->deleteLater();
    ncw_thread->wait();

    qDebug() << "Service::handleSigHup delete ftp";
    m_nodeftp->deleteLater();
    node_thread_ftp->wait();


    //mongodb_->kill ();
    //mongodb_->deleteLater();

    snHup->setEnabled(true);

    qDebug() << "nodeftp shutdown init";
    qApp->exit();
}



Service::~Service()
{
    qDebug() << "Service::~Service delete ncw";
    ncw->deleteLater();
    ncw_thread->wait();


    qDebug() << "Service::~Service delete ftp";
    m_nodeftp->deleteLater();
    node_thread_ftp->wait();
}



Service::Service(params a_params, QObject *parent) : m_params(a_params), QObject(parent)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
        qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("Couldn't create TERM socketpair");
    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));



    int port;
    m_params.ftp_server_port == 0 ? port = 2121 : port = m_params.ftp_server_port;


    ncw_thread = new QThread(this);
    ncw = new Io();
    connect(ncw, SIGNAL(destroyed()), ncw_thread, SLOT(quit()), Qt::DirectConnection);


    node_thread_ftp = new QThread(this);
    m_nodeftp = new Nodeftp(m_params.base_directory, port);
    connect(m_nodeftp, SIGNAL(destroyed()), node_thread_ftp, SLOT(quit()), Qt::DirectConnection);
    connect(node_thread_ftp, SIGNAL(started()), m_nodeftp, SLOT(ftp_init()));
    connect(ncw, SIGNAL(parseData(QString)), m_nodeftp, SLOT(receive_payload(QString)), Qt::QueuedConnection);


    ncw->moveToThread(ncw_thread);
    ncw_thread->start();

    m_nodeftp->moveToThread(node_thread_ftp);
    node_thread_ftp->start();
}

