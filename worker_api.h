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

#ifndef ZEROMQ_API_H
#define ZEROMQ_API_H

#include "mongodb.h"
#include "zeromq.h"

class Worker_api : public QObject
{
    Q_OBJECT
public:
    Worker_api(QString basedirectory, QObject *parent = 0);
    ~Worker_api();


private:
    void replay_pubsub_payload(bson::bo l_payload);
    void get_ftp_users(bson::bo a_payload);
    void replay_ftp_user(bson::bo a_payload);

    QSocketNotifier *check_payload;

    zmq::socket_t *z_receive_api;
    zmq::socket_t *z_push_api;
    zmq::socket_t *z_publish_api;

    Mongodb *mongodb_;
    Zeromq *zeromq_;

public slots:
    void pubsub_payload(bson::bo l_payload);
    void publish_command(QString dest, QString command);

private slots:
    void receive_payload();
    void replay_pushpull_payload(bson::bo a_payload);
};

#endif // ZEROMQ_API_H
