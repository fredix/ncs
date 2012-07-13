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

#include "xmpp_server.h"


Xmpp_server::Xmpp_server(QString a_domain, int a_xmpp_client_port, int a_xmpp_server_port) : m_domain(a_domain), m_xmpp_client_port(a_xmpp_client_port), m_xmpp_server_port(a_xmpp_server_port)
{
    qDebug() << "Xmpp_server construct param";


    m_checker.nosql_ = Nosql::getInstance_front();
    m_checker.m_username = "ncs";
    m_checker.m_password = "scn";

    //m_logger.setLoggingType(QXmppLogger::StdoutLogging);
    //m_logger.setLoggingType(QXmppLogger::FileLogging);


    //const QString domain = QString::fromLocal8Bit("localhost");

    m_server.setDomain(a_domain);
    m_server.setLogger(&m_logger);
    m_server.setPasswordChecker(&m_checker);

    QHostAddress bind_ip(QHostAddress::Any);

    m_server.listenForClients(bind_ip, m_xmpp_client_port);
    m_server.listenForServers(bind_ip, m_xmpp_server_port);
}
