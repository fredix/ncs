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


Http_api::Http_api(QxtAbstractWebSessionManager * sm, Nosql &a, Zeromq &z, QObject * parent): QxtWebSlotService(sm,parent), nosql_(a), zeromq_(z)
{
    z_message = new zmq::message_t(2);
    //m_context = new zmq::context_t(1);
    z_push_api = new zmq::socket_t(*zeromq_.m_context, ZMQ_PUSH);
    //z_push_api->bind("tcp://*:5555");
    z_push_api->bind("inproc://http");
}


Http_api::~Http_api()
{}


bool Http_api::checkAuth(QString header, BSONObjBuilder &payload_builder)
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





/********** CREATE HOST ************/
void Http_api::payload(QxtWebRequestEvent* event, QString action)
{
    qDebug() << "CREATE HOST : " << "action : " << action << " headers : " << event->headers;
    QString bodyMessage;

    //QString uuid = QUuid::createUuid().toString();

    QUuid uuid = QUuid::createUuid();
    QUuid pub_uuid = QUuid::createUuid();
    QString str_uuid = uuid.toString().mid(1,36);
    QString str_pub_uuid = pub_uuid.toString().mid(1,36);

    //QString uuid = QUuid::createUuid().toString().mid(1,36).toUpper();



    QString payloadfilename = event->headers.value("X-payloadfilename");
    qDebug() << "FILENAME : " << payloadfilename;


    BSONObjBuilder payload_builder;
    payload_builder.append("action", "dispatcher.create");
    payload_builder.append("uuid", str_uuid.toStdString());
    payload_builder.append("pub_uuid", str_pub_uuid.toStdString());

    //std::cout << "payload builder : " << payload_builder.obj() << std::cout;

    bool res = checkAuth(event->headers.value("Authorization"), payload_builder);

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

        /*QFile zfile("/tmp/in.dat");
        zfile.open(QIODevice::WriteOnly);
        zfile.write(requestContent);
        zfile.close();*/

        bo gfs_file_struct = nosql_.WriteFile(payloadfilename.toStdString(), requestContent.constData (), requestContent.size ());


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
            qDebug() << "PUSH API PAYLOAD";
            z_message->rebuild(payload.objsize());
            memcpy(z_message->data(), (char*)payload.objdata(), payload.objsize());
            z_push_api->send(*z_message, ZMQ_NOBLOCK);
            /************************/
        }
        bodyMessage = buildResponse("create", str_uuid, str_pub_uuid);
    }
    qDebug() << bodyMessage;


    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));
}


/********** UPDATE HOST ************/
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
    payload_builder.append("action", "dispatcher.update");
    payload_builder.append("uuid", uuid.toStdString());


    QString payloadfilename = event->headers.value("X-payloadfilename");
    qDebug() << "FILENAME : " << payloadfilename;


    bool res = checkAuth(event->headers.value("Authorization"), payload_builder);

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

        bo gfs_file_struct = nosql_.WriteFile(payloadfilename.toStdString(), requestContent.constData (), requestContent.size ());

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
            index["prot"]="PROT";

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
    else if (action == "create")
    {
        data.insert("uuid", data1);
        data.insert("pub_uuid", data2);
    }
    else if (action == "error")
    {
        data.insert("error", data1);
    }
    body = QxtJSON::stringify(data);

    return body;
}
