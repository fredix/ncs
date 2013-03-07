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

#include "mongodb.h"

Mongodb *Mongodb::_singleton = NULL;


Mongodb::~Mongodb()
{}



Mongodb* Mongodb::getInstance() {
/*    if (NULL == _singleton)
        {
          qDebug() << "creating singleton.";
          _singleton =  new Mongodb(mongodb_ip, mongodb_base);
        }
      else
        {
          qDebug() << "singleton already created!";
        }*/
      return _singleton;
}


void Mongodb::kill ()
  {
    if (NULL != _singleton)
      {
        delete _singleton;
        _singleton = NULL;
      }
  }




Mongodb::Mongodb(QString a_server, QString a_database) : m_server(a_server), m_database(a_database)
{
    qDebug() << "Mongodb construct param";

    m_mutex = new QMutex();
    m_rf_mutex = new QMutex();

    user_buffer_lock = new QMutex();
    torrent_buffer_lock = new QMutex();
    peer_buffer_lock = new QMutex();
    snatch_buffer_lock = new QMutex();
    user_token_lock = new QMutex();
    peer_hist_buffer_lock = new QMutex();


    u_active = false; t_active = false; p_active = false; s_active = false; tok_active = false; hist_active = false;

    ScopedDbConnection *replicaset;

    try {
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );

        if (!replicaset->ok())
        {
            std::cout << "couldn't connect to mongodb : " << m_server.toStdString() << std::endl;
            exit(1);
        }

        std::cout << "connected to mongoDB : " << this->m_server.toStdString() << std::endl;

        /************ SAFE WRITE ************/
        BSONObj const database_options = BSON("getlasterror" << 1 << "j" << true);
        string const database = this->m_database.toStdString();
        BSONObj info;
        //m_mongo_connection.runCommand(database, database_options, info);
        replicaset->conn().runCommand(database, database_options, info);
        replicaset->done();

        std::cout << "MONGODB OPTIONS : " << info << std::endl;
        /************************************/

        //m_gfs = new GridFS(m_mongo_connection, m_database.toAscii().data());
        //std::cout << "init GridFS" << std::endl;


    } catch(mongo::DBException &e ) {
        std::cout << "caught " << e.what() << std::endl;
        exit(1);
      }


    connect(this, SIGNAL(forward_payload(BSONObj)), this, SLOT(push_payload(BSONObj)), Qt::DirectConnection);


/*
    m_socket_payload = new zmq::socket_t (*m_context, ZMQ_PULL);
    m_socket_payload->setsockopt(ZMQ_SNDHWM, &hwm, sizeof (hwm));
    m_socket_payload->setsockopt(ZMQ_RCVHWM, &hwm, sizeof (hwm));
    m_socket_payload->connect("ipc:///tmp/nodecast/" + instance_type.toStdString());




    int payload_socket_fd;
    size_t payload_socket_size = sizeof(payload_socket_fd);
    m_socket_payload->getsockopt(ZMQ_FD, &payload_socket_fd, &payload_socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << payload_socket_fd << " errno : " << zmq_strerror (errno);

    check_payload_response = new QSocketNotifier(payload_socket_fd, QSocketNotifier::Read, this);
    connect(check_payload_response, SIGNAL(activated(int)), this, SLOT(payload_receive()));
*/


    _singleton = this;

}


void Mongodb::payload_receive()
{
/*    check_payload_response->setEnabled(false);
    std::cout << "Mongodb::payload_response" << std::endl;

    qint32 events = 0;
    std::size_t eventsSize = sizeof(events);
    m_socket_payload->getsockopt(ZMQ_EVENTS, &events, &eventsSize);

    std::cout << "Mongodb::payload_response ZMQ_EVENTS : " <<  events << std::endl;


    if (events & ZMQ_POLLIN)
    {
        std::cout << "Mongodb::payload_response ZMQ_POLLIN" <<  std::endl;

        while (true)
        {
            flush_socket:

            zmq::message_t request;

            int res = m_socket_payload->recv(&request, ZMQ_NOBLOCK);
            if (res == -1 && zmq_errno () == EAGAIN) break;


            if (request.size() == 0) {
                std::cout << "Zpull::worker_response received request 0" << std::endl;
                break;
            }


            BSONObj data;
            try {
                data = BSONObj((char*)request.data());

                if (data.isValid() && !data.isEmpty())
                {
                    std::cout << "Zpull received : " << res << " data : " << data  << std::endl;

                    std::cout << "!!!!!!! BEFORE FORWARD PAYLOAD !!!!" << std::endl;
                    emit forward_payload(data.copy());
                    std::cout << "!!!!!!! AFTER FORWARD PAYLOAD !!!!" << std::endl;
                }
                else
                {
                    std::cout << "DATA NO VALID !" << std::endl;
                }

            }
            catch (mongo::MsgAssertionException &e)
            {
                std::cout << "error on data : " << data << std::endl;
                std::cout << "error on data BSON : " << e.what() << std::endl;
                goto flush_socket;
            }

        }
    }



    check_payload_response->setEnabled(true);*/
}


void Mongodb::push_payload(BSONObj a_payload)
{
    std::cout << "Mongodb::push_payload : " << a_payload << std::endl;

    BSONElement table = a_payload.getField("table");
    BSONElement query = a_payload.getField("query");
    BSONElement data = a_payload.getField("data");

//    this->Update(table.toString(false), session_id, l_session);

}



int Mongodb::Count(QString a_document)
{
    QMutexLocker locker(m_mutex);

    qDebug() << "Mongodb::Count";

    QString tmp;
    tmp.append(this->m_database).append(".").append(a_document);
    //int counter = this->m_mongo_connection.count(tmp.toStdString());

    ScopedDbConnection *replicaset =  ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );

    int counter = replicaset->conn().count(tmp.toStdString());
    replicaset->done();

    return counter;
}



void Mongodb::Flush(string a_document, BSONObj query)
{
    QMutexLocker locker(m_mutex);

    std::cout << "Mongodb::Flush, query : " << query.toString() << std::endl;

    QString tmp;
    tmp.append(this->m_database).append(".").append(a_document.data());
    qDebug() << "m_database.a_document" << tmp;

    try {
        qDebug() << "before REMOVE";
        //qDebug() << "before cursor created";
        //auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(tmp.toStdString(), mongo::Query(a_query));
        //qDebug() << "after cursor created";

        m_mongo_connection.remove(tmp.toStdString(), mongo::Query(query));
        qDebug() << "AFTER REMOVE";
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on remove into " << m_server.toAscii().data() << "." << a_document.data() << " : " << e.what() << std::endl;
    }

}

BSONObj Mongodb::Find(string a_document, const bo a_query)
{
    QMutexLocker locker(m_mutex);
    qDebug() << "Mongodb::Find";

    QString tmp;
    tmp.append(this->m_database).append(".").append(a_document.data());

    qDebug() << "m_database.a_document" << tmp;

    std::cout << "element : " << a_query.jsonString(Strict) << std::endl;

     ScopedDbConnection *replicaset;

    try {        
        qDebug() << "before cursor created";
        //auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(tmp.toStdString(), mongo::Query(a_query));

        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );

        auto_ptr<DBClientCursor> cursor = replicaset->conn().query(tmp.toStdString(), mongo::Query(a_query));



        qDebug() << "after cursor created";
/*
        while( cursor->more() ) {
            result = cursor->next();
            //std::cout << "Mongodb::Find pub uuid : " << datas.getField("os_version").valuestr() << std::endl;
            std::cout << "Mongodb::Find _id : " << result.getField("_id").jsonString(Strict) << std::endl;
            // pub_uuid.append(host.getField("pub_uuid").valuestr());
        }*/

        if ( !cursor->more() ) {
            replicaset->done();
            return BSONObj();
        }
        else
        {
            BSONObj data = cursor->nextSafe().copy();
            replicaset->done();
            return data;
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on find into " << m_server.toAscii().data() << "." << a_document.data() << " : " << e.what() << std::endl;
        exit(1);
    }

}



BSONObj Mongodb::Find(string a_document, const BSONObj a_query, BSONObj *a_fields)
{
    QMutexLocker locker(m_mutex);
    qDebug() << "Mongodb::Find with field's filter";

    QString tmp;
    tmp.append(this->m_database).append(".").append(a_document.data());

    qDebug() << "m_database.a_document" << tmp;

    //std::cout << "element : " << datas.jsonString(TenGen) << std::endl;

    std::cout << "element : " << a_query.jsonString(Strict) << std::endl;
    ScopedDbConnection *replicaset;


    try {
        qDebug() << "before cursor created";
        //auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(tmp.toStdString(), mongo::Query(a_query), 0, 0, a_fields);
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        auto_ptr<DBClientCursor> cursor = replicaset->conn().query(tmp.toStdString(), mongo::Query(a_query), 0, 0, a_fields);

        qDebug() << "after cursor created";

        if ( !cursor->more() ) {
            replicaset->done();
            return BSONObj();
        }
        else
        {
            BSONObj data = cursor->nextSafe().copy();
            replicaset->done();
            return data;
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on find into " << m_server.toAscii().data() << "." << a_document.data() << " : " << e.what() << std::endl;
        exit(1);
    }

}



QList <BSONObj> Mongodb::FindAll(string a_document, const bo datas)
{
    QMutexLocker locker(m_mutex);
    qDebug() << "Mongodb::FindAll";

    QString tmp;
    tmp.append(this->m_database).append(".").append(a_document.data());

    qDebug() << "m_database.a_document" << tmp;
    std::cout << "element : " << datas.jsonString(Strict) << std::endl;

    QList <BSONObj> res;
    ScopedDbConnection *replicaset;

    try {
        qDebug() << "before cursor created";
       // auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(tmp.toAscii().data(), mongo::Query(datas));
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        auto_ptr<DBClientCursor> cursor = replicaset->conn().query(tmp.toStdString(), mongo::Query(datas));

        qDebug() << "after cursor created";

        while( cursor->more() ) {
            res << cursor->nextSafe().copy();
        }
        replicaset->done();
        return res;
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on find into " << m_server.toAscii().data() << "." << a_document.data() << " : " << e.what() << std::endl;
        exit(1);
    }

}



bo Mongodb::ExtractJSON(const be &gfs_id)
{        
    QMutexLocker locker(m_mutex);

    qDebug() << "Mongodb::ExtractJSON";
    bo m_bo_json;

    cout << "gfs_id : " << gfs_id.jsonString(TenGen) << endl;

    //Query req = Query("{" + uuid.jsonString(TenGen) + "}");

    //m_mutex->lock();
    if (ReadFile(gfs_id, &m_gf))
    {
        if (!m_gf->exists()) {
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
                qDebug() << "Mongodb::ExtractJSON ERROR ON GRIDFS";
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



QBool Mongodb::ExtractBinary(const be &gfs_id, string path, QString &filename)
{
    QMutexLocker locker(m_mutex);

    qDebug() << "Mongodb::ExtractBinary";

    bo gfsid = BSON("_id" << gfs_id);

    cout << "gfs_id : " << gfsid.firstElement() << endl;


    if (ReadFile(gfsid.firstElement(), &m_gf) )
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
            delete(m_gf);
            //qDebug() << "Find file : delete";
        }            
        return QBool(true);
    }            
    return QBool(false);
}

QBool Mongodb::ReadFile(const be &gfs_id, const mongo::GridFile **a_gf)
{
    std::cout << "Mongodb::ReadFile : " << gfs_id << std::endl;
    ScopedDbConnection *replicaset;

    try {
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (!replicaset->ok())
        {
            GridFS gfs(replicaset->conn(), m_database.toAscii().data());

            for (int i = 0; i < 5; i++)
            {
                qDebug() << "Mongodb::ReadFile BEFORE GRID";
                *a_gf = new mongo::GridFile(gfs.findFile(gfs_id.wrap()));

                qDebug() << "Mongodb::ReadFile OPEN";
                if (!(*a_gf)->exists()) {
                    std::cout << "file not found, sleep and retry, counter : " << i << std::endl;
                    delete(*a_gf);
                    sleep(1);
                }
                else break;
            }
            replicaset->done();
            return QBool(true);
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on get file : " << e.what() << std::endl;
        qDebug() << "Mongodb::ReadFile ERROR ON GRIDFS";
        return QBool(false);
    }
}

int Mongodb::GetNumChunck(const be &gfs_id)
{
    QMutexLocker locker(m_mutex);
    ScopedDbConnection *replicaset;

    std::cout << "Mongodb::GetNumChunck : " << gfs_id << std::endl;
    try {
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (!replicaset->ok())
        {
            GridFS gfs(replicaset->conn(), m_database.toAscii().data());



            qDebug() << "Mongodb::ExtractByChunck BEFORE GRID";
            const mongo::GridFile *m_grid_file = new mongo::GridFile(gfs.findFile(gfs_id.wrap()));

            qDebug() << "Mongodb::ExtractByChunck OPEN";
            if (!m_grid_file->exists()) {
                std::cout << "Mongodb::ExtractByChunck file not found" << std::endl;
                delete(m_grid_file);
                return -1;
            }
            else
            {
                const int num = m_grid_file->getNumChunks();
                std::cout << "Mongodb::ExtractByChunck FILE NAME : " << m_grid_file->getFilename() << std::endl;
                std::cout << "Mongodb::ExtractByChunck NUM CHUCK : " << num << std::endl;

                return num;
            }
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on get file : " << e.what() << std::endl;
        qDebug() << "Mongodb::GetNumChunck ERROR ON GRIDFS";
        return -1;
    }
}


string Mongodb::GetFilename(const be &gfs_id)
{
    QMutexLocker locker(m_mutex);

    std::cout << "Mongodb::GetFilename : " << gfs_id << std::endl;

    ScopedDbConnection *replicaset;


    try {
        qDebug() << "Mongodb::GetFilename BEFORE GRID";
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (!replicaset->ok())
        {
            GridFS gfs(replicaset->conn(), m_database.toAscii().data());

            //const mongo::GridFile *m_grid_file = new mongo::GridFile(this->m_gfs->findFile(gfs_id.wrap()));
            const mongo::GridFile *m_grid_file = new mongo::GridFile(gfs.findFile(gfs_id.wrap()));

            qDebug() << "Mongodb::GetFilename OPEN";
            if (!m_grid_file->exists()) {
                std::cout << "Mongodb::GetFilename file not found" << std::endl;
                delete(m_grid_file);
                replicaset->done();
                return "";
            }
            else
            {
                string filename = m_grid_file->getFilename();
                std::cout << "Mongodb::GetFilename FILE NAME : " << filename << std::endl;
                replicaset->done();
                return filename;
            }
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on get file : " << e.what() << std::endl;
        qDebug() << "Mongodb::GetFilename ERROR ON GRIDFS";
        return "";
    }
}



BSONObj Mongodb::GetGfsid(const string filename)
{
    QMutexLocker locker(m_mutex);
    ScopedDbConnection *replicaset;

    std::cout << "Mongodb::GetGfsid : " << filename << std::endl;
    BSONElement gfsid;
    BSONObj gfsid_;

    try {
        qDebug() << "Mongodb::GetFilename BEFORE GRID";
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (!replicaset->ok())
        {
            GridFS gfs(replicaset->conn(), m_database.toAscii().data());


            const mongo::GridFile *m_grid_file = new mongo::GridFile(gfs.findFile(filename));

            qDebug() << "Mongodb::GetGfsid OPEN";
            if (!m_grid_file->exists()) {
                std::cout << "Mongodb::GetGfsid file not found" << std::endl;
                delete(m_grid_file);
                return gfsid_;
            }
            else
            {
                gfsid = m_grid_file->getFileField("_id");
                std::cout << "Mongodb::GetGfsid : " << gfsid << std::endl;
                gfsid_ = BSON("_id" << gfsid);
                std::cout << "Mongodb::GetGfsid2 : " << gfsid_ << std::endl;
                return gfsid_;
            }
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on get file : " << e.what() << std::endl;
        qDebug() << "Mongodb::GetGfsid ERROR ON GRIDFS";
        return gfsid_;
    }
}




QBool Mongodb::ExtractByChunck(const be &gfs_id, int chunk_index, QByteArray &chunk_data, int &chunk_length)
{
    QMutexLocker locker(m_mutex);
    ScopedDbConnection *replicaset;

    std::cout << "Mongodb::ExtractByChunck : " << gfs_id << std::endl;
    try {
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (!replicaset->ok())
        {
            GridFS gfs(replicaset->conn(), m_database.toAscii().data());


            for (int i = 0; i < 5; i++)
            {
                qDebug() << "Mongodb::ExtractByChunck BEFORE GRID";
                //const mongo::GridFile *m_grid_file = new mongo::GridFile(this->m_gfs->findFile(gfs_id.wrap()));
                const mongo::GridFile *m_grid_file = new mongo::GridFile(gfs.findFile(gfs_id.wrap()));

                qDebug() << "Mongodb::ExtractByChunck OPEN";
                if (!m_grid_file->exists()) {
                    std::cout << "file not found, sleep and retry, counter : " << i << std::endl;
                    delete(m_grid_file);
                    sleep(1);
                }
                else
                {
                    std::cout << "Mongodb::ExtractByChunck BEFORE GET CHUNCK " << std::endl;
                    GridFSChunk chunk = m_grid_file->getChunk( chunk_index );
                    std::cout << "Mongodb::ExtractByChunck AFTER GET CHUNCK " << std::endl;

                    const char *l_chunk_data = chunk.data( chunk_length );

                    std::cout << "Mongodb::ExtractByChunck CHUNCK LEN : " << chunk_length << std::endl;


                    if (chunk_length != 0)
                    {
                        //*chunk_data = (char*) malloc(chunk_length);
                        //memcpy(*chunk_data, l_chunk_data, chunk_length);

                        //chunk_data.setRawData(l_chunk_data, chunk_length);
                        chunk_data.append(l_chunk_data, chunk_length);

                        std::cout << "Mongodb::ExtractByChunck CHUNCK SIZE : " << chunk_data.size() << std::endl;

                        delete(m_grid_file);
                        replicaset->done();
                        return QBool(true);
                    }
                    else
                    {
                        //*chunk_data = NULL;
                        chunk_data.clear();
                        delete(m_grid_file);
                        replicaset->done();
                        return QBool(false);
                    }
                }
            }

        }

    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on get file : " << e.what() << std::endl;
        qDebug() << "Mongodb::ExtractByChunck ERROR ON GRIDFS";
        return QBool(false);
    }
}








BSONObj Mongodb::WriteFile(const string filename, const char *data, int size)
{        
    QMutexLocker locker(m_mutex);
    ScopedDbConnection *replicaset;

    BSONObj struct_file;
    try {
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (!replicaset->ok())
        {
            //struct_file = this->m_gfs->storeFile(data, size, filename, "application/octet-stream");
            GridFS gfs(replicaset->conn(), m_database.toAscii().data());

            struct_file = gfs.storeFile(data, size, filename, "application/octet-stream");

        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on write file : " << e.what() << std::endl;
        qDebug() << "Mongodb::WriteFile ERROR ON GRIDFS";
    }        
    replicaset->done();
    return struct_file;
}

QBool Mongodb::Insert(QString a_document, BSONObj a_datas)
{        
    QMutexLocker locker(m_mutex);

    qDebug() << "Mongodb::Insert";
    QString tmp;
    tmp.append(m_database).append(".").append(a_document);

    qDebug() << "Mongodb::Insert tmp : " << tmp;
    ScopedDbConnection *replicaset;


 try {
        //this->m_mongo_connection.insert(tmp.toAscii().data(), a_datas);
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (replicaset->ok())
        {
            replicaset->conn().insert(tmp.toAscii().data(), a_datas);
            replicaset->done();
            return QBool(true);
        }
        qDebug() << m_server + "." + a_document + " inserted";
        //return QBool(true);
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on insert into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
        //return QBool(false);
    }
    return QBool(false);
}






QBool Mongodb::Update(QString a_document, const BSONObj &element_id, const BSONObj &a_datas, bool upsert, bool multi)
{        
    QMutexLocker locker(m_mutex);

    qDebug() << "Mongodb::Update";
    QString tmp;
    BSONObj data = BSON( "$set" << a_datas);

    std::cout << "QUERY DATA : " << data << std::endl;

    tmp.append(m_database).append(".").append(a_document);
    qDebug() << "Mongodb::Update tmp : " << tmp;
    ScopedDbConnection *replicaset;

 try {
        //this->m_mongo_connection.update(tmp.toAscii().data(), mongo::Query(element_id), data, upsert ,multi);
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (replicaset->ok())
        {
            replicaset->conn().update(tmp.toAscii().data(), mongo::Query(element_id), data, upsert ,multi);
            replicaset->done();
            qDebug() << m_server + "." + a_document + " updated";
            return QBool(true);
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on update into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
        return QBool(false);
    }
    return QBool(false);
}



QBool Mongodb::Update(QString a_document, const BSONObj &element_id, const BSONObj &a_datas, BSONObj a_options, bool upsert, bool multi)
{
    QMutexLocker locker(m_mutex);

    qDebug() << "Mongodb::Update with options";
    QString tmp;
    BSONObjBuilder b_data;
    b_data << "$set" << a_datas;
    b_data.appendElements(a_options);
    BSONObj data = b_data.obj();

    std::cout << "QUERY DATA : " << data << std::endl;

    tmp.append(m_database).append(".").append(a_document);
    qDebug() << "Mongodb::Update tmp : " << tmp;
    ScopedDbConnection *replicaset;

 try {
        //this->m_mongo_connection.update(tmp.toAscii().data(), mongo::Query(element_id), data, upsert ,multi);
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (replicaset->ok())
        {
            replicaset->conn().update(tmp.toAscii().data(), mongo::Query(element_id), data, upsert ,multi);
            replicaset->done();
            qDebug() << m_server + "." + a_document + " updated";
            return QBool(true);
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on update into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
        return QBool(false);
    }
    return QBool(false);
}


QBool Mongodb::Update_raw(mongo_query a_query)
{
    QMutexLocker locker(m_mutex);

    qDebug() << "Mongodb::Update_raw";
    QString db;

    //std::cout << "QUERY DATA : " << a_query << std::endl;

    db.append(m_database).append(".").append(a_query.document);
    qDebug() << "Mongodb::Update db : " << db;
    ScopedDbConnection *replicaset;

 try {
        //this->m_mongo_connection.update(db.toAscii().data(), a_query.query, a_query.data, true);
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (replicaset->ok())
        {
            replicaset->conn().update(db.toAscii().data(), a_query.query, a_query.data, true);
            replicaset->done();
            qDebug() << m_server + "." + a_query.document + " updated";
            return QBool(true);
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on update into " << db.toAscii().data() << " : " << e.what() << std::endl;
        return QBool(false);
    }
    return QBool(false);
}


QBool Mongodb::Addtoarray(QString a_document, const BSONObj &element_id, const BSONObj &a_datas)
{
    QMutexLocker locker(m_mutex);

    qDebug() << "Mongodb::Addtoarray";
    QString tmp;
    BSONObj data = BSON( "$addToSet" << a_datas);

    tmp.append(m_database).append(".").append(a_document);
    qDebug() << "Mongodb::Addtoarray tmp : " << tmp;
    ScopedDbConnection *replicaset;

 try {
        //this->m_mongo_connection.update(tmp.toAscii().data(), mongo::Query(element_id), data);
        replicaset = ScopedDbConnection::getScopedDbConnection( m_server.toStdString() );
        if (replicaset->ok())
        {
            replicaset->conn().update(tmp.toAscii().data(), mongo::Query(element_id), data);
            replicaset->done();
            qDebug() << m_server + "." + a_document + " updated";
            return QBool(true);
        }
    }
    catch(mongo::DBException &e ) {
        std::cout << "caught on update into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
        return QBool(false);
    }
    return QBool(false);
}



/*****************************
           BITTORRENT
*****************************/



void Mongodb::bt_load_torrents(QHash<QString, bt_torrent> &bt_torrents) {
   mongo::BSONObj fields = BSON("id" << 1 << "info_hash" << 1 << "freetorrent" << 1 << "snatched" << 1);

   QString db;
   db.append(this->m_database).append(".").append(".torrents");

   auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(db.toStdString(),
                                                                    mongo::Query().sort("id"), 0, 0, &fields);


   mongo::BSONObj doc;

   try {
       while(cursor->more())
       {
          doc=cursor->next();
          QString info_hash = doc.getStringField("info_hash");
          info_hash = QString::fromStdString(hextostr(info_hash.toStdString()));
          std::cout << "Info hash loaded: " << info_hash.toStdString() << std::endl << "error: " << this->m_mongo_connection.getLastError() << std::endl;

          bt_torrent t;
          t.id = doc.getIntField("id");
          t.free_torrent = (doc.getIntField("freetorrent") == 1) ? FREE
             : (doc.getIntField("freetorrent") == 2) ? NEUTRAL  : NORMAL;
          t.balance = 0;
          t.completed = doc.getIntField("snatched");
          t.last_selected_seeder = "";
          bt_torrents[info_hash] = t;
       }
   }
   catch(mongo::DBException &e ) {
       std::cout << "caught on find into " << m_server.toAscii().data() << ".torrents" << " : " << e.what() << std::endl;
       exit(1);
   }



}

void Mongodb::bt_load_users(QHash<QString, bt_user> &bt_users) {
   //"SELECT ID, can_leech, torrent_pass FROM users_main WHERE Enabled='1';"
   mongo::BSONObj fields = BSON("id" << 1 << "can_leech" << 1 << "torrent_pass" << 1);
   QString db;
   db.append(this->m_database).append(".").append(".bt_users");

   auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(db.toStdString(),
                                                                    mongo::Query(BSON("enabled" << 1)), 0, 0, &fields);


   mongo::BSONObj doc;

   try {
       while(cursor->more())
       {
          doc=cursor->next();
          QString passkey = doc.getStringField("torrent_pass");

          bt_user u;
          u.id = doc.getIntField("id");
          u.can_leech = doc.getIntField("can_leech");
          bt_users[passkey] = u;
       }
   }
   catch(mongo::DBException &e ) {
       std::cout << "caught on find into " << m_server.toAscii().data() << ".bt_users" << " : " << e.what() << std::endl;
       exit(1);
   }

}

void Mongodb::bt_load_tokens(QHash<QString, bt_torrent> &bt_torrents) {
   //"SELECT uf.UserID, t.info_hash FROM users_freeleeches AS uf JOIN torrents AS t ON t.ID = uf.TorrentID WHERE uf.Expired = '0';"
   //
   mongo::BSONObj fields = BSON("id" << 1 <<  "freeleeches.info_hash" << 1);
   QString db;
   db.append(this->m_database).append(".").append(".bt_users");
   auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(db.toStdString(),
                                                                    mongo::Query(BSON("freeleeches.expired" << 0)), 0, 0, &fields);

   mongo::BSONObj doc;
   try {
       while(cursor->more())
       {
          doc=cursor->next();
          QString info_hash = doc.getStringField("freeleeches.info_hash");
          QHash<QString, bt_torrent>::iterator it = bt_torrents.find(info_hash);
          if (it != bt_torrents.end()) {
             bt_torrent &tor = it.value();
             tor.tokened_users.insert(doc.getIntField("id"));
          }
       }
   }
   catch(mongo::DBException &e ) {
       std::cout << "caught on find into " << m_server.toAscii().data() << ".bt_users" << " : " << e.what() << std::endl;
       exit(1);
   }
}


void Mongodb::bt_load_whitelist(QVector<QString> &whitelist) {
   //"SELECT peer_id FROM xbt_client_whitelist;"
   mongo::BSONObj fields = BSON("peer_id" << 1);
   QString db;
   db.append(this->m_database).append(".").append(".torrent_client_whitelist");
   auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(db.toStdString(),
                                                                    mongo::Query(), 0, 0, &fields);

   mongo::BSONObj doc;
   try {
       while(cursor->more())
       {
          doc = cursor->next();
          whitelist.push_back(doc.getStringField("peer_id"));
       }
   }
   catch(mongo::DBException &e ) {
       std::cout << "caught on find into " << m_server.toAscii().data() << ".torrent_client_whitelist" << " : " << e.what() << std::endl;
       exit(1);
   }
}

void Mongodb::bt_record_token(int uid, int tid, long long downloaded_change) {
   QMutexLocker locker(user_token_lock);

   mongo_query m;
   m.document = ".bt_users";
   m.query = mongo::Query(BSON("id" << uid << "freeleeches.id" << tid));
   m.data = BSON("$inc" << BSON("downloaded" << downloaded_change));

   update_token_buffer.push_back(m);
}

void Mongodb::bt_record_user(int id, long long uploaded_change, long long downloaded_change) {
   QMutexLocker locker(user_buffer_lock);

   mongo_query m;
   m.document = ".users";
   m.query = mongo::Query(BSON("id" << id));
   m.data = BSON("$inc" << BSON("uploaded" << uploaded_change << "downloaded" << downloaded_change));

   //update_user_buffer.push_back(m);

   Update_raw(m);
}


//TODO: last_action = IF(VALUES(Seeders) > 0, NOW(), last_action);
void Mongodb::bt_record_torrent(int tid, int seeders, int leechers, int snatched_change, int balance) {
   QMutexLocker locker(torrent_buffer_lock);

   mongo_query m;
   m.document = ".torrents";
   m.query = mongo::Query(BSON("id" << tid));
   m.data = BSON("$set" << BSON("seeders" << seeders << "leechers" << leechers << "balance" << balance) << "$inc" << BSON("snatched" << snatched_change));

   update_torrent_buffer.push_back(m);
}

/*void record_peer(int uid, int fid, int active, std::string peerid, std::string useragent, std::string &ip, long long uploaded, long long downloaded, long long upspeed, long long downspeed, long long left, time_t timespent, unsigned int announces) {
   boost::mutex::scoped_lock lock(peer_buffer_lock);
   //q << record << mongopp::quote << ip << ',' << mongopp::quote << peer_id << ',' << mongopp::quote << useragent << "," << time(NULL) << ')';

   update_peer_buffer += q.str();
}*/

// TODO: Double check timespet is correctly upserted and mtime
// Deal with ip, peer_id, useragent better
void Mongodb::bt_record_peer(int uid, int fid, int active, std::string peerid, std::string useragent, std::string &ip, long long uploaded, long long downloaded, long long upspeed, long long downspeed, long long left, time_t timespent, unsigned int announces) {
   QMutexLocker locker(peer_buffer_lock);

   mongo_query m;
   m.document = ".bt_users";
   m.query = mongo::Query(BSON("id" << uid << "files.id" << fid));
   m.data = BSON("$set" << BSON("files.active" << active << "files.uploaded" << uploaded << "files.downloaded" << downloaded << "files.upspeed" << upspeed << "files.downspeed" << downspeed << "files.remaining" << left << "files.timespent" << static_cast<long long>(timespent) << "files.announced" << announces << "files.useragent" << useragent << "files.ip" << ip << "files.peerid" << peerid << "files.mtime" << static_cast<long long>(time(NULL))));
   //q << record << ',' << mongopp::quote << peer_id << ',' << tid << ',' << time(NULL) << ')';
   update_peer_buffer.push_back(m);
}

void Mongodb::bt_record_peer_hist(int uid, long long downloaded, long long left, long long uploaded, long long upspeed, long long downspeed, long long tstamp, std::string &peer_id, int tid) {
   QMutexLocker locker(peer_hist_buffer_lock);

   mongo_query m;
   m.document = ".bt_users";
   m.query = mongo::Query(BSON("id" << uid << "history.tid" << tid));
   m.data = BSON("$set" << BSON("history.downloaded" << downloaded << "history.remaining" << left << "history.uploaded" << uploaded << "history.upspeed" << upspeed << "history.downspeed" << downspeed << "history.timespent" << (tstamp) << "history.peer_id" << peer_id));

   update_peer_hist_buffer.push_back(m);
}

void Mongodb::bt_record_snatch(int uid, int tid, time_t tstamp, std::string ip) {
   QMutexLocker locker(snatch_buffer_lock);

   mongo_query m;
   m.document = ".bt_users";
   m.query = mongo::Query(BSON("id" << uid));
   m.data = BSON("snatches.id" << tid << "snatches.tstamp" << static_cast<long long>(tstamp) << "snatches.ip" << ip);

   update_snatch_buffer.push_back(m);
}

bool Mongodb::bt_all_clear() {
   return (user_queue.size() == 0 && torrent_queue.size() == 0 && peer_queue.size() == 0 && snatch_queue.size() == 0 && token_queue.size() == 0 && peer_hist_queue.size() == 0);
}

void Mongodb::bt_flush() {
   bt_flush_users();
   bt_flush_torrents();
   bt_flush_snatches();
   bt_flush_peers();
   bt_flush_peer_hist();
   bt_flush_tokens();
}

void Mongodb::bt_flush_users() {
   QMutexLocker locker(user_buffer_lock);

   if (update_user_buffer.empty()) {
      return;
   }
   user_queue.push(update_user_buffer);
   update_user_buffer.clear();
   if (user_queue.size() == 1 && u_active == false) {
      //boost::thread thread(&Mongodb::bt_do_flush_users, this);
   }
}

void Mongodb::bt_flush_torrents() {
   QMutexLocker locker(torrent_buffer_lock);

   if (update_torrent_buffer.empty()) {
      return;
   }
   torrent_queue.push(update_torrent_buffer);
   update_torrent_buffer.clear();

   if (torrent_queue.size() == 1 && t_active == false) {
      //boost::thread thread(&Mongodb::bt_do_flush_torrents, this);
   }
}

void Mongodb::bt_flush_snatches() {
   QMutexLocker locker(snatch_buffer_lock);

   if (update_snatch_buffer.empty()) {
      return;
   }
   snatch_queue.push(update_snatch_buffer);
   update_snatch_buffer.clear();
   if (snatch_queue.size() == 1 && s_active == false) {
      //boost::thread thread(&Mongodb::bt_do_flush_snatches, this);
   }
}

void Mongodb::bt_flush_peers() {
   QMutexLocker locker(peer_buffer_lock);

   // because xfu inserts are slow and ram is not infinite we need to
   // limit this queue's size
   if (peer_queue.size() >= 1000) {
      peer_queue.pop();
   }
   if (update_peer_buffer.empty()) {
      return;
   }

   /*if (peer_queue.size() == 0) {
      sql = "SET session sql_log_bin = 0";
      peer_queue.push(sql);
      sql.clear();
   }*/

   peer_queue.push(update_peer_buffer);
   update_peer_buffer.clear();
   if (peer_queue.size() == 1 && p_active == false) {
      //boost::thread thread(&Mongodb::bt_do_flush_peers, this);
   }
}

void Mongodb::bt_flush_peer_hist() {
   QMutexLocker locker(peer_hist_buffer_lock);

   if (update_peer_hist_buffer.empty()) {
      return;
   }

   /*if (peer_hist_queue.size() == 0) {
      sql = "SET session sql_log_bin = 0";
      peer_hist_queue.push(sql);
      sql.clear();
   }*/

   peer_hist_queue.push(update_peer_hist_buffer);
   update_peer_hist_buffer.clear();
   if (peer_hist_queue.size() == 1 && hist_active == false) {
      //boost::thread thread(&Mongodb::bt_do_flush_peer_hist, this);
   }
}

void Mongodb::bt_flush_tokens() {
   std::string sql;
   QMutexLocker locker(user_token_lock);

   if (update_token_buffer.empty()) {
      return;
   }
   token_queue.push(update_token_buffer);
   update_token_buffer.clear();
   if (token_queue.size() == 1 && tok_active == false) {
      //boost::thread(&Mongodb::bt_do_flush_tokens, this);
   }
}

void Mongodb::bt_do_flush_users() {
   u_active = true;
   mongo::DBClientConnection c;
  // c.connect(server);
   while (user_queue.size() > 0) {
      QVector<mongo_query> qv = user_queue.front();
      while(!qv.empty())
      {
         mongo_query q = qv.back();
         //c.update(db + q.table, q.query, q.data, true);
         if(!c.getLastError().empty())
         {
            std::cout << "User flush error: " << c.getLastError() << ". " << user_queue.size() << " don't remain." << std::endl;
            sleep(3);
            continue;
         }
         else
         {
            qv.pop_back();
            std::cout << "Users flushed (" << user_queue.size() << " remain)" << std::endl;
         }
      }
      QMutexLocker locker(user_buffer_lock);

      user_queue.pop();
   }
   u_active = false;
}

void Mongodb::bt_do_flush_torrents() {
   t_active = true;
   mongo::DBClientConnection c;
   //c.connect(server);
   while (torrent_queue.size() > 0) {
      QVector<mongo_query> qv = torrent_queue.front();
      while(!qv.empty())
      {
         mongo_query q = qv.back();
         //c.update(db + q.table, q.query, q.data, true);

         if(!c.getLastError().empty())
         {
            std::cout << "Torrent flush error: " << c.getLastError() << ". " << torrent_queue.size() << " don't remain." << std::endl;
            sleep(3);
            continue;
         }
         else
         {
            qv.pop_back();
            std::cout << "Torrents flushed (" << torrent_queue.size() << " remain)" << std::endl;
         }
      }
      QMutexLocker locker(torrent_buffer_lock);

      torrent_queue.pop();
   }
   t_active = false;
}

void Mongodb::bt_do_flush_peers() {
   p_active = true;
   mongo::DBClientConnection c;
   //c.connect(server);
   while (peer_queue.size() > 0) {
      QVector<mongo_query> qv = peer_queue.front();
      while(!qv.empty())
      {
         mongo_query q = qv.back();
         //c.update(db + q.table, q.query, q.data, true);

         if(!c.getLastError().empty())
         {
            std::cout << "Peer flush error: " << c.getLastError() << ". " << peer_queue.size() << " don't remain." << std::endl;
            sleep(3);
            continue;
         }
         else
         {
            qv.pop_back();
            std::cout << "Peer flushed (" << peer_queue.size() << " remain)" << std::endl;
         }
      }
      QMutexLocker locker(peer_buffer_lock);

      peer_queue.pop();
   }
   p_active = false;
}

void Mongodb::bt_do_flush_peer_hist() {
   hist_active = true;
   mongo::DBClientConnection c;
   //c.connect(server);
   while (peer_hist_queue.size() > 0) {
      QVector<mongo_query> qv = peer_hist_queue.front();
      while(!qv.empty())
      {
         mongo_query q = qv.back();
        // c.update(db + q.table, q.query, q.data, true);

         if(!c.getLastError().empty())
         {
            std::cout << "Peer hist flush error: " << c.getLastError() << ". " << peer_hist_queue.size() << " don't remain." << std::endl;
            sleep(3);
            continue;
         }
         else
         {
            qv.pop_back();
            std::cout << "Peer hist flushed (" << peer_hist_queue.size() << " remain)" << std::endl;
         }
      }
      QMutexLocker locker(peer_hist_buffer_lock);

      peer_hist_queue.pop();
   }
   hist_active = false;
}

void Mongodb::bt_do_flush_snatches() {
   s_active = true;
   mongo::DBClientConnection c;
   //c.connect(server);
   while (snatch_queue.size() > 0) {
      QVector<mongo_query> qv = snatch_queue.front();
      while(!qv.empty())
      {
         mongo_query q = qv.back();
        // c.update(db + q.table, q.query, q.data, true);

         if(!c.getLastError().empty())
         {
            std::cout << "Snatch flush error: " << c.getLastError() << ". " << snatch_queue.size() << " don't remain." << std::endl;
            sleep(3);
            continue;
         }
         else
         {
            qv.pop_back();
            std::cout << "Snatch flushed (" << snatch_queue.size() << " remain)" << std::endl;
         }
      }
      QMutexLocker locker(snatch_buffer_lock);

      snatch_queue.pop();
   }
   s_active = false;
}

void Mongodb::bt_do_flush_tokens() {
   tok_active = true;
   mongo::DBClientConnection c;
   //c.connect(server);
   while (token_queue.size() > 0) {
      QVector<mongo_query> qv = token_queue.front();
      while(!qv.empty())
      {
         mongo_query q = qv.back();
        // c.update(db + q.table, q.query, q.data, true);

         if(!c.getLastError().empty())
         {
            std::cout << "Token flush error: " << c.getLastError() << ". " << token_queue.size() << " don't remain." << std::endl;
            sleep(3);
            continue;
         }
         else
         {
            qv.pop_back();
            std::cout << "Token flushed (" << token_queue.size() << " remain)" << std::endl;
         }
      }
      QMutexLocker locker(user_token_lock);

      token_queue.pop();
   }
   tok_active = false;
}


