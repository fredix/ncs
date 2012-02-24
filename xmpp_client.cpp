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


#include "xmpp_client.h"

Xmpp_client::Xmpp_client(Nosql& a, Zeromq &z, QString a_domain, int a_xmpp_client_port, QObject *parent) : QXmppClient(parent), nosql_(a), zeromq_(z), m_domain(a_domain), m_xmpp_client_port(a_xmpp_client_port)
{
    qDebug() << "Xmpp_client::Xmpp_client !!!";


    z_message = new zmq::message_t(2);
    //m_context = new zmq::context_t(1);
    z_push_api = new zmq::socket_t(*zeromq_.m_context, ZMQ_PUSH);
    //z_push_api = new zmq::socket_t(*m_context, ZMQ_PUSH);
    //z_push_api->bind("tcp://*:5556");
    z_push_api->bind("inproc://xmpp");



    bool check = connect(this, SIGNAL(messageReceived(const QXmppMessage&)),
           SLOT(messageReceived(const QXmppMessage&)));
       Q_ASSERT(check);
       Q_UNUSED(check);

    check = connect(this, SIGNAL(presenceReceived(const QXmppPresence&)),
                    SLOT(presenceReceived(const QXmppPresence&)));


    check = connect(this, SIGNAL(connected()),
                    SLOT(connectedToServer()));


    check = connect(this, SIGNAL(error(QXmppClient::Error)),
                    SLOT(connectedError()));



    //this->logger()->setLoggingType(QXmppLogger::StdoutLogging);

    //this->logger()->setLoggingType(QXmppLogger::FileLogging);

    this->configuration().setPort(m_xmpp_client_port);
    this->configuration().setJid("ncs@" + m_domain);
    this->configuration().setPassword("scn");
    this->configuration().setResource("cli");

    this->connectToServer(this->configuration());

    //this->connectToServer("ncs@localhost/server", "scn");


   /* subscribe.setTo("geekast@localhost");
    subscribe.setType(QXmppPresence::Subscribe);
    this->sendPacket(subscribe);*/
}


Xmpp_client::~Xmpp_client()
{}




bool Xmpp_client::checkAuth(QString credentials, BSONObjBuilder &payload_builder)
{

    QStringList arr_auth =  credentials.split(":");

    QString email = arr_auth.at(0);
    QString key = arr_auth.at(1);

    qDebug() << "email : " << email << " key : " << key;

    bo auth = BSON("email" << email.toStdString() << "authentication_token" << key.toStdString());


    bo user = nosql_.Find("users", auth);
    if (user.nFields() == 0)
    {
        qDebug() << "auth failed !";
        return false;
    }

    be login = user.getField("login");
    std::cout << "user : " << login << std::endl;

    payload_builder.append("email", email.toStdString());
    return true;
}


void Xmpp_client::connectedError()
{
    qDebug() << "Xmpp_client::connectedError";
    qDebug() << "Connection failed !";

}

void Xmpp_client::connectedToServer()
{
    qDebug() << "Xmpp_client::connectedToServer";
    qDebug() << "Connection successfull !";

}

void Xmpp_client::messageReceived(const QXmppMessage& message)
{
    qDebug() << "Xmpp_client::messageReceived !!!";

    QString from = message.from();

    QByteArray msg = QByteArray::fromBase64(message.body().toUtf8());

    bo payload;
    try {
        payload = mongo::fromjson(msg.data());
    }
    catch(mongo::MsgAssertionException &e ) {
        std::cout << "caught error on parsing json : " << e.what() << std::endl;
        return;
    }

    if (payload.nFields() == 0)
    {
        std::cout << "caught error on create bson" << std::endl;
        return;
    }


    be action = payload["action"];
    be credentials = payload["credentials"];

    std::cout << "uuid : " << payload["uuid"] << std::endl;
    std::cout << "action : " << payload["action"] << std::endl;
    std::cout << "device : " << payload["device"] << std::endl;


    QString bodyMessage;
    BSONObjBuilder payload_builder;


    if (qstrcmp(action.valuestr(), "create") == 0)
    {
        QUuid uuid = QUuid::createUuid();
        QUuid pub_uuid = QUuid::createUuid();
        QString str_uuid = uuid.toString().mid(1,36);
        QString str_pub_uuid = pub_uuid.toString().mid(1,36);

        payload_builder.append("uuid", str_uuid.toStdString());
        payload_builder.append("pub_uuid", str_pub_uuid.toStdString());
        payload_builder.append("action", "dispatcher.create");

        bodyMessage = buildResponse(action.valuestr(), str_uuid, str_pub_uuid);
    }
    else
    {
        payload_builder.append("action", "dispatcher.update");
        payload_builder.append("uuid", payload["uuid"].valuestr());
        bodyMessage = buildResponse(action.valuestr(), "proceed");
     }




     bool res = checkAuth(credentials.valuestr(), payload_builder);

     if (!res)
     {
         bodyMessage = buildResponse("error", "auth");
     }
     else
     {

         bo gfs_file_struct = nosql_.WriteFile(payload["datas"].toString(false));


         if (gfs_file_struct.nFields() == 0)
         {
             qDebug() << "write on gridFS failed !";
         }
         else
         {
             std::cout << "writefile : " << gfs_file_struct << std::endl;
             //std::cout << "writefile id : " << gfs_file_struct.getField("_id") << " date : " << gfs_file_struct.getField("uploadDate") << std::endl;

             be uploaded_at = gfs_file_struct.getField("uploadDate");
             std::cout << "uploaded : " << uploaded_at << std::endl;

            payload_builder.append("created_at", uploaded_at.date());
            payload_builder.append(gfs_file_struct.getField("_id"));


            bo b_payload = payload_builder.obj();

            std::cout << "PAYLOAD !!!! >>: " << b_payload << std::endl;


            /****** PUSH API PAYLOAD *******/
            qDebug() << "PUSH XMPP PAYLOAD";
            z_message->rebuild(b_payload.objsize());
            memcpy(z_message->data(), (char*)b_payload.objdata(), b_payload.objsize());
            z_push_api->send(*z_message, ZMQ_NOBLOCK);
            /************************/
         }
     }
     sendMessage(from, bodyMessage);
}






void Xmpp_client::presenceReceived(const QXmppPresence& presence)
{
    qDebug() << "Xmpp_client::presenceReceived !!!";
    QString from = presence.from();

    QString message;
    switch(presence.type())
    {
    case QXmppPresence::Subscribe:
        {
            QXmppPresence subscribed;
            subscribed.setTo(from);

            subscribed.setType(QXmppPresence::Subscribed);
            this->sendPacket(subscribed);

            // reciprocal subscription
            QXmppPresence subscribe;
            subscribe.setTo(from);
            subscribe.setType(QXmppPresence::Subscribe);
            this->sendPacket(subscribe);

            return;
        }
        break;
    case QXmppPresence::Subscribed:
        message = "<B>%1</B> accepted your request";
        break;
    case QXmppPresence::Unsubscribe:
        message = "<B>%1</B> unsubscribe";
        break;
    case QXmppPresence::Unsubscribed:
        message = "<B>%1</B> unsubscribed";
        break;
    default:
        return;
        break;
    }

    if(message.isEmpty())
        return;
}




QString Xmpp_client::buildResponse(QString action, QString data1, QString data2)
{
    QVariantMap data;
    QString body;
    if (action == "update")
    {
        //body.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?><host><status>" + data1 + "</status></host>");
        data.insert("status", data1);
    }
    else if (action == "create")
    {
        //body.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?><host><uuid>" + data1 + "</uuid><pub_uuid>" + data2 + "</pub_uuid></host>");
        data.insert("uuid", data1);
        data.insert("pub_uuid", data2);
    }
    else if (action == "error")
    {
        //body.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?><host><error>" + data1 + "</error></host>");
        data.insert("error", data1);
    }
    body = QxtJSON::stringify(data);

    return body;
}
