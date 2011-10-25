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

#ifndef HTTP_API_H
#define HTTP_API_H

#include "nosql.h"
#include <zmq.hpp>
#include <QxtWeb/QxtWebSlotService>
#include <QxtWeb/QxtWebPageEvent>
#include <QxtWeb/QxtWebContent>
#include <QUuid>

using namespace mongo;
using namespace bson;

class Http_api : public QxtWebSlotService
{    
    Q_OBJECT
public:
    Http_api(QxtAbstractWebSessionManager * sm, Nosql& a, QObject * parent = 0);
    //Http_api(QxtAbstractWebSessionManager * sm, Nosql& a);
    ~Http_api();



public slots:
        void host(QxtWebRequestEvent* event, QString action, QString uuid);
        void host(QxtWebRequestEvent* event, QString action);

private:
        zmq::context_t *m_context;
        zmq::socket_t *z_push_api;
        zmq::message_t *z_message;

        Nosql &nosql_;
        bool checkAuth(QString header, BSONObjBuilder &payload);
        QString buildResponse(QString action, QString status);
};


#endif // HTTP_API_H
