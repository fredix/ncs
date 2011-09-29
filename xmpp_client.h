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

#ifndef XMPP_CLIENT_H
#define XMPP_CLIENT_H

#include <QObject>
#include <QDebug>

#include "QXmppMessage.h"
#include "QXmppLogger.h"
#include "QXmppClient.h"


class Xmpp_client : public QXmppClient
{    
    Q_OBJECT

public:
    Xmpp_client(QObject *parent = 0);
    ~Xmpp_client();

private:
    QXmppLogger m_logger;


public slots:
    void messageReceived(const QXmppMessage&);
};

#endif // XMPP_CLIENT_H
