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


#include "alert.h"
#include <iostream>

Alert::Alert(QString a_host, QString a_username, QString a_password, QString a_sender, QString a_recipient) : m_host(a_host), m_username(a_username), m_password(a_password), m_sender(a_sender), m_recipient(a_recipient)
{
    qDebug() << "ALERT:ALERT CONSTRUCT";
    m_mutex = new QMutex();

    m_smtp = NULL;
    m_message = NULL;

}


void Alert::sendEmail(QString worker)
{
    m_mutex->lock();
    qDebug() << "!!!!!! Alert::sendEmail  : " << worker;


    if (m_smtp) delete(m_smtp);
    if (m_message) delete(m_message);
    m_smtp = NULL;
    m_message = NULL;



    m_smtp = new QxtSmtp();

    connect(m_smtp, SIGNAL(mailFailed(int,int)), this, SLOT(failed()));
    connect(m_smtp, SIGNAL(mailSent(int)), this, SLOT(success()));


    m_message = new QxtMailMessage();

    m_message->setSender(m_sender.toAscii());
    m_message->setSubject("ncs alert : worker down");
    m_message->setBody(worker + " DOWN");
    m_message->addRecipient(m_recipient.toAscii());

    QHash<QString,QString> headers;
    headers.insert("MIME-Version","1.0");
    headers.insert("Content-type","text/html; charset=utf-8");
    headers.insert("from", m_sender.toAscii());
    m_message->setExtraHeaders(headers);

    m_smtp->connectToSecureHost(m_host.toAscii());
    m_smtp->setStartTlsDisabled(true);
    m_smtp->setUsername(m_username.toAscii());
    m_smtp->setPassword(m_password.toAscii());


    m_smtp->send(*m_message);
}

void Alert::failed()
{
    qDebug() << "Alert::failed";

    m_smtp->disconnectFromHost();
    m_mutex->unlock();
}


void Alert::success()
{
    qDebug() << "Alert::sucess";

    m_smtp->disconnectFromHost();
    m_mutex->unlock();
}


Alert::~Alert()
{}
