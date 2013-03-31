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

#include "service.h"

Service::Service(params ftp_params, QObject *parent) : m_ftp_params(ftp_params), QObject(parent)
{
    m_nodeftp = NULL;
 }

Service::~Service()
{
    qDebug() << "delete ftp";
    delete(m_nodeftp);
}



void Service::Nodeftp_init()
{      
    int port;
    m_ftp_params.server_port == 0 ? port = 2121 : port = m_ftp_params.server_port;

   QThread *node_ftp = new QThread;
   m_nodeftp = new Nodeftp(m_ftp_params.base_directory, port);
   m_nodeftp->moveToThread(node_ftp);
   node_ftp->start();

   m_nodeftp->connect(node_ftp, SIGNAL(started()), SLOT(ftp_init()));
}

