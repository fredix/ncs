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


#include "xmpp_client.h"

Xmpp_client::Xmpp_client(QObject *parent) : QXmppClient(parent)
{
    bool check = connect(this, SIGNAL(messageReceived(const QXmppMessage&)),
           SLOT(messageReceived(const QXmppMessage&)));
       Q_ASSERT(check);
       Q_UNUSED(check);
}


Xmpp_client::~Xmpp_client()
{}



void Xmpp_client::messageReceived(const QXmppMessage& message)
{
    QString from = message.from();
    QString msg = message.body();

    //sendPacket(QXmppMessage("", from, "Your message: " + msg));

    qDebug() << "Xmpp_client::messageReceived : OOOOOOOOOKKKKKKKKK : ";
    //std::cout << "Xmpp_client::messageReceived : FROM : " << from << " MSG : " << msg;
}
