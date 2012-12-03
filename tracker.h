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

#ifndef TRACKER_H
#define TRACKER_H

#include "ncs_global.h"
#include "nosql.h"
#include "zeromq.h"
#include <QxtWeb/QxtWebSlotService>
#include <QxtWeb/QxtWebPageEvent>
#include <QxtWeb/QxtWebContent>
#include <QxtWeb/QxtHtmlTemplate>
#include <QxtJSON>
#include <QUuid>
#include <QCryptographicHash>
#include <QDebug>

using namespace mongo;
using namespace bson;

typedef QMap<QString, MethodType> StringToEnumMap;


//Peer announce interval (Seconds)
#define __INTERVAL 1800;

//Time out if peer is this late to re-announce (Seconds)
#define __TIMEOUT 120;

//Minimum announce interval (Seconds)
//Most clients obey this, but not all
#define __INTERVAL_MIN 60;

// By default, never encode more than this number of peers in a single request
#define __MAX_PPR 20;

class Tracker : public QxtWebSlotService
{
    Q_OBJECT

public:
    Tracker(QxtAbstractWebSessionManager *sm, QObject * parent = 0);
    //Http_api(QxtAbstractWebSessionManager * sm, Nosql& a);
    ~Tracker();

public slots:
    void index(QxtWebRequestEvent* event);
    void admin(QxtWebRequestEvent* event, QString action);
    void announce(QxtWebRequestEvent* event);


private:
    void announce_get(QxtWebRequestEvent* event);
    QString getkey(QUrl url, QString key, bool &error, bool fixed_size=false);


    StringToEnumMap enumToHTTPmethod;

    zmq::socket_t *z_push_api;
    zmq::message_t *z_message;

    Nosql *nosql_;
    Zeromq *zeromq_;
    QBool checkAuth(QString header, BSONObjBuilder &payload, bo &a_user);
};


#endif // TRACKER_H
