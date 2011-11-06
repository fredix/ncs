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

#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "nosql.h"
#include "zeromq.h"

using namespace mongo;
using namespace bson;



class Payload : public QObject
{
    Q_OBJECT
public:
//    explicit stats(QObject *parent = 0);
    Payload(Nosql &a, QObject *parent = 0);
 //   Stats();
    ~Payload();


protected:
     Nosql &nosql_;     


public slots:
     void s_job_receive(bson::bo data);

signals:
    void payload_cpu(bson::bo data);
    void payload_load(bson::bo data);
    void payload_network(bson::bo data);
    void payload_memory(bson::bo data);
    void payload_uptime(bson::bo data);
    void payload_process(bson::bo data);
};



#endif // PAYLOAD_H
