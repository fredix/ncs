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




GridfsTask::GridfsTask(QHash <QString, QString> a_zerogw) : m_zerogw(a_zerogw)
{
    mongodb_ = Mongodb::getInstance();
    qDebug() << "GridfsTask::GridfsTask CONSTRUCT";
}

void GridfsTask::run()
{
        qDebug() << "Hello world from thread" << QThread::currentThread();
        qDebug() << "GridfsTask::GridfsTask RUN";

        BSONObjBuilder payload_builder;
        payload_builder.genOID();


        if (!m_zerogw["X-payload-filename"].isEmpty() && m_zerogw["X-payload-mime"] != "json")
        {

            BSONObj gfs_file_struct = mongodb_->WriteFile(m_zerogw["X-payload-filename"].toStdString(), m_requestContent->constData (), m_requestContent->size ());
            if (gfs_file_struct.nFields() == 0)
            {
                qDebug() << "write on gridFS failed !";
                m_zerogw["gfs_id"] = "-1";
            }
            else
            {
                delete(m_requestContent);

                std::cout << "writefile : " << gfs_file_struct << std::endl;
                //std::cout << "writefile id : " << gfs_file_struct.getField("_id") << " date : " << gfs_file_struct.getField("uploadDate") << std::endl;

                be uploaded_at = gfs_file_struct.getField("uploadDate");
                be filename = gfs_file_struct.getField("filename");
                be length = gfs_file_struct.getField("length");

                std::cout << "uploaded : " << uploaded_at << std::endl;


                payload_builder.append("created_at", uploaded_at.date());
                payload_builder.append("filename", filename.str());
                payload_builder.append("length", length.numberLong());
                payload_builder.append("gfs_id", gfs_file_struct.getField("_id").OID());
                payload_builder.append("gridfs", true);

                m_zerogw["gfs_id"] = QString::fromStdString(gfs_file_struct.getField("_id").OID().toString());
            }

        }
        // not a binary
        else
        {
            payload_builder.append("gridfs", false);
            m_zerogw["data"] = QString::fromAscii(m_requestContent->constData(), m_requestContent->size());
            payload_builder.append("data", m_zerogw["data"].toStdString());

            delete(m_requestContent);
        }



        payload_builder.append("timestamp", m_zerogw["timestamp"].toUInt());


        QUuid payload_uuid = QUuid::createUuid();
        QString str_payload_uuid = payload_uuid.toString().mid(1,36);
        payload_builder.append("payload_uuid", str_payload_uuid.toStdString());

        int counter = mongodb_->Count("payloads");
        payload_builder.append("counter", counter + 1);

        qDebug() << "NODE ID : " << m_zerogw["node_id"];

//            payload_builder.append("node_id", mongo::OID(m_zerogw["node_id"].toStdString()));
        payload_builder.append("node_uuid", m_zerogw["X-node-uuid"].toStdString());
        payload_builder.append("node_password", m_zerogw["X-node-password"].toStdString());
        payload_builder.append("workflow_uuid", m_zerogw["X-workflow-uuid"].toStdString());
        payload_builder.append("payload_type", m_zerogw["X-payload-type"].toStdString());
        payload_builder.append("payload_mime", m_zerogw["X-payload-mime"].toStdString());
        payload_builder.append("action", m_zerogw["X-payload-action"].toStdString());


        BSONObj payload = payload_builder.obj();
        mongodb_->Insert("payloads", payload);
        std::cout << "payload inserted" << payload << std::endl;

        BSONObjBuilder session_builder;
        session_builder.genOID();
        session_builder.append("uuid", m_zerogw["session_uuid"].toStdString());
        session_builder.append("user_id", mongo::OID(m_zerogw["user_id"].toStdString()));
        session_builder.append("payload_id", payload.getField("_id").OID());
        session_builder.append("workflow_id", mongo::OID(m_zerogw["workflow_id"].toStdString()));
        session_builder.append("start_timestamp", payload.getField("timestamp"));
        BSONObj session = session_builder.obj();
        mongodb_->Insert("sessions", session);
        /***********************************/
        //bodyMessage = buildResponse(zerogw["X-payload-action"], session_uuid);
        //qDebug() << "SESSION UUID : " << bodyMessage;

        std::cout << "session inserted : " << session << std::endl;

        BSONObj l_payload = BSON("action" << m_zerogw["X-payload-action"].toStdString() <<
                                 "payload_type" << m_zerogw["X-payload-type"].toStdString() <<
                                 //"publish" << (!publish.isEmpty() && publish == "true")? "true" : "false" <<
                                 "session_uuid" << m_zerogw["session_uuid"].toStdString() <<
                                 "timestamp" << payload.getField("timestamp"));


        qDebug() << "GridfsTask::GridfsTask BEFORE EMIT";
        emit forward_payload(l_payload);
        qDebug() << "GridfsTask::GridfsTask AFTER EMIT";

}






Zerogw::Zerogw(QString basedirectory, int port, QString ipc_name="", QObject *parent) : m_basedirectory(basedirectory), m_port(port), m_ipc_name(ipc_name), QObject(parent)
{
    std::cout << "Zerogw::Zerogw construct" << std::endl;

    mongodb_ = Mongodb::getInstance();
    zeromq_ = Zeromq::getInstance ();
    m_context = zeromq_->m_context;
}

void Zerogw::init()
{
    // Socket to ZPULL class
    z_push_api = new zmq::socket_t(*m_context, ZMQ_PUSH);

    int hwm1 = 50000;
    z_push_api->setsockopt (ZMQ_SNDHWM, &hwm1, sizeof (hwm1));
    z_push_api->setsockopt (ZMQ_RCVHWM, &hwm1, sizeof (hwm1));

    QString directory = "ipc://" + m_basedirectory + "/http";
    z_push_api->connect(directory.toLatin1());


    // socket from ZEROGW
    m_socket_zerogw = new zmq::socket_t (*m_context, ZMQ_REP);
    int hwm = 50;
    m_socket_zerogw->setsockopt(ZMQ_SNDHWM, &hwm, sizeof (hwm));
    m_socket_zerogw->setsockopt(ZMQ_RCVHWM, &hwm, sizeof (hwm));

    if (m_port == 0 && !m_ipc_name.isEmpty())
    {
//        QString uri = "tcp://127.0.0.1:2504" + QString::number(port);
        QString uri = "ipc://" + m_basedirectory + m_ipc_name;
        m_socket_zerogw->connect(uri.toAscii());
    }
    else
    {
        QString uri = "tcp://*:" + QString::number(m_port);
        m_socket_zerogw->bind(uri.toAscii());
    }

    int http_socket_fd;
    size_t socket_size = sizeof(http_socket_fd);
    m_socket_zerogw->getsockopt(ZMQ_FD, &http_socket_fd, &socket_size);
    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << http_socket_fd << " errno : " << zmq_strerror (errno);
    check_http_data = new QSocketNotifier(http_socket_fd, QSocketNotifier::Read, this);
    connect(check_http_data, SIGNAL(activated(int)), this, SLOT(receive_http_payload()), Qt::DirectConnection);
}


Zerogw::~Zerogw()
{
    qDebug() << "Zerogw destruct";   

    check_http_data->setEnabled(false);
    z_push_api->close();
    m_socket_zerogw->close();

    delete(m_socket_zerogw);
    delete(z_push_api);
}


void Zerogw::receive_http_payload()
{}

void Zerogw::forward_payload_to_zpull(BSONObj payload)
{
    qDebug() << "Zerogw::forward_payload_to_zpull";

    if (payload.nFields() != 0)
    {
        /****** PUSH API PAYLOAD *******/
        std::cout << "PUSH HTTP API PAYLOAD : " << payload << std::endl;
        zmq::message_t z_message(payload.objsize());
        memcpy(z_message.data(), (char*)payload.objdata(), payload.objsize());
        z_push_api->send(z_message, ZMQ_NOBLOCK);
        //z_push_api->send(*z_message);
        /************************/
    }

}



QBool Zerogw::checkAuth(QString token, QHash <QString, QString> &hash_builder, BSONObj &a_user)
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

    hash_builder["email"] = QString::fromStdString(l_user.getField("email").str());
    hash_builder["user_id"] = QString::fromStdString(l_user.getField("_id").OID().toString());

    a_user = l_user;
    return QBool(true);
}


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




Api_payload::Api_payload(QString basedirectory, int port, QString ipc_name) : Zerogw(basedirectory, port, ipc_name)
{
    std::cout << "Api_payload::Api_payload constructeur" << std::endl;

    enumToZerogwHeaderPayload.insert(0, HTTP_METHOD);
    enumToZerogwHeaderPayload.insert(1, URI);
    enumToZerogwHeaderPayload.insert(2, X_user_token);
    enumToZerogwHeaderPayload.insert(3, X_node_uuid);
    enumToZerogwHeaderPayload.insert(4, X_node_password);
    enumToZerogwHeaderPayload.insert(5, X_workflow_uuid);
    enumToZerogwHeaderPayload.insert(6, X_payload_filename);
    enumToZerogwHeaderPayload.insert(7, X_payload_type);
    enumToZerogwHeaderPayload.insert(8, X_payload_mime);
    enumToZerogwHeaderPayload.insert(9, X_payload_action);
    enumToZerogwHeaderPayload.insert(10, BODY);
}



void Api_payload::receive_http_payload()
{
    check_http_data->setEnabled(false);

    std::cout << "Api_payload::receive_payload : API : " << m_port << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    BSONObj workflow;
    BSONObj user_nodes;
    QString bodyMessage="";


    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_zerogw->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_zerogw->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        //std::cout << "Api_payload::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;


        QString data_from_zerogw;



        qDebug() << "COUNTER : " << counter;



        switch(enumToZerogwHeaderPayload[counter])
        {
        case HTTP_METHOD:
            // check METHOD
            if (request.size() == 0) bodyMessage ="header METHOD is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["METHOD"] = data_from_zerogw;

                if (zerogw["METHOD"] != "POST")
                    bodyMessage = "BAD REQUEST : " + zerogw["METHOD"];
                else
                {
                    QDateTime timestamp = QDateTime::currentDateTime();
                    zerogw["timestamp"] = QString::number(timestamp.toTime_t());


                    /********* CREATE SESSION **********/
                    QUuid tmp_session_uuid = QUuid::createUuid();
                    QString session_uuid = tmp_session_uuid.toString().mid(1,36);
                    zerogw["session_uuid"] = session_uuid;
                }
            }
            break;

        case URI:
            if (!bodyMessage.isEmpty()) break;

            // Check URI
            if (request.size() == 0) bodyMessage ="header URI is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["URI"] = data_from_zerogw;
                qDebug() << "URI : " << data_from_zerogw;
            }
            break;

        case X_user_token:
            if (!bodyMessage.isEmpty()) break;

            // Check X-user-token
            if (request.size() == 0) bodyMessage ="header X-user-token is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-user-token"] = data_from_zerogw;
                BSONObj user;
                QBool res = checkAuth(zerogw["X-user-token"], zerogw, user);
                if (!res)
                {
                    bodyMessage = buildResponse("error", "auth");
                }
            }
            break;

        case X_node_uuid:
            if (!bodyMessage.isEmpty()) break;

            // Check X-node-uuid
            if (request.size() == 0) bodyMessage ="header X-node-uuid is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-node-uuid"] = data_from_zerogw;
            }
            break;


        case X_node_password:
            if (!bodyMessage.isEmpty()) break;

            // Check X-node-password
            if (request.size() == 0) bodyMessage ="header X-node-password is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-node-password"] = data_from_zerogw;

                BSONObj node_auth = BSON("node_uuid" << zerogw["X-node-uuid"].toStdString() << "node_password" << zerogw["X-node-password"].toStdString());
                BSONObj node = mongodb_->Find("nodes", node_auth);
                if (node.nFields() == 0)
                    bodyMessage = buildResponse("error", "node unknown");
                else
                {
                    BSONObj user_search = BSON("_id" << node.getField("user_id"));
                    user_nodes = mongodb_->Find("users", user_search);

                    if (user_nodes.nFields() == 0)
                        bodyMessage = buildResponse("error", "node", "unknown");

                    else
                    {
                        zerogw["user_id"] = QString::fromStdString(user_nodes.getField("_id").OID().toString());
                        BSONObj nodes = user_nodes.getField("nodes").Obj();

                        list<be> list_nodes;
                        nodes.elems(list_nodes);
                        list<be>::iterator i;

                        bool node_found = false;
                        /********   Iterate over each user's nodes   *******/
                        /********  find node with uuid and set the node id to payload collection *******/
                        for(i = list_nodes.begin(); i != list_nodes.end(); ++i)
                        {
                            BSONObj l_node = (*i).embeddedObject ();
                            be node_id;
                            l_node.getObjectID (node_id);

                            //std::cout << "NODE1 => " << node_id.OID()   << std::endl;
                            //std::cout << "NODE2 => " << node_uuid.toStdString() << std::endl;

                            if (zerogw["X-node-uuid"].toStdString().compare(l_node.getField("uuid").str()) == 0)
                            {
                                zerogw["node_id"] = QString::fromStdString(node_id.OID().toString());
                                node_found = true;
                                break;
                            }
                        }                        
                        //if (!node_found) {bodyMessage = buildResponse("error", "node", "unknown");}


                    }
                }

            }
            break;

        case X_workflow_uuid:
            if (!bodyMessage.isEmpty()) break;

            // Check X-workflow-uuid
            if (request.size() == 0) bodyMessage ="header X-workflow-uuid is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-workflow-uuid"] = data_from_zerogw;

                // check workflow
                BSONObj workflow_search = BSON("uuid" <<  zerogw["X-workflow-uuid"].toStdString());
                workflow = mongodb_->Find("workflows", workflow_search);
                if (workflow.nFields() == 0)
                    bodyMessage = buildResponse("error", "workflow unknown");                            
                else zerogw["workflow_id"] = QString::fromStdString(workflow.getField("_id").OID().toString());
            }
            break;

        case X_payload_filename:
            if (!bodyMessage.isEmpty()) break;

            // Check X-payload-filename, facultatif header, not usefull for json  payload
            data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
            zerogw["X-payload-filename"] = data_from_zerogw;
            break;

        case X_payload_type:
            if (!bodyMessage.isEmpty()) break;

            // Check X-payload-type
            if (request.size() == 0) bodyMessage ="header X-payload-type is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-payload-type"] = data_from_zerogw;
            }
            break;

        case X_payload_mime:
            if (!bodyMessage.isEmpty()) break;

            // Check X-payload-mime
            if (request.size() == 0) bodyMessage ="header X-payload-mime is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-payload-mime"] = data_from_zerogw;
            }
            break;

        case X_payload_action:
            if (!bodyMessage.isEmpty()) break;

            // Check X-payload-action
            if (request.size() == 0) bodyMessage ="header X-payload-action is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-payload-action"] = data_from_zerogw;
            }
            break;

        case BODY:
            if (!bodyMessage.isEmpty()) break;          

            if (request.size() == 0)
            {
                zerogw["gfs_id"] = "-1";
                bodyMessage = buildResponse("error", "empty file");
            }
            else
            {
                //QByteArray requestContent((char*)request.data(), request.size());

                GridfsTask *gfstask = new GridfsTask(zerogw);
                gfstask->setAutoDelete(true);
                gfstask->m_requestContent = new QByteArray ((char*)request.data(), request.size());
                connect(gfstask, SIGNAL(forward_payload(BSONObj)), this, SLOT(forward_payload_to_zpull(BSONObj)), Qt::QueuedConnection);

                QThreadPool::globalInstance()->start(gfstask);
            }
            break;
        }



        if (!(events & ZMQ_RCVMORE))
        {
            qDebug() << "ZMQ_RCVMORE";

            if (bodyMessage.isEmpty())
            {
                bodyMessage = buildResponse(zerogw["X-payload-action"], zerogw["session_uuid"]);
            }

            // send response to client
            zmq::message_t m_message(bodyMessage.length() +1);
            memcpy(m_message.data(), bodyMessage.toAscii(), bodyMessage.length() +1);

            m_socket_zerogw->send(m_message, 0);
            qDebug() << "returning : " << bodyMessage;
            bodyMessage="";
            counter=-1;
       }
        counter++;
    }
    check_http_data->setEnabled(true);
}



/****************/
Api_node::Api_node(QString basedirectory, int port) : Zerogw(basedirectory, port)
{
    std::cout << "Api_node::Api_node constructeur" << std::endl;

}


void Api_node::receive_http_payload()
{
    check_http_data->setEnabled(false);

    std::cout << "Api_node::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_zerogw->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_zerogw->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

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
            zmq::message_t m_message(bodyMessage.length());
            memcpy(m_message.data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_zerogw->send(m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
}



/*********************/
Api_workflow::Api_workflow(QString basedirectory, int port) : Zerogw(basedirectory, port)
{
    std::cout << "Api_workflow::Api_workflow constructeur" << std::endl;

}



void Api_workflow::receive_http_payload()
{
    check_http_data->setEnabled(false);

    std::cout << "Api_workflow::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_zerogw->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_zerogw->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        std::cout << "Api_workflow::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

        BSONObj b_workflow;
        QString bodyMessage;
        try {
            b_workflow = mongo::fromjson(QString::fromAscii((char*)request.data()).toStdString());

            if (!b_workflow.isValid() || b_workflow.isEmpty())
            {
                std::cout << "WORKFLOW NO VALID !" << std::endl;
                std::cout << "Api_workflow received : " << res << " WORKFLOW : " << b_workflow  << std::endl;
                bodyMessage = buildResponse("error", "error on parsing JSON");
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
            key = counter == 3? "X-workflow-name" : key;

            // body
            if (counter == 4)
                bodyMessage = buildResponse("error", "error on parsing JSON");
            else
            {
                tmp = QString::fromAscii((char*)request.data(), request.size());
                zerogw[key] = tmp;
            }

            //std::cout << "error on data : " <<  tmp.toStdString() << std::endl;
            //std::cout << "error on data BSON : " << e.what() << std::endl;
        }

        if (!(events & ZMQ_RCVMORE))
        {
            BSONObjBuilder workflow_builder;
            BSONObj user;


            if (zerogw["METHOD"] != "POST")
                bodyMessage = "BAD REQUEST : " + zerogw["METHOD"];

            else if (zerogw["X-user-token"].isEmpty() ||
                     zerogw["X-workflow-name"].isEmpty())
            {
                bodyMessage ="header is empty";
            }
            else
            {
                workflow_builder.genOID();

                QBool res = checkAuth(zerogw["X-user-token"], workflow_builder, user);

                if (!res)                                    
                    bodyMessage = buildResponse("error", "auth");
            }

            if (bodyMessage.isEmpty())
            {
                    BSONElement user_id = user.getField("_id");

                    QUuid workflow_uuid = QUuid::createUuid();
                    QString str_workflow_uuid = workflow_uuid.toString().mid(1,36);

                    bodyMessage = buildResponse("create", str_workflow_uuid);
                    qDebug() << "WORKFLOW UUID : " << bodyMessage;

                    workflow_builder.append("workflow", zerogw["X-workflow-name"].toStdString());
                    workflow_builder.append("uuid", str_workflow_uuid.toStdString());
                    workflow_builder.append("workers", b_workflow);
                    workflow_builder.append("user_id", user_id.str());
                    workflow_builder.append("payload_counter", 0);

                    BSONObj l_workflow = workflow_builder.obj();
                    BSONObj workflow = BSON("workflows" << l_workflow);

                    mongodb_->Addtoarray("users", user_id.wrap(), workflow);
                    mongodb_->Insert("workflows", l_workflow);

            }
            zmq::message_t m_message(bodyMessage.length());
            memcpy(m_message.data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_zerogw->send(m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
}



/******************************/


Api_user::Api_user(QString basedirectory, int port) : Zerogw(basedirectory, port)
{
    std::cout << "Api_user::Api_user constructeur" << std::endl;

}


void Api_user::receive_http_payload()
{
    check_http_data->setEnabled(false);

    std::cout << "Api_user::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_zerogw->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_zerogw->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        std::cout << "Api_user::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

        BSONObj b_user;
        QString bodyMessage;

        try {
            b_user = mongo::fromjson(QString::fromAscii((char*)request.data()).toStdString());

            if (!b_user.isValid() || b_user.isEmpty())
            {
                std::cout << "USER NOT VALID !" << std::endl;
                std::cout << "Api_user received : " << res << " USER : " << b_user  << std::endl;
                bodyMessage = buildResponse("error", "error on parsing JSON");
            }
            else if (!b_user.hasField("email") || !b_user.hasField("password"))
            {
                bodyMessage = buildResponse("error", "json", "missing email or password");
            }
        }
        catch (mongo::MsgAssertionException &e)
        {
            // received not a json
            QString tmp;

            qDebug() << "COUNTER : " << counter;
            key = counter == 0? "METHOD" : "";
            key = counter == 1? "URI" : key;
            key = counter == 2? "X-admin-token" : key;

            // body
            if (counter == 3)
                bodyMessage = buildResponse("error", "error on parsing JSON");
            else
            {
                tmp = QString::fromAscii((char*)request.data(), request.size());
                zerogw[key] = tmp;
            }

//            std::cout << "error on data : " <<  tmp.toStdString() << std::endl;
//            std::cout << "error on data BSON : " << e.what() << std::endl;
        }

        if (!(events & ZMQ_RCVMORE))
        {
            BSONObjBuilder user_builder;
            BSONObj user;

            if (zerogw["METHOD"] != "POST")
                bodyMessage = "BAD REQUEST : " + zerogw["METHOD"];

            else if (zerogw["X-admin-token"].isEmpty())
            {
                bodyMessage ="X-admin-token is empty";
            }
            else
            {
                user_builder.genOID();

                QBool res = checkAuth(zerogw["X-admin-token"], user_builder, user);

                if (!res)
                {
                    bodyMessage = buildResponse("error", "auth");
                }
                else if (!user.getBoolField("admin"))
                {
                    bodyMessage = buildResponse("error", "user", "admin only");
                }
            }


            if (bodyMessage.isEmpty())
            {
                QUuid token = QUuid::createUuid();
                QString str_token = token.toString().mid(1,36);

                QUuid tracker_token = QUuid::createUuid();
                QString str_tracker_token = tracker_token.toString().remove(QChar('-')).mid(1,32);

                QUuid ftp_token = QUuid::createUuid();
                QString str_ftp_token = ftp_token.toString().remove(QChar('-')).mid(1,32);


                QUuid ftp_directory = QUuid::createUuid();
                QString str_ftp_directory = ftp_directory.toString().remove(QChar('-')).mid(1,32);


                QUuid xmpp_token = QUuid::createUuid();
                QString str_xmpp_token = xmpp_token.toString().remove(QChar('-')).mid(1,32);

                QUuid api_token = QUuid::createUuid();
                QString str_api_token = api_token.toString().remove(QChar('-')).mid(1,32);


                QString password = QString::fromStdString(b_user.getField("password").str());

                QCryptographicHash cipher( QCryptographicHash::Sha1 );
                cipher.addData(password.simplified().toAscii());
                QByteArray password_hash = cipher.result();

                qDebug() << "password_hash : " << password_hash.toHex();


                QString login = b_user.hasField("login")? QString::fromStdString(b_user.getField("login").str()) : "anonymous";
                bool ftp = b_user.hasField("ftp")? b_user.getBoolField("ftp") : false;
                bool tracker = b_user.hasField("tracker")? b_user.getBoolField("tracker") : false;
                bool xmpp = b_user.hasField("xmpp")? b_user.getBoolField("xmpp") : false;
                bool api = b_user.hasField("api")? b_user.getBoolField("api") : false;

                user_builder.append("admin", false);
                user_builder.append("login", login.toStdString());
                user_builder.append(b_user.getField("email"));
                user_builder.append("password", QString::fromLatin1(password_hash.toHex()).toStdString());
                user_builder.append("token", str_token.toStdString());

                user_builder.append("ftp", BSON("token" << str_ftp_token.toStdString() << "activated" << ftp << "directory" << str_ftp_directory.toStdString()));
                user_builder.append("tracker" , BSON("token" << str_tracker_token.toStdString()  << "activated" << tracker));
                user_builder.append("xmpp" , BSON("token" << str_xmpp_token.toStdString()  << "activated" << xmpp));
                user_builder.append("api" , BSON("token" << str_api_token.toStdString()  << "activated" << api));

                BSONObj l_user = user_builder.obj();
                mongodb_->Insert("users", l_user);

                if (ftp) {
                    QString command = "{\"email\": \"" + QString::fromStdString(b_user.getField("email").str()) + "\", \"password\": \"" + str_ftp_token + "\", \"path\": \"" + str_ftp_directory + "\"}";
                    emit create_ftp_user("nodeftp", command);
                    qDebug() << "EMIT CREATE FTP USER";
                }
                bodyMessage = buildResponse("token", str_token);
            }
            zmq::message_t m_message(bodyMessage.length());
            memcpy(m_message.data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_zerogw->send(m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
}



/*********************/





/*********************/
Api_session::Api_session(QString basedirectory, int port) : Zerogw(basedirectory, port)
{
    std::cout << "Api_session::Api_session constructeur" << std::endl;

    enumToZerogwHeaderSession.insert(0, HTTP_METHOD);
    enumToZerogwHeaderSession.insert(1, URI);
    enumToZerogwHeaderSession.insert(2, X_user_token);
    enumToZerogwHeaderSession.insert(3, X_session_uuid);

}



void Api_session::receive_http_payload()
{
    check_http_data->setEnabled(false);

    std::cout << "Api_session::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    QString bodyMessage="";

    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_zerogw->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_zerogw->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        std::cout << "Api_session::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;


        BSONObjBuilder session_builder;
        session_builder.genOID();
        QString data_from_zerogw;



        switch(enumToZerogwHeaderSession[counter])
        {
        case HTTP_METHOD:
            // check METHOD
            if (request.size() == 0) bodyMessage ="header METHOD is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["METHOD"] = data_from_zerogw;

                if (zerogw["METHOD"] != "GET")
                    bodyMessage = "BAD REQUEST : " + zerogw["METHOD"];
            }
            break;

        case URI:
            if (!bodyMessage.isEmpty()) break;

            // Check URI
            if (request.size() == 0) bodyMessage ="header URI is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["URI"] = data_from_zerogw;
            }
            break;

        case X_user_token:
            if (!bodyMessage.isEmpty()) break;

            // Check X-user-token
            if (request.size() == 0) bodyMessage ="header X-user-token is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-user-token"] = data_from_zerogw;
                BSONObj user;
                QBool res = checkAuth(zerogw["X-user-token"], session_builder, user);
                if (!res)
                {
                    bodyMessage = buildResponse("error", "auth");
                }
            }
            break;

        case X_session_uuid:
            if (!bodyMessage.isEmpty()) break;

            // Check X-node-uuid
            if (request.size() == 0) bodyMessage ="header X-session-uuid is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-session-uuid"] = data_from_zerogw;

                BSONObj session_search = BSON("uuid" <<  zerogw["X-session-uuid"].toStdString());
                BSONObj session = mongodb_->Find("sessions", session_search);
                if (session.nFields() == 0)
                {
                    bodyMessage = buildResponse("error", "session unknown");

                }
            }
            break;
        }



        if (!(events & ZMQ_RCVMORE))
        {
            qDebug() << "ZMQ_RCVMORE";

            if (bodyMessage.isEmpty())
            {
                BSONObj session_search = BSON("uuid" <<  zerogw["X-session-uuid"].toStdString());
                BSONObj session = mongodb_->Find("sessions", session_search);
                if (session.nFields() == 0)
                {
                    bodyMessage = buildResponse("status", "session unknown");
                }

                if (!session.hasField("counter"))
                {
                    bodyMessage = buildResponse("status", "traitement queued");

                }
                else
                {
                    BSONObj workflow_search = BSON("_id" <<  session.getField("workflow_id"));
                    BSONObj workflow = mongodb_->Find("workflows", workflow_search);
                    if (workflow.nFields() == 0)
                    {
                        bodyMessage = buildResponse("status", "workflow unknown");

                    }
                    else
                    {
                        BSONObj workers = mongo::fromjson(workflow.getField("workers").toString(false));

                        for( BSONObj::iterator i = workers.begin(); i.more();)
                        {
                            BSONElement worker = i.next();
                            qDebug() << "WORKER : " <<  QString::fromStdString(worker.toString());
                        }
                        QString last_worker = QString::fromStdString(session.getField("last_worker").valuestr());
                        bodyMessage = buildResponse("status", last_worker);
                    }
                }



            }
            zmq::message_t m_message(bodyMessage.length());
            memcpy(m_message.data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_zerogw->send(m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
}



/******************************/






/*********************/
Api_app::Api_app(QString basedirectory, int port) : Zerogw(basedirectory, port)
{
    std::cout << "Api_app::Api_app constructeur" << std::endl;

    enumToZerogwHeaderApp.insert(0, HTTP_METHOD);
    enumToZerogwHeaderApp.insert(1, URI);
    enumToZerogwHeaderApp.insert(2, X_user_token);
}



void Api_app::receive_http_payload()
{
    check_http_data->setEnabled(false);

    std::cout << "Api_app::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    QString bodyMessage="";

    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_zerogw->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_zerogw->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        std::cout << "Api_app::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;


        BSONObjBuilder session_builder;
        session_builder.genOID();
        QString data_from_zerogw;



        switch(enumToZerogwHeaderApp[counter])
        {
        case HTTP_METHOD:
            // check METHOD
            if (request.size() == 0) bodyMessage ="header METHOD is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["METHOD"] = data_from_zerogw;

                if (zerogw["METHOD"] != "GET")
                    bodyMessage = "BAD REQUEST : " + zerogw["METHOD"];
            }
            break;

        case URI:
            if (!bodyMessage.isEmpty()) break;

            // Check URI
            if (request.size() == 0) bodyMessage ="header URI is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["URI"] = data_from_zerogw;
                qDebug() << "URI : " << data_from_zerogw;
            }
            break;

        case X_user_token:
            if (!bodyMessage.isEmpty()) break;

            // Check X-user-token
            if (request.size() == 0) bodyMessage ="header X-user-token is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-user-token"] = data_from_zerogw;
                BSONObj user;
                QBool res = checkAuth(zerogw["X-user-token"], session_builder, user);
                if (!res)
                {
                    bodyMessage = buildResponse("error", "auth");
                }
                else
                {
                    BSONObj user_search = BSON("user_id" <<  user.getField("_id").OID());
                    BSONObj user_app = mongodb_->Find("apps", user_search);
                    if (user_app.nFields() == 0)
                    {
                        bodyMessage = buildResponse("status", "app unknown");
                    }

                }
            }
            break;
        }



        if (!(events & ZMQ_RCVMORE))
        {
            qDebug() << "ZMQ_RCVMORE";

            if (bodyMessage.isEmpty())
            {
                qDebug() << "URI : " << zerogw["URI"];
                bodyMessage = buildResponse("status", zerogw["URI"]);

                QList <QString> uri_params = zerogw["URI"].split("/");
                qDebug() << "URI PARAMS " << uri_params[2];

                BSONObj app_search = BSON("appname" <<  uri_params[2].toStdString());

                BSONObj app = mongodb_->Find("apps", app_search);
                if (app.nFields() == 0)
                {
                    bodyMessage = buildResponse("status", "app unknown");
                }


            }
            zmq::message_t m_message(bodyMessage.length());
            memcpy(m_message.data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_zerogw->send(m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
}



/******************************/







/*********************/
Api_ftpauth::Api_ftpauth(QString basedirectory, int port) : Zerogw(basedirectory, port)
{
    std::cout << "Api_ftpauth::Api_ftpauth constructeur" << std::endl;

    enumToZerogwHeaderFtpauth.insert(0, HTTP_METHOD);
    enumToZerogwHeaderFtpauth.insert(1, URI);
    enumToZerogwHeaderFtpauth.insert(2, X_user_email);
    enumToZerogwHeaderFtpauth.insert(3, X_user_password);

}



void Api_ftpauth::receive_http_payload()
{
    check_http_data->setEnabled(false);

    std::cout << "Api_ftpauth::receive_payload" << std::endl;

    QHash <QString, QString> zerogw;
    QString key;
    int counter=0;
    QString bodyMessage="";

    while (true)
    {
        zmq::message_t request;
        bool res = m_socket_zerogw->recv(&request, ZMQ_NOBLOCK);
        if (!res && zmq_errno () == EAGAIN) break;

        qint32 events = 0;
        std::size_t eventsSize = sizeof(events);
        m_socket_zerogw->getsockopt(ZMQ_RCVMORE, &events, &eventsSize);

        std::cout << "Api_ftpauth::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;


        BSONObjBuilder session_builder;
        session_builder.genOID();
        QString data_from_zerogw;



        switch(enumToZerogwHeaderFtpauth[counter])
        {
        case HTTP_METHOD:
            // check METHOD
            if (request.size() == 0) bodyMessage ="header METHOD is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["METHOD"] = data_from_zerogw;

                if (zerogw["METHOD"] != "GET")
                    bodyMessage = "BAD REQUEST : " + zerogw["METHOD"];
            }
            break;

        case URI:
            if (!bodyMessage.isEmpty()) break;

            // Check URI
            if (request.size() == 0) bodyMessage ="header URI is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["URI"] = data_from_zerogw;
            }
            break;

        case X_user_email:
            if (!bodyMessage.isEmpty()) break;

            // Check X-user-email
            if (request.size() == 0) bodyMessage ="header X-user-email is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-user-email"] = data_from_zerogw;
            }
            break;

        case X_user_password:
            if (!bodyMessage.isEmpty()) break;

            // Check X-user-password
            if (request.size() == 0) bodyMessage ="header X-user-password is empty";
            else
            {
                data_from_zerogw = QString::fromAscii((char*)request.data(), request.size());
                zerogw["X-user-password"] = data_from_zerogw;
            }
            break;
        }



        if (!(events & ZMQ_RCVMORE))
        {
            qDebug() << "ZMQ_RCVMORE";

            if (bodyMessage.isEmpty())
            {
                QCryptographicHash cipher( QCryptographicHash::Sha1 );
                cipher.addData(zerogw["X-user-password"].simplified().toAscii());
                QByteArray password_hash = cipher.result();

                qDebug() << "Api_ftpauth::receive_payload password_hash : " << password_hash.toHex();

                BSONObj bauth = BSON("email" << zerogw["X-user-email"].toStdString() << "password" << QString::fromLatin1(password_hash.toHex()).toStdString());
                BSONObj l_user = mongodb_->Find("users", bauth);

                if (l_user.nFields() == 0)
                {
                    qDebug() << "Api_ftpauth::receive_payload auth failed !";
                    bodyMessage = buildResponse("error", "auth", "unknown user");
                }
                else
                {
                    bodyMessage = buildResponse("token", QString::fromStdString(l_user.getFieldDotted("ftp.token").str()));
                    qDebug() << "Api_ftpauth::receive_payload TOKEN : " << bodyMessage;
                }
            }
            zmq::message_t m_message(bodyMessage.length());
            memcpy(m_message.data(), bodyMessage.toAscii(), bodyMessage.length());

            m_socket_zerogw->send(m_message, 0);
            qDebug() << "returning : " << bodyMessage;
        }
        counter++;
    }
    qDebug() << "ZEROGW : " << zerogw;
    check_http_data->setEnabled(true);
}



/******************************/
