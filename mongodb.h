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

#ifndef MONGODB_H
#define MONGODB_H


#include "mongo/client/dbclient.h"
#include "mongo/client/gridfs.h"
#include "mongo/bson/bson.h"
#include "nodetrack/util.h"
//#include "zeromq.h"

#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QDebug>
#include <QVariant>
#include <QMutex>
#include <queue>
#include <vector>
#include <QStack>
#include <QStringList>


using namespace mongo;
using namespace bson;

typedef QVariantHash Hash;

class Mongodb : public QObject
{
    Q_OBJECT
public:    
    Mongodb(QString a_server, QString a_database);
    static Mongodb *getInstance();

    static void kill();

    BSONObj Find(string a_document, const BSONObj a_datas);        
    BSONObj Find(string a_document, const BSONObj a_query, BSONObj *a_fields);
    QList <BSONObj> FindAll(string a_document, const BSONObj datas);
    int Count(QString a_document);
    QBool Insert(QString a_document, BSONObj a_datas);
    BSONObj ExtractJSON(const be &gfs_id);
    QBool ExtractBinary(const be &gfs_id, string path, QString &filename);
    QBool Update(QString a_document, const BSONObj &element_id, const BSONObj &a_datas, bool upsert=false, bool multi=false);
    QBool Update(QString a_document, const BSONObj &element_id, const BSONObj &a_datas, BSONObj a_options, bool upsert=false, bool multi=false);
    QBool Addtoarray(QString a_document, const BSONObj &element_id, const BSONObj &a_datas);
    BSONObj WriteFile(const string filename, const char *data, int size);
    QBool ReadFile(const be &gfs_id, const mongo::GridFile **a_gf);        
    int GetNumChunck(const be &gfs_id);
    string GetFilename(const be &gfs_id);
    BSONObj GetGfsid(const string filename);
    QBool ExtractByChunck(const be &gfs_id, int chunk_index, QByteArray &chunk_data, int &chunk_length);
    void Flush(string a_document, BSONObj query);


    /*****************************
               BITTORRENT
    *****************************/
    void bt_load_torrents(QHash<QString, bt_torrent> &bt_torrents);
    void bt_load_users(QHash<QString, bt_user> &bt_users);
    void bt_load_tokens(QHash<QString, bt_torrent> &bt_torrents);
    void bt_load_whitelist(QVector<QString> &whitelist);

    void bt_record_user(int id, long long uploaded_change, long long downloaded_change); // (id,uploaded_change,downloaded_change)
    void bt_record_torrent(int tid, int seeders, int leechers, int snatched_change, int balance); // (id,seeders,leechers,snatched_change,balance)
    void bt_record_snatch(int uid, int tid, time_t tstamp, std::string ip); // (uid,fid,tstamp,ip)
    void bt_record_peer(int uid, int fid, int active, std::string peerid, std::string useragent, std::string &ip, long long uploaded, long long downloaded, long long upspeed, long long downspeed, long long left, time_t timespent, unsigned int announces);
    void bt_record_token(int uid, int tid, long long downloaded_change);
    void bt_record_peer_hist(int uid, long long downloaded, long long left, long long uploaded, long long upspeed, long long downspeed, long long tstamp, std::string &peer_id, int tid);
    void bt_flush();
    bool bt_all_clear();

    void bt_flush_users();
    void bt_flush_torrents();
    void bt_flush_snatches();
    void bt_flush_peers();
    void bt_flush_peer_hist();
    void bt_flush_tokens();


    void bt_do_flush_users();
    void bt_do_flush_torrents();
    void bt_do_flush_snatches();
    void bt_do_flush_peers();
    void bt_do_flush_peer_hist();
    void bt_do_flush_tokens();

private:   
    static Mongodb *_singleton;
    //zmq::socket_t *m_socket_payload;
    //QSocketNotifier *check_payload_response;


    ~Mongodb();

    QString m_server;
    QString m_database;
    QString m_document;
    DBClientConnection m_mongo_connection;
    DBClientReplicaSet *m_mongo_replicaset_connection;
    string m_errmsg;
    //mongo::GridFS *m_gfs;
    const mongo::GridFile *m_gf;
    BSONObj m_grid_file;
    BSONObj m_datas;
    QMutex *m_mutex;
    QMutex *m_rf_mutex;

    QMutex *user_buffer_lock;
    QMutex *torrent_buffer_lock;
    QMutex *peer_buffer_lock;
    QMutex *snatch_buffer_lock;
    QMutex *user_token_lock;
    QMutex *peer_hist_buffer_lock;

    bool u_active, t_active, p_active, s_active, tok_active, hist_active;

public:
    struct mongo_query {
                      QString document;
                      mongo::Query query;
                      mongo::BSONObj data;
                   };

    QVector<mongo_query> update_user_buffer;
    QVector<mongo_query> update_torrent_buffer;
    QVector<mongo_query> update_peer_buffer;
    QVector<mongo_query> update_snatch_buffer;
    QVector<mongo_query> update_token_buffer;
    QVector<mongo_query> update_peer_hist_buffer;

    typedef QVector<mongo_query> qmongo_query;


    QStack<qmongo_query> user_queue;
    QStack<qmongo_query> torrent_queue;
    QStack<qmongo_query> peer_queue;
    QStack<qmongo_query> snatch_queue;
    QStack<qmongo_query> token_queue;
    QStack<qmongo_query> peer_hist_queue;

    QBool Update_raw(mongo_query a_query);


signals:
    void forward_payload(BSONObj data);


private slots:
    void payload_receive();
    void push_payload(BSONObj a_payload);

};


#endif // MONGODB_H
