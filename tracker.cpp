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


#include "tracker.h"


Tracker::Tracker(QxtAbstractWebSessionManager * sm, QObject * parent): QxtWebSlotService(sm,parent)
{
    enumToHTTPmethod.insert(QString("GET"), GET);
    enumToHTTPmethod.insert(QString("POST"), POST);
    enumToHTTPmethod.insert(QString("PUT"), PUT);
    enumToHTTPmethod.insert(QString("DELETE"), DELETE);



    mongodb_ = Mongodb::getInstance ();
    zeromq_ = Zeromq::getInstance ();

    z_message = new zmq::message_t(2);
    //m_context = new zmq::context_t(1);
    z_push_api = new zmq::socket_t(*zeromq_->m_context, ZMQ_PUSH);

    int hwm = 50000;
    z_push_api->setsockopt(ZMQ_SNDHWM, &hwm, sizeof (hwm));
    z_push_api->setsockopt(ZMQ_RCVHWM, &hwm, sizeof (hwm));

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


    bo l_user = mongodb_->Find("users", auth);

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
    bool error=false;

    /*
    http://some.tracker.com:999/announce
    ?info_hash=12345678901234567890
    &peer_id=ABCDEFGHIJKLMNOPQRST
    &ip=255.255.255.255
    &port=6881
    &downloaded=1234
    &left=98765
    &event=stopped
    */



    QString http_user_agent = event->headers.value("User-Agent");
    if (http_user_agent.isEmpty()) http_user_agent = "N/A";


    QString info_hash = getkey(url, "info_hash", error, true);
    if (error)
    {
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     info_hash.toUtf8()));
        return;
    }

    QString peer_id = getkey(url, "peer_id", error, true);
    if (error)
    {
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     peer_id.toUtf8()));
        return;
    }


    QString port = getkey(url, "port", error);
    if (error)
    {
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     port.toUtf8()));
        return;
    }

    error = true;

    QString ip = getkey(url, "ip", error);
    QString downloaded = getkey(url, "downloaded", error);
    QString left = getkey(url, "left", error);
    QString l_event = getkey(url, "event", error);
    QString key = getkey(url, "key", error);

    QString bodyMessage = "INFO HASH : " + info_hash + " peer_id : " + peer_id +
            " port : " + port + " ip : " + ip + " downloaded : " + downloaded +
            " left : " + left + " event : " + l_event + " key : " + key  +
            " HTTP_USER_AGENT : " + http_user_agent;



    BSONObjBuilder peer_builder;
    peer_builder.genOID();


    std::string info_hash_decoded = hex_decode(info_hash.toStdString());

    peer_builder.append("hash", info_hash_decoded);


    peer_builder.append("user_agent", http_user_agent.toStdString());
    peer_builder.append("ip_address", ip.toStdString());
    QByteArray crypt_key = QCryptographicHash::hash(key.toAscii(), QCryptographicHash::Sha1);
    //std::cout << " CRYPT KEY : " << crypt_key << std::endl;


    peer_builder.append("key", QString(crypt_key).toStdString());
    peer_builder.append("port", port.toInt());

    BSONObj peer = peer_builder.obj();


    mongodb_->Insert("peers", peer);

    /*
    mysql_query('INSERT INTO `peer` (`hash`, `user_agent`, `ip_address`, `key`, `port`) '
            . "VALUES ('" . mysql_real_escape_string(bin2hex($_GET['peer_id'])) . "', '" . mysql_real_escape_string(substr($_SERVER['HTTP_USER_AGENT'], 0, 80))
            . "', INET_ATON('" . mysql_real_escape_string($_SERVER['REMOTE_ADDR']) . "'), '" . mysql_real_escape_string(sha1($_GET['key'])) . "', " . intval($_GET['port']) . ") "
            . 'ON DUPLICATE KEY UPDATE `user_agent` = VALUES(`user_agent`), `ip_address` = VALUES(`ip_address`), `port` = VALUES(`port`), `id` = LAST_INSERT_ID(`peer`.`id`)')
            or die(track('Cannot update peer: '.mysql_error()));
    */


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
        bodyMessage = "HTTP POST not implemented";

        break;
    case PUT:
        qDebug() << "HTTP PUT not implemented";
        bodyMessage = "HTTP PUT not implemented";

        break;
    case DELETE:
        qDebug() << "HTTP DELETE not implemented";
        bodyMessage = "HTTP DELETE not implemented";

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




//Do some input validation
QString Tracker::getkey(QUrl url, QString key, bool &error, bool fixed_size) {
    QString val = url.queryItemValue(key);
    QString msg;

    if (!error && val.isEmpty ())
    {
        error = true;
        msg = "Invalid request, missing data";
        msg.prepend("d14:failure reason msg.size():");
        msg.append("e");
        return msg;

    }
    else if (!error && fixed_size && val.size () != 20)
    {
        error = true;
        msg = "Invalid request, length on fixed argument not correct";
        msg.prepend("d14:failure reason msg.size():");
        msg.append("e");
        return msg;
    }
    else if (!error && val.size () > 80)
    { //128 chars should really be enough
        error = true;
        msg = "Request too long";
        msg.prepend("d14:failure reason msg.size():");
        msg.append("e");
        return msg;
    }

    if (val.isEmpty() && (key == "downloaded" || key == "uploaded" || key == "left"))
            val="0";

    return val;
}



std::string Tracker::hex_decode(const std::string &in) {
    std::string out;
    out.reserve(20);
    unsigned int in_length = in.length();
    for(unsigned int i = 0; i < in_length; i++) {
        unsigned char x = '0';
        if(in[i] == '%' && (i + 2) < in_length) {
            i++;
            if(in[i] >= 'a' && in[i] <= 'f') {
                x = static_cast<unsigned char>((in[i]-87) << 4);
            } else if(in[i] >= 'A' && in[i] <= 'F') {
                x = static_cast<unsigned char>((in[i]-55) << 4);
            } else if(in[i] >= '0' && in[i] <= '9') {
                x = static_cast<unsigned char>((in[i]-48) << 4);
            }

            i++;
            if(in[i] >= 'a' && in[i] <= 'f') {
                x += static_cast<unsigned char>(in[i]-87);
            } else if(in[i] >= 'A' && in[i] <= 'F') {
                x += static_cast<unsigned char>(in[i]-55);
            } else if(in[i] >= '0' && in[i] <= '9') {
                x += static_cast<unsigned char>(in[i]-48);
            }
        } else {
            x = in[i];
        }
        out.push_back(x);
    }
    return out;
}



/*
QString Tracker::hex2bin(QString peer_id) {
        QString r = "";
        bool ok;



        for (i=0; i < peer_id.size(); i+=2) {
            //$r .= chr(hexdec($hex{$i}.$hex{($i+1)}));

            uint dec = peer_id.toUInt(&ok, 10);     // dec == 0, ok == false
        }
        return r;
}
*/
