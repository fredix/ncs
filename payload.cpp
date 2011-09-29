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


#include "payload.h"


Payload::~Payload()
{}


Payload::Payload(Nosql& a, QObject *parent) : nosql_(a), QObject(parent)
{
    std::cout << "Payload::construct" << std::endl;
}


void Payload::s_job_receive(bo data)
{

    std::cout << "Payload::s_job_receive" << std::endl;

    QDomDocument l_xml_datas;
    Hash l_hash;
    Hash r_hash;

    bo msg = data;

    be uuid = msg.getField("uuid");
    be created_at = msg.getField("created_at");
    be gfs_id = msg.getField("_id");


    be qname = msg.getField("action");
    std::cout << "action : " << qname.toString() << std::endl;


    QString queue_name = QString::fromAscii (qname.valuestr(), -1);

    std::cout << "Payload::s_job_receive, queue_name : " << queue_name.toAscii().data() << std::endl;

    cout << uuid.toString(false) << endl;
    cout << created_at.jsonString(TenGen) << endl;
    cout << created_at.jsonString(JS) << endl;
    cout << created_at.jsonString(Strict) << endl;
    cout << gfs_id.jsonString(TenGen) << endl;


    cout << "UUID BEFORE EXTRACT : " << uuid.toString() << endl;
    l_xml_datas = nosql_.ExtractXML(gfs_id);
    cout << "UUID AFTER EXTRACT : " << uuid.toString() << endl;

    QDomNode n = l_xml_datas.firstChild();
    if (n.isNull()) {
        std::cout << "Payload::s_job_receive, XML FROM GRIDFS IS EMPTY" << std::endl;
        return;
    }

    QDomElement l_xml = l_xml_datas.documentElement();
    l_hash = nosql_.XMLtoHash(l_xml);


    r_hash.insert("bo", QString::fromStdString(data.jsonString(TenGen)));

    std::cout << "hash: " << l_hash.count() << std::endl;

    qDebug() << "hash value : " << l_hash["cpu_hardware"].toHash()["cpu_mhz"];
    qDebug() << "hash value : " << l_hash["version"];
    qDebug() << "hash value : " << l_hash["process"].toHash()["processus"].toHash()["state_name"].toString();


    if (queue_name=="dispatcher.update")
    {
        qDebug() << "DISPATCHER UPDATE";
    }

    else
    {
        qDebug() << "DISPATCHER : " << queue_name;
    }



    if (queue_name=="dispatcher.update")
    {
        std::cout << "QUEUE : DISPATCHER UPDATE" << std::endl;

        bo host = nosql_.Find("hosts", uuid.wrap());
        be host_id = host.getField("_id");
        be os_version = host.getField("os_version");


        std::cout << "Payload::s_job_receive, _id : " << host.getField("_id") << std::endl;

        std::cout << "Payload::s_job_receive, FIND : " << host.getField("_id").eoo() << std::endl;


        if (host_id.eoo())
        {
            std::cout << "Payload::s_job_receive : Host not found : return" << std::endl;
            return;
        }
        else
        {
            std::cout << "os_version : " << os_version.toString() << std::endl;
        }


        if (host.getField("public").boolean() != QVariant(l_hash["public"]).toBool() )
        {
            bo bo_public = BSON("public" << QVariant(l_hash["public"]).toBool());
            std::cout << "Payload::s_job_receive, BO PUBLIC : " << bo_public << std::endl;
            nosql_.Update("hosts", host_id.wrap(), bo_public);
        }


    }
    else if (queue_name=="dispatcher.create")
    {
        std::cout << "QUEUE : DISPATCHER ADD" << std::endl;

        be email = msg.getField("email");
        bo user = nosql_.Find("users", email.wrap());

        be user_id = user.getField("_id");
        //bo bo_user_id = BSON("user_id" << user.getField("_id"));
        be user_hosts_number = user.getField("hosts_number");

        std::cout << "Payload::s_job_receive, user city : " << user.getField("city") << std::endl;
        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;


        std::cout << "Payload::s_job_receive, before create host" << std::endl;
        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;

        bo host = nosql_.CreateHost(l_hash, data, user_id);
        be host_id = host.getField("_id");

        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;
        std::cout << "Payload::s_job_receive, after create host" << std::endl;


        //bo os = BSON("vendor" << l_hash["vendor"].toString().toLower().toStdString() << "vendor_version" << l_hash["vendor_version"].toString().toLower().toStdString());
        bo os = BSON("vendor" << l_hash["vendor"].toString().toLower().toStdString());

        //bo os = BSON("vendor" << "prot" << "vendor_version" << l_hash["vendor_version"].toString().toStdString());

        std::cout << "Payload::s_job_receive, OS : " << os.toString() << std::endl;

        bo osystem = nosql_.Find("osystems", os);

        std::cout << "Payload::s_job_receive, OSYSTEM : " << osystem.toString() << std::endl;
        std::cout << "Payload::s_job_receive, OSYSTEM NB : " << osystem.nFields() << std::endl;
        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;


        // OS not found, so we add it
        if (osystem.nFields() == 0)
        {
            std::cout << "Payload::s_job_receive, OSYSTEM NOT FOUND" << std::endl;
            osystem = nosql_.CreateOsystem(l_hash, data);
        }

        be osystem_id = osystem.getField("_id");
        be osystem_hosts_number = osystem.getField("hosts_number");
        bo hosts_number = BSON("hosts_number" << osystem_hosts_number.numberInt() + 1);
        std::cout << "Payload::s_job_receive, OSYSTEM's HOSTS NUMBER : " << hosts_number.toString() << std::endl;
        nosql_.Update("osystems", osystem_id.wrap(), hosts_number);


        bo bo_os = BSON("osystem_id" << osystem.getField("_id"));





        std::cout << "Payload::s_job_receive, BO OS : " << bo_os << std::endl;

        std::cout << "Payload::s_job_receive, BEFORE HOST UPDATE : " << bo_os.toString() << std::endl;
        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;

        nosql_.Update("hosts", host_id.wrap(), bo_os);

        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;
        std::cout << "Payload::s_job_receive, AFTER HOST UPDATE : " << bo_os.toString() << std::endl;





        bo version = BSON("osystem_id" << osystem.getField("_id") << "vendor_version" << l_hash["vendor_version"].toString().toLower().toStdString());
        bo os_version = nosql_.Find("os_versions", version);

        if (os_version.nFields() == 0)
        {
            std::cout << "Payload::s_job_receive, OSVERSION NOT FOUND, version : " << version.toString() << std::endl;

            bo data_version = BSON(mongo::GENOID <<
                                    "created_at" << data.getField("created_at") <<
                                    "osystem_id" << osystem.getField("_id") <<
                                    "vendor_version" << l_hash["vendor_version"].toString().toLower().toStdString() <<
                                    "vendor_code_name" << l_hash["vendor_code_name"].toString().toLower().toStdString() <<
                                    "hosts_number" << 1);

            os_version = nosql_.CreateOsversion(data_version);
        }

        be os_version_id = os_version.getField("_id");
        be os_version_hosts_number = os_version.getField("hosts_number");
        hosts_number = BSON("hosts_number" << os_version_hosts_number.numberInt() + 1);
        std::cout << "Payload::s_job_receive, OS_VERSION's HOSTS NUMBER : " << hosts_number.toString() << std::endl;
        nosql_.Update("os_versions", os_version_id.wrap(), hosts_number);



        bo bo_version = BSON("os_version_id" << os_version.getField("_id"));




        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;


        std::cout << "Payload::s_job_receive, BO OS : " << bo_os << std::endl;

        std::cout << "Payload::s_job_receive, BEFORE HOST UPDATE : " << bo_os.toString() << std::endl;
        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;

        nosql_.Update("hosts", host_id.wrap(), bo_version);

        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;
        std::cout << "Payload::s_job_receive, AFTER HOST UPDATE : " << bo_os.toString() << std::endl;



        // update host's profil_id
        std::cout << "profil : " << l_hash["profil"].toString().toStdString() << std::endl;
        bo profil_filter = BSON("context" << l_hash["profil"].toString().toLower().toStdString() << "user_id" << user_id);
        std::cout << "Payload::s_job_receive, profil_filter : " << profil_filter.toString() << std::endl;

        bo profil = nosql_.Find("profils", profil_filter);
        std::cout << "Payload::s_job_receive, profil : " << profil.toString() << std::endl;


        if (profil.nFields() == 0)
        {
            std::cout << "Payload::s_job_receive, profil not found" << std::endl;
            return;
            /************* TODO ***************/
            //profil = nosql_.CreateProfil(l_hash, data);
        }
        //std::cout << "Payload::s_job_receive, user profil nickname : " << profil.getField("nickname") << std::endl;

        bo bo_profil = BSON("profil_id" << profil.getField("_id"));

        std::cout << "Payload::s_job_receive, BEFORE HOST UPDATE, BO PROFIL : " << bo_profil.toString() << std::endl;
        nosql_.Update("hosts", host_id.wrap(), bo_profil);
        std::cout << "Payload::s_job_receive, AFTER HOST UPDATE : " << bo_profil.toString() << std::endl;




        // update user's hosts number

        std::cout << "Payload::s_job_receive, BEFORE UPDATE USER's HOSTS NUMBER" << std::endl;

        hosts_number = BSON("hosts_number" << user_hosts_number.numberInt() + 1);

        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;
        std::cout << "Payload::s_job_receive, USER's HOSTS NUMBER : " << hosts_number.toString() << std::endl;

        nosql_.Update("users", user_id.wrap(), hosts_number);

        std::cout << "Payload::s_job_receive, AFTER UPDATE USER's HOSTS NUMBER" << std::endl;
    }




    /*
      Serializing datas and send to workers
    */

    qDebug() << "Serializing datas and send to workers";

    QByteArray ar;
    //QByteArray al;
    QString l_payload;
    //QString deserialize;



    if (l_hash["activated_cpu"].toBool())
    {

        //Serializing
        QDataStream out(&ar,QIODevice::WriteOnly);   // write the data
        r_hash.insert("xml", l_hash["cpu_usage"].toHash());
        //out << l_hash["cpu_usage"].toHash();
        out << r_hash;
        l_payload = ar.toBase64();

        qDebug() << "emit payload_cpu(l_payload)";
        emit payload_cpu(l_payload);
    }


    if (l_hash["activated_load"].toBool())
    {
        //Serializing
        QDataStream out1(&ar,QIODevice::WriteOnly);   // write the data
        //out1 << l_hash["load"].toHash();
        r_hash.insert("xml", l_hash["load"].toHash());
        //out << l_hash["cpu_usage"].toHash();
        out1 << r_hash;
        l_payload = ar.toBase64();

        qDebug() << "emit payload_load(l_payload)";
        emit payload_load(l_payload);
    }

    if (l_hash["activated_network"].toBool())
    {
        //Serializing
        QDataStream out2(&ar,QIODevice::WriteOnly);   // write the data
        //out2 << l_hash["network"].toHash();
        r_hash.insert("xml", l_hash["network"].toHash());
        out2 << r_hash;
        l_payload = ar.toBase64();

        qDebug() << "emit payload_network(l_payload)";
        emit payload_network(l_payload);
    }

    if (l_hash["activated_memory"].toBool())
    {
        //Serializing
        QDataStream out3(&ar,QIODevice::WriteOnly);   // write the data
        r_hash.insert("xml", l_hash["memory"].toHash());
        //out3 << l_hash["memory"].toHash();
        out3 << r_hash;
        l_payload = ar.toBase64();

        qDebug() << "emit payload_memory(l_payload)";
        emit payload_memory(l_payload);
    }

    if (l_hash["activated_uptime"].toBool())
    {
        //Serializing
        QDataStream out4(&ar,QIODevice::WriteOnly);   // write the data
        r_hash.insert("xml", l_hash["uptime"].toHash());
        //out4 << l_hash["uptime"].toHash();
        out4 << r_hash;
        l_payload = ar.toBase64();

        qDebug() << "emit payload_uptime(l_payload)";
        emit payload_uptime(l_payload);
    }


    if (l_hash["activated_process"].toBool())
    {
        //Serializing
        QDataStream out5(&ar,QIODevice::WriteOnly);   // write the data        

        QString tmp = QString::fromStdString(msg.jsonString(TenGen));
        out5 << tmp;
        l_payload = ar.toBase64();


        qDebug() << "emit payload_process(l_payload)";
        //emit payload_process(data.jsonString(TenGen));
        emit payload_process(l_payload);
    }


    /*
    Deserializing
    al = QByteArray::fromBase64(serialize.toAscii());

    //qDebug() << "al value: " << al.toBase64();


    QDataStream in(&al,QIODevice::ReadOnly);   // read the data serialized from the file
    in >> r_hash;

    qDebug() << "r_hash value: " << r_hash.value("version").toString();

    */
}
