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


#include "tracker.h"


Tracker::Tracker(QxtAbstractWebSessionManager * sm, QObject * parent): QxtWebSlotService(sm,parent)
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

    z_push_api->bind("ipc:///tmp/nodecast/tracker");
}


Tracker::~Tracker()
{
    qDebug() << "Tracker : close socket";
    z_push_api->close ();
    delete(z_push_api);
}


QBool Tracker::checkAuth(QString header, BSONObjBuilder &payload_builder, bo &a_user)
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





void Tracker::announce_get(QxtWebRequestEvent* event)
{

    QDateTime timestamp = QDateTime::currentDateTime();    
    QUrl url = event->url;

    QString bodyMessage = "INFO HASH : " + url.queryItemValue("info_hash") + " peer_id : " + url.queryItemValue("peer_id");

    QString info_hash = url.queryItemValue("info_hash");
    QString peer_id = url.queryItemValue("peer_id");
    QString ip = url.queryItemValue("ip");
    QString port = url.queryItemValue("port");
    QString downloaded = url.queryItemValue("downloaded");
    QString left = url.queryItemValue("left");
    QString l_event = url.queryItemValue("event");


    //bodyMessage = buildResponse(action, reply);

    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));

    qDebug() << bodyMessage;


}




/**************************************************
PAYLOAD ACTION :
    METHOD POST => push/publish payload, workflow uuid
    METHOD PUT => update payload, payload uuid
    DELETE => delete payload, payload uuid
****************************************************/
void Tracker::announce(QxtWebRequestEvent* event)
{

    qDebug() << "RECEIVE CREATE PAYLOAD : " << "METHOD : " << event->method << " headers : " << event->headers;

    QString bodyMessage="";


    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        qDebug() << "HTTP GET";
        announce_get(event);

        break;
    case POST:
        qDebug() << "HTTP POST";
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
void Tracker::index(QxtWebRequestEvent* event)
{
/*    QString bodyMessage;
    bodyMessage = buildResponse("error", "ncs version 0.9.1");

    qDebug() << bodyMessage;

  */

    QxtHtmlTemplate index;
    QxtWebPageEvent *page;

        if(!index.open("html_templates/torrent_index.html"))
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
void Tracker::admin(QxtWebRequestEvent* event, QString action)
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





QString Tracker::buildResponse(QString action, QString data1, QString data2)
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
