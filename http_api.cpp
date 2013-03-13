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


#include "http_api.h"


Http_api::Http_api(QxtAbstractWebSessionManager * sm, QObject * parent): QxtWebSlotService(sm,parent)
{
    enumToHTTPmethod.insert(QString("GET"), GET);
    enumToHTTPmethod.insert(QString("POST"), POST);
    enumToHTTPmethod.insert(QString("PUT"), PUT);
    enumToHTTPmethod.insert(QString("DELETE"), DELETE);

/*

    //QxtWebServiceDirectory* top = new QxtWebServiceDirectory(sm, sm);
    QxtWebServiceDirectory* service_admin = new QxtWebServiceDirectory(sm, this);
  //  QxtWebServiceDirectory* service2 = new QxtWebServiceDirectory(sm, this);
    QxtWebServiceDirectory* service_admin_login = new QxtWebServiceDirectory(sm, service_admin);
 //   QxtWebServiceDirectory* service1b = new QxtWebServiceDirectory(sm, service1);
    this->addService("admin", service_admin);
//    this->addService("2", service2);
    service_admin->addService("login", service_admin_login);
 //   service1->addService("b", service1b);

*/

    mongodb_ = Mongodb::getInstance ();
    zeromq_ = Zeromq::getInstance ();

    z_message = new zmq::message_t(2);
    //m_context = new zmq::context_t(1);
    z_push_api = new zmq::socket_t(*zeromq_->m_context, ZMQ_PUSH);

    int hwm = 50000;
    z_push_api->setsockopt (ZMQ_SNDHWM, &hwm, sizeof (hwm));
    z_push_api->setsockopt (ZMQ_RCVHWM, &hwm, sizeof (hwm));

    //z_push_api->bind("inproc://http");
    z_push_api->bind("ipc:///tmp/nodecast/http");
}


Http_api::~Http_api()
{
    qDebug() << "Http_api : close socket";
    z_push_api->close ();
    delete(z_push_api);
}

bool Http_api::check_user_login(QxtWebRequestEvent *event, QString &alert)
{
    QxtWebRedirectEvent *redir;
    QString user_uuid = event->cookies.value("nodecast");

    if (user_bson.contains(user_uuid))
    {
        //user = user_session[user_uuid];

        if (user_alert.contains(user_uuid))
        {
            alert = user_alert[user_uuid];
            user_alert.remove(user_uuid);
        }

       // qDebug() << "USER FOUND : " << user;
        return true;
    }
    else
    {
        qDebug() << "USER NOT CONNECTED, REDIR !";
        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "login", 302 );
        postEvent(redir);
        return false;
    }

}


void Http_api::set_user_alert(QxtWebRequestEvent *event, QString alert)
{
    QString user_uuid = event->cookies.value("nodecast");
    user_alert[user_uuid] = alert;
}



QBool Http_api::http_auth(QString auth, QHash <QString, QString> &hauth, QString &str_session_uuid)
{

    QStringList list_field = auth.split("&");
   // QHash <QString, QString> hash_fields;

    for (int i = 0; i < list_field.size(); ++i)
    {
        QStringList field = list_field.at(i).split("=");
        hauth[field[0]] = field[1].replace(QString("%40"), QString("@"));
    }

    qDebug() << "password " << hauth["password"] << " email " << hauth["email"];



    QCryptographicHash cipher( QCryptographicHash::Sha1 );
    cipher.addData(hauth["password"].simplified().toAscii());
    QByteArray password_hash = cipher.result();

    qDebug() << "password_hash : " << password_hash.toHex();

    BSONObj bauth = BSON("email" << hauth["email"].toStdString() << "password" << QString::fromLatin1(password_hash.toHex()).toStdString());


    BSONObj l_user = mongodb_->Find("users", bauth);

    if (l_user.nFields() == 0)
    {
        qDebug() << "auth failed !";
        return QBool(false);
    }

    // create session id
    QUuid session_uuid = QUuid::createUuid();
    str_session_uuid = session_uuid.toString().mid(1,36);

    user_bson[str_session_uuid] = l_user.copy();

    //hauth["admin"] = QString::fromStdString(l_user.getField("admin").toString(false));
    //qDebug() << "IS ADMIN ? : " << hauth["admin"];

   // be login = user.getField("login");
    // std::cout << "user : " << login << std::endl;

  //  payload_builder.append("email", email.toStdString());
//    payload_builder.append("user_id", l_user.getField("_id").OID());

  //  a_user = l_user;

    //hauth = bauth;

    return QBool(true);
}


QBool Http_api::checkAuth(QString token, BSONObjBuilder &payload_builder, bo &a_user)
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





/********** REGISTER API ************/
void Http_api::node(QxtWebRequestEvent* event, QString token)
{
    QxtWebPageEvent *page;
    QxtHtmlTemplate body;

    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case POST:
    {
        // POST create a new node
        // Token is the user's token
        qDebug() << "CREATE NODE : " << "token : " << token << " headers : " << event->headers;
        QString bodyMessage;

        //QString uuid = QUuid::createUuid().toString();
        //QString uuid = QUuid::createUuid().toString().mid(1,36).toUpper();


        if (event->content.isNull())
        {
            bodyMessage = buildResponse("error", "data", "empty payload");
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

        QString content = requestContent;

        qDebug() << "requestCONTENT : " << content.toAscii();

        BSONObj b_payload;
        try {
            b_payload = mongo::fromjson(content.toAscii());
            std::cout << "b_payload : " << b_payload << std::endl;

            if (!b_payload.hasField("nodename")) throw QString("nodename");
            if (b_payload.getField("nodename").size() == 0) throw QString("nodename");
        }
        catch (mongo::MsgAssertionException &e)
            {
                std::cout << "error on parsing JSON : " << e.what() << std::endl;
                bodyMessage = buildResponse("error", "error on parsing JSON");
                postEvent(new QxtWebPageEvent(event->sessionID,
                                             event->requestID,
                                             bodyMessage.toUtf8()));
                return;
            }
        catch (QString error)
            {
                //std::cout << "JSON field : " << a_error << std::endl;
                bodyMessage = buildResponse("error", "error on field " + error);
                postEvent(new QxtWebPageEvent(event->sessionID,
                                             event->requestID,
                                             bodyMessage.toUtf8()));
                return;
            }





        BSONObjBuilder payload_builder;
        payload_builder.genOID();       
        payload_builder << b_payload.getField("nodename");


        //std::cout << "payload builder : " << payload_builder.obj() << std::cout;

        BSONObj user;
        QBool res = checkAuth(token, payload_builder, user);

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
            //mongodb_.Addtoarray("users", user_id.wrap(), node_uuid);

            bo l_node = payload_builder.obj();

            bo node = BSON("nodes" << l_node);
            mongodb_->Addtoarray("users", user_id.wrap(), node);

            mongodb_->Insert("nodes", l_node);


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
        break;
    }
    case GET:
        body["content"]="error 404";
        page = new QxtWebPageEvent(event->sessionID,
                                   event->requestID,
                                   body.render().toUtf8());
        page->status = 404;
        qDebug() << "error 404";
        postEvent(page);
        break;
    }
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

            mongodb_->Addtoarray("users", user_id.wrap(), workflow);
            mongodb_->Insert("workflows", l_workflow);


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
        BSONObj res = mongodb_->Find("nodes", auth);
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
        BSONObj user_nodes = mongodb_->Find("users", user_search);


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

        BSONObj gfs_file_struct = mongodb_->WriteFile(filename.toStdString(), requestContent.constData (), requestContent.size ());


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



void Http_api::payload_post(QxtWebRequestEvent* event, QString action)
{

    QDateTime timestamp = QDateTime::currentDateTime();
    QString bodyMessage;


    // set filename if we want to store data to gridfs (data > 1Mo).
    QString payload_filename = event->headers.value("X-payload-filename");
    QString payload_type = event->headers.value("X-payload-type");
    QString node_uuid = event->headers.value("X-node-uuid");
    QString node_password = event->headers.value("X-node-password");
    QString workflow_uuid = event->headers.value("X-workflow-uuid");


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
    BSONObj workflow = mongodb_->Find("workflows", workflow_search);
    if (workflow.nFields() == 0)
    {
        bodyMessage = buildResponse("error", "workflow unknown");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    BSONObj auth = BSON("node_uuid" << node_uuid.toStdString() << "node_password" << node_password.toStdString());
    BSONObj node = mongodb_->Find("nodes", auth);
    if (node.nFields() == 0)
    {
        //std::cout << "node uuid " << node_uuid.toStdString() << " node pass " < node_password.toStdString() << std::endl;
        bodyMessage = buildResponse("error", "node unknown");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    BSONObj user_search = BSON("_id" << node.getField("user_id"));
    BSONObj user_nodes = mongodb_->Find("users", user_search);


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

    int counter = mongodb_->Count("payloads");
    payload_builder.append("counter", counter + 1);


    /*QFile zfile("/tmp/in.dat");
    zfile.open(QIODevice::WriteOnly);
    zfile.write(requestContent);
    zfile.close();*/

    if (payload_filename.length() != 0)
    {

        BSONObj gfs_file_struct = mongodb_->WriteFile(payload_filename.toStdString(), requestContent.constData (), requestContent.size ());



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
        }

    }
    // SEND a JSON payload
    else
    {
        payload_builder.append("gridfs", false);
        payload_builder.append("data", requestContent.constData ());
    }

    BSONObj payload = payload_builder.obj();

    mongodb_->Insert("payloads", payload);

    std::cout << "payload inserted" << payload << std::endl;

    /********* CREATE SESSION **********/

    BSONObjBuilder session_builder;
    session_builder.genOID();
    session_builder.append("uuid", session_uuid.toStdString());
    session_builder.append("payload_id", payload.getField("_id").OID());
    session_builder.append("workflow_id", workflow.getField("_id").OID());
    session_builder.append("start_timestamp", timestamp.toTime_t());
    BSONObj session = session_builder.obj();
    mongodb_->Insert("sessions", session);
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




void Http_api::session_get(QxtWebRequestEvent* event, QString uuid)
{

    QDateTime timestamp = QDateTime::currentDateTime();
    QString bodyMessage;


    qDebug() << "SESSION UUID : " << uuid;




    BSONObj session_search = BSON("uuid" <<  uuid.toStdString());
    BSONObj session = mongodb_->Find("sessions", session_search);
    if (session.nFields() == 0)
    {
        bodyMessage = buildResponse("status", "session unknown");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }

    if (!session.hasField("counter"))
    {
        bodyMessage = buildResponse("status", "traitement queued");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    BSONObj workflow_search = BSON("_id" <<  session.getField("workflow_id"));
    BSONObj workflow = mongodb_->Find("workflows", workflow_search);
    if (workflow.nFields() == 0)
    {
        bodyMessage = buildResponse("status", "workflow unknown");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }

    BSONObj workers = mongo::fromjson(workflow.getField("workers").toString(false));


    for( BSONObj::iterator i = workers.begin(); i.more();)
    {
        BSONElement worker = i.next();
        qDebug() << "WORKER : " <<  QString::fromStdString(worker.toString());


    }



    QString last_worker = QString::fromStdString(session.getField("last_worker").valuestr());

    bodyMessage = buildResponse("status", last_worker);
    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
    return;

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
        qDebug() << "HTTP POST : " << uuid;
        payload_post(event, action.toLower());

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



/**************************************************
PAYLOAD ACTION :
    METHOD POST => push/publish payload, workflow uuid
    METHOD PUT => update payload, payload uuid
    DELETE => delete payload, payload uuid
****************************************************/
void Http_api::session(QxtWebRequestEvent* event, QString uuid)
{

    qDebug() << "RECEIVE SESSION : " << "METHOD : " << event->method << " headers : " << event->headers << " UUID " << uuid;

    QString bodyMessage="";


    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        qDebug() << "HTTP GET : " << uuid;
        session_get(event, uuid.toLower());

        break;
    case POST:
        qDebug() << "HTTP POST not implemented";
        bodyMessage = buildResponse("error", "HTTP POST not implemented");

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
void Http_api::admin_logout(QxtWebRequestEvent* event)
{
    QxtWebRedirectEvent *redir;

    user_bson.remove(event->cookies.value("nodecast"));

    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "/", 302 );
    postEvent(redir);
}

/********** INDEX PAGE ************/
void Http_api::index(QxtWebRequestEvent* event)
{
/*    QString bodyMessage;
    bodyMessage = buildResponse("error", "ncs version 0.9.1");

    qDebug() << bodyMessage;

  */

    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;

    QxtWebPageEvent *page;

        if(!body.open("html_templates/index.html"))
        {
            body["content"]="error 404";
            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       body.render().toUtf8());
            page->status=404;
            qDebug() << "error 404";
        }
        else
        {
            header.open("html_templates/header.html");
            footer.open("html_templates/footer.html");

            if (!event->cookies.value("nodecast").isEmpty() && user_bson.contains(event->cookies.value("nodecast")))
            {
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";

            }
            else header["connection"] = "<li><a href=\"/admin/login\">Login</a></li>";


            body["ncs_version"]="0.9.7";

            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       header.render().toUtf8() +
                                       body.render().toUtf8() +
                                       footer.render().toUtf8());

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

        qDebug() << "ADMIN : " << "METHOD : " << event->method << " headers : " << event->headers << " ACTION " << action;

        QString bodyMessage="";
        QBool res = QBool(false);
        BSONObjBuilder payload_builder;
        payload_builder.genOID();

        bo user;


        switch (enumToHTTPmethod[event->method.toUpper()])
        {
        case GET:
            qDebug() << "HTTP GET : " << action;
            qDebug() << "HEADERS : " << event->headers.value("Authorization");

/*
            res = checkAuth(event->headers.value("Authorization"), payload_builder, user);
            if (!res)
            {
                bodyMessage = buildResponse("error", "auth");
                //bodyMessage = buildResponse("error", "auth");
            }
            else
            {
                if (action == "users")
                    admin_users_get(event);
            }
            */


            if (action == "login")
            {
                admin_login(event);
            }
            else if (action == "logout")
            {
                admin_logout(event);
            }
            else if (action == "users")
            {

                admin_users_get(event);
            }
            else if (action == "createuser")
            {

                admin_user_post(event);
            }
            else if (action == "nodes")
            {

                admin_nodes_get(event);
            }
            else if (action == "createnode")
            {

                admin_node_post(event);
            }
            else if (action == "workflows")
            {

                admin_workflows_get(event);
            }
            else if (action == "createworkflow")
            {

                admin_workflow_post(event);
            }
            else if (action == "workers")
            {

                admin_workers_get(event);
            }
            else if (action == "sessions")
            {

                admin_sessions_get(event);
            }

            else if (action == "payloads")
            {

                admin_payloads_get(event);
            }
            else if (action == "lost_pushpull_payloads")
            {
                admin_lost_pushpull_payloads_get(event);
            }

            else
            {
                QxtHtmlTemplate index;
                index["content"]="error 404";
                QxtWebPageEvent *page = new QxtWebPageEvent(event->sessionID,
                                                            event->requestID,
                                                            index.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";

                postEvent(page);
                return;
            }

            break;
        case POST:
            //qDebug() << "HTTP POST not implemented";
            //bodyMessage = buildResponse("error", "HTTP POST not implemented");

            if (action == "login")
            {
                admin_login(event);
                return;
            }
            else if (action == "createuser")
            {
                admin_user_post(event);
                return;
            }
            else if (action == "createnode")
            {
                admin_node_post(event);
                return;
            }
            else if (action == "deletenode")
            {
                admin_node_or_workflow_delete(event, "nodes");
                return;
            }
            else if (action == "deleteworkflow")
            {
                admin_node_or_workflow_delete(event, "workflows");
                return;
            }
            else if (action == "createworkflow")
            {

                admin_workflow_post(event);
            }

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
        {
            postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        }
     /*   else
        {
            QxtHtmlTemplate index;
            index["content"]="error 404";
            QxtWebPageEvent *page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());
            page->status = 404;
            qDebug() << "error 404";

            postEvent(page);
        }
        */

}



/********** ADMIN PAGE ************/
void Http_api::admin_login(QxtWebRequestEvent* event)
{

    QxtHtmlTemplate index;
    QxtHtmlTemplate header;
    QxtHtmlTemplate footer;

    QxtWebPageEvent *page;
    QxtWebContent *form;

    QxtWebStoreCookieEvent *cookie_session;
    QxtWebRedirectEvent *redir=NULL;


    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        if(!index.open("html_templates/login.html"))
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
            header.open("html_templates/header.html");
            footer.open("html_templates/footer.html");
            header["connection"] = "<li><a href=\"/admin/login\">Login</a></li>";


            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       header.render().toUtf8() +
                                       index.render().toUtf8() +
                                       footer.render().toUtf8());

            page->contentType="text/html";
        }

        postEvent(page);


        /* postEvent(new QxtWebPageEvent(event->sessionID,
                                      event->requestID,
                                      index.render().toUtf8()));

        */
        break;
    case POST:
        if(!index.open("html_templates/login.html"))
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
            form = event->content;
            form->waitForAllContent();

            QByteArray requestContent = form->readAll();
            //RECEIVE :  "firstname=fred&lastname=ix&email=fredix%40gmail.com"

            qDebug() << "RECEIVE : " << requestContent;

            QHash <QString, QString> hauth;
            QString str_session_uuid;

            if (!http_auth(QString::fromUtf8(requestContent), hauth, str_session_uuid))
            {
                qDebug() << "AUTH FAILED !";
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "login", 302 );
            }
            else
            {
                //QxtWebStoreCookieEvent cookie_mail (event->sessionID, "email", hauth["email"],  QDateTime::currentDateTime().addMonths(1));
                //QxtWebStoreCookieEvent cookie_pass (event->sessionID, "password", hauth["password"],  QDateTime::currentDateTime().addMonths(1));

              //  QUuid session_uuid = QUuid::createUuid();
              //  QString str_session_uuid = session_uuid.toString().mid(1,36);

                //user_session[str_session_uuid] = hauth["email"];
                //user_admin[str_session_uuid] = (hauth["admin"] == "true")? true : false;

                user_alert[str_session_uuid] = errorMessage("Logged in successfully", "notice");

                cookie_session = new QxtWebStoreCookieEvent (event->sessionID, "nodecast", str_session_uuid, QDateTime::currentDateTime().addMonths(1));
                qDebug() << "SESSION ID : " << str_session_uuid;

                /*
                index["content"]=" welcome " + hauth["email"];


                QString html;
                html.append("<select>");

                for (int i=0; i<5; i++)
                {
                    html.append("<option value=\"ok\">Ok</option>");
                }

                html.append("</select>");

                index["prot"]=html;

                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           index.render().toUtf8());


                page->contentType="text/html";
                */

                postEvent(cookie_session);

                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "users", 302 );


                //QxtWebEvent cookie (QxtWebEvent::StoreCookie, event->sessionID);

                //postEvent()

            }

        }

        //postEvent(page);
        if (redir) postEvent(redir);

        break;
    }

}



/********** ADMIN PAGE ************/
void Http_api::admin_users_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;
    QxtWebRedirectEvent *redir=NULL;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open("html_templates/admin_users_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;


                // redirect to createuser if not users
                if (mongodb_->Count("users") == 0)
                {
                    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createuser", 302 );
                    postEvent(redir);
                    return;
                }

                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;

                // only an admin user can go to the users list
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    set_user_alert(event, errorMessage("you are not an admin user", "error"));
                    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "nodes", 302 );
                    postEvent(redir);
                    return;
                }


                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj empty;
                QList <BSONObj> users = mongodb_->FindAll("users", empty);

                QString output;
                int counter=0;
                foreach (BSONObj user, users)
                {
                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id\" value=\"1\"></td><td>1</td><td>" +
                            QString::fromStdString(user.getField("login").str()) + "</td>" +
                            "<td>" + QString::fromStdString(user.getField("email").str()) + "</td>" +
                            "<td>" + QString::fromStdString(user.getField("token").str()) + "</td>" +
                            "<td>" + QString::fromStdString(user.getFieldDotted("nodes.payload_counter").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);

                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}


/********** ADMIN PAGE ************/
void Http_api::admin_user_post(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;
    QxtWebContent *form;
    QxtWebRedirectEvent *redir=NULL;


//            if(!body.open("html_templates/admin_users_get.html"))



    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        if(!body.open("html_templates/admin_user_post.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");




                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;

                if (mongodb_->Count("users") != 0)
                {
                    if (!check_user_login(event, l_alert)) return;
                }
                else header["alert"] = errorMessage("please create an admin user", "notice");

                if (!l_alert.isEmpty()) header["alert"] = l_alert;


                // only an admin user can create new users
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    set_user_alert(event, errorMessage("you are not an admin user", "error"));
                    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "nodes", 302 );
                    postEvent(redir);
                    return;
                }


                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);

        break;
    case POST:

        qDebug() << "COOKIES : " << event->cookies.value("nodecast");
        QString l_alert;

        bool create_admin = (mongodb_->Count("users") != 0)? false : true;
        if (!create_admin && !check_user_login(event, l_alert)) return;

        form = event->content;
        form->waitForAllContent();

        QByteArray requestContent = form->readAll();
        //RECEIVE :  "firstname=fred&lastname=ix&email=fredix%40gmail.com"

        qDebug() << "RECEIVE : " << requestContent;

        //QxtWebStoreCookieEvent cookie_mail (event->sessionID, "email", hauth["email"],  QDateTime::currentDateTime().addMonths(1));
        //QxtWebStoreCookieEvent cookie_pass (event->sessionID, "password", hauth["password"],  QDateTime::currentDateTime().addMonths(1));


        QHash <QString, QString> form_field;
        QStringList list_field = QString::fromAscii(requestContent).split("&");

        for (int i = 0; i < list_field.size(); ++i)
        {
            QStringList field = list_field.at(i).split("=");

            if (field[1].isEmpty())
            {
                set_user_alert(event, errorMessage("field " + field[0] + " is empty", "error"));
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createuser", 302 );
                postEvent(redir);
                return;
            }
            form_field[field[0]] = field[1].replace(QString("%40"), QString("@"));
        }





        QUuid token = QUuid::createUuid();
        QString str_token = token.toString().mid(1,36);

        QUuid tracker_token = QUuid::createUuid();
        QString str_tracker_token = tracker_token.toString().remove(QChar('-')).mid(1,32);

        QCryptographicHash cipher( QCryptographicHash::Sha1 );
        cipher.addData(form_field["password"].simplified().toAscii());
        QByteArray password_hash = cipher.result();

        qDebug() << "password_hash : " << password_hash.toHex();

        BSONObj t_user = BSON(GENOID << "admin" << create_admin << "login" << form_field["login"].toStdString() << "password" << QString::fromLatin1(password_hash.toHex()).toStdString() << "email" << form_field["email"].toStdString() << "token" << str_token.toStdString() << "tracker" << BSON ("token" << str_tracker_token.toStdString()));
        mongodb_->Insert("users", t_user);

        //doc = { login : 'user', email : 'user@email.com', authentication_token : 'token'}
        //db.users.insert(doc);





        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "users", 302 );
        postEvent(redir);

        break;
    }

}


/********** ADMIN PAGE ************/
void Http_api::admin_nodes_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;


{ "_id" : ObjectId("50f91cbda159811513a8b729"), "nodename" : "samsung@dev", "email" : "fredix@gmail.com", "user_id" : ObjectId("50f91c85a7a5dbe8e2f35b29"), "node_uuid" : "0d7f9bdc-37a2-4290-be41-62598bd7a525", "node_password" : "6786a141-6dff-4f91-891a-a9107915ad76" }


      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open("html_templates/admin_nodes_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }

                QList <BSONObj> nodes = mongodb_->FindAll("nodes", filter);

                QString output;
                int counter=0;
                foreach (BSONObj node, nodes)
                {

                    BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id_" + QString::number(counter) + "\" value=\"" + QString::fromStdString(node.getField("_id").OID().str()) + "\"></td><td>" +
                            QString::fromStdString(node.getField("nodename").str()) + "</td>" +
                            "<td>" + QString::fromStdString(node.getField("email").str()) + "</td>" +
                            "<td>" + QString::fromStdString(node.getField("node_uuid").str()) + "</td>" +
                            "<td>" + QString::fromStdString(node.getField("node_password").str()) + "</td>" +
                            "<td>" + QString::fromStdString(user.getField("email").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);


                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}


/********** ADMIN PAGE ************/
void Http_api::admin_node_or_workflow_delete(QxtWebRequestEvent* event, QString collection)
{
    QxtWebContent *form;
    QxtWebRedirectEvent *redir=NULL;


    QString l_alert;
    if (!check_user_login(event, l_alert)) return;

    form = event->content;
    form->waitForAllContent();

    QByteArray requestContent = form->readAll();

    qDebug() << "RECEIVE : " << requestContent;
    // RECEIVE :  "id=513f4f462845b4fa97461b10&id=513f57d8380e6714db9146fa"

    QHash <QString, QString> form_field;
    QStringList list_field = QString::fromAscii(requestContent).split("&");

    for (int i = 0; i < list_field.size(); ++i)
    {
        QStringList field = list_field.at(i).split("=");

        if (field[1].isEmpty())
        {
            set_user_alert(event, errorMessage("field " + field[0] + " is empty", "error"));
            redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, collection, 302 );
            postEvent(redir);
            return;
        }

        form_field[field[0]] = field[1];


        BSONObj node_id = BSON("_id" << mongo::OID(field[1].toStdString()));
        mongodb_->Remove(collection, node_id);
    }

    qDebug() << "form field : " << form_field;

    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, collection, 302 );
    postEvent(redir);


}

/********** ADMIN PAGE ************/
void Http_api::admin_node_post(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;
    QxtWebContent *form;
    QxtWebRedirectEvent *redir=NULL;


//            if(!body.open("html_templates/admin_users_get.html"))



    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        if(!body.open("html_templates/admin_node_post.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");

                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;

                if (user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    QString output="<label class=\"label\">Choose a owner</label>";
                    BSONObj empty;
                    QList <BSONObj> users = mongodb_->FindAll("users", empty);

                    output.append("<select name=\"user\">");

                    foreach (BSONObj user, users)
                    {
                        output.append("<option value=\"" + QString::fromStdString(user.getField("_id").OID().str()) + "\">" +  QString::fromStdString(user.getField("login").str()) + "</option>");
                    }
                    output.append("</select>");


                    body["users"] = output;
                }
                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);

        break;
    case POST:

        qDebug() << "COOKIES : " << event->cookies.value("nodecast");
        QString l_alert;
        if (!check_user_login(event, l_alert)) return;

        form = event->content;
        form->waitForAllContent();

        QByteArray requestContent = form->readAll();
        //RECEIVE :  "firstname=fred&lastname=ix&email=fredix%40gmail.com"

        qDebug() << "RECEIVE : " << requestContent;

        //QxtWebStoreCookieEvent cookie_mail (event->sessionID, "email", hauth["email"],  QDateTime::currentDateTime().addMonths(1));
        //QxtWebStoreCookieEvent cookie_pass (event->sessionID, "password", hauth["password"],  QDateTime::currentDateTime().addMonths(1));


        //RECEIVE :  "login=fredix&email=fredix%40gmail.com"

        QHash <QString, QString> form_field;
        QStringList list_field = QString::fromAscii(requestContent).split("&");

        for (int i = 0; i < list_field.size(); ++i)
        {
            QStringList field = list_field.at(i).split("=");

            if (field[1].isEmpty())
            {
                set_user_alert(event, errorMessage("field " + field[0] + " is empty", "error"));
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createnode", 302 );
                postEvent(redir);
                return;
            }

            form_field[field[0]] = field[1].replace(QString("%40"), QString("@"));
        }

        qDebug() << "form field : " << form_field;



        QUuid node_uuid = QUuid::createUuid();
        QString str_node_uuid = node_uuid.toString().mid(1,36);

        QUuid node_token = QUuid::createUuid();
        QString str_node_token = node_token.toString().mid(1,36);

        qDebug() << "str_node_uuid : " << str_node_uuid << " str_node_token : " << str_node_token;

        BSONObj user_id;
        if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
        {
            user_id = BSON("_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
        }
        else user_id = BSON("_id" << mongo::OID(form_field["user"].toStdString()));

        BSONObj t_user = mongodb_->Find("users", user_id);

        std::cout << "T USER : " << t_user.toString() << std::endl;

        BSONObj t_node = BSON(GENOID <<
                              "nodename" << form_field["nodename"].toStdString()  <<
                              "email" << t_user.getField("email").str() <<
                              "user_id" << t_user.getField("_id").OID() <<
                              "node_uuid" << str_node_uuid.toStdString() <<
                              "node_password" << str_node_token.toStdString());
        mongodb_->Insert("nodes", t_node);

        //doc = { login : 'user', email : 'user@email.com', authentication_token : 'token'}
        //db.users.insert(doc);





        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "nodes", 302 );
        postEvent(redir);

        break;
    }

}



/********** ADMIN PAGE ************/
void Http_api::admin_workflows_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open("html_templates/admin_workflows_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }

                QList <BSONObj> workflows = mongodb_->FindAll("workflows", filter);

                QString output;
                int counter=0;
                foreach (BSONObj workflow, workflows)
                {
                    BSONObj user_id = BSON("_id" << workflow.getField("user_id"));
                    BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id_" + QString::number(counter) + "\" value=\"" + QString::fromStdString(workflow.getField("_id").OID().str()) + "\"></td><td>" +
                            QString::fromStdString(workflow.getField("workflow").str()) + "</td>" +
                            "<td>" + QString::fromStdString(workflow.getField("email").str()) + "</td>" +
                            "<td>" + QString::fromStdString(workflow.getField("uuid").str()) + "</td>" +
                            "<td>" + QString::fromStdString(workflow.getField("workers").jsonString(Strict, false)) + "</td>" +
                            "<td>" + QString::fromStdString(user.getField("email").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);




                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}



/********** ADMIN PAGE ************/
void Http_api::admin_workflow_post(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;
    QxtWebContent *form;
    QxtWebRedirectEvent *redir=NULL;


//            if(!body.open("html_templates/admin_users_get.html"))



    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        if(!body.open("html_templates/admin_workflow_post.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;

                if (user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    QString output="<label class=\"label\">Choose a owner</label>";
                    BSONObj empty;
                    QList <BSONObj> users = mongodb_->FindAll("users", empty);

                    output.append("<select name=\"user\">");

                    foreach (BSONObj user, users)
                    {
                        output.append("<option value=\"" + QString::fromStdString(user.getField("_id").OID().str()) + "\">" +  QString::fromStdString(user.getField("login").str()) + "</option>");
                    }
                    output.append("</select>");


                    body["users"] = output;
                }
                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);

        break;
    case POST:

        qDebug() << "COOKIES : " << event->cookies.value("nodecast");
        QString l_alert;
        if (!check_user_login(event, l_alert)) return;

        form = event->content;
        form->waitForAllContent();

        QByteArray requestContent = form->readAll();
        //RECEIVE :  "firstname=fred&lastname=ix&email=fredix%40gmail.com"

        qDebug() << "RECEIVE : " << requestContent;

        //QxtWebStoreCookieEvent cookie_mail (event->sessionID, "email", hauth["email"],  QDateTime::currentDateTime().addMonths(1));
        //QxtWebStoreCookieEvent cookie_pass (event->sessionID, "password", hauth["password"],  QDateTime::currentDateTime().addMonths(1));



        //RECEIVE :  "login=fredix&email=fredix%40gmail.com"

        QHash <QString, QString> form_field;
        QStringList list_field = QString::fromAscii(requestContent).split("&");

        for (int i = 0; i < list_field.size(); ++i)
        {
            QStringList field = list_field.at(i).split("=");
            if (field[1].isEmpty())
            {
                set_user_alert(event, errorMessage("field " + field[0] + " is empty", "error"));
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createworkflow", 302 );
                postEvent(redir);
                return;
            }
            form_field[field[0]] = field[1].replace(QString("%40"), QString("@"));
        }

        qDebug() << "form field : " << form_field;
//      QHash(("workers", "%7B+%22myworkerA%22+%3A+1%2C+%22myworkerB%22+%3A+2%2C+%22myworkerC%22+%3A+3+%7D")("workflow", "test")("user", "51126e85544a4eee6d9521e2"))


        QUuid workflow_uuid = QUuid::createUuid();
        QString str_workflow_uuid = workflow_uuid.toString().mid(1,36);

        qDebug() << "str_workflow_uuid : " << str_workflow_uuid;


        QUrl worker = QUrl::fromPercentEncoding(form_field["workers"].toAscii().replace("+", "%20"));
        qDebug() << "URL : " << worker.toString();

        BSONObj b_workflow;

        try {
            b_workflow = mongo::fromjson(worker.toString().toAscii());
        }
        catch (mongo::MsgAssertionException &e)
        {
            set_user_alert(event, errorMessage("workers's' field is not a JSON format", "error"));
            redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createworkflow", 302 );
            postEvent(redir);
            return;
        }
         std::cout << "WORKFLOW : " << b_workflow.toString() << std::endl;

        BSONObj user_id;
        if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
        {
            user_id = BSON("_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
        }
        else user_id = BSON("_id" << mongo::OID(form_field["user"].toStdString()));


        BSONObj t_user = mongodb_->Find("users", user_id);

        std::cout << "T USER : " << t_user.toString() << std::endl;

        BSONObj t_workflow = BSON(GENOID <<
                                  "workflow" << form_field["workflow"].toStdString()  <<
                                  "email" << t_user.getField("email").str() <<
                                  "user_id" << t_user.getField("_id").OID() <<
                                  "uuid" << str_workflow_uuid.toStdString() <<
                                  "workers" << b_workflow);
        mongodb_->Insert("workflows", t_workflow);

        //doc = { login : 'user', email : 'user@email.com', authentication_token : 'token'}
        //db.users.insert(doc);





        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "workflows", 302 );
        postEvent(redir);

        break;
    }

}


/********** ADMIN PAGE ************/
void Http_api::admin_workers_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open("html_templates/admin_workers_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));

                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }
                QList <BSONObj> workers = mongodb_->FindAll("workers", filter);

                QString output;
                int counter=0;
                foreach (BSONObj worker, workers)
                {

                    //BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    //BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id\" value=\"1\"></td><td>1</td><td>" +
                            QString::fromStdString(worker.getField("name").str()) + "</td>" +
                            "<td>" + QString::number(worker.getField("port").number()) + "</td>" +
                            "<td>" + QString::fromStdString(worker.getField("type").str()) + "</td>" +
                            "<td>" + QString::fromStdString(worker.getField("nodes").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);






                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}




/********** ADMIN PAGE ************/
void Http_api::admin_sessions_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;


{ "_id" : ObjectId("50f91cbda159811513a8b729"), "nodename" : "samsung@dev", "email" : "fredix@gmail.com", "user_id" : ObjectId("50f91c85a7a5dbe8e2f35b29"), "node_uuid" : "0d7f9bdc-37a2-4290-be41-62598bd7a525", "node_password" : "6786a141-6dff-4f91-891a-a9107915ad76" }


      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open("html_templates/admin_sessions_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }
                QList <BSONObj> sessions = mongodb_->FindAll("sessions", filter);

                QString output;
                int counter=0;
                foreach (BSONObj session, sessions)
                {

                    //BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    //BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id\" value=\"1\"></td><td>1</td><td>" +
                            QString::fromStdString(session.getField("uuid").str()) + "</td>" +
                            "<td>" + QString::fromStdString(session.getField("counter").str()) + "</td>" +
                            "<td>" + QString::fromStdString(session.getField("last_worker").str()) + "</td>" +
                            "<td>" + QString::fromStdString(session.getField("start_timestamp").str()) + "</td>" +
                            "<td>" + QString::fromStdString(session.getField("workflow").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);


                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}



/********** ADMIN PAGE ************/
void Http_api::admin_payloads_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open("html_templates/admin_payloads_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";

                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }
                QList <BSONObj> payloads = mongodb_->FindAll("payloads", filter);

                QString output;
                int counter=0;
                foreach (BSONObj payload, payloads)
                {

                    //BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    //BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";
                    QDateTime timestamp;
                    timestamp.setTime_t(payload.getField("timestamp").Number());


                    BSONObj steps = payload.getField("steps").Obj();
                    list <BSONElement> list_steps;
                    steps.elems(list_steps);

                    BSONObj l_step;
                    list<be>::iterator i;
                    QString li_step;

                    li_step.append("<table class=\"table\">");


                    for(i = list_steps.begin(); i != list_steps.end(); ++i) {
                        l_step = (*i).embeddedObject ();
                        //std::cout << "l_step => " << l_step  << std::endl;
                        li_step.append("<tr>");

                        li_step.append("<td>" + QString::fromStdString(l_step.getField("action").str()) + "</td>");
                        li_step.append("<td>" + QString::fromStdString(l_step.getField("name").str()) + "</td>");
                        li_step.append("<td>" + QString::number(l_step.getField("order").Number()) + "</td>");
                        li_step.append("<td>" + QString::fromStdString(l_step.getField("data").str()) + "</td>");

                        li_step.append("</tr>");

                    }
                    li_step.append("</table>");



                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id\" value=\"1\"></td><td>1</td><td>" +                                                              
                                  QString::fromStdString(payload.getField("action").str()) + "</td>" +
                                  "<td>" + QString::fromStdString(payload.getField("counter").str()) + "</td>" +
                                  "<td>" + timestamp.toString(Qt::SystemLocaleLongDate) + "</td>" +
                                  "<td>" + QString::fromStdString(payload.getField("workflow_uuid").str()) + "</td>" +
                                  "<td>" + QString::fromStdString(payload.getField("data").str()) + "</td>" +
                                  "<td>" + li_step + "</td>" +
                                  "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);




                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}


//{ "_id" : ObjectId("5110f4667b30b3ebd41fce64"),
//  "data" : { "payload" : { "data" : "{ \"command\" : \"get_file\", \"filename\" : \"ftest\", \"payload_type\" : \"test\", \"session_uuid\" : \"e2329bc3-2be1-4209-8dd6-ead2cacb76e9\" }", "action" : "push", "session_uuid" : "e2329bc3-2be1-4209-8dd6-ead2cacb76e9" } },
//  "pushed" : false, "timestamp" : 1360065638, "worker" : "transcode" }


/********** ADMIN PAGE ************/
void Http_api::admin_lost_pushpull_payloads_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open("html_templates/admin_lost_pushpull_payloads_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                footer.open("html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";

                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }
                QList <BSONObj> lost_pushpull_payloads = mongodb_->FindAll("lost_pushpull_payloads", filter);

                QString output;
                int counter=0;
                foreach (BSONObj lost_pushpull_payload, lost_pushpull_payloads)
                {
                    //BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    //BSONObj user = mongodb_->Find("users", user_id);


                    QString trclass = (counter %2 == 0) ? "odd" : "even";
                    QDateTime timestamp;
                    timestamp.setTime_t(lost_pushpull_payload.getField("timestamp").Number());



                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id\" value=\"1\"></td><td>1</td><td>" +
                            QString::fromStdString(lost_pushpull_payload.getFieldDotted("data.payload.action").str()) + "</td>" +
                            "<td>" + QString::fromStdString(lost_pushpull_payload.getFieldDotted("data.payload.session_uuid").str()) + "</td>" +
                            "<td>" + QString::fromStdString(lost_pushpull_payload.getField("pushed").boolean()==true? "true" : "false") + "</td>" +
                            "<td>" + timestamp.toString(Qt::SystemLocaleLongDate) + "</td>" +
                            "<td>" + QString::fromStdString(lost_pushpull_payload.getField("worker").str()) + "</td>" +
                            "<td>" + QString::fromStdString(lost_pushpull_payload.getFieldDotted("data.payload.data").jsonString(Strict, false)) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);




                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}



/********** ADMIN PAGE ************/
void Http_api::admin_template(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open("html_templates/admin_lost_pushpull_payloads_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open("html_templates/header.html");
                body.open("html_templates/template.html");
                footer.open("html_templates/footer.html");

                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                       //    header.render().toUtf8() +
                                           body.render().toUtf8());
                                        //   footer.render().toUtf8());


                page->contentType="text/html";
            }
            postEvent(page);
}


void Http_api::staticfile(QxtWebRequestEvent* event, QString directory, QString filename)
{

    qDebug() << "URL : " <<event->url;

    QxtWebPageEvent *page;
    QString response;

    qDebug() << "STATIC FILE " << directory << " filename " << filename;
    QFile *static_file;
    static_file = new QFile("html_templates/" + directory + "/" + filename);

    if (!static_file->open(QIODevice::ReadOnly))
    {
        response = "FILE NOT FOUND";

        page = new QxtWebPageEvent(event->sessionID,
                                   event->requestID,
                                   response.toUtf8());
        page->contentType="text/html";
    }
    else
    {
        QFileInfo fi(static_file->fileName());
        QString ext = fi.suffix();
        qDebug() << "EXTENSION : " << ext;

        page = new QxtWebPageEvent(event->sessionID,
                                   event->requestID,
                                   static_file);
        page->chunked = true;
        page->streaming = false;

        if (ext == "js")
            page->contentType="text/javascript";
        else if (ext == "css")
            page->contentType="text/css";
        else if (ext == "png")
            page->contentType="image/png";
    }

    postEvent(page);
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
    else if (action == "status")
    {
        data.insert("status", data1);
    }
    body = QxtJSON::stringify(data);

    return body;
}


QString Http_api::errorMessage(QString msg, QString level)
{
    QString error;
    return error = "<div class=\"message " + level + "\"><p>" + msg + "</p></div>";
}
