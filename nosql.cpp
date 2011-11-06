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


bo Nosql::Find(QString a_document, const bo &datas)
{
    qDebug() << "Nosql::Find";
    QString tmp;
    tmp.append(this->m_database).append(".").append(a_document);

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
        std::cout << "caught on find into " << m_server.toAscii().data() << "." << a_document.toAscii().data() << " : " << e.what() << std::endl;
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




bo Nosql::CreateHost(bo &payload, const bo &data, const be &user_id)
{
    std::cout << "Nosql::CreateHost" << std::endl;
    std::cout << "data : " << data << std::endl;

    QString tmp;
    tmp.append(m_database).append(".hosts");

    bo l_bo_host;
    bob l_bob_host;
    bob l_bob_host_cpu;
    bob l_bob_host_ram;
    bob l_bob_host_network;

    l_bob_host << mongo::GENOID;
    l_bob_host.append(data.getField("uuid"));
    l_bob_host.append(data.getField("pub_uuid"));
    //l_bob_host.append(user_id.getField("user_id"));
    l_bob_host << "user_id" << user_id;
    l_bob_host.append(data.getField("created_at"));
    l_bob_host << "updated_at" << data.getField("created_at");
    l_bob_host << "public" << payload["public"].boolean();
    l_bob_host << "blocked" << false;
    l_bob_host << "host_type" << payload["device"];


    l_bob_host << "os_version_number" << payload["sysinfo"]["version"];
    l_bob_host << "patch_level" << payload["sysinfo"]["patch_level"];
    l_bob_host << "architecture" << payload["sysinfo"]["architecture"];


    // find the last inserted row to increase counter
    auto_ptr<DBClientCursor> cursor = this->m_mongo_connection.query(tmp.toAscii().data(), mongo::Query(BSONObj()).sort("_id", -1),1);
    bo last_host = cursor->more() ? cursor->nextSafe().copy() : BSON("counter" << 0);
    l_bob_host << "counter" << last_host.getField("counter").Int() + 1;


    // embedded CPU
    l_bob_host_cpu << "vendor" << payload["cpu_hardware"]["vendor"];
    l_bob_host_cpu << "model" << payload["cpu_hardware"]["model"];
    l_bob_host_cpu << "mhz" << payload["cpu_hardware"]["mhz"];
    l_bob_host_cpu << "cache_size" << payload["cpu_hardware"]["cache_size"];
    l_bob_host_cpu << "number" << payload["cpu_hardware"]["number"];
    l_bob_host_cpu << "total_cores" << payload["cpu_hardware"]["total_cores"];
    l_bob_host_cpu << "total_sockets" << payload["cpu_hardware"]["total_sockets"];
    l_bob_host_cpu << "cores_per_socket" << payload["cpu_hardware"]["cores_per_socket"];


    // embedded RAM
    l_bob_host_ram << "mem_ram" << payload["memory"]["mem_ram"];
    l_bob_host_ram << "mem_total" << payload["memory"]["mem_total"];
    l_bob_host_ram << "swap_total" << payload["memory"]["swap_total"];

    // embedded NETWORK
    l_bob_host_network << "hostname" << payload["network"]["hostname"];
    l_bob_host_network << "domain_name" << payload["network"]["domain_name"];
    l_bob_host_network << "default_gateway" << payload["network"]["default_gateway"];
    l_bob_host_network << "primary_dns" << payload["network"]["primary_dns"];
    l_bob_host_network << "secondary_dns" << payload["network"]["secondary_dns"];
    l_bob_host_network << "primary_interface" << payload["network"]["primary_interface"];
    l_bob_host_network << "primary_addr" << payload["network"]["primary_addr"];

    l_bob_host << "cpu" << l_bob_host_cpu.obj();
    l_bob_host << "ram" << l_bob_host_ram.obj();
    l_bob_host << "network" << l_bob_host_network.obj();

    l_bo_host = l_bob_host.obj();

    std::cout << "HOST : " << l_bo_host.toString() << std::endl;
    Insert("hosts", l_bo_host);


    return l_bo_host;
}




bo Nosql::CreateOsystem(bo &payload, const bo &data)
{
    std::cout << "Nosql::CreateOsystem" << std::endl;

    bo l_bo_os;
    bob l_bob_os;


    l_bob_os << mongo::GENOID;
    l_bob_os.append(data.getField("created_at"));
    l_bob_os << "updated_at" << data.getField("created_at");
    l_bob_os << "name" << payload["sysinfo"]["name"];
    l_bob_os << "vendor" << payload["sysinfo"]["vendor"];
    //l_bob_os << "vendor_version" << r_hash["vendor_version"].toString().toLower().toStdString();
    //l_bob_os << "vendor_code_name" << r_hash["vendor_code_name"].toString().toLower().toStdString();
    l_bob_os << "description" << payload["sysinfo"]["description"];
    l_bob_os << "os_base" << payload["sysinfo"]["os_base"];
    l_bob_os << "os_type" << payload["sysinfo"]["os_type"];
    l_bob_os << "hosts_number" << 1;

    l_bo_os = l_bob_os.obj();

    std::cout << "OSYSTEM : " << l_bo_os.toString() << std::endl;
    Insert("osystems", l_bo_os);

    return l_bo_os;
}




bo Nosql::CreateOsversion(bo &data)
{
    std::cout << "Nosql::CreateOsversion" << std::endl;

    bo l_bo_os;
    bob l_bob_os;


    //l_bob_os = data;
    //l_bob_os << "hosts_number" << 1;



    //l_bo_os = data.obj();

    std::cout << "OS_VERSION : " << data.toString() << std::endl;
    Insert("os_versions", data);


    //std::cout << "Nosql::XMLtoHash : l_hash = " << l_hash.count() << std::endl;
    return data;
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
