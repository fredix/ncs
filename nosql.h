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

#ifndef NOSQL_H
#define NOSQL_H


#include "mongo/client/dbclient.h"
#include "mongo/client/gridfs.h"
#include "mongo/bson/bson.h"


#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QDebug>
#include <QVariant>
#include <QMutex>



using namespace mongo;
using namespace bson;

typedef QVariantHash Hash;

class Nosql : public QObject
{
    Q_OBJECT
public:    
    Nosql(QString instance_type, QString a_server, QString a_database);
    static Nosql *getInstance_front();
    static Nosql *getInstance_back();
    static void kill_front();

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
    QBool ReadFile(const be &gfs_id, mongo::GridFile **a_gf);
    void Flush(string a_document, BSONObj query);

private:   
    static Nosql *_singleton_front;
    static Nosql *_singleton_back;
    ~Nosql();

    QString m_server;
    QString m_database;
    QString m_document;
    DBClientConnection m_mongo_connection;
    string m_errmsg;
    mongo::GridFS *m_gfs;
    mongo::GridFile *m_gf;
    BSONObj m_grid_file;
    BSONObj m_datas;
    QMutex *m_mutex;
    QMutex *m_rf_mutex;
};


#endif // NOSQL_H
