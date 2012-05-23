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

Alert::Alert()
{
    qDebug() << "ALERT:ALERT CONSTRUCT";
    m_mutex = new QMutex();

    m_smtp = NULL;
    m_message = NULL;

}


void Alert::sendEmail(QString worker)
{
    qDebug() << "!!!!!! Alert::sendEmail  : " << worker;

    m_mutex->lock();

    if (m_smtp) delete(m_smtp);
    if (m_message) delete(m_message);
    m_smtp = NULL;
    m_message = NULL;



    m_smtp = new QxtSmtp();

    connect(m_smtp, SIGNAL(mailFailed(int,int)), this, SLOT(failed()));
    connect(m_smtp, SIGNAL(mailSent(int)), this, SLOT(success()));


    m_message = new QxtMailMessage();

    m_message->setSender("frederic.logier@ubicmedia.com");
    m_message->setSubject("ncs alert : worker down");
    m_message->setBody(worker + " DOWN");
    m_message->addRecipient("frederic.logier@ubicmedia.com");
    //m_message->addRecipient("alert@pumit.com");

    QHash<QString,QString> headers;
    headers.insert("MIME-Version","1.0");
    headers.insert("Content-type","text/html; charset=utf-8");
    headers.insert("from","frederic.logier@ubicmedia.com");
    m_message->setExtraHeaders(headers);

    m_smtp->connectToSecureHost("mail.gandi.net");
    m_smtp->setStartTlsDisabled(true);
    m_smtp->setUsername("frederic.logier@ubicmedia.com");
    m_smtp->setPassword("4307n554e");


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
