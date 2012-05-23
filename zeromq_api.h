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

#ifndef ZEROMQ_API_H
#define ZEROMQ_API_H

#include "nosql.h"
#include "zeromq.h"
#include <zmq.hpp>

class Zeromq_api : public QObject
{
    Q_OBJECT
public:
    Zeromq_api();
    ~Zeromq_api();


private:
    void on_receive();


    zmq::socket_t *z_receive_api;
    zmq::socket_t *z_push_api;
    zmq::message_t *z_message;

    Nosql *nosql_;
    Zeromq *zeromq_;

signals:
    void on_zmessage(bson::bo payload);


private slots:
    void push_payload(bson::bo payload);
};

#endif // ZEROMQ_API_H
