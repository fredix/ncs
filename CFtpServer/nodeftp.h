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

#ifndef NODEFTP_H
#define NODEFTP_H

#include "CFtpServer.h"
#include <QObject>

class Nodeftp : public QObject
{
    Q_OBJECT
public:
    Nodeftp(int port);
    ~Nodeftp();

private:
    int m_port;

    CFtpServer *FtpServer;

public slots:
    void ftp_init();
    void add_user(QString username, QString userpassword, QString userpath);
    //void remove_user(QString username);
};

#endif // NODEFTP_H
