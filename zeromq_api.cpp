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
    z_push_api->send(*z_message, ZMQ_NOBLOCK);
    /************************/
}


Zeromq_api::~Zeromq_api()
{
}
