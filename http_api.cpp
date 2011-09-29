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


Http_api::Http_api(QxtAbstractWebSessionManager * sm, Nosql &a, QObject * parent): QxtWebSlotService(sm,parent)
{}


Http_api::~Http_api()
{}


bool Http_api::checkAuth(QString header)
{


    QString b64 = header.section("Basic ", 1 ,1);
    qDebug() << "auth : " << b64;

    QByteArray tmp_auth_decode = QByteArray::fromBase64(b64.toAscii());
    QString auth_decode =  tmp_auth_decode.data();
    QStringList arr_auth =  auth_decode.split(":");

    QString email = arr_auth.at(0);
    QString key = arr_auth.at(1);


    qDebug() << "email : " << email << " key : " << key;


    /********** TODO CHECK AUTH TO MONGODB ************/


/*     @current_user = User.where(:email => @auth.credentials.first,
:authentication_token => @auth.credentials.last).first


*/



    return true;


}

void Http_api::host(QxtWebRequestEvent* event, QString a, QString b)
{
    qDebug() << "a : " << a << " b : " << b << " headers : " << event->headers;

/*
     QHash<QString, QString>::const_iterator i = event->headers.constBegin();

     while (i != event->headers.constEnd()) {
         qDebug() << i.key() << ": " << i.value();
         ++i;
     }
*/


    bool res = checkAuth(event->headers.value("Authorization"));




    QxtWebContent *myContent = event->content;

    qDebug() << "Bytes to read: " << myContent->unreadBytes();
    myContent->waitForAllContent();

    //QByteArray requestContent = myContent->readAll();
    //qDebug() << "Content: ";
    //qDebug() << requestContent;


    //myContent->read

    QString bodyMessage = buildResponse("proceed");
    qDebug() << bodyMessage;

    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));


    //postEvent(new QxtWebPageEvent(event->sessionID, event->requestID, "<h1>It Works!</h1>"));
}


QString Http_api::buildResponse(QString status)
{
    QString body;
    body.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?><host><status>" + status + "</status></host>");
    return body;
}
