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


#include "http_api.h"


Http_api::Http_api(QxtAbstractWebSessionManager * sm, QObject * parent): QxtWebSlotService(sm,parent)
{
    nosql_ = Nosql::getInstance_front();
    zeromq_ = Zeromq::getInstance ();

    z_message = new zmq::message_t(2);
    //m_context = new zmq::context_t(1);
    z_push_api = new zmq::socket_t(*zeromq_->m_context, ZMQ_PUSH);

    uint64_t hwm = 5000;
    zmq_setsockopt (z_push_api, ZMQ_HWM, &hwm, sizeof (hwm));

    z_push_api->bind("inproc://http");
}


Http_api::~Http_api()
{}


QBool Http_api::checkAuth(QString header, BSONObjBuilder &payload_builder, bo &a_user)
{
    QString b64 = header.section("Basic ", 1 ,1);
    qDebug() << "auth : " << b64;

    QByteArray tmp_auth_decode = QByteArray::fromBase64(b64.toAscii());
    QString auth_decode =  tmp_auth_decode.data();
    QStringList arr_auth =  auth_decode.split(":");

    QString email = arr_auth.at(0);
    QString key = arr_auth.at(1);

    qDebug() << "email : " << email << " key : " << key;

    bo auth = BSON("email" << email.toStdString() << "authentication_token" << key.toStdString());


    bo l_user = nosql_->Find("users", auth);

    if (l_user.nFields() == 0)
    {
        qDebug() << "auth failed !";
        return QBool(false);
    }

   // be login = user.getField("login");
    // std::cout << "user : " << login << std::endl;

    payload_builder.append("email", email.toStdString());
    payload_builder.append("user_id", l_user.getField("_id").OID());

    a_user = l_user;
    return QBool(true);
}





/********** REGISTER API ************/
void Http_api::node(QxtWebRequestEvent* event, QString action)
{
    qDebug() << "CREATE NODE : " << "action : " << action << " headers : " << event->headers;
    QString bodyMessage;

    //QString uuid = QUuid::createUuid().toString();
    //QString uuid = QUuid::createUuid().toString().mid(1,36).toUpper();

    QString nodename = event->headers.value("X-nodename");
    if (nodename.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-nodename");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }

    BSONObjBuilder payload_builder;
    payload_builder.genOID();
    payload_builder.append("nodename", nodename.toStdString());


    //std::cout << "payload builder : " << payload_builder.obj() << std::cout;

    bo user;
    QBool res = checkAuth(event->headers.value("Authorization"), payload_builder, user);

    if (!res)
    {
        bodyMessage = buildResponse("error", "auth");
        bodyMessage = buildResponse("error", "auth");
    }
    else
    {
        QUuid node_uuid = QUuid::createUuid();
        QString str_node_uuid = node_uuid.toString().mid(1,36);

        QUuid node_password = QUuid::createUuid();
        QString str_node_password = node_password.toString().mid(1,36);


        payload_builder.append("node_uuid", str_node_uuid.toStdString());
        payload_builder.append("node_password", str_node_password.toStdString());


        be user_id = user.getField("_id");

        //bo node_uuid = BSON("nodes" << BSON(GENOID << "uuid" << str_uuid.toStdString() <<  "nodename" << nodename.toStdString()));
        //nosql_.Addtoarray("users", user_id.wrap(), node_uuid);

        bo l_node = payload_builder.obj();

        bo node = BSON("nodes" << l_node);
        nosql_->Addtoarray("users", user_id.wrap(), node);

        nosql_->Insert("nodes", l_node);


        /****** PUSH API PAYLOAD *******
        std::cout << "NODE API PAYLOAD : " << payload << std::endl;
        z_message->rebuild(payload.objsize());
        memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
        z_push_api->send(*z_message, ZMQ_NOBLOCK);
        ************************/

        bodyMessage = buildResponse("register", str_node_uuid, str_node_password);
    }
    qDebug() << bodyMessage;


    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
}




/********** CREATE PAYLOAD ************/
void Http_api::payload(QxtWebRequestEvent* event, QString action)
{
    qDebug() << "CREATE PAYLOAD : " << "action : " << action << " headers : " << event->headers;
    QString bodyMessage;




    QString payloadfilename = event->headers.value("X-payloadfilename");
    QString node_uuid = event->headers.value("X-node-uuid");
    QString node_password = event->headers.value("X-node-password");
    QString task = event->headers.value("X-task");
    qDebug() << "FILENAME : " << payloadfilename << " length " << payloadfilename.length();
    qDebug() << "NODE UUID : " << node_uuid << " length " << node_uuid.length();
    qDebug() << "NODE PASSWORD : " << node_password << " length " << node_password.length();


    if (payloadfilename.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-payloadfilename");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }
    if (node_uuid.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-node-uuid");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }

    if (node_password.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-node-password");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }
    if (task.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-task");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    BSONObjBuilder payload_builder;
    payload_builder.append("action", "payload.create");
    payload_builder.append("node_uuid", node_uuid.toStdString());
    payload_builder.append("node_password", node_password.toStdString());
    payload_builder.append("task", task.toStdString());


    //std::cout << "payload builder : " << payload_builder.obj() << std::cout;
    bo user;
    QBool res = checkAuth(event->headers.value("Authorization"), payload_builder, user);

    if (!res)
    {
        bodyMessage = buildResponse("error", "auth");
    }
    else
    {
        //bo node_search = BSON("_id" << user.getField("_id") <<  "nodes.uuid" << node_uuid.toStdString());
        bo node_search = BSON("_id" << user.getField("_id"));

        bo user_nodes = nosql_->Find("users", node_search);


        if (user_nodes.nFields() == 0)
        {
            bodyMessage = buildResponse("error", "node", "unknown");
            postEvent(new QxtWebPageEvent(event->sessionID,
                                         event->requestID,
                                         bodyMessage.toUtf8()));
            return;
        }



        bo nodes = user_nodes.getField("nodes").Obj();

        list<be> list_nodes;
        nodes.elems(list_nodes);
        list<be>::iterator i;

        /********   Iterate over each user's nodes   *******/
        /********  find node with uuid and set the node id to payload collection *******/
        for(i = list_nodes.begin(); i != list_nodes.end(); ++i) {
            bo l_node = (*i).embeddedObject ();
            be node_id;
            l_node.getObjectID (node_id);

            //std::cout << "NODE1 => " << node_id.OID()   << std::endl;
            //std::cout << "NODE2 => " << node_uuid.toStdString() << std::endl;

            if (node_uuid.toStdString().compare(l_node.getField("uuid").str()) == 0)
            {
                payload_builder.append("node_id", node_id.OID());
                break;
            }
        }


        QUuid payload_uuid = QUuid::createUuid();
        QString str_payload_uuid = payload_uuid.toString().mid(1,36);
        payload_builder.append("uuid", str_payload_uuid.toStdString());

        int counter = nosql_->Count("payload_track");
        payload_builder.append("counter", counter + 1);

        QxtWebContent *myContent = event->content;

        qDebug() << "Bytes to read: " << myContent->unreadBytes();
        myContent->waitForAllContent();

        //QByteArray requestContent = QByteArray::fromBase64(myContent->readAll());
        QByteArray requestContent = myContent->readAll();

        /*QFile zfile("/tmp/in.dat");
        zfile.open(QIODevice::WriteOnly);
        zfile.write(requestContent);
        zfile.close();*/

        bo gfs_file_struct = nosql_->WriteFile(payloadfilename.toStdString(), requestContent.constData (), requestContent.size ());


        if (gfs_file_struct.nFields() == 0)
        {
            qDebug() << "write on gridFS failed !";
        }
        else
        {
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


            bo payload = payload_builder.obj();

            nosql_->Insert("payload_track", payload);

            qDebug() << "payload_track inserted";


            /****** PUSH API PAYLOAD *******/
            qDebug() << "PUSH API PAYLOAD";
            z_message->rebuild(payload.objsize());
            memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
            z_push_api->send(*z_message, ZMQ_NOBLOCK);
            /************************/
        }
        bodyMessage = buildResponse("create", str_payload_uuid);
    }
    qDebug() << bodyMessage;


    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
}


/********** UPDATE PAYLOAD ************/
void Http_api::payload(QxtWebRequestEvent* event, QString action, QString uuid)
{
    qDebug()  << "UPDATE HOST : " << "action : " << action << " uuid : " << uuid << " headers : " << event->headers;

/*
     QHash<QString, QString>::const_iterator i = event->headers.constBegin();

     while (i != event->headers.constEnd()) {
         qDebug() << i.key() << ": " << i.value();
         ++i;
     }
*/
    QString bodyMessage;

    QUuid session_uuid = QUuid::createUuid();
    QString str_session_uuid = session_uuid.toString().mid(1,36);


    BSONObjBuilder payload_builder;
    payload_builder.append("action", "payload.update");
    payload_builder.append("uuid", uuid.toStdString());


    QString payloadfilename = event->headers.value("X-payloadfilename");
    qDebug() << "FILENAME : " << payloadfilename;

    if (payloadfilename.length() == 0)
    {
        bodyMessage = buildResponse("error", "payloadfilename");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }  bo node = BSON("nodes" << payload_builder.obj());
    //nosql_.Addtoarray("users", user_id.wrap(), node);




    bo user;
    QBool res = checkAuth(event->headers.value("Authorization"), payload_builder, user);

    if (!res)
    {
        bodyMessage = buildResponse("error", "auth");
    }
    else
    {      
        QxtWebContent *myContent = event->content;

        qDebug() << "Bytes to read: " << myContent->unreadBytes();
        myContent->waitForAllContent();

        //QByteArray requestContent = QByteArray::fromBase64(myContent->readAll());
        QByteArray requestContent = myContent->readAll();
        //qDebug() << "Content: ";
        qDebug() << "RECEIVE PAYLOAD !!!!!  : " << requestContent.data();

        bo gfs_file_struct = nosql_->WriteFile(payloadfilename.toStdString(), requestContent.constData (), requestContent.size ());

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


            bo payload = payload_builder.obj();
            /****** PUSH API PAYLOAD *******/
            qDebug() << "PUSH HTTP PAYLOAD";
            z_message->rebuild(payload.objsize());
            memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
            z_push_api->send(*z_message, ZMQ_NOBLOCK);
            /************************/
        }
        bodyMessage = buildResponse("update", str_session_uuid);
    }
    qDebug() << bodyMessage;

    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
}


/**************  GET PAGE **********************/


/********** INDEX PAGE ************/
void Http_api::index(QxtWebRequestEvent* event)
{
/*    QString bodyMessage;
    bodyMessage = buildResponse("error", "ncs version 0.9.1");

    qDebug() << bodyMessage;

  */

    QxtHtmlTemplate index;
    QxtWebPageEvent *page;

        if(!index.open("html_templates/index.html"))
        {
            index["content"]="error 404";
            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());
            page->status=404;
            qDebug() << "error 404";
        }
        else
        {
            index["ncs_version"]="0.9.5";

            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());

            page->contentType="text/html";
        }

        postEvent(page);


        /* postEvent(new QxtWebPageEvent(event->sessionID,
                                      event->requestID,
                                      index.render().toUtf8()));

        */
}

/********** ADMIN PAGE ************/
void Http_api::admin(QxtWebRequestEvent* event, QString action)
{
/*    QString bodyMessage;
    bodyMessage = buildResponse("error", "ncs version 0.9.1");

    qDebug() << bodyMessage;

  */

    QxtHtmlTemplate index;
    QxtWebPageEvent *page;

        if(!index.open("html_templates/admin.html"))
        {
            index["content"]="error 404";
            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());
            page->status = 404;
            qDebug() << "error 404";
        }
        else
        {
            index["content"]="Ncs admin pages";


            QString html;
            html.append("<select>");

            for (int i=0; i<5; i++)
            {
                html.append("<option value=\"volvo\">Volvo</option>");
            }

            html.append("</select>");

            index["prot"]=html;

            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());

            page->contentType="text/html";
        }

        postEvent(page);


        /* postEvent(new QxtWebPageEvent(event->sessionID,
                                      event->requestID,
                                      index.render().toUtf8()));

        */
}









QString Http_api::buildResponse(QString action, QString data1, QString data2)
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
    else if (action == "create")
    {
        data.insert("uuid", data1);
    }
    else if (action == "error")
    {
        data.insert("error", data1);
        data.insert("code", data2);
    }
    body = QxtJSON::stringify(data);

    return body;
}
