/****************************************************************************
**   ncs is the backend's server of nodecast
**   Copyright (C) 2010-2013  Frédéric Logier <frederic@logier.org>
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

#include "zerogw.h"



Zerogw::Zerogw(int port, QObject *parent) : QObject(parent)
{
    std::cout << "Zerogw::Zerogw constructeur" << std::endl;

    mongodb_ = Mongodb::getInstance();
    zeromq_ = Zeromq::getInstance ();
    m_context = zeromq_->m_context;

    m_mutex = new QMutex();

    m_message = new zmq::message_t(2);
    m_socket_http = new zmq::socket_t (*m_context, ZMQ_REP);
    int hwm = 50000;
    m_socket_http->setsockopt(ZMQ_SNDHWM, &hwm, sizeof (hwm));
    m_socket_http->setsockopt(ZMQ_RCVHWM, &hwm, sizeof (hwm));
    QString uri = "tcp://*:" + QString::number(port);
    m_socket_http->bind(uri.toAscii());


    int http_socket_fd;
    size_t socket_size = sizeof(http_socket_fd);
    m_socket_http->getsockopt(ZMQ_FD, &http_socket_fd, &socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << http_socket_fd << " errno : " << zmq_strerror (errno);

    check_http_data = new QSocketNotifier(http_socket_fd, QSocketNotifier::Read, this);
    connect(check_http_data, SIGNAL(activated(int)), this, SLOT(receive_http_payload()), Qt::DirectConnection);


}


Zerogw::~Zerogw()
{
    qDebug() << "Zerogw destruct";
    m_mutex->lock();
    check_http_data->setEnabled(false);
    m_socket_http->close ();
    delete(m_socket_http);
}


void Zerogw::receive_http_payload()
{}



QBool Zerogw::checkAuth(QString token, BSONObjBuilder &payload_builder, BSONObj &a_user)
{
 //   QString b64 = header.section("Basic ", 1 ,1);
 //   qDebug() << "auth : " << b64;
/*
    QByteArray tmp_auth_decode = QByteArray::fromBase64(b64.toAscii());
    QString auth_decode =  tmp_auth_decode.data();
    QStringList arr_auth =  auth_decode.split(":");

    QString email = arr_auth.at(0);
    QString key = arr_auth.at(1);

    qDebug() << "email : " << email << " key : " << key;

    bo auth = BSON("email" << email.toStdString() << "token" << key.toStdString());
*/

    BSONObj auth = BSON("token" << token.toStdString());

    std::cout << "USER TOKEN : " << auth << std::endl;

    BSONObj l_user = mongodb_->Find("users", auth);
    if (l_user.nFields() == 0)
    {
        qDebug() << "auth failed !";
        return QBool(false);
    }

   // be login = user.getField("login");
    // std::cout << "user : " << login << std::endl;

    payload_builder << l_user.getField("email");
    payload_builder.append("user_id", l_user.getField("_id").OID());

    a_user = l_user;
    return QBool(true);
}



QString Zerogw::buildResponse(QString action, QString data1, QString data2)
{
    QVariantMap data;
    QString body;


    if (action == "update")
    {
        data.insert("uuid", data1);
    }
    else if (action == "register")
    {
        data.insert("node_uuid", data1);
        data.insert("node_password", data2);
    }
    else if (action == "push" || action == "publish" || action == "create")
    {
        data.insert("uuid", data1);
    }
    else if (action == "file")
    {
        data.insert("gridfs", data1);
    }
    else if (action == "error")
    {
        data.insert("error", data1);
        data.insert("code", data2);
    }
    else if (action == "status")
    {
        data.insert("status", data1);
    }
    else
    {
        data.insert(action, data1);
    }
    body = QxtJSON::stringify(data);

    return body;
}




Api_payload::Api_payload(int port) : Zerogw(port)
{
    std::cout << "Api_payload::Api_payload constructeur" << std::endl;

}



void Api_payload::receive_http_payload()
{
    m_mutex->lock();

    check_http_data->setEnabled(false);

    std::cout << "Zapi::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_http->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_http->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        std::cout << "Api_payload::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

        BSONObj data;
        try {
            data = mongo::fromjson(QString::fromAscii((char*)request.data()).toStdString());

            if (data.isValid() && !data.isEmpty())
            {
                std::cout << "Api_payload received : " << res << " data : " << data  << std::endl;
                std::cout << "!!!!!!! BEFORE FORWARD PAYLOAD !!!!" << std::endl;
                //emit forward_payload(data.copy());

                QString fieldname = QString::fromAscii(data.firstElementFieldName());
                QString fieldjson = QString::fromStdString(data.firstElement().toString(false));

                zerogw[fieldname] = fieldjson;

                std::cout << "!!!!!!! AFTER FORWARD PAYLOAD !!!!" << std::endl;
            }
            else
            {
                std::cout << "DATA NO VALID !" << std::endl;
            }

        }
        catch (mongo::MsgAssertionException &e)
        {
            // received not a json
            QString tmp;

            qDebug() << "COUNTER : " << counter;
            key = counter == 0? "METHOD" : "";
            key = counter == 1? "URI" : key;
            key = counter == 2? "X-node-uuid" : key;
            key = counter == 3? "X-node-password" : key;
            key = counter == 4? "X-workflow-uuid" : key;
            key = counter == 5? "X-payload-filename" : key;
            key = counter == 6? "X-payload-type" : key;

            // body
            if (counter==7)
            {
                key = "gfs_id";

                QByteArray requestContent((char*)request.data(), request.size());
                BSONObj gfs_file_struct = mongodb_->WriteFile(zerogw["X-payload-filename"].toStdString(), requestContent.constData (), requestContent.size ());
                if (gfs_file_struct.nFields() == 0)
                {
                    qDebug() << "write on gridFS failed !";
                    tmp = "42";
                }
                else tmp = QString::fromStdString(gfs_file_struct.getField("_id").OID().toString());
            }
            else tmp = QString::fromAscii((char*)request.data(), request.size());

            zerogw[key] = tmp.replace("/", "");

            std::cout << "error on data : " <<  tmp.toStdString() << std::endl;
            std::cout << "error on data BSON : " << e.what() << std::endl;
        }

        if (!(events & ZMQ_RCVMORE))
        {
            QString bodyMessage;

            if (zerogw["METHOD"] == "POST")
            {
                QUuid tmp_session_uuid = QUuid::createUuid();
                QString session_uuid = tmp_session_uuid.toString().mid(1,36);
                bodyMessage = buildResponse("push", session_uuid);
                //qDebug() << "SESSION UUID : " << bodyMessage;
            }
            else bodyMessage = "BAD REQUEST";

            m_message->rebuild(bodyMessage.length());
            memcpy(m_message->data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_http->send(*m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
    m_mutex->unlock();
}



/****************/





Api_node::Api_node(int port) : Zerogw(port)
{
    std::cout << "Api_node::Api_node constructeur" << std::endl;

}


void Api_node::receive_http_payload()
{
    m_mutex->lock();

    check_http_data->setEnabled(false);

    std::cout << "Api_node::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_http->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_http->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        std::cout << "Api_node::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

        BSONObj data;
        try {
            data = mongo::fromjson(QString::fromAscii((char*)request.data()).toStdString());

            if (data.isValid() && !data.isEmpty())
            {
                std::cout << "Api_node received : " << res << " data : " << data  << std::endl;
                std::cout << "!!!!!!! BEFORE FORWARD PAYLOAD !!!!" << std::endl;
                //emit forward_payload(data.copy());

                QString fieldname = QString::fromAscii(data.firstElementFieldName());
                QString fieldjson = QString::fromStdString(data.firstElement().toString(false));

                zerogw[fieldname] = fieldjson;

                std::cout << "!!!!!!! AFTER FORWARD PAYLOAD !!!!" << std::endl;
            }
            else
            {
                std::cout << "DATA NO VALID !" << std::endl;
            }

        }
        catch (mongo::MsgAssertionException &e)
        {
            // received not a json
            QString tmp;

            qDebug() << "COUNTER : " << counter;
            key = counter == 0? "METHOD" : "";
            key = counter == 1? "URI" : key;
            key = counter == 2? "X-user-token" : key;
            key = counter == 3? "X-node-name" : key;


            tmp = QString::fromAscii((char*)request.data(), request.size());

            zerogw[key] = tmp.replace("/", "");

            std::cout << "error on data : " <<  tmp.toStdString() << std::endl;
            std::cout << "error on data BSON : " << e.what() << std::endl;
        }

        if (!(events & ZMQ_RCVMORE))
        {
            QString bodyMessage;

            if (zerogw["METHOD"] == "POST")
            {

                BSONObjBuilder payload_builder;
                payload_builder.genOID();

                BSONObj user;
                QBool res = checkAuth(zerogw["X-user-token"], payload_builder, user);

                if (!res)
                {
                    bodyMessage = buildResponse("error", "auth");
                }
                else if (zerogw["X-node-name"].isEmpty()) bodyMessage = "X-node-name empty";
                else
                {
                    QUuid node_uuid = QUuid::createUuid();
                    QString str_node_uuid = node_uuid.toString().mid(1,36);

                    QUuid node_password = QUuid::createUuid();
                    QString str_node_password = node_password.toString().mid(1,36);

                    bodyMessage = buildResponse("register", str_node_uuid, str_node_password);
                    qDebug() << "NODE UUID : " << bodyMessage;

                    payload_builder.append("nodename", zerogw["X-node-name"].toStdString());
                    payload_builder.append("node_uuid", str_node_uuid.toStdString());
                    payload_builder.append("node_password", str_node_password.toStdString());


                    BSONElement user_id = user.getField("_id");
                    BSONObj l_node = payload_builder.obj();
                    BSONObj node = BSON("nodes" << l_node);
                    mongodb_->Addtoarray("users", user_id.wrap(), node);

                    mongodb_->Insert("nodes", l_node);
                }

            }
            else bodyMessage = "BAD REQUEST";

            m_message->rebuild(bodyMessage.length());
            memcpy(m_message->data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_http->send(*m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
    m_mutex->unlock();
}



/*********************/



Api_workflow::Api_workflow(int port) : Zerogw(port)
{
    std::cout << "Api_workflow::Api_workflow constructeur" << std::endl;

}



void Api_workflow::receive_http_payload()
{
    m_mutex->lock();

    check_http_data->setEnabled(false);

    std::cout << "Api_workflow::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_http->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_http->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        std::cout << "Api_workflow::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

        BSONObj data;
        try {
            data = mongo::fromjson(QString::fromAscii((char*)request.data()).toStdString());

            if (data.isValid() && !data.isEmpty())
            {
                std::cout << "Api_workflow received : " << res << " data : " << data  << std::endl;
                std::cout << "!!!!!!! BEFORE FORWARD PAYLOAD !!!!" << std::endl;
                //emit forward_payload(data.copy());

                QString fieldname = QString::fromAscii(data.firstElementFieldName());
                QString fieldjson = QString::fromStdString(data.firstElement().toString(false));

                zerogw[fieldname] = fieldjson;

                std::cout << "!!!!!!! AFTER FORWARD PAYLOAD !!!!" << std::endl;
            }
            else
            {
                std::cout << "DATA NO VALID !" << std::endl;
            }

        }
        catch (mongo::MsgAssertionException &e)
        {
            // received not a json
            QString tmp;

            qDebug() << "COUNTER : " << counter;
            key = counter == 0? "METHOD" : "";
            key = counter == 1? "URI" : key;
            key = counter == 2? "X-user-token" : key;


            tmp = QString::fromAscii((char*)request.data(), request.size());

            zerogw[key] = tmp.replace("/", "");

            std::cout << "error on data : " <<  tmp.toStdString() << std::endl;
            std::cout << "error on data BSON : " << e.what() << std::endl;
        }

        if (!(events & ZMQ_RCVMORE))
        {
            QString bodyMessage;

            if (zerogw["METHOD"] == "POST")
            {

                BSONObjBuilder workflow_builder;
                workflow_builder.genOID();

                BSONObj user;
                QBool res = checkAuth(zerogw["X-user-token"], workflow_builder, user);

                if (!res)
                {
                    bodyMessage = buildResponse("error", "auth");
                }
                else
                {
                    BSONElement user_id = user.getField("_id");

                    QUuid workflow_uuid = QUuid::createUuid();
                    QString str_workflow_uuid = workflow_uuid.toString().mid(1,36);

                    bodyMessage = buildResponse("create", str_workflow_uuid);
                    qDebug() << "WORKFLOW UUID : " << bodyMessage;

                    workflow_builder.append("uuid", str_workflow_uuid.toStdString());
                    workflow_builder.append("workers", zerogw["workers"].toStdString());
                    workflow_builder.append("user_id", user_id.str());
                    workflow_builder.append("payload_counter", 0);

                    BSONObj l_workflow = workflow_builder.obj();
                    BSONObj workflow = BSON("workflows" << l_workflow);

                    mongodb_->Addtoarray("users", user_id.wrap(), workflow);
                    mongodb_->Insert("workflows", l_workflow);
                }

            }
            else bodyMessage = "BAD REQUEST";

            m_message->rebuild(bodyMessage.length());
            memcpy(m_message->data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_http->send(*m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
    m_mutex->unlock();
}
