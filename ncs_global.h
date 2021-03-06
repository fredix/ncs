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

#ifndef NCS_GLOBAL_H
#define NCS_GLOBAL_H

#include "alert.h"


enum HTTPMethodType {
    GET=1,
    POST=2,
    PUT=3,
    DELETE=4
};


struct params {
    QString base_directory;
    QString mongodb_ip;
    QString mongodb_base;
    QString domain_name;
    int xmpp_client_port;
    int xmpp_server_port;
    int ftp_server_port;
    int admin_port;
    int api_port;
    int tracker_port;
    email alert_email;
};


#endif // NCS_GLOBAL_H
