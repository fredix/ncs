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

#include "nosql.h"


Nosql::Nosql()
{
    qDebug() << "Nosql construct";
}


Nosql::~Nosql()
{}


Nosql::Nosql(QString a_server, QString a_database) : m_server(a_server), m_database(a_database)
{
    qDebug() << "Nosql construct param";

    //m_server = server;
    //m_database = database;

    //this->gfs = new GridFS(this->mongo_connection, this->database.toAscii().data());
    //return this->gfs;

    try {
        if (!this->m_mongo_connection.connect(this->m_server.toAscii().data(), m_errmsg))
        {
            std::cout << "couldn't connect : " << m_errmsg << std::endl;
            exit(1);
        }

        std::cout << "connected to mongoDB : " << this->m_server.toAscii().data() << std::endl;

        m_gfs = new GridFS(m_mongo_connection, m_database.toAscii().data());
        std::cout << "init GridFS" << std::endl;

    } catch(mongo::DBException &e ) {
        std::cout << "caught " << e.what() << std::endl;
        exit(1);
      }
}


bo Nosql::Find(string a_document, const bo &datas)
{
    qDebug() << "Nosql::Find";
    QString tmp;
    tmp.append(this->m_database).append(".").append(a_document.data());

    qDebug() << "m_database.a_document" << tmp;

    //std::cout << "element : " << datas.jsonString(TenGen) << std::endl;

    std::cout << "element : " << datas.jsonString(Strict) << std::endl;


    //Query req = Query("{" + datas.jsonString(TenGen) + "}" );

    //Query req = datas;

    //Query req = Query("{" + element.jsonString(TenGen) + "}, { _id : 1}" );

    try {        
        auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(tmp.toAscii().data(), mongo::Query(datas));

        qDebug() << "cursor created";
/*
        while( cursor->more() ) {
            result = cursor->next();
            //std::cout << "Nosql::Find pub uuid : " << datas.getField("os_version").valuestr() << std::endl;
            std::cout << "Nosql::Find _id : " << result.getField("_id").jsonString(Strict) << std::endl;
            // pub_uuid.append(host.getField("pub_uuid").valuestr());
        }*/

        if ( !cursor->more() )
                   return BSONObj();

        return cursor->nextSafe().copy();

    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on find into " << m_server.toAscii().data() << "." << a_document.data() << " : " << e.what() << std::endl;
        exit(1);
    }

}

/*
 *
 Struct of the job :
  job = {
    :email => @current_user.email,
    :uuid => params[:id], (Private host's uuid)
    :created_at => Time.now.utc,
    :_id => file_id (ID of the XML (GridFS id) sent from nodecastGUI)
  }
 *
 */

bo Nosql::ExtractJSON(const be &gfs_id)
{
    qDebug() << "Nosql::ExtractJSON";
    bo m_bo_json;

    cout << "gfs_id : " << gfs_id.jsonString(TenGen) << endl;

    //Query req = Query("{" + uuid.jsonString(TenGen) + "}");

    if (ReadFile(gfs_id))
    {
        if (!this->m_gf->exists()) {
            std::cout << "file not found" << std::endl;
        }
        else {
            std::cout << "Find file !" << std::endl;

            QFile json_tmp("/tmp/ncs.json");

            m_gf->write(json_tmp.fileName().toStdString().c_str());

            json_tmp.open(QIODevice::ReadOnly);

            QString json = QString::fromUtf8(json_tmp.readAll());

            //std::cout << "json : " << json.toStdString() << std::endl;

            try {
                m_bo_json = mongo::fromjson(json.toStdString());
            }
            catch(mongo::DBException &e ) {
                std::cout << "caught on parsing json file : " << e.what() << std::endl;
                qDebug() << "Nosql::ExtractJSON ERROR ON GRIDFS";
            }

            //std::cout << "m_bo_json : " << m_bo_json << std::endl;

            if (m_bo_json.nFields() == 0)
            {
                std::cout << "can not read JSON file" << std::endl;
            }
            else
            {
                std::cout << "JSON created !" << std::endl;
            }

            json_tmp.close();
            delete(this->m_gf);
        }
    }
    return m_bo_json;
}


QBool Nosql::ReadFile(const be &gfs_id)
{
    std::cout << "Nosql::ReadFile : " << gfs_id << std::endl;
    try {
        for (int i = 0; i < 5; i++)
        {
            qDebug() << "Nosql::ReadFile BEFORE GRID";
            this->m_gf = new mongo::GridFile(this->m_gfs->findFile(gfs_id.wrap()));

            qDebug() << "Nosql::ReadFile OPEN";
            if (!this->m_gf->exists()) {
                std::cout << "file not found, sleep and retry, counter : " << i << std::endl;
                delete(this->m_gf);
                sleep(1);
            }
            else break;
        }
        return QBool(true);
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on get file : " << e.what() << std::endl;
        qDebug() << "Nosql::ReadFile ERROR ON GRIDFS";
        return QBool(false);
    }

}

bo Nosql::WriteFile(const string json)
{
    std::cout << "Nosql::WriteFile : " << std::endl;

    bo struct_file;
    try {
        struct_file = this->m_gfs->storeFile(json.c_str(), json.size(), "", "json");

    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on write file : " << e.what() << std::endl;
        qDebug() << "Nosql::WriteFile ERROR ON GRIDFS";
    }

    return struct_file;
}

QBool Nosql::Insert(QString a_document, bo a_datas)
{
    qDebug() << "Nosql::Insert";
    QString tmp;
    tmp.append(m_database).append(".").append(a_document);

    qDebug() << "Nosql::Insert tmp : " << tmp;


 try {
        this->m_mongo_connection.insert(tmp.toAscii().data(), a_datas);
        qDebug() << m_server + "." + a_document + " inserted";
        //return QBool(true);
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on insert into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
        //return QBool(false);
    }
    return QBool(true);
}






QBool Nosql::Update(QString a_document, const bo &element_id, const bo &a_datas)
{
    qDebug() << "Nosql::Update";
    QString tmp;
    bo data = BSON( "$set" << a_datas);

    tmp.append(m_database).append(".").append(a_document);
    qDebug() << "Nosql::Update tmp : " << tmp;

 try {
        this->m_mongo_connection.update(tmp.toAscii().data(), mongo::Query(element_id), data);
        qDebug() << m_server + "." + a_document + " updated";
        return QBool(true);
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on update into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
        return QBool(false);
    }
}
