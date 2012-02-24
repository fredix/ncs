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

#ifndef MAIN_H
#define MAIN_H

#include <syslog.h>
//#include <iostream>
#include <QDebug>
#include <QDomDocument>

#include "nosql.h"
#include "zeromq.h"
#include "api.h"


class Dispatcher : public QObject
{
    Q_OBJECT
public:
    Dispatcher(QString mongodb_ip, QString mongodb_base, QString domain_name, int xmpp_client_port, int xmpp_server_port);
    ~Dispatcher();

    Nosql *nosql;
    Zeromq *zeromq;
    //Payload *payload;
    Api *api;
};




#endif // MAIN_H
