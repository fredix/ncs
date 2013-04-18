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

#ifndef MAIN_H
#define MAIN_H

#include <syslog.h>
#include <QDebug>
#include <QSettings>
#include <QDir>

#include "mongodb.h"
#include "zeromq.h"
#include "service.h"
#include "ncs_global.h"


class Dispatcher : public QObject
{
    Q_OBJECT
public:    
    Dispatcher(params ncs_params);
    ~Dispatcher();

    Mongodb *mongodb_;

    Zeromq *zeromq;
    Service *service;
    Alert *alert;


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

    QThread *thread_alert;
};




#endif // MAIN_H
