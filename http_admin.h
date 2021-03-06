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


#ifndef HTTP_ADMIN_H
#define HTTP_ADMIN_H


#include "ncs_global.h"
#include "mongodb.h"
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


class Http_admin : public QxtWebSlotService
{
    Q_OBJECT

public:
    Http_admin(QString basedirectory, QxtAbstractWebSessionManager *sm, QObject * parent = 0);
    ~Http_admin();

public slots:
    void index(QxtWebRequestEvent* event);
    void admin(QxtWebRequestEvent* event, QString action);
    void admin_template(QxtWebRequestEvent* event);
    void staticfile(QxtWebRequestEvent* event, QString directory, QString filename);

private:
    void admin_users_get(QxtWebRequestEvent* event);
    void admin_user_post(QxtWebRequestEvent* event);
    void admin_apps_get(QxtWebRequestEvent* event);
    void admin_app_post(QxtWebRequestEvent* event);
    void admin_nodes_get(QxtWebRequestEvent* event);
    void admin_node_post(QxtWebRequestEvent* event);
    void admin_node_or_workflow_delete(QxtWebRequestEvent* event, QString collection);
    void admin_workflows_get(QxtWebRequestEvent* event);
    void admin_workflow_post(QxtWebRequestEvent* event);
    void admin_workers_get(QxtWebRequestEvent* event);
    void admin_sessions_get(QxtWebRequestEvent* event);
    void admin_payloads_get(QxtWebRequestEvent* event);
    void admin_lost_pushpull_payloads_get(QxtWebRequestEvent* event);


    void admin_login(QxtWebRequestEvent* event);
    void admin_logout(QxtWebRequestEvent* event);
    bool check_user_login(QxtWebRequestEvent* event, QString &alert);
    QString errorMessage(QString msg, QString level);
    void set_user_alert(QxtWebRequestEvent *event, QString alert);

    StringToHTTPEnumMap enumToHTTPmethod;

    QString m_basedirectory;
    QHash <QString, QString> user_alert;
    QHash <QString, BSONObj> user_bson;


    Mongodb *mongodb_;
    QBool checkAuth(QString token, BSONObjBuilder &payload, bo &a_user);
    QBool http_auth(QString auth, QHash <QString, QString> &hauth, QString &str_session_uuid);

signals:
    void create_api_user(QString);
    void create_ftp_user(QString, QString);
    void create_xmpp_user(QString);
    void create_bittorrent_user(QString);
};

#endif // HTTP_ADMIN_H
