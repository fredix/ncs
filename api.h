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

#ifndef API_H
#define API_H

#include <QThread>
#include "nosql.h"
#include "zeromq.h"
#include "http_api.h"
#include "worker_api.h"
#include <QxtHttpServerConnector>
#include <QxtHttpSessionManager>
#include "xmpp_server.h"
#include "xmpp_client.h"

class Api : public QObject
{
    Q_OBJECT
public:
    Api(QObject *parent = 0);
    ~Api();

    void Http_init();
    void Xmpp_init(QString domain_name, int xmpp_client_port, int xmpp_server_port);
    void Worker_init();
    Worker_api *worker_api;


private:
    QxtHttpServerConnector m_connector;
    QxtHttpSessionManager m_session;
    Http_api *m_http_api;
    Xmpp_server *m_xmpp_server;
    Xmpp_client *m_xmpp_client;
};

#endif // API_H
