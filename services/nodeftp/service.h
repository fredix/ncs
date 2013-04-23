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

#ifndef SERVICE_H
#define SERVICE_H

#include <QThread>
#include "nodeftp.h"


struct params {
    QString base_directory;
    int ftp_server_port;
    QString mongodb_ip;
    QString mongodb_base;
};



class Io : public QObject
{
    Q_OBJECT

public:
    Io();
    ~Io();

private slots:
    void readStdin();

private:
    QFile *io_log;
    QSocketNotifier* notifier;
    QTextStream* input;

signals:
    void parseData(QString s);

};


class Service : public QObject
{
    Q_OBJECT
public:
    Service(params a_ncs_params, QObject *parent = 0);
    ~Service();
    void Nodeftp_init();    

    // Unix signal handlers.
    static void hupSignalHandler(int unused);
    static void termSignalHandler(int unused);


public slots:
    void handleSigHup();
    void handleSigTerm();


private:
    static int sighupFd[2];
    static int sigtermFd[2];

    QSocketNotifier *snHup;
    QSocketNotifier *snTerm;

    Nodeftp *m_nodeftp;
    Io *ncw;

    params m_params;
    QThread *node_thread_ftp;
    QThread *ncw_thread;

};

#endif // SERVICE_H
