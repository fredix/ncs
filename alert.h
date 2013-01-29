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


#ifndef ALERT_H
#define ALERT_H

#include <QObject>
#include <QDebug>
#include <QMutex>
#include <QxtJSON>
#include <QxtNetwork>

struct email {
    QString smtp_hostname;
    QString smtp_username;
    QString smtp_password;
    QString smtp_sender;
    QString smtp_recipient;
};

class Alert : public QObject
{
    Q_OBJECT
public:
    Alert(email a_alert_email);
    ~Alert();

private:
    QString m_host;
    QString m_username;
    QString m_password;
    QString m_sender;
    QString m_recipient;
    QMutex *m_mutex;
    QxtSmtp *m_smtp;
    QxtMailMessage *m_message;

public slots:
    void sendEmail(QString worker);
    void failed();
    void success();
};

#endif // ALERT_H
