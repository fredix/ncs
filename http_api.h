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

#ifndef HTTP_API_H
#define HTTP_API_H

#include "ncs_global.h"
#include "mongodb.h"
#include "zeromq.h"
#include <QxtWeb/QxtWebServiceDirectory>
#include <QxtWeb/QxtWebSlotService>
#include <QxtWeb/QxtWebPageEvent>
#include <QxtWeb/QxtWebContent>
#include <QxtWeb/QxtHtmlTemplate>
#include <QxtWebStoreCookieEvent>
#include <QxtJSON>
#include <QUuid>
#include <QFileInfo>
#include <QCryptographicHash>

using namespace mongo;
using namespace bson;


typedef QMap<QString, HTTPMethodType> StringToHTTPEnumMap;


class Http_api : public QxtWebSlotService
{    
    Q_OBJECT

public:
    Http_api(QString basedirectory, QxtAbstractWebSessionManager *sm, QObject * parent = 0);
    //Http_api(QxtAbstractWebSessionManager * sm, Nosql& a);    
    ~Http_api();

public slots:        
    void index(QxtWebRequestEvent* event);
    void payload(QxtWebRequestEvent* event, QString action, QString uuid="");
    void session(QxtWebRequestEvent* event, QString uuid);
    void ftp(QxtWebRequestEvent* event);
    void file(QxtWebRequestEvent* event, QString action);
    void node(QxtWebRequestEvent* event, QString token);
    void workflow(QxtWebRequestEvent* event, QString action);
    void user(QxtWebRequestEvent* event);
    void staticfile(QxtWebRequestEvent* event, QString directory, QString filename);

private:
    void payload_post(QxtWebRequestEvent* event, QString action);
    void session_get(QxtWebRequestEvent* event, QString uuid);
    QString errorMessage(QString msg, QString level);
    QBool checkAuth(QString token, BSONObjBuilder &payload, bo &a_user);
    QString buildResponse(QString action, QString data1, QString data2="");
    QString m_basedirectory;
    StringToHTTPEnumMap enumToHTTPmethod;
    QHash <QString, QString> user_alert;
    QHash <QString, BSONObj> user_bson;

    zmq::socket_t *z_push_api;
    zmq::message_t *z_message;

    Mongodb *mongodb_;
    Zeromq *zeromq_;
 };


#endif // HTTP_API_H
