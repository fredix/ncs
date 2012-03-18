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

#ifndef XMPP_SERVER_H
#define XMPP_SERVER_H

#include "QXmppLogger.h"
//#include "QXmppPasswordChecker.h"
#include "QXmppServer.h"
#include "QXmppIncomingClient.h"
#include "nosql.h"



class passwordChecker : public QXmppPasswordChecker
{

    /// Checks that the given credentials are valid.
    QXmppPasswordChecker::Error checkPassword(const QString &username, const QString &password)
    {
        // construct a bson object with username and token
        bo auth = BSON("pub_uuid" << username.toStdString() << "uuid" << password.toStdString());
        std::cout << "AUTH !!!!!!!!   :     " << auth << std::endl;


        // search user through users's collection with the bson object payload
        bo host = nosql_->Find("hosts", auth);
        if (host.nFields() != 0)
        {
            be host_id = host.getField("_id");
            std::cout << "host_id : " << host_id << std::endl;
            return QXmppPasswordChecker::NoError;
        }

        qDebug() << "mongoDB auth failed !";

        // used only for the qxmpp client embedded into the ncs server.
        if (username == m_username && password == m_password)
            return QXmppPasswordChecker::NoError;
        else
            return QXmppPasswordChecker::AuthorizationError;

        /*

        if (username == m_username && password == m_password)
            return QXmppPasswordChecker::NoError;
        else
            return QXmppPasswordChecker::AuthorizationError;
            */
    };

    /// Retrieves the password for the given username.
    bool getPassword(const QString &username, QString &password)
    {

        bo auth = BSON("pub_uuid" << username.toStdString() << "uuid" << password.toStdString());


        std::cout << "AUTH !!!!!!!!   :     " << auth << std::endl;


        bo user = nosql_->Find("users", auth);
        if (user.nFields() != 0)
        {
            be login = user.getField("login");
            std::cout << "user : " << login << std::endl;
            return true;
        }

        qDebug() << "mongoDB auth failed !";

        if (username == m_username)
        {
            password = m_password;
            return true;
        } else {
            return false;
        }
    };

    /// Retrieves the password for the given username.
    /*QXmppPasswordReply::Error getPassword(const QXmppPasswordRequest &request, QString &password)
    {
        if (request.username() == m_username)
            {
                password = m_password;
                return QXmppPasswordReply::NoError;
            } else {
                return QXmppPasswordReply::AuthorizationError;
            }
    }*/

    /// Returns true as we implemented getPassword().
    bool hasGetPassword() const
    {
        return false;
    }

public:
    QString m_username;
    QString m_password;        
    Nosql *nosql_;
};

class Xmpp_server : public QObject
{    
    Q_OBJECT
public:
    Xmpp_server(QString a_domain, int a_xmpp_client_port, int a_xmpp_server_port);


private:
    //Nosql &nosql_;
    QString m_domain;
    int m_xmpp_client_port;
    int m_xmpp_server_port;
    QString m_jabberid;
    QString m_jabberpassword;
    passwordChecker m_checker;
    QXmppLogger m_logger;
    QXmppServer m_server;
};

#endif // XMPP_SERVER_H
