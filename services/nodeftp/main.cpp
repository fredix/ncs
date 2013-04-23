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

#include "main.h"
#include <QCoreApplication>


int Service::sighupFd[2]={};
int Service::sigtermFd[2]={};

QSettings settings("nodecast", "nodeftp");


// http://doc.qt.nokia.com/4.7/unix-signals.html
static void setup_unix_signal_handlers()
{
    struct sigaction hup, term;

    hup.sa_handler = Service::hupSignalHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    /*if (sigaction(SIGHUP, &hup, 0) > 0)
       return 1;*/

    term.sa_handler = Service::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    /*if (sigaction(SIGTERM, &term, 0) > 0)
       return 2;

    return 0;*/

    sigaction (SIGINT, &hup, NULL);
    sigaction (SIGTERM, &term, NULL);
}



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    params nodeftp_params;
    bool debug;
    bool verbose;


    QxtCommandOptions options;
    options.add("debug", "show debug informations");
    options.alias("debug", "d");

    options.add("base-directory", "set the ftp base directory", QxtCommandOptions::Required);
    options.alias("base-directory", "bd");

    options.add("ftp-server-port", "set the ftp server port", QxtCommandOptions::Optional);
    options.alias("ftp-server-port", "fsp");

    options.add("help", "show this help text");
    options.alias("help", "h");
    options.parse(QCoreApplication::arguments());
    if(options.count("help") || options.showUnrecognizedWarning()) {
        options.showUsage();
        return -1;
    }


    if(options.count("base-directory")) {
        nodeftp_params.base_directory = options.value("base-directory").toString();
        settings.setValue("base-directory", nodeftp_params.base_directory);
    }
    else if(settings.contains("base-directory"))
    {
        nodeftp_params.base_directory = settings.value("base-directory").toString();
    }
    else {
        std::cout << "ncs: --ncs-base-directory requires a parameter" << std::endl;
        options.showUsage();
        return -1;
    }


    if(options.count("ftp-server-port")) {
        nodeftp_params.ftp_server_port = options.value("ftp-server-port").toInt();
        settings.setValue("ftp-server-port", nodeftp_params.ftp_server_port);
    }
    else if(settings.contains("ftp-server-port"))
    {
        nodeftp_params.ftp_server_port = settings.value("ftp-server-port").toInt();
    }
    else nodeftp_params.ftp_server_port = 0;


    settings.sync();
    setup_unix_signal_handlers();


    Service *service;
    service = new Service(nodeftp_params);

    return a.exec();
}
