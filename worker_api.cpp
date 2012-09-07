/****************************************************************************
**   ncs is the backend's server of nodecast
**   Copyright (C) 2010-2012  Frédéric Logier <frederic@logier.org>
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

#include "worker_api.h"

Worker_api::Worker_api()
{
    nosql_ = Nosql::getInstance_front();
    zeromq_ = Zeromq::getInstance ();
    z_message = new zmq::message_t(2);
    z_message_publish = new zmq::message_t(2);

    z_receive_api = new zmq::socket_t (*zeromq_->m_context, ZMQ_PULL);
    uint64_t hwm = 50000;
    z_receive_api->setsockopt(ZMQ_HWM, &hwm, sizeof (hwm));
    z_receive_api->bind("tcp://*:5555");


    int socket_receive_fd;
    size_t socket_size = sizeof(socket_receive_fd);
    z_receive_api->getsockopt(ZMQ_FD, &socket_receive_fd, &socket_size);

    qDebug() << "RES getsockopt : " << "res" <<  " FD : " << socket_receive_fd << " errno : " << zmq_strerror (errno);

    check_payload = new QSocketNotifier(socket_receive_fd, QSocketNotifier::Read, this);
    connect(check_payload, SIGNAL(activated(int)), this, SLOT(receive_payload()), Qt::DirectConnection);


    /********* PUB / SUB *************/
    z_publish_api = new zmq::socket_t (*zeromq_->m_context, ZMQ_PUB);
    uint64_t pub_hwm = 50000;
    z_publish_api->setsockopt(ZMQ_HWM, &pub_hwm, sizeof (pub_hwm));
    z_publish_api->bind("tcp://*:5557");
    /*********************************/


    z_push_api = new zmq::socket_t(*zeromq_->m_context, ZMQ_PUSH);
    z_push_api->bind("ipc:///tmp/nodecast/workers");
}


void Worker_api::pubsub_payload(bson::bo l_payload)
{
    std::cout << "Worker_api::pubsub_payload : " << l_payload << std::endl;
    BSONElement from = l_payload.getField("worker_name");
    BSONElement dest = l_payload.getFieldDotted("payload.dest");
    BSONElement data_type = l_payload.getFieldDotted("payload.data_type");
    QString payload = QString::fromStdString(dest.str()) + " @";
    payload.append(QString::fromStdString(l_payload.getFieldDotted("payload.data").str()));

    std::cout << "payload send : " << payload.toStdString() << std::endl;


    /** ALL KIND OF WORKERS ARE CONNECTED TO THE PUB SOCKET
        SO ZEROMQ CANNOT CHECK IF A DEST WORKER RECEIVE OR NOT THE PAYLOAD.
        SO, I MUST STORE ALL PAYLOAD THROUGH THE PUB/SUB SOCKET INTO MONGODB,
        A WORKER CAN RETREIVE LATER A PAYLOAD (replay_pubsub_payload)
    **/


    QDateTime timestamp = QDateTime::currentDateTime();
    BSONObj t_payload = BSON(GENOID <<                                                          
                             "from" << from.str() <<
                             "dest" << dest.str() <<
                             "data_type" << data_type.str() <<
                             "timestamp" << timestamp.toTime_t() <<
                             "data" << l_payload.getFieldDotted("payload.data").str());
    nosql_->Insert("pubsub_payloads", t_payload);

    /****** PUBLISH API PAYLOAD *******/
    qDebug() << "Worker_api::publish_payload PUBLISH PAYLOAD";

    QByteArray s_payload = payload.toAscii();

    z_message_publish->rebuild(s_payload.size()+1);
    memcpy(z_message_publish->data(), s_payload.constData(), s_payload.size()+1);
    z_publish_api->send(*z_message_publish);
    /************************/
}


/********** RESEND PAYLOAD PAYLOAD ************/
void Worker_api::replay_pubsub_payload(bson::bo a_payload)
{
    std::cout << "Worker_api::replay_pubsub_payload : " << a_payload << std::endl;

    QString dest = QString::fromStdString(a_payload.getFieldDotted("payload.dest").str());

    if ("self" == dest)
    {
        QString node_uuid = QString::fromStdString(a_payload.getField("node_uuid").str());
        QString worker_name = QString::fromStdString(a_payload.getField("worker_name").str());
        QString worker_uuid = QString::fromStdString(a_payload.getField("uuid").str());

        dest = node_uuid + "." + worker_name + "." + worker_uuid + " @";
    }
    else dest.append(" @");

    BSONObj search = BSON("dest" << a_payload.getFieldDotted("payload.from").str() << "data_type" << a_payload.getFieldDotted("payload.data_type"));
    QList <BSONObj> pubsub_payloads_list = nosql_->FindAll("pubsub_payloads", search);

    foreach (BSONObj pubsub_payload, pubsub_payloads_list)
    {
        std::cout << "pubsub_payload : " << pubsub_payload.getField("data") << std::endl;

        QString payload = dest;
        payload.append(QString::fromStdString(pubsub_payload.getField("data").str()));

        std::cout << "Worker_api::replay_pubsub_payload payload send : " << payload.toStdString() << std::endl;


        /****** REPLAY API PAYLOAD *******/
        qDebug() << "Worker_api::publish_payload REPLAY PAYLOAD";

        QByteArray s_payload = payload.toAscii();

        z_message_publish->rebuild(s_payload.size()+1);
        memcpy(z_message_publish->data(), s_payload.constData(), s_payload.size()+1);
        z_publish_api->send(*z_message_publish);
        /************************/
    }
}

/********** CREATE PAYLOAD ************/
void Worker_api::receive_payload()
{
    check_payload->setEnabled(false);

    std::cout << "Worker_api::receive_payload" << std::endl;

    qint32 events = 0;
    std::size_t eventsSize = sizeof(events);

    z_receive_api->getsockopt(ZMQ_EVENTS, &events, &eventsSize);


    std::cout << "Worker_api::receive_payload ZMQ_EVENTS : " <<  events << std::endl;



    if (events & ZMQ_POLLIN)
    {
        std::cout << "Worker_api::receive_payload ZMQ_POLLIN" <<  std::endl;

        while (true)
        {
            zmq::message_t request;

            bool res = z_receive_api->recv(&request, ZMQ_NOBLOCK);
            if (!res && zmq_errno () == EAGAIN) break;

            std::cout << "Worker_api::receive_payload received request: [" << (char*) request.data() << "]" << std::endl;

            char *plop = (char*) request.data();
            if (strlen(plop) == 0) {
                std::cout << "Worker_api::receive_payload STRLEN received request 0" << std::endl;
                break;
            }



            BSONObj payload;
            try {
                payload = BSONObj((char*)request.data());

                if (!payload.isValid() || payload.isEmpty())
                {
                    qDebug() << "Worker_api::receive_payload PAYLOAD INVALID !!!";
                    return;
                }

            }
            catch (mongo::MsgAssertionException &e)
            {
                std::cout << "error on data : " << payload << std::endl;
                std::cout << "error on data BSON : " << e.what() << std::endl;
                break;
            }


            std::cout << "Worker_api::receive PAYLOAD : " << payload << std::endl;


            QString payload_action = QString::fromStdString(payload.getFieldDotted("payload.action").str());


            if (payload_action == "publish")
            {
                std::cout << "RECEIVE PUBLISH : " << payload << std::endl;

                pubsub_payload(payload.copy());
            }
            else if (payload_action == "replay")
            {
                std::cout << "RECEIVE REPLAY : " << payload << std::endl;

                replay_pubsub_payload(payload.copy());
            }
            else if (payload_action == "create")
            {
                std::cout << "Worker_api::payload CREATE PAYLOAD : " << payload <<std::endl;

                QDateTime timestamp = QDateTime::currentDateTime();

                BSONElement data_type = payload.getFieldDotted("payload.data_type");
                BSONElement datas = payload.getFieldDotted("payload.data");
                BSONElement node_uuid = payload.getField("node_uuid");
                BSONElement node_password = payload.getField("node_password");
                BSONElement workflow_uuid = payload.getFieldDotted("payload.workflow_uuid");
                std::cout << "DATA : " << datas << std::endl;
                std::cout << "NODE UUID : " << node_uuid << std::endl;
                std::cout << "NODE PASSWORD : " << node_password << std::endl;
                std::cout << "workflow_uuid : " << workflow_uuid << std::endl;


                if (datas.size() == 0)
                {
                    std::cout << "ERROR : DATA EMPTY" << std::endl;
                    return;
                }

                if (node_uuid.size() == 0)
                {
                    std::cout << "ERROR : NODE UUID EMPTY" << std::endl;
                    return;
                }

                if (node_password.size() == 0)
                {
                    std::cout << "ERROR : NODE PASSWORD EMPTY" << std::endl;
                    return;
                }

                if (workflow_uuid.size() == 0)
                {
                    std::cout << "ERROR : WORKFLOW EMPTY" << std::endl;
                    return;
                }


                BSONObj workflow_search = BSON("uuid" <<  workflow_uuid.str());
                BSONObj workflow = nosql_->Find("workflows", workflow_search);
                if (workflow.nFields() == 0)
                {
                    std::cout << "ERROR : WORKFLOW NOT FOUND" << std::endl;
                    return;
                }


                BSONObj auth = BSON("node_uuid" << node_uuid.str() << "node_password" << node_password.str());
                BSONObj node = nosql_->Find("nodes", auth);
                if (node.nFields() == 0)
                {
                    std::cout << "ERROR : NODE NOT FOUND" << std::endl;
                    return;
                }



                BSONObjBuilder payload_builder;
                payload_builder.genOID();
                payload_builder.append("action", "create");
                payload_builder.append("timestamp", timestamp.toTime_t());
                payload_builder.append("data_type", data_type.str());
                payload_builder.append("node_uuid", node_uuid.str());
                payload_builder.append("node_password", node_password.str());
                payload_builder.append("workflow_uuid", workflow_uuid.str());



            //bo node_search = BSON("_id" << user.getField("_id") <<  "nodes.uuid" << node_uuid.toStdString());
                BSONObj user_search = BSON("_id" << node.getField("user_id"));
                BSONObj user_nodes = nosql_->Find("users", user_search);


                if (user_nodes.nFields() == 0)
                {
                    std::cout << "ERROR : USER NOT FOUND" << std::endl;
                    return;
                }



                BSONObj nodes = user_nodes.getField("nodes").Obj();

                list<be> list_nodes;
                nodes.elems(list_nodes);
                list<be>::iterator i;

                /********   Iterate over each user's nodes   *******/
                /********  find node with uuid and set the node id to payload collection *******/
                for(i = list_nodes.begin(); i != list_nodes.end(); ++i) {
                    BSONObj l_node = (*i).embeddedObject ();
                    be node_id;
                    l_node.getObjectID (node_id);

                    //std::cout << "NODE1 => " << node_id.OID()   << std::endl;
                    //std::cout << "NODE2 => " << node_uuid.toStdString() << std::endl;

                    if (node_uuid.str().compare(l_node.getField("uuid").str()) == 0)
                    {
                        payload_builder.append("node_id", node_id.OID());
                        break;
                    }
                }


                QUuid payload_uuid = QUuid::createUuid();
                QString str_payload_uuid = payload_uuid.toString().mid(1,36);
                payload_builder.append("payload_uuid", str_payload_uuid.toStdString());

                int counter = nosql_->Count("payloads");
                payload_builder.append("counter", counter + 1);


                /*QFile zfile("/tmp/in.dat");
                zfile.open(QIODevice::WriteOnly);
                zfile.write(requestContent);
                zfile.close();*/

                BSONObj t_payload = payload.getField("payload").Obj();

                if (t_payload.hasField("gridfs") && t_payload.getField("gridfs").Bool() == false)
                {
                    payload_builder.append("gridfs", false);
                    payload_builder.append("data", datas.valuestr());
                }
                else
                {
                    BSONObj gfs_file_struct = nosql_->WriteFile(data_type.str(), datas.valuestr(), datas.objsize());
                    if (gfs_file_struct.nFields() == 0)
                    {
                        qDebug() << "write on gridFS failed !";
                        break;
                    }


                    std::cout << "writefile : " << gfs_file_struct << std::endl;
                    //std::cout << "writefile id : " << gfs_file_struct.getField("_id") << " date : " << gfs_file_struct.getField("uploadDate") << std::endl;

                    be uploaded_at = gfs_file_struct.getField("uploadDate");
                    be filename = gfs_file_struct.getField("filename");
                    be length = gfs_file_struct.getField("length");

                    std::cout << "uploaded : " << uploaded_at << std::endl;


                    payload_builder.append("created_at", uploaded_at.date());
                    payload_builder.append("filename", filename.str());
                    payload_builder.append("length", length.numberLong());
                    payload_builder.append("gfs_id", gfs_file_struct.getField("_id").OID());
                    payload_builder.append("gridfs", true);
                }
                BSONObj s_payload = payload_builder.obj();

                nosql_->Insert("payloads", s_payload);
                std::cout << "payload inserted" << s_payload << std::endl;

                /********* CREATE SESSION **********/
                QUuid session_uuid = QUuid::createUuid();
                QString str_session_uuid = session_uuid.toString().mid(1,36);

                BSONObjBuilder session_builder;
                session_builder.genOID();
                session_builder.append("uuid", str_session_uuid.toStdString());
                session_builder.append("payload_id", s_payload.getField("_id").OID());
                session_builder.append("workflow_id", workflow.getField("_id").OID());
                session_builder.append("start_timestamp", timestamp.toTime_t());
                BSONObj session = session_builder.obj();
                nosql_->Insert("sessions", session);
                /***********************************/
                std::cout << "session inserted : " << session << std::endl;

                BSONObj l_payload = BSON("action" << "create" << "session_uuid" << str_session_uuid.toStdString() << "timestamp" << timestamp.toTime_t());

                std::cout << "PUSH WORKER API PAYLOAD : " << l_payload << std::endl;

                /****** PUSH API PAYLOAD *******/
                qDebug() << "PUSH WORKER CREATE PAYLOAD";
                z_message->rebuild(l_payload.objsize());
                memcpy(z_message->data(), (char*)l_payload.objdata(), l_payload.objsize());
                //z_push_api->send(*z_message, ZMQ_NOBLOCK);
                z_push_api->send(*z_message);
                /************************/

            }
            else
            {
                // WORKER RESPONSE
                std::cout << "RECEIVE ACTION : " << payload_action.toStdString() << std::endl;

                BSONObjBuilder b_payload;
                b_payload.append(payload.getFieldDotted("payload.name"));
                b_payload.append(payload.getFieldDotted("payload.action"));
                b_payload.append(payload.getFieldDotted("payload.timestamp"));
                b_payload.append(payload.getFieldDotted("payload.session_uuid"));

                /*** ACTION TERMINATE ***/
                BSONObj tmp = payload.getField("payload").Obj();

                if (tmp.hasField("data")) b_payload.append(payload.getFieldDotted("payload.data"));
                if (tmp.hasField("exitcode")) b_payload.append(payload.getFieldDotted("payload.exitcode"));
                if (tmp.hasField("exitstatus")) b_payload.append(payload.getFieldDotted("payload.exitstatus"));

                BSONObj l_payload = b_payload.obj();

                std::cout << "PAYLOAD FORWARD : " << l_payload << std::endl;


                /****** PUSH API PAYLOAD *******/
                qDebug() << "PUSH WORKER NEXT PAYLOAD";
                z_message->rebuild(l_payload.objsize());
                memcpy(z_message->data(), (char*)l_payload.objdata(), l_payload.objsize());
                //z_push_api->send(*z_message, ZMQ_NOBLOCK);
                z_push_api->send(*z_message);
                /************************/
            }
        }
    }

    check_payload->setEnabled(true);
}




Worker_api::~Worker_api()
{
    z_push_api->close();
}
