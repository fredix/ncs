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

#include "nodeftp.h"


void OnServerEvent( int Event )
{
    switch( Event )
    {
        case CFtpServer::START_LISTENING:
            //printf("* Server is listening !\r\n");
            break;

        case CFtpServer::START_ACCEPTING:
            //printf("* Server is accepting incoming connexions !\r\n");
            break;

        case CFtpServer::STOP_LISTENING:
            printf("* Server stopped listening !\r\n");
            break;

        case CFtpServer::STOP_ACCEPTING:
            printf("* Server stopped accepting incoming connexions !\r\n");
            break;
        case CFtpServer::MEM_ERROR:
            printf("* Warning, the CFtpServer class could not allocate memory !\r\n");
            break;
        case CFtpServer::THREAD_ERROR:
            printf("* Warning, the CFtpServer class could not create a thread !\r\n");
            break;
        case CFtpServer::ZLIB_VERSION_ERROR:
            printf("* Warning, the Zlib header version differs from the Zlib library version !\r\n");
            break;
        case CFtpServer::ZLIB_STREAM_ERROR:
            printf("* Warning, error during compressing/decompressing data !\r\n");
            break;
    }
}

void OnUserEvent( int Event, CFtpServer::CUserEntry *pUser, void *pArg )
{
    switch( Event )
    {
        case CFtpServer::NEW_USER:
           /* printf("* A new user has been created:\r\n"
                    "\tLogin: %s\r\n" "\tPassword: %s\r\n" "\tStart directory: %s\r\n",
                pUser->GetLogin(), pUser->GetPassword(), pUser->GetStartDirectory() );*/
            break;

        case CFtpServer::DELETE_USER:
            printf("* \"%s\"user is being deleted: \r\n", pUser->GetLogin() );
            break;
    }
}

void OnClientEvent( int Event, CFtpServer::CClientEntry *pClient, void *pArg )
{
    switch( Event )
    {
        case CFtpServer::NEW_CLIENT:
          /*  printf( "* A new client has been created:\r\n"
                "\tClient IP: [%s]\r\n\tServer IP: [%s]\r\n",
                inet_ntoa( *pClient->GetIP() ), inet_ntoa( *pClient->GetServerIP() ) );*/
            break;

        case CFtpServer::DELETE_CLIENT:
            printf( "* A client is being deleted.\r\n" );
            break;

        case CFtpServer::CLIENT_AUTH:
            //printf( "* A client has logged-in as \"%s\".\r\n", pClient->GetUser()->GetLogin() );
            break;

        case CFtpServer::CLIENT_SOFTWARE:
            printf( "* A client has proceed the CLNT FTP command: %s.\r\n", (char*) pArg );
            break;

        case CFtpServer::CLIENT_DISCONNECT:
            printf( "* A client has disconnected.\r\n" );
            break;

        case CFtpServer::CLIENT_UPLOAD:
            printf( "* A client logged-on as \"%s\" is uploading a file: \"%s\"\r\n",
                pClient->GetUser()->GetLogin(), (char*)pArg );
            break;

        case CFtpServer::CLIENT_DOWNLOAD:
            printf( "* A client logged-on as \"%s\" is downloading a file: \"%s\"\r\n",
                pClient->GetUser()->GetLogin(), (char*)pArg );
            break;

        case CFtpServer::CLIENT_LIST:
            printf( "* A client logged-on as \"%s\" is listing a directory: \"%s\"\r\n",
                pClient->GetUser()->GetLogin(), (char*)pArg );
            break;

        case CFtpServer::CLIENT_CHANGE_DIR:
            printf( "* A client logged-on as \"%s\" has changed its working directory:\r\n"
                "\tFull path: \"%s\"\r\n\tWorking directory: \"%s\"\r\n",
                pClient->GetUser()->GetLogin(), (char*)pArg, pClient->GetWorkingDirectory() );
            break;

        case CFtpServer::RECVD_CMD_LINE:
            printf( "* Received: %s (%s)>  %s\r\n",
                pClient->GetUser() ? pClient->GetUser()->GetLogin() : "(Not logged in)",
                inet_ntoa( *pClient->GetIP() ),
                (char*) pArg );
            break;

        case CFtpServer::SEND_REPLY:
            printf( "* Sent: %s (%s)>  %s\r\n",
                pClient->GetUser() ? pClient->GetUser()->GetLogin() : "(Not logged in)",
                inet_ntoa( *pClient->GetIP() ),
                (char*) pArg );
            break;

        case CFtpServer::TOO_MANY_PASS_TRIES:
            printf( "* Too many pass tries for (%s)\r\n",
                inet_ntoa( *pClient->GetIP() ) );
            break;
    }
}


Nodeftp::Nodeftp(QString a_directory, int port) : m_directory(a_directory), m_port(port)
{
    FtpServer = new CFtpServer();

#ifdef CFTPSERVER_ENABLE_EVENTS
    FtpServer->SetServerCallback( OnServerEvent );
    FtpServer->SetUserCallback( OnUserEvent );
    FtpServer->SetClientCallback( OnClientEvent );
#endif

    FtpServer->SetMaxPasswordTries( 3 );
    FtpServer->SetNoLoginTimeout( 45 ); // seconds
    FtpServer->SetNoTransferTimeout( 90 ); // seconds
    FtpServer->SetListeningPort( port );
    //FtpServer->SetDataPortRange( 100, 900 ); // data TCP-Port range = [100-999]
    FtpServer->SetDataPortRange( 1025, 1900 ); // data TCP-Port range = [100-999]
    FtpServer->SetCheckPassDelay( 500 ); // milliseconds. Bruteforcing protection.


#ifdef CFTPSERVER_ENABLE_ZLIB
    FtpServer->EnableModeZ( true );
#endif
}


bool Nodeftp::add_ftp_user(QString email, QString password, QString path)
{
    qDebug() << "Nodeftp::add_user : " << email;
    // check auth
    QString token, directory;
    //if (ncs_auth(email, token, directory))
    //{
        // create user's directory

        QString userdirectory = m_directory + "/" + path;
        if (!QDir(userdirectory).exists()) QDir().mkdir(userdirectory);


        CFtpServer::CUserEntry *pUser = FtpServer->AddUser( email.toAscii(), password.toAscii(), userdirectory.toAscii() );
        if( pUser )
        {
            //printf( "-User successfully created ! :)\r\n" );
            pUser->SetMaxNumberOfClient( 5 ); // 0 Unlimited

    /*        pUser->SetPrivileges( CFtpServer::READFILE | CFtpServer::WRITEFILE |
                CFtpServer::LIST | CFtpServer::DELETEFILE | CFtpServer::CREATEDIR |
                CFtpServer::DELETEDIR );
    */

            pUser->SetPrivileges(CFtpServer::WRITEFILE | CFtpServer::READFILE | CFtpServer::LIST);

    #ifdef CFTPSERVER_ENABLE_EXTRACMD // See "CFtpServer/config.h". not defined by default
            pUser->SetExtraCommand( CFtpServer::ExtraCmd_EXEC );
            // Security Warning ! Only here for example.
            // the last command allow the user to call the 'system()' C function!
    #endif

        }
        else return false;
    //}
    //else return false;

    return true;
}

void Nodeftp::ftp_init()
{
    output = new QTextStream( stdout, QIODevice::WriteOnly );

    log = new QFile("/tmp/ftp");
     if (!log->open(QIODevice::Append | QIODevice::Text))
             return;


    // If you only want to listen on the TCP Loopback interface,
    // replace 'INNADDR_ANY' by 'inet_addr("127.0.0.1")'.
    if( FtpServer->StartListening( INADDR_ANY, m_port ) ) {
       // qDebug( "FTP : Server is listening !" );

        if( FtpServer->StartAccepting() ) {

         //   qDebug( "FTP : Server successfuly started !" );
            writeStdout("{\"action\": \"get_ftp_users\", \"from\": \"ftp\", \"dest\": \"self\"}");

        } else
            qDebug( "FTP : Unable to accept incoming connections" );


    } else
        qErrnoWarning( "FTP : Unable to listen" );

}




Nodeftp::~Nodeftp()
{
    qErrnoWarning( "FTP Exiting" );
    FtpServer->StopListening();
    delete FtpServer;
    log->close();
}



void Nodeftp::writeStdout(QString s)
{
    (*output) << s;
    //while (!(*output).atEnd()) {};
    (*output).flush();
}


void Nodeftp::receive_payload(QString s)
{

   // writeStdout("PARSE : " + s);

    //return;


    //std::cout << s.toStdString() << std::endl;

    // Do something with your received data here
    //writeStdout(s);


    QVariant json = QxtJSON::parse(s);



    if (json.toMap().contains("email") &&
            json.toMap().contains("password") &&
            json.toMap().contains("path"))
    {
         user_email = json.toMap()["email"].toString();
         user_password = json.toMap()["password"].toString();
         user_path = json.toMap()["path"].toString();


      /*  writeStdout("event : " + event);
        qDebug() << "usageid : " << usageid;
        qDebug() << "value : " << value;
        qDebug() << "contentid : " << contentid;
        */

        //$datetime=date('Y-m-d H:i:s',$dt);
        //QDateTime l_timestamp = QDateTime::currentDateTimeUtc();
        //QString timestamp = l_timestamp.toString("yyyy-MM-dd HH:mm:ss");

         bool res = add_ftp_user(user_email, user_password, user_path);


        //writeStdout("REQ SQL : " + req);


        /*query->exec("SELECT VERSION()");

        while (query->next())
        {
            QString name = query->value(0).toString();
            //qDebug() << name;
            writeStdout(name);
        }*/


        QString t_bool = res ? "ok" : "ko";
        writeStdout("{\"action\": \"terminate\", \"status\": \"" + t_bool + "\"}");
    }
    else
        writeStdout("{\"error\": \"parsing json\", \"status\": \"false\"}");



}
