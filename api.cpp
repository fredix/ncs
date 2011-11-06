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

#include "api.h"

Api::Api(Nosql& a, QObject *parent) : nosql_(a), QObject(parent)
{}

Api::~Api()
{}


void Api::Http_init()
{
    m_session.setPort(4567);
    m_session.setConnector(&m_connector);

    //Http_api s1(&session);

    m_http_api = new Http_api(&m_session, nosql_);

    m_session.setStaticContentService(m_http_api);
    m_session.start();
}



void Api::Xmpp_init()
{
    qRegisterMetaType<QXmppLogger::MessageType>("QXmppLogger::MessageType");

    QThread *thread_xmpp_server = new QThread;
    m_xmpp_server = new Xmpp_server("ncs", "scn");
    m_xmpp_server->moveToThread(thread_xmpp_server);
    thread_xmpp_server->start();

    QThread *thread_xmpp_client = new QThread;
    m_xmpp_client = new Xmpp_client(nosql_);
    m_xmpp_client->moveToThread(thread_xmpp_client);
    thread_xmpp_client->start();
}
