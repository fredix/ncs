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

    bo l_json_datas;

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
    l_json_datas = nosql_.ExtractJSON(gfs_id);
    cout << "UUID AFTER EXTRACT : " << uuid.toString() << endl;


    std::cout << "Payload::s_job_receive, JSON sysinfo : " << l_json_datas["sysinfo"] << std::endl;

    if (l_json_datas.nFields() == 0) {
        std::cout << "Payload::s_job_receive, JSON FROM GRIDFS IS EMPTY" << std::endl;
        return;
    }


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

        //if (host.getField("public").boolean() != QVariant(l_hash["public"]).toBool() )
        if (host.getField("public").boolean() != l_json_datas.getField("public").boolean() )
        {   
            bo bo_public = BSON("public" << l_json_datas.getField("public"));
            std::cout << "Payload::s_job_receive, BO PUBLIC : " << bo_public << std::endl;
            nosql_.Update("hosts", host_id.wrap(), l_json_datas.getField("public").wrap());
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

        bo host = nosql_.CreateHost(l_json_datas, data, user_id);
        be host_id = host.getField("_id");

        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;
        std::cout << "Payload::s_job_receive, after create host" << std::endl;


        //bo os = BSON("vendor" << l_hash["vendor"].toString().toLower().toStdString() << "vendor_version" << l_hash["vendor_version"].toString().toLower().toStdString());
        bo os = BSON("vendor" << l_json_datas["sysinfo"]["vendor"]);

        //bo os = BSON("vendor" << "prot" << "vendor_version" << l_hash["vendor_version"].toString().toStdString());

        std::cout << "Payload::s_job_receive, OS : " << os.toString() << std::endl;

        bo osystem = nosql_.Find("osystems", l_json_datas["sysinfo"]["vendor"].wrap());

        std::cout << "Payload::s_job_receive, OSYSTEM : " << osystem.toString() << std::endl;
        std::cout << "Payload::s_job_receive, OSYSTEM NB : " << osystem.nFields() << std::endl;
        std::cout << "Payload::s_job_receive, user id : " << user_id.toString() << std::endl;


        // OS not found, so we add it
        if (osystem.nFields() == 0)
        {
            std::cout << "Payload::s_job_receive, OSYSTEM NOT FOUND" << std::endl;
            osystem = nosql_.CreateOsystem(l_json_datas, data);
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





        //bo version = BSON("osystem_id" << osystem.getField("_id") << "vendor_version" << l_hash["vendor_version"].toString().toLower().toStdString());
        bo version = BSON("osystem_id" << osystem.getField("_id") << "vendor_version" << l_json_datas["sysinfo"]["vendor_version"]);


        bo os_version = nosql_.Find("os_versions", version);

        if (os_version.nFields() == 0)
        {
            std::cout << "Payload::s_job_receive, OSVERSION NOT FOUND, version : " << version.toString() << std::endl;

            bo data_version = BSON(mongo::GENOID <<
                                    "created_at" << data.getField("created_at") <<
                                    "osystem_id" << osystem.getField("_id") <<
                                    "vendor_version" << l_json_datas["sysinfo"]["vendor_version"] <<
                                    "vendor_code_name" << l_json_datas["sysinfo"]["vendor_code_name"] <<
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
        std::cout << "profil : " << l_json_datas["profil"] << std::endl;
        bo profil_filter = BSON("context" << l_json_datas["profil"] << "user_id" << user_id);
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
      Send payload to workers
    */

    qDebug() << "Serializing datas and send to workers";

    if (l_json_datas["cpu_usage"]["activated"].boolean())
    {
        qDebug() << "emit payload_cpu(l_payload)";

        bo payload = BSON("headers" << data << "cpu_usage" << l_json_datas["cpu_usage"]);
        emit payload_cpu(payload);
    }


    if (l_json_datas["load"]["activated"].boolean())
    {
        qDebug() << "emit payload_load(l_payload)";

        bo payload = BSON("headers" << data << "load" << l_json_datas["load"]);
        emit payload_load(payload);
    }

    if (l_json_datas["network"]["activated"].boolean())
    {
        qDebug() << "emit payload_network(l_payload)";

        bo payload = BSON("headers" << data << "network" << l_json_datas["network"]);
        emit payload_network(payload);
    }

    if (l_json_datas["memory"]["activated"].boolean())
    {
        qDebug() << "emit payload_memory(l_payload)";

        bo payload = BSON("headers" << data << "memory" << l_json_datas["memory"]);
        emit payload_memory(payload);
    }

    if (l_json_datas["uptime"]["activated"].boolean())
    {
        qDebug() << "emit payload_uptime(l_payload)";

        bo payload = BSON("headers" << data << "uptime" << l_json_datas["uptime"]);
        emit payload_uptime(payload);
    }


    if (l_json_datas["process"]["activated"].boolean())
    {
        qDebug() << "emit payload_process(l_payload)";

        bo payload = BSON("headers" << data << "process" << l_json_datas["process"]);
        emit payload_process(payload);
    }


}
