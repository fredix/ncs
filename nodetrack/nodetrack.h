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


#ifndef NODETRACK_H
#define NODETRACK_H

#include "util.h"

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

typedef QMap<QString, HTTPMethodType> StringToHTTPEnumMap;


//Peer announce interval (Seconds)
#define __INTERVAL 1800;

//Time out if peer is this late to re-announce (Seconds)
#define __TIMEOUT 120;

//Minimum announce interval (Seconds)
//Most clients obey this, but not all
#define __INTERVAL_MIN 60;

// By default, never encode more than this number of peers in a single request
#define __MAX_PPR 20;


enum TorrentUpdateAction {
    change_passkey=1,
    add_torrent=2,
    update_torrent=3,
    update_torrents=4,
    add_token=5,
    remove_token=6,
    delete_torrent=7,
    add_user=8,
    remove_user=9,
    remove_users=10,
    update_user=11,
    add_whitelist=12,
    remove_whitelist=13,
    edit_whitelist=14,
    update_announce_interval=15,
    info_torrent=16
};
typedef QMap<QString, TorrentUpdateAction> StringToTorrentEnumMap;



class Nodetrack : public QxtWebSlotService
{
    Q_OBJECT

public:
    Nodetrack(QxtAbstractWebSessionManager *sm, QObject * parent = 0);
    //Http_api(QxtAbstractWebSessionManager * sm, Nosql& a);
    ~Nodetrack();

public slots:
    void index(QxtWebRequestEvent* event);
    void admin(QxtWebRequestEvent* event, QString action);
    //void announce(QxtWebRequestEvent* event);
    //void u(QxtWebRequestEvent* event, QString user_token, QString action, QString info_hash, QString peer_id, QString ip, QString port, QString uploaded, QString downloaded, QString left, QString numwant, QString key, QString compact, QString supportcrypto, QString a_event);
    void torrent(QxtWebRequestEvent* event, QString user_token);

    void u(QxtWebRequestEvent* event, QString user_token, QString action);
    void scrape(QxtWebRequestEvent* event, QString key);
    void reap_peers();
    void do_reap_peers();


private:
    void get_announce(QxtWebRequestEvent* event, QString user_token);
    void torrent_post(QxtWebRequestEvent* event, QString user_token);



    void update(QxtWebRequestEvent* event, QString action);


    // std::string hex_decode(const std::string &in);
  //  QString getkey(QUrl url, QString key, bool &error, bool fixed_size=false);

    StringToTorrentEnumMap enumToTorrentUpdate;

    StringToHTTPEnumMap enumToHTTPmethod;

    zmq::socket_t *z_push_api;
    zmq::message_t *z_message;

    Nosql *nosql_;
    Zeromq *zeromq_;
    QBool checkAuth(QString header, BSONObjBuilder &payload, bo &a_user);
    QString buildResponse(QString action, QString data1, QString data2="");
};






#endif // NODETRACK_H
