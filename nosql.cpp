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

Nosql *Nosql::_singleton_front = NULL;
Nosql *Nosql::_singleton_back = NULL;


Nosql::~Nosql()
{}



Nosql* Nosql::getInstance_front() {
/*    if (NULL == _singleton)
        {
          qDebug() << "creating singleton.";
          _singleton =  new Nosql(mongodb_ip, mongodb_base);
        }
      else
        {
          qDebug() << "singleton already created!";
        }*/
      return _singleton_front;
}


Nosql* Nosql::getInstance_back() {
/*    if (NULL == _singleton)
        {
          qDebug() << "creating singleton.";
          _singleton =  new Nosql(mongodb_ip, mongodb_base);
        }
      else
        {
          qDebug() << "singleton already created!";
        }*/
      return _singleton_back;
}

void Nosql::kill_front ()
  {
    if (NULL != _singleton_front)
      {
        delete _singleton_front;
        _singleton_front = NULL;
      }
  }


Nosql::Nosql(QString instance_type, QString a_server, QString a_database) : m_server(a_server), m_database(a_database)
{
    qDebug() << "Nosql construct param";

    //m_server = server;
    //m_database = database;

    //this->gfs = new GridFS(this->mongo_connection, this->database.toAscii().data());
    //return this->gfs;

    //mongo::ScopedDbConnection con (this->m_server.toAscii().data());

    m_mutex = new QMutex();


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

    if (instance_type == "front")
    {
            _singleton_front = this;
    }
    else if (instance_type =="back")
    {
            _singleton_back = this;
    }

}


bo Nosql::Find(string a_document, const bo datas)
{
    qDebug() << "Nosql::Find";        
    m_mutex->lock();

    QString tmp;
    tmp.append(this->m_database).append(".").append(a_document.data());

    qDebug() << "m_database.a_document" << tmp;

    //std::cout << "element : " << datas.jsonString(TenGen) << std::endl;

    std::cout << "element : " << datas.jsonString(Strict) << std::endl;


    //Query req = Query("{" + datas.jsonString(TenGen) + "}" );

    //Query req = datas;

    //Query req = Query("{" + element.jsonString(TenGen) + "}, { _id : 1}" );

    try {        
        qDebug() << "before cursor created";
        auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(tmp.toAscii().data(), mongo::Query(datas));
        qDebug() << "after cursor created";
/*
        while( cursor->more() ) {
            result = cursor->next();
            //std::cout << "Nosql::Find pub uuid : " << datas.getField("os_version").valuestr() << std::endl;
            std::cout << "Nosql::Find _id : " << result.getField("_id").jsonString(Strict) << std::endl;
            // pub_uuid.append(host.getField("pub_uuid").valuestr());
        }*/

        if ( !cursor->more() ) {
                    cursor.release();
            m_mutex->unlock();
            return BSONObj();
        }
        m_mutex->unlock();
        return cursor->nextSafe().copy();
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on find into " << m_server.toAscii().data() << "." << a_document.data() << " : " << e.what() << std::endl;
        m_mutex->unlock();
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
    m_mutex->lock();

    qDebug() << "Nosql::ExtractJSON";
    bo m_bo_json;

    cout << "gfs_id : " << gfs_id.jsonString(TenGen) << endl;

    //Query req = Query("{" + uuid.jsonString(TenGen) + "}");

    //m_mutex->lock();
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
    m_mutex->unlock();
    return m_bo_json;
}



QBool Nosql::ExtractBinary(const be &gfs_id, string path, QString &filename)
{
    m_mutex->lock();

    qDebug() << "Nosql::ExtractBinary";

    bo gfsid = BSON("_id" << gfs_id);

    cout << "gfs_id : " << gfsid.firstElement() << endl;

    //Query req = Query("{" + uuid.jsonString(TenGen) + "}");



    if (ReadFile(gfsid.firstElement()))
    {
        if (!m_gf->exists()) {
            std::cout << "file not found" << std::endl;
        }
        else {
            filename = QString::fromStdString (m_gf->getFilename());
            std::cout << "Find file : " << m_gf->getFilename ()<< std::endl;
            string l_path = path + m_gf->getFilename();
            QFile binary_tmp(l_path.data());
            m_gf->write(binary_tmp.fileName().toStdString().c_str());
            delete(this->m_gf);
            qDebug() << "Find file : delete";
        }            
        m_mutex->unlock();
        return QBool(true);
    }            
    m_mutex->unlock();
    return QBool(false);
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

bo Nosql::WriteFile(const string filename, const char *data, int size)
{        
    m_mutex->lock();

    bo struct_file;        
    //m_write_mutex->lock();
    try {
        struct_file = this->m_gfs->storeFile(data, size, filename, "application/octet-stream");
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on write file : " << e.what() << std::endl;
        qDebug() << "Nosql::WriteFile ERROR ON GRIDFS";
    }        
    m_mutex->unlock();
    return struct_file;
}

QBool Nosql::Insert(QString a_document, bo a_datas)
{        
    m_mutex->lock();

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

    m_mutex->unlock();
    return QBool(true);
}






QBool Nosql::Update(QString a_document, const bo &element_id, const bo &a_datas)
{        
    m_mutex->lock();

    qDebug() << "Nosql::Update";
    QString tmp;
    bo data = BSON( "$set" << a_datas);

    tmp.append(m_database).append(".").append(a_document);
    qDebug() << "Nosql::Update tmp : " << tmp;

 try {
        this->m_mongo_connection.update(tmp.toAscii().data(), mongo::Query(element_id), data);
        qDebug() << m_server + "." + a_document + " updated";
        m_mutex->unlock();
        return QBool(true);
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on update into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
            m_mutex->unlock();
        return QBool(false);
    }
}





QBool Nosql::Addtoarray(QString a_document, const bo &element_id, const bo &a_datas)
{
    m_mutex->lock();

    qDebug() << "Nosql::Addtoarray";
    QString tmp;
    bo data = BSON( "$addToSet" << a_datas);

    tmp.append(m_database).append(".").append(a_document);
    qDebug() << "Nosql::Addtoarray tmp : " << tmp;

 try {
        this->m_mongo_connection.update(tmp.toAscii().data(), mongo::Query(element_id), data);
        qDebug() << m_server + "." + a_document + " updated";
        m_mutex->unlock();
        return QBool(true);
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on update into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
        m_mutex->unlock();
        return QBool(false);
    }
}

