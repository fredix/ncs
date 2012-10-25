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


#include "http_api.h"


Http_api::Http_api(QxtAbstractWebSessionManager * sm, QObject * parent): QxtWebSlotService(sm,parent)
{
    enumToHTTPmethod.insert(QString("GET"), GET);
    enumToHTTPmethod.insert(QString("POST"), POST);
    enumToHTTPmethod.insert(QString("PUT"), PUT);
    enumToHTTPmethod.insert(QString("DELETE"), DELETE);



    nosql_ = Nosql::getInstance_front();
    //nosql_ = Nosql::getInstance_back();
    zeromq_ = Zeromq::getInstance ();

    z_message = new zmq::message_t(2);
    //m_context = new zmq::context_t(1);
    z_push_api = new zmq::socket_t(*zeromq_->m_context, ZMQ_PUSH);

    uint64_t hwm = 50000;
    zmq_setsockopt (z_push_api, ZMQ_HWM, &hwm, sizeof (hwm));

    //z_push_api->bind("inproc://http");
    z_push_api->bind("ipc:///tmp/nodecast/http");
}


Http_api::~Http_api()
{
    qDebug() << "Http_api : close socket";
    z_push_api->close ();
    delete(z_push_api);
}


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
        //bodyMessage = buildResponse("error", "auth");
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








/********** CREATE WORKFLOW ************/
void Http_api::workflow(QxtWebRequestEvent* event, QString action)
{
    qDebug() << "CREATE WORKFLOW : " << "action : " << action << " headers : " << event->headers;
    QString bodyMessage;




    QString workflow = event->headers.value("X-workflow");
    qDebug() << "workflow : " << workflow << " length " << workflow.length();


    if (workflow.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-workflow");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    BSONObjBuilder workflow_builder;
    workflow_builder.genOID();
    workflow_builder.append("workflow", workflow.toStdString());


    //std::cout << "payload builder : " << payload_builder.obj() << std::cout;
    bo user;
    QBool res = checkAuth(event->headers.value("Authorization"), workflow_builder, user);

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

        QString content = requestContent;

        qDebug() << "requestCONTENT : " << content.toAscii();



        BSONObj b_workflow;
        try {
            b_workflow = mongo::fromjson(content.toAscii());

            std::cout << "b_workflow : " << b_workflow << std::endl;


            be user_id = user.getField("_id");


            QUuid workflow_uuid = QUuid::createUuid();
            QString str_workflow_uuid = workflow_uuid.toString().mid(1,36);
            workflow_builder.append("uuid", str_workflow_uuid.toStdString());
            workflow_builder.append("workers", b_workflow);
            workflow_builder.append("user_id", user_id.str());
            workflow_builder.append("payload_counter", 0);

            BSONObj l_workflow = workflow_builder.obj();

            BSONObj workflow = BSON("workflows" << l_workflow);

            nosql_->Addtoarray("users", user_id.wrap(), workflow);
            nosql_->Insert("workflows", l_workflow);


            bodyMessage = buildResponse("create", str_workflow_uuid);

        }
        catch (mongo::MsgAssertionException &e)
        {
            std::cout << "error on parsing JSON : " << e.what() << std::endl;
            bodyMessage = buildResponse("error", "error on parsing JSON");
        }


    }
    qDebug() << bodyMessage;


    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
}






/********** FILE PUSH ************/
void Http_api::file(QxtWebRequestEvent* event, QString action)
{
    QString bodyMessage;


    if (action == "push")
    {
        qDebug() << "RECEIVE FILE PUSH : " << "action : " << action << " headers : " << event->headers;

        QString filename = event->headers.value("X-filename");
        QString node_uuid = event->headers.value("X-node-uuid");
        QString node_password = event->headers.value("X-node-password");
        qDebug() << "FILENAME : " << filename << " length " << filename.length();
        qDebug() << "NODE UUID : " << node_uuid << " length " << node_uuid.length();
        qDebug() << "NODE PASSWORD : " << node_password << " length " << node_password.length();


        if (event->content.isNull())
        {
            bodyMessage = buildResponse("error", "data", "empty payload");
            postEvent(new QxtWebPageEvent(event->sessionID,
                                         event->requestID,
                                         bodyMessage.toUtf8()));
            return;
        }


        if (filename.length() == 0)
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



        BSONObj auth = BSON("node_uuid" << node_uuid.toStdString() << "node_password" << node_password.toStdString());
        BSONObj res = nosql_->Find("nodes", auth);
        if (res.nFields() == 0)
        {
            bodyMessage = buildResponse("error", "node unknown");
            postEvent(new QxtWebPageEvent(event->sessionID,
                                         event->requestID,
                                         bodyMessage.toUtf8()));
            return;
        }



        QxtWebContent *myContent = event->content;
        qDebug() << "Bytes to read: " << myContent->unreadBytes();
        myContent->waitForAllContent();

        //QByteArray requestContent = QByteArray::fromBase64(myContent->readAll());
        QByteArray requestContent = myContent->readAll();




    //bo node_search = BSON("_id" << user.getField("_id") <<  "nodes.uuid" << node_uuid.toStdString());
        BSONObj user_search = BSON("_id" << res.getField("user_id"));
        BSONObj user_nodes = nosql_->Find("users", user_search);


        if (user_nodes.nFields() == 0)
        {
            bodyMessage = buildResponse("error", "node", "unknown");
            postEvent(new QxtWebPageEvent(event->sessionID,
                                         event->requestID,
                                         bodyMessage.toUtf8()));
            return;
        }



        BSONObj nodes = user_nodes.getField("nodes").Obj();

        list<be> list_nodes;
        nodes.elems(list_nodes);
        list<be>::iterator i;

        /********   Iterate over each user's nodes   *******/
        /********  find node with uuid and set the node id to payload collection *******
        for(i = list_nodes.begin(); i != list_nodes.end(); ++i) {
            BSONObj l_node = (*i).embeddedObject ();
            be node_id;
            l_node.getObjectID (node_id);

            //std::cout << "NODE1 => " << node_id.OID()   << std::endl;
            //std::cout << "NODE2 => " << node_uuid.toStdString() << std::endl;

            if (node_uuid.toStdString().compare(l_node.getField("uuid").str()) == 0)
            {
                payload_builder.append("node_id", node_id.OID());
                break;
            }
        }*/




        /*QFile zfile("/tmp/in.dat");
        zfile.open(QIODevice::WriteOnly);
        zfile.write(requestContent);
        zfile.close();*/

        BSONObj gfs_file_struct = nosql_->WriteFile(filename.toStdString(), requestContent.constData (), requestContent.size ());


        if (gfs_file_struct.nFields() == 0)
        {
            qDebug() << "write on gridFS failed !";
            bodyMessage = buildResponse("error", "write on gridFS failed !");
        }
        else
        {
            std::cout << "writefile : " << gfs_file_struct << std::endl;
            //std::cout << "writefile id : " << gfs_file_struct.getField("_id").__oid() << " date : " << gfs_file_struct.getField("uploadDate") << std::endl;

//            BSONElement gfs_id;
            //gfs_file_struct.getObjectID (gfs_id);

            //BSONObjBuilder gfs_builder;
            //gfs_builder.append("_id" , gfs_file_struct.getField("_id").Obj().getField("_id").OID());

            string gfs_id =  gfs_file_struct.getField("_id").__oid().str();

            //gfs_file_struct.getField("_id").Obj().getObjectID(gfs_id);

            std::cout << "gfs_id : " << gfs_id << std::endl;


            //BSONElement gfs_id = gfs_file_struct.getField("_id").str();
            //bodyMessage = buildResponse("file", QString::fromStdString(gfs_id.str()));

            //BSONElement gfs_id = gfs_file_struct.getField("_id").str();
            bodyMessage = buildResponse("file", QString::fromStdString(gfs_id));
        }


        qDebug() << bodyMessage;
    }


    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
}



void Http_api::payload_post(QxtWebRequestEvent* event, QString action, QString workflow_uuid)
{

    QDateTime timestamp = QDateTime::currentDateTime();
    QString bodyMessage;



    QString payload_filename = event->headers.value("X-payload-filename");
    QString payload_type = event->headers.value("X-payload-type");
    QString node_uuid = event->headers.value("X-node-uuid");
    QString node_password = event->headers.value("X-node-password");

    qDebug() << "FILENAME : " << payload_filename << " length " << payload_filename.length();
    qDebug() << "NODE UUID : " << node_uuid << " length " << node_uuid.length();
    qDebug() << "NODE PASSWORD : " << node_password << " length " << node_password.length();
    qDebug() << "workflow_uuid : " << workflow_uuid << " length " << workflow_uuid.length();
    qDebug() << "payload_type : " << payload_type << " length " << payload_type.length();



    if (event->content.isNull())
    {
        bodyMessage = buildResponse("error", "data", "empty payload");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    if (payload_filename.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-payload-filename");
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
    if (workflow_uuid.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-workflow-uuid");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }
    if (payload_type.length() == 0)
    {
        bodyMessage = buildResponse("error", "headers", "X-payload-type");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }






    BSONObj workflow_search = BSON("uuid" <<  workflow_uuid.toStdString());
    BSONObj workflow = nosql_->Find("workflows", workflow_search);
    if (workflow.nFields() == 0)
    {
        bodyMessage = buildResponse("error", "workflow unknown");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    BSONObj auth = BSON("node_uuid" << node_uuid.toStdString() << "node_password" << node_password.toStdString());
    BSONObj node = nosql_->Find("nodes", auth);
    if (node.nFields() == 0)
    {
        bodyMessage = buildResponse("error", "node unknown");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    BSONObj user_search = BSON("_id" << node.getField("user_id"));
    BSONObj user_nodes = nosql_->Find("users", user_search);


    if (user_nodes.nFields() == 0)
    {
        bodyMessage = buildResponse("error", "node", "unknown");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }





    // return session uuid to the user's request
    QUuid tmp_session_uuid = QUuid::createUuid();
    QString session_uuid = tmp_session_uuid.toString().mid(1,36);

    bodyMessage = buildResponse(action, session_uuid);

    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));

    qDebug() << bodyMessage;






    QxtWebContent *myContent = event->content;
    qDebug() << "Bytes to read: " << myContent->unreadBytes();
    myContent->waitForAllContent();

    //QByteArray requestContent = QByteArray::fromBase64(myContent->readAll());
    QByteArray requestContent = myContent->readAll();





    BSONObjBuilder payload_builder;
    payload_builder.genOID();
    payload_builder.append("action", action.toStdString());
    payload_builder.append("timestamp", timestamp.toTime_t());
    payload_builder.append("node_uuid", node_uuid.toStdString());
    payload_builder.append("node_password", node_password.toStdString());
    payload_builder.append("workflow_uuid", workflow_uuid.toStdString());
    payload_builder.append("payload_type", payload_type.toStdString());


    //std::cout << "payload builder : " << payload_builder.obj() << std::cout;
    //bo user;
    //QBool res = checkAuth(event->headers.value("Authorization"), payload_builder, user);



//bo node_search = BSON("_id" << user.getField("_id") <<  "nodes.uuid" << node_uuid.toStdString());



    BSONObj nodes = user_nodes.getField("nodes").Obj();

    list<be> list_nodes;
    nodes.elems(list_nodes);
    list<be>::iterator i;

    /********   Iterate over each user's nodes   *******/
    /********  find node with uuid and set the node id to payload collection *******/
    for(i = list_nodes.begin(); i != list_nodes.end(); ++i) {
        BSONObj l_node = (*i).embeddedObject ();
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
    payload_builder.append("payload_uuid", str_payload_uuid.toStdString());

    int counter = nosql_->Count("payloads");
    payload_builder.append("counter", counter + 1);


    /*QFile zfile("/tmp/in.dat");
    zfile.open(QIODevice::WriteOnly);
    zfile.write(requestContent);
    zfile.close();*/

    BSONObj gfs_file_struct = nosql_->WriteFile(payload_filename.toStdString(), requestContent.constData (), requestContent.size ());



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
        payload_builder.append("gridfs", true);
        BSONObj payload = payload_builder.obj();

        nosql_->Insert("payloads", payload);

        std::cout << "payload inserted" << payload << std::endl;

        /********* CREATE SESSION **********/

        BSONObjBuilder session_builder;
        session_builder.genOID();
        session_builder.append("uuid", session_uuid.toStdString());
        session_builder.append("payload_id", payload.getField("_id").OID());
        session_builder.append("workflow_id", workflow.getField("_id").OID());
        session_builder.append("start_timestamp", timestamp.toTime_t());
        BSONObj session = session_builder.obj();
        nosql_->Insert("sessions", session);
        /***********************************/
        std::cout << "session inserted : " << session << std::endl;

        BSONObj l_payload = BSON("action" << action.toStdString() <<
                                 "payload_type" << payload_type.toStdString()<<
                                 //"publish" << (!publish.isEmpty() && publish == "true")? "true" : "false" <<
                                 "session_uuid" << session_uuid.toStdString() <<
                                 "timestamp" << timestamp.toTime_t());

        /****** PUSH API PAYLOAD *******/
        qDebug() << "PUSH HTTP API PAYLOAD";
        z_message->rebuild(l_payload.objsize());
        memcpy(z_message->data(), (char*)l_payload.objdata(), l_payload.objsize());
        //z_push_api->send(*z_message, ZMQ_NOBLOCK);
        z_push_api->send(*z_message);
        /************************/
    }

}




/**************************************************
PAYLOAD ACTION :
    METHOD POST => push/publish payload, workflow uuid
    METHOD PUT => update payload, payload uuid
    DELETE => delete payload, payload uuid
****************************************************/
void Http_api::payload(QxtWebRequestEvent* event, QString action, QString uuid)
{

    qDebug() << "RECEIVE CREATE PAYLOAD : " << "METHOD : " << event->method << " action : " << action << " headers : " << event->headers;

    QString bodyMessage="";


    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        qDebug() << "HTTP GET not implemented";
        bodyMessage = buildResponse("error", "HTTP GET not implemented");

        break;
    case POST:
        qDebug() << "HTTP POST";
        payload_post(event, action.toLower(), uuid);

        break;
    case PUT:
        qDebug() << "HTTP PUT not implemented";
        bodyMessage = buildResponse("error", "HTTP PUT not implemented");

        break;
    case DELETE:
        qDebug() << "HTTP DELETE not implemented";
        bodyMessage = buildResponse("error", "HTTP DELETE not implemented");

        break;
    }


    if (!bodyMessage.isEmpty())
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
            index["ncs_version"]="0.9.6";

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
    body = QxtJSON::stringify(data);

    return body;
}
