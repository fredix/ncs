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

#include "xmpp_server.h"


Xmpp_server::Xmpp_server(Nosql &a, QString a_domain) : m_domain(a_domain)
{
    qDebug() << "Xmpp_server construct param";


    m_checker.nosql_ = &a;
    m_checker.m_username = "ncs";
    m_checker.m_password = "scn";

    m_logger.setLoggingType(QXmppLogger::StdoutLogging);
    //m_logger.setLoggingType(QXmppLogger::FileLogging);


    //const QString domain = QString::fromLocal8Bit("localhost");

    m_server.setDomain(a_domain);
    m_server.setLogger(&m_logger);
    m_server.setPasswordChecker(&m_checker);
    m_server.listenForClients();
    m_server.listenForServers();

}
