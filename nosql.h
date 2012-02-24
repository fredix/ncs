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


#include "client/dbclient.h"
#include "client/gridfs.h"
#include "bson/bson.h"


#include <QObject>
#include <QDomDocument>
#include <QFile>
#include <QDebug>
#include <QVariant>



using namespace mongo;
using namespace bson;

typedef QVariantHash Hash;

class Nosql : public QObject
{
    Q_OBJECT
public:
    Nosql(QString a_server, QString a_database);
    Nosql();
    ~Nosql();
    bo Find(string a_document, const bo &datas);
    QBool Insert(QString a_document, bo a_datas);
    bo ExtractJSON(const be &gfs_id);
    bo CreateHost(bo &payload, const bo &data, const be &user_id);
    bo CreateOsystem(bo &payload, const bo &data);
    bo CreateOsversion(bo &data);
    QBool Update(QString a_document, const bo &element_id, const bo &a_datas);
    bo WriteFile(const string json);


protected:
    QBool ReadFile(const be &gfs_id);

private:
    QString m_server;
    QString m_database;
    QString m_document;
    DBClientConnection m_mongo_connection;
    string m_errmsg;
    mongo::GridFS *m_gfs;
    mongo::GridFile *m_gf;
    bo m_grid_file;
    bo m_datas;
};


#endif // NOSQL_H
