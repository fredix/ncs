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

#include "zeromq_api.h"

Zeromq_api::Zeromq_api()
{
    nosql_ = Nosql::getInstance_back();
    zeromq_ = Zeromq::getInstance ();

    z_message = new zmq::message_t(2);
    //m_context = new zmq::context_t(1);

    z_receive_api = new zmq::socket_t (*zeromq_->m_context, ZMQ_PULL);
    z_receive_api->bind("tcp://*:5555");

    z_push_api = new zmq::socket_t(*zeromq_->m_context, ZMQ_PUSH);
    z_push_api->bind("inproc://zeromq");

    connect(this, SIGNAL(on_zmessage(bson::bo)), this, SLOT(push_payload(bson::bo)));
}


void Zeromq_api::on_receive()
{
    //  Process tasks forever
    while (true) {
        qDebug() << "Zeromq_api::on_receive WHILE : ";
        zmq::message_t message;
        z_receive_api->recv(&message);

        //std::cout << "Received request: [" << (char*) message.data() << "]" << std::endl;

        bo l_payload = bo((char*)message.data());

        emit on_zmessage(l_payload);
    }
}

void Zeromq_api::push_payload(bson::bo payload)
{
    /****** PUSH API PAYLOAD *******/
    qDebug() << "PUSH API PAYLOAD";
    z_message->rebuild(payload.objsize());
    memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
    //z_push_api->send(*z_message, ZMQ_NOBLOCK);
    z_push_api->send(*z_message);
    /************************/
}


Zeromq_api::~Zeromq_api()
{
}
