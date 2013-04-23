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


#include "http_admin.h"



Http_admin::Http_admin(QString basedirectory, QxtAbstractWebSessionManager * sm, QObject * parent): m_basedirectory(basedirectory), QxtWebSlotService(sm,parent)
{
    enumToHTTPmethod.insert(QString("GET"), GET);
    enumToHTTPmethod.insert(QString("POST"), POST);
    enumToHTTPmethod.insert(QString("PUT"), PUT);
    enumToHTTPmethod.insert(QString("DELETE"), DELETE);

/*

    //QxtWebServiceDirectory* top = new QxtWebServiceDirectory(sm, sm);
    QxtWebServiceDirectory* service_admin = new QxtWebServiceDirectory(sm, this);
  //  QxtWebServiceDirectory* service2 = new QxtWebServiceDirectory(sm, this);
    QxtWebServiceDirectory* service_admin_login = new QxtWebServiceDirectory(sm, service_admin);
 //   QxtWebServiceDirectory* service1b = new QxtWebServiceDirectory(sm, service1);
    this->addService("admin", service_admin);
//    this->addService("2", service2);
    service_admin->addService("login", service_admin_login);
 //   service1->addService("b", service1b);

*/

    mongodb_ = Mongodb::getInstance ();
}


Http_admin::~Http_admin()
{
    qDebug() << "Http_admin : close";
}

bool Http_admin::check_user_login(QxtWebRequestEvent *event, QString &alert)
{
    QxtWebRedirectEvent *redir;
    QString user_uuid = event->cookies.value("nodecast");

    if (user_bson.contains(user_uuid))
    {
        //user = user_session[user_uuid];

        if (user_alert.contains(user_uuid))
        {
            alert = user_alert[user_uuid];
            user_alert.remove(user_uuid);
        }

       // qDebug() << "USER FOUND : " << user;
        return true;
    }
    else
    {
        qDebug() << "USER NOT CONNECTED, REDIR !";
        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "login", 302 );
        postEvent(redir);
        return false;
    }

}


void Http_admin::set_user_alert(QxtWebRequestEvent *event, QString alert)
{
    QString user_uuid = event->cookies.value("nodecast");
    user_alert[user_uuid] = alert;
}



QBool Http_admin::http_auth(QString auth, QHash <QString, QString> &hauth, QString &str_session_uuid)
{

    QStringList list_field = auth.split("&");
   // QHash <QString, QString> hash_fields;

    for (int i = 0; i < list_field.size(); ++i)
    {
        QStringList field = list_field.at(i).split("=");
        hauth[field[0]] = field[1].replace(QString("%40"), QString("@"));
    }

    qDebug() << "password " << hauth["password"] << " email " << hauth["email"];



    QCryptographicHash cipher( QCryptographicHash::Sha1 );
    cipher.addData(hauth["password"].simplified().toAscii());
    QByteArray password_hash = cipher.result();

    qDebug() << "password_hash : " << password_hash.toHex();

    BSONObj bauth = BSON("email" << hauth["email"].toStdString() << "password" << QString::fromLatin1(password_hash.toHex()).toStdString());


    BSONObj l_user = mongodb_->Find("users", bauth);

    if (l_user.nFields() == 0)
    {
        qDebug() << "auth failed !";
        return QBool(false);
    }

    // create session id
    QUuid session_uuid = QUuid::createUuid();
    str_session_uuid = session_uuid.toString().mid(1,36);

    user_bson[str_session_uuid] = l_user.copy();

    //hauth["admin"] = QString::fromStdString(l_user.getField("admin").toString(false));
    //qDebug() << "IS ADMIN ? : " << hauth["admin"];

   // be login = user.getField("login");
    // std::cout << "user : " << login << std::endl;

  //  payload_builder.append("email", email.toStdString());
//    payload_builder.append("user_id", l_user.getField("_id").OID());

  //  a_user = l_user;

    //hauth = bauth;

    return QBool(true);
}


QBool Http_admin::checkAuth(QString token, BSONObjBuilder &payload_builder, bo &a_user)
{
 //   QString b64 = header.section("Basic ", 1 ,1);
 //   qDebug() << "auth : " << b64;
/*
    QByteArray tmp_auth_decode = QByteArray::fromBase64(b64.toAscii());
    QString auth_decode =  tmp_auth_decode.data();
    QStringList arr_auth =  auth_decode.split(":");

    QString email = arr_auth.at(0);
    QString key = arr_auth.at(1);

    qDebug() << "email : " << email << " key : " << key;

    bo auth = BSON("email" << email.toStdString() << "token" << key.toStdString());
*/

    BSONObj auth = BSON("token" << token.toStdString());

    std::cout << "USER TOKEN : " << auth << std::endl;

    BSONObj l_user = mongodb_->Find("users", auth);
    if (l_user.nFields() == 0)
    {
        qDebug() << "auth failed !";
        return QBool(false);
    }

   // be login = user.getField("login");
    // std::cout << "user : " << login << std::endl;

    payload_builder << l_user.getField("email");
    payload_builder.append("user_id", l_user.getField("_id").OID());

    a_user = l_user;
    return QBool(true);
}








/**************  GET PAGE **********************/


/********** INDEX PAGE ************/
void Http_admin::admin_logout(QxtWebRequestEvent* event)
{
    QxtWebRedirectEvent *redir;

    user_bson.remove(event->cookies.value("nodecast"));

    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "/", 302 );
    postEvent(redir);
}

/********** INDEX PAGE ************/
void Http_admin::index(QxtWebRequestEvent* event)
{
/*    QString bodyMessage;
    bodyMessage = buildResponse("error", "ncs version 0.9.1");

    qDebug() << bodyMessage;

  */

    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;

    QxtWebPageEvent *page;

        if(!body.open(m_basedirectory + "/html_templates/index.html"))
        {
            body["content"]="error 404";
            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       body.render().toUtf8());
            page->status=404;
            qDebug() << "error 404";
        }
        else
        {
            header.open(m_basedirectory + "/html_templates/header.html");
            footer.open(m_basedirectory + "/html_templates/footer.html");

            if (!event->cookies.value("nodecast").isEmpty() && user_bson.contains(event->cookies.value("nodecast")))
            {
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";

            }
            else header["connection"] = "<li><a href=\"/admin/login\">Login</a></li>";


            body["ncs_version"]="0.9.7";

            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       header.render().toUtf8() +
                                       body.render().toUtf8() +
                                       footer.render().toUtf8());

            page->contentType="text/html";
        }

        postEvent(page);


        /* postEvent(new QxtWebPageEvent(event->sessionID,
                                      event->requestID,
                                      index.render().toUtf8()));

        */
}

/********** ADMIN PAGE ************/
void Http_admin::admin(QxtWebRequestEvent* event, QString action)
{

        qDebug() << "ADMIN : " << "METHOD : " << event->method << " headers : " << event->headers << " ACTION " << action;

        QString bodyMessage="";
        QBool res = QBool(false);
        BSONObjBuilder payload_builder;
        payload_builder.genOID();

        bo user;


        switch (enumToHTTPmethod[event->method.toUpper()])
        {
        case GET:
            qDebug() << "HTTP GET : " << action;
            qDebug() << "HEADERS : " << event->headers.value("Authorization");

/*
            res = checkAuth(event->headers.value("Authorization"), payload_builder, user);
            if (!res)
            {
                bodyMessage = buildResponse("error", "auth");
                //bodyMessage = buildResponse("error", "auth");
            }
            else
            {
                if (action == "users")
                    admin_users_get(event);
            }
            */


            if (action == "login")
            {
                admin_login(event);
            }
            else if (action == "logout")
            {
                admin_logout(event);
            }
            else if (action == "users")
            {

                admin_users_get(event);
            }
            else if (action == "createuser")
            {

                admin_user_post(event);
            }
            else if (action == "nodes")
            {

                admin_nodes_get(event);
            }
            else if (action == "createnode")
            {

                admin_node_post(event);
            }
            else if (action == "workflows")
            {

                admin_workflows_get(event);
            }
            else if (action == "createworkflow")
            {

                admin_workflow_post(event);
            }
            else if (action == "workers")
            {

                admin_workers_get(event);
            }
            else if (action == "sessions")
            {

                admin_sessions_get(event);
            }

            else if (action == "payloads")
            {

                admin_payloads_get(event);
            }
            else if (action == "lost_pushpull_payloads")
            {
                admin_lost_pushpull_payloads_get(event);
            }

            else
            {
                QxtHtmlTemplate index;
                index["content"]="error 404";
                QxtWebPageEvent *page = new QxtWebPageEvent(event->sessionID,
                                                            event->requestID,
                                                            index.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";

                postEvent(page);
                return;
            }

            break;
        case POST:
            //qDebug() << "HTTP POST not implemented";
            //bodyMessage = buildResponse("error", "HTTP POST not implemented");

            if (action == "login")
            {
                admin_login(event);
                return;
            }
            else if (action == "createuser")
            {
                admin_user_post(event);
                return;
            }
            else if (action == "createnode")
            {
                admin_node_post(event);
                return;
            }
            else if (action == "deletenode")
            {
                admin_node_or_workflow_delete(event, "nodes");
                return;
            }
            else if (action == "deleteworkflow")
            {
                admin_node_or_workflow_delete(event, "workflows");
                return;
            }
            else if (action == "createworkflow")
            {

                admin_workflow_post(event);
            }

            break;
        case PUT:
            qDebug() << "HTTP PUT not implemented";
            bodyMessage = "HTTP PUT not implemented";

            break;
        case DELETE:
            qDebug() << "HTTP DELETE not implemented";
            bodyMessage = "HTTP DELETE not implemented";

            break;
        }



        if (!bodyMessage.isEmpty())
        {
            postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        }
     /*   else
        {
            QxtHtmlTemplate index;
            index["content"]="error 404";
            QxtWebPageEvent *page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());
            page->status = 404;
            qDebug() << "error 404";

            postEvent(page);
        }
        */

}



/********** ADMIN PAGE ************/
void Http_admin::admin_login(QxtWebRequestEvent* event)
{

    QxtHtmlTemplate index;
    QxtHtmlTemplate header;
    QxtHtmlTemplate footer;

    QxtWebPageEvent *page;
    QxtWebContent *form;

    QxtWebStoreCookieEvent *cookie_session;
    QxtWebRedirectEvent *redir=NULL;


    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        if(!index.open(m_basedirectory + "/html_templates/login.html"))
        {
            index["content"]="error 404";
            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());
            page->status = 404;
            qDebug() << "error 404";
        }
        else
        {
            header.open(m_basedirectory + "/html_templates/header.html");
            footer.open(m_basedirectory + "/html_templates/footer.html");
            header["connection"] = "<li><a href=\"/admin/login\">Login</a></li>";


            // redirect to createuser if not users
            if (mongodb_->Count("users") == 0)
            {
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createuser", 302 );
                postEvent(redir);
                return;
            }

            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       header.render().toUtf8() +
                                       index.render().toUtf8() +
                                       footer.render().toUtf8());

            page->contentType="text/html";
        }

        postEvent(page);


        /* postEvent(new QxtWebPageEvent(event->sessionID,
                                      event->requestID,
                                      index.render().toUtf8()));

        */
        break;
    case POST:
        if(!index.open(m_basedirectory + "/html_templates/login.html"))
        {
            index["content"]="error 404";
            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());
            page->status = 404;
            qDebug() << "error 404";
        }
        else
        {
            form = event->content;
            form->waitForAllContent();

            QByteArray requestContent = form->readAll();
            //RECEIVE :  "firstname=fred&lastname=ix&email=fredix%40gmail.com"

            qDebug() << "RECEIVE : " << requestContent;

            QHash <QString, QString> hauth;
            QString str_session_uuid;

            if (!http_auth(QString::fromUtf8(requestContent), hauth, str_session_uuid))
            {
                qDebug() << "AUTH FAILED !";
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "login", 302 );
            }
            else
            {
                //QxtWebStoreCookieEvent cookie_mail (event->sessionID, "email", hauth["email"],  QDateTime::currentDateTime().addMonths(1));
                //QxtWebStoreCookieEvent cookie_pass (event->sessionID, "password", hauth["password"],  QDateTime::currentDateTime().addMonths(1));

              //  QUuid session_uuid = QUuid::createUuid();
              //  QString str_session_uuid = session_uuid.toString().mid(1,36);

                //user_session[str_session_uuid] = hauth["email"];
                //user_admin[str_session_uuid] = (hauth["admin"] == "true")? true : false;

                user_alert[str_session_uuid] = errorMessage("Logged in successfully", "notice");

                cookie_session = new QxtWebStoreCookieEvent (event->sessionID, "nodecast", str_session_uuid, QDateTime::currentDateTime().addMonths(1));
                qDebug() << "SESSION ID : " << str_session_uuid;

                /*
                index["content"]=" welcome " + hauth["email"];


                QString html;
                html.append("<select>");

                for (int i=0; i<5; i++)
                {
                    html.append("<option value=\"ok\">Ok</option>");
                }

                html.append("</select>");

                index["prot"]=html;

                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           index.render().toUtf8());


                page->contentType="text/html";
                */

                postEvent(cookie_session);

                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "users", 302 );


                //QxtWebEvent cookie (QxtWebEvent::StoreCookie, event->sessionID);

                //postEvent()

            }

        }

        //postEvent(page);
        if (redir) postEvent(redir);

        break;
    }

}



/********** ADMIN PAGE ************/
void Http_admin::admin_users_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;
    QxtWebRedirectEvent *redir=NULL;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open(m_basedirectory + "/html_templates/admin_users_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;


                // redirect to createuser if not users
                if (mongodb_->Count("users") == 0)
                {
                    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createuser", 302 );
                    postEvent(redir);
                    return;
                }

                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;

                // only an admin user can go to the users list
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    set_user_alert(event, errorMessage("you are not an admin user", "error"));
                    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "nodes", 302 );
                    postEvent(redir);
                    return;
                }


                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj empty;
                QList <BSONObj> users = mongodb_->FindAll("users", empty);

                QString output;
                int counter=0;
                foreach (BSONObj user, users)
                {
                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id\" value=\"1\"></td><td>1</td><td>" +
                            QString::fromStdString(user.getField("login").str()) + "</td>" +
                            "<td>" + QString::fromStdString(user.getField("email").str()) + "</td>" +
                            "<td>" + QString::fromStdString(user.getField("token").str()) + "</td>" +
                            "<td>" + QString::fromStdString(user.getFieldDotted("nodes.payload_counter").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);

                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}


/********** ADMIN PAGE ************/
void Http_admin::admin_user_post(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */

    qDebug() << "Http_admin::admin_user_post";

    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;
    QxtWebContent *form;
    QxtWebRedirectEvent *redir=NULL;


//            if(!body.open("html_templates/admin_users_get.html"))



    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        if(!body.open(m_basedirectory + "/html_templates/admin_user_post.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");




                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;

                if (mongodb_->Count("users") != 0)
                {
                    if (!check_user_login(event, l_alert)) return;

                    // only an admin user can create new users
                    if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                    {
                        set_user_alert(event, errorMessage("you are not an admin user", "error"));
                        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "nodes", 302 );
                        postEvent(redir);
                        return;
                    }


                }
                else header["alert"] = errorMessage("please create an admin user", "notice");

                if (!l_alert.isEmpty()) header["alert"] = l_alert;


                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);

        break;
    case POST:

        qDebug() << "COOKIES : " << event->cookies.value("nodecast");
        QString l_alert;

        bool create_admin = (mongodb_->Count("users") != 0)? false : true;
        if (!create_admin && !check_user_login(event, l_alert)) return;

        form = event->content;
        form->waitForAllContent();

        QByteArray requestContent = form->readAll();
        //RECEIVE :  "login=fff&password=dsdsd&email=alert%40pumit.com&checkbox=ftp&checkbox=xmpp"

        qDebug() << "RECEIVE : " << requestContent;

        //QxtWebStoreCookieEvent cookie_mail (event->sessionID, "email", hauth["email"],  QDateTime::currentDateTime().addMonths(1));
        //QxtWebStoreCookieEvent cookie_pass (event->sessionID, "password", hauth["password"],  QDateTime::currentDateTime().addMonths(1));


        QHash <QString, QString> form_field;
        QStringList list_field = QString::fromAscii(requestContent).split("&");

        for (int i = 0; i < list_field.size(); ++i)
        {
            QStringList field = list_field.at(i).split("=");

            if (field[1].isEmpty())
            {
                set_user_alert(event, errorMessage("field " + field[0] + " is empty", "error"));
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createuser", 302 );
                postEvent(redir);
                return;
            }
            form_field[field[0]] = field[1].replace(QString("%40"), QString("@"));
        }
        qDebug() << "form_field : " << form_field;

        bool api = form_field.contains("api")||create_admin? true : false;
        bool ftp = form_field.contains("ftp")||create_admin? true : false;
        bool xmpp = form_field.contains("xmpp")||create_admin? true : false;
        bool bittorrent = form_field.contains("bittorrent")||create_admin? true : false;

        QUuid token = QUuid::createUuid();
        QString str_token = token.toString().mid(1,36);

        QUuid tracker_token = QUuid::createUuid();
        QString str_tracker_token = tracker_token.toString().remove(QChar('-')).mid(1,32);

        QUuid ftp_token = QUuid::createUuid();
        QString str_ftp_token = ftp_token.toString().remove(QChar('-')).mid(1,32);

        QUuid ftp_directory = QUuid::createUuid();
        QString str_ftp_directory = ftp_directory.toString().remove(QChar('-')).mid(1,32);


        QUuid xmpp_token = QUuid::createUuid();
        QString str_xmpp_token = xmpp_token.toString().remove(QChar('-')).mid(1,32);

        QUuid api_token = QUuid::createUuid();
        QString str_api_token = api_token.toString().remove(QChar('-')).mid(1,32);


        QCryptographicHash cipher( QCryptographicHash::Sha1 );
        cipher.addData(form_field["password"].simplified().toAscii());
        QByteArray password_hash = cipher.result();

        qDebug() << "password_hash : " << password_hash.toHex();

        BSONObj t_user = BSON(GENOID << "admin" << create_admin << "login" << form_field["login"].toStdString() <<
                              "password" << QString::fromLatin1(password_hash.toHex()).toStdString() <<
                              "email" << form_field["email"].toStdString() <<
                              "token" << str_token.toStdString() <<
                              "ftp" << BSON ("token" << str_ftp_token.toStdString() << "activated" << ftp << "directory" << str_ftp_directory.toStdString()) <<
                              "tracker" << BSON ("token" << str_tracker_token.toStdString())  << "activated" << bittorrent <<
                              "xmpp" << BSON ("token" << str_xmpp_token.toStdString())  << "activated" << xmpp <<
                              "api" << BSON ("token" << str_api_token.toStdString())  << "activated" << api);

        mongodb_->Insert("users", t_user);

        if (ftp) {
            QString command = "{\"email\": \"" + form_field["email"] + "\", \"password\": \"" + str_ftp_token + "\", \"path\": \"" + str_ftp_directory + "\"}";
            emit create_ftp_user("ftp", command);
            qDebug() << "EMIT CREATE FTP USER : " << command;
        }


        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "users", 302 );
        postEvent(redir);

        break;
    }

}


/********** ADMIN PAGE ************/
void Http_admin::admin_nodes_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;


{ "_id" : ObjectId("50f91cbda159811513a8b729"), "nodename" : "samsung@dev", "email" : "fredix@gmail.com", "user_id" : ObjectId("50f91c85a7a5dbe8e2f35b29"), "node_uuid" : "0d7f9bdc-37a2-4290-be41-62598bd7a525", "node_password" : "6786a141-6dff-4f91-891a-a9107915ad76" }


      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open(m_basedirectory + "/html_templates/admin_nodes_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }

                QList <BSONObj> nodes = mongodb_->FindAll("nodes", filter);

                QString output;
                int counter=0;
                foreach (BSONObj node, nodes)
                {

                    BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id_" + QString::number(counter) + "\" value=\"" + QString::fromStdString(node.getField("_id").OID().str()) + "\"></td><td>" +
                            QString::fromStdString(node.getField("nodename").str()) + "</td>" +
                            "<td>" + QString::fromStdString(node.getField("email").str()) + "</td>" +
                            "<td>" + QString::fromStdString(node.getField("node_uuid").str()) + "</td>" +
                            "<td>" + QString::fromStdString(node.getField("node_password").str()) + "</td>" +
                            "<td>" + QString::fromStdString(user.getField("email").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);


                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}


/********** ADMIN PAGE ************/
void Http_admin::admin_node_or_workflow_delete(QxtWebRequestEvent* event, QString collection)
{
    QxtWebContent *form;
    QxtWebRedirectEvent *redir=NULL;


    QString l_alert;
    if (!check_user_login(event, l_alert)) return;

    form = event->content;
    form->waitForAllContent();

    QByteArray requestContent = form->readAll();

    qDebug() << "RECEIVE : " << requestContent;
    // RECEIVE :  "id=513f4f462845b4fa97461b10&id=513f57d8380e6714db9146fa"

    QHash <QString, QString> form_field;
    QStringList list_field = QString::fromAscii(requestContent).split("&");

    for (int i = 0; i < list_field.size(); ++i)
    {
        QStringList field = list_field.at(i).split("=");

        if (field[1].isEmpty())
        {
            set_user_alert(event, errorMessage("field " + field[0] + " is empty", "error"));
            redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, collection, 302 );
            postEvent(redir);
            return;
        }

        form_field[field[0]] = field[1];


        BSONObj node_id = BSON("_id" << mongo::OID(field[1].toStdString()));
        mongodb_->Remove(collection, node_id);
    }

    qDebug() << "form field : " << form_field;

    redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, collection, 302 );
    postEvent(redir);


}

/********** ADMIN PAGE ************/
void Http_admin::admin_node_post(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;
    QxtWebContent *form;
    QxtWebRedirectEvent *redir=NULL;


//            if(!body.open("html_templates/admin_users_get.html"))



    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        if(!body.open(m_basedirectory + "/html_templates/admin_node_post.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");

                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;

                if (user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    QString output="<label class=\"label\">Choose a owner</label>";
                    BSONObj empty;
                    QList <BSONObj> users = mongodb_->FindAll("users", empty);

                    output.append("<select name=\"user\">");

                    foreach (BSONObj user, users)
                    {
                        output.append("<option value=\"" + QString::fromStdString(user.getField("_id").OID().str()) + "\">" +  QString::fromStdString(user.getField("login").str()) + "</option>");
                    }
                    output.append("</select>");


                    body["users"] = output;
                }
                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);

        break;
    case POST:

        qDebug() << "COOKIES : " << event->cookies.value("nodecast");
        QString l_alert;
        if (!check_user_login(event, l_alert)) return;

        form = event->content;
        form->waitForAllContent();

        QByteArray requestContent = form->readAll();
        //RECEIVE :  "firstname=fred&lastname=ix&email=fredix%40gmail.com"

        qDebug() << "RECEIVE : " << requestContent;

        //QxtWebStoreCookieEvent cookie_mail (event->sessionID, "email", hauth["email"],  QDateTime::currentDateTime().addMonths(1));
        //QxtWebStoreCookieEvent cookie_pass (event->sessionID, "password", hauth["password"],  QDateTime::currentDateTime().addMonths(1));


        //RECEIVE :  "login=fredix&email=fredix%40gmail.com"

        QHash <QString, QString> form_field;
        QStringList list_field = QString::fromAscii(requestContent).split("&");

        for (int i = 0; i < list_field.size(); ++i)
        {
            QStringList field = list_field.at(i).split("=");

            if (field[1].isEmpty())
            {
                set_user_alert(event, errorMessage("field " + field[0] + " is empty", "error"));
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createnode", 302 );
                postEvent(redir);
                return;
            }

            form_field[field[0]] = field[1].replace(QString("%40"), QString("@"));
        }

        qDebug() << "form field : " << form_field;



        QUuid node_uuid = QUuid::createUuid();
        QString str_node_uuid = node_uuid.toString().mid(1,36);

        QUuid node_token = QUuid::createUuid();
        QString str_node_token = node_token.toString().mid(1,36);

        qDebug() << "str_node_uuid : " << str_node_uuid << " str_node_token : " << str_node_token;

        BSONObj user_id;
        if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
        {
            user_id = BSON("_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
        }
        else user_id = BSON("_id" << mongo::OID(form_field["user"].toStdString()));

        BSONObj t_user = mongodb_->Find("users", user_id);

        std::cout << "T USER : " << t_user.toString() << std::endl;

        BSONObj t_node = BSON(GENOID <<
                              "nodename" << form_field["nodename"].toStdString()  <<
                              "email" << t_user.getField("email").str() <<
                              "user_id" << t_user.getField("_id").OID() <<
                              "node_uuid" << str_node_uuid.toStdString() <<
                              "node_password" << str_node_token.toStdString());
        mongodb_->Insert("nodes", t_node);

        be t_user_id = t_user.getField("_id");
        bo node = BSON("nodes" << t_node);
        mongodb_->Addtoarray("users", t_user_id.wrap(), node);



        //doc = { login : 'user', email : 'user@email.com', authentication_token : 'token'}
        //db.users.insert(doc);





        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "nodes", 302 );
        postEvent(redir);

        break;
    }

}



/********** ADMIN PAGE ************/
void Http_admin::admin_workflows_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open(m_basedirectory + "/html_templates/admin_workflows_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }

                QList <BSONObj> workflows = mongodb_->FindAll("workflows", filter);

                QString output;
                int counter=0;
                foreach (BSONObj workflow, workflows)
                {
                    BSONObj user_id = BSON("_id" << workflow.getField("user_id"));
                    BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id_" + QString::number(counter) + "\" value=\"" + QString::fromStdString(workflow.getField("_id").OID().str()) + "\"></td><td>" +
                            QString::fromStdString(workflow.getField("workflow").str()) + "</td>" +
                            "<td>" + QString::fromStdString(workflow.getField("email").str()) + "</td>" +
                            "<td>" + QString::fromStdString(workflow.getField("uuid").str()) + "</td>" +
                            "<td>" + QString::fromStdString(workflow.getField("workers").jsonString(Strict, false)) + "</td>" +
                            "<td>" + QString::fromStdString(user.getField("email").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);




                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}



/********** ADMIN PAGE ************/
void Http_admin::admin_workflow_post(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;
    QxtWebContent *form;
    QxtWebRedirectEvent *redir=NULL;


//            if(!body.open("html_templates/admin_users_get.html"))



    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        if(!body.open(m_basedirectory + "/html_templates/admin_workflow_post.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;

                if (user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    QString output="<label class=\"label\">Choose a owner</label>";
                    BSONObj empty;
                    QList <BSONObj> users = mongodb_->FindAll("users", empty);

                    output.append("<select name=\"user\">");

                    foreach (BSONObj user, users)
                    {
                        output.append("<option value=\"" + QString::fromStdString(user.getField("_id").OID().str()) + "\">" +  QString::fromStdString(user.getField("login").str()) + "</option>");
                    }
                    output.append("</select>");


                    body["users"] = output;
                }
                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);

        break;
    case POST:

        qDebug() << "COOKIES : " << event->cookies.value("nodecast");
        QString l_alert;
        if (!check_user_login(event, l_alert)) return;

        form = event->content;
        form->waitForAllContent();

        QByteArray requestContent = form->readAll();
        //RECEIVE :  "firstname=fred&lastname=ix&email=fredix%40gmail.com"

        qDebug() << "RECEIVE : " << requestContent;

        //QxtWebStoreCookieEvent cookie_mail (event->sessionID, "email", hauth["email"],  QDateTime::currentDateTime().addMonths(1));
        //QxtWebStoreCookieEvent cookie_pass (event->sessionID, "password", hauth["password"],  QDateTime::currentDateTime().addMonths(1));



        //RECEIVE :  "login=fredix&email=fredix%40gmail.com"

        QHash <QString, QString> form_field;
        QStringList list_field = QString::fromAscii(requestContent).split("&");

        for (int i = 0; i < list_field.size(); ++i)
        {
            QStringList field = list_field.at(i).split("=");
            if (field[1].isEmpty())
            {
                set_user_alert(event, errorMessage("field " + field[0] + " is empty", "error"));
                redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createworkflow", 302 );
                postEvent(redir);
                return;
            }
            form_field[field[0]] = field[1].replace(QString("%40"), QString("@"));
        }

        qDebug() << "form field : " << form_field;
//      QHash(("workers", "%7B+%22myworkerA%22+%3A+1%2C+%22myworkerB%22+%3A+2%2C+%22myworkerC%22+%3A+3+%7D")("workflow", "test")("user", "51126e85544a4eee6d9521e2"))


        QUuid workflow_uuid = QUuid::createUuid();
        QString str_workflow_uuid = workflow_uuid.toString().mid(1,36);

        qDebug() << "str_workflow_uuid : " << str_workflow_uuid;


        QUrl worker = QUrl::fromPercentEncoding(form_field["workers"].toAscii().replace("+", "%20"));
        qDebug() << "URL : " << worker.toString();

        BSONObj b_workflow;
        try
        {
            b_workflow = mongo::fromjson(worker.toString().toStdString());
        }
        catch (mongo::MsgAssertionException &e)
        {
            std::cout << "e : " << e.what() << std::endl;
            set_user_alert(event, errorMessage("workers's' field is not a JSON format", "error"));
            redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "createworkflow", 302 );
            postEvent(redir);
            return;
        }
         std::cout << "WORKFLOW : " << b_workflow.toString() << std::endl;

        BSONObj user_id;
        if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
        {
            user_id = BSON("_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
        }
        else user_id = BSON("_id" << mongo::OID(form_field["user"].toStdString()));


        BSONObj t_user = mongodb_->Find("users", user_id);

        std::cout << "T USER : " << t_user.toString() << std::endl;

        BSONObj t_workflow = BSON(GENOID <<
                                  "workflow" << form_field["workflow"].toStdString()  <<
                                  "email" << t_user.getField("email").str() <<
                                  "user_id" << t_user.getField("_id").OID() <<
                                  "uuid" << str_workflow_uuid.toStdString() <<
                                  "workers" << b_workflow);
        mongodb_->Insert("workflows", t_workflow);

        //doc = { login : 'user', email : 'user@email.com', authentication_token : 'token'}
        //db.users.insert(doc);





        redir = new QxtWebRedirectEvent( event->sessionID, event->requestID, "workflows", 302 );
        postEvent(redir);

        break;
    }

}


/********** ADMIN PAGE ************/
void Http_admin::admin_workers_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open(m_basedirectory + "/html_templates/admin_workers_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));

                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }
                QList <BSONObj> workers = mongodb_->FindAll("workers", filter);

                QString output;
                int counter=0;
                foreach (BSONObj worker, workers)
                {

                    //BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    //BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id_" + QString::number(counter) + "\" value=\"" + QString::fromStdString(worker.getField("_id").OID().str()) + "\"></td><td>" +
                            QString::fromStdString(worker.getField("name").str()) + "</td>" +
                            "<td>" + QString::number(worker.getField("port").number()) + "</td>" +
                            "<td>" + QString::fromStdString(worker.getField("type").str()) + "</td>" +
                            "<td>" + QString::fromStdString(worker.getField("nodes").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);






                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}




/********** ADMIN PAGE ************/
void Http_admin::admin_sessions_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;


{ "_id" : ObjectId("50f91cbda159811513a8b729"), "nodename" : "samsung@dev", "email" : "fredix@gmail.com", "user_id" : ObjectId("50f91c85a7a5dbe8e2f35b29"), "node_uuid" : "0d7f9bdc-37a2-4290-be41-62598bd7a525", "node_password" : "6786a141-6dff-4f91-891a-a9107915ad76" }


      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open(m_basedirectory + "/html_templates/admin_sessions_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";


                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }
                QList <BSONObj> sessions = mongodb_->FindAll("sessions", filter);

                QString output;
                int counter=0;
                foreach (BSONObj session, sessions)
                {

                    //BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    //BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id_" + QString::number(counter) + "\" value=\"" + QString::fromStdString(session.getField("_id").OID().str()) + "\"></td><td>" +
                            QString::fromStdString(session.getField("uuid").str()) + "</td>" +
                            "<td>" + QString::fromStdString(session.getField("counter").str()) + "</td>" +
                            "<td>" + QString::fromStdString(session.getField("last_worker").str()) + "</td>" +
                            "<td>" + QString::fromStdString(session.getField("start_timestamp").str()) + "</td>" +
                            "<td>" + QString::fromStdString(session.getField("workflow").str()) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);


                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}



/********** ADMIN PAGE ************/
void Http_admin::admin_payloads_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open(m_basedirectory + "/html_templates/admin_payloads_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";

                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }
                QList <BSONObj> payloads = mongodb_->FindAll("payloads", filter);

                QString output;
                int counter=0;
                foreach (BSONObj payload, payloads)
                {

                    //BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    //BSONObj user = mongodb_->Find("users", user_id);

                    QString trclass = (counter %2 == 0) ? "odd" : "even";
                    QDateTime timestamp;
                    timestamp.setTime_t(payload.getField("timestamp").Number());

                    QString li_step;
                    if (payload.hasField("steps"))
                    {
                        BSONObj steps = payload.getField("steps").Obj();
                        list <BSONElement> list_steps;
                        steps.elems(list_steps);

                        BSONObj l_step;
                        list<be>::iterator i;

                        li_step.append("<table class=\"table\">");


                        for(i = list_steps.begin(); i != list_steps.end(); ++i) {
                            l_step = (*i).embeddedObject ();
                            //std::cout << "l_step => " << l_step  << std::endl;
                            li_step.append("<tr>");

                            li_step.append("<td>" + QString::fromStdString(l_step.getField("action").str()) + "</td>");
                            li_step.append("<td>" + QString::fromStdString(l_step.getField("name").str()) + "</td>");
                            li_step.append("<td>" + QString::number(l_step.getField("order").Number()) + "</td>");
                            li_step.append("<td>" + QString::fromStdString(l_step.getField("data").str()) + "</td>");

                            li_step.append("</tr>");

                        }
                        li_step.append("</table>");
                    }

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id_" + QString::number(counter) + "\" value=\"" + QString::fromStdString(payload.getField("_id").OID().str()) + "\"></td><td>" +
                                  QString::fromStdString(payload.getField("action").str()) + "</td>" +
                                  "<td>" + QString::fromStdString(payload.getField("counter").str()) + "</td>" +
                                  "<td>" + timestamp.toString(Qt::SystemLocaleLongDate) + "</td>" +
                                  "<td>" + QString::fromStdString(payload.getField("workflow_uuid").str()) + "</td>" +
                                  "<td>" + QString::fromStdString(payload.getField("data").str()) + "</td>" +
                                  "<td>" + li_step + "</td>" +
                                  "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);




                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}


//{ "_id" : ObjectId("5110f4667b30b3ebd41fce64"),
//  "data" : { "payload" : { "data" : "{ \"command\" : \"get_file\", \"filename\" : \"ftest\", \"payload_type\" : \"test\", \"session_uuid\" : \"e2329bc3-2be1-4209-8dd6-ead2cacb76e9\" }", "action" : "push", "session_uuid" : "e2329bc3-2be1-4209-8dd6-ead2cacb76e9" } },
//  "pushed" : false, "timestamp" : 1360065638, "worker" : "transcode" }


/********** ADMIN PAGE ************/
void Http_admin::admin_lost_pushpull_payloads_get(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open(m_basedirectory + "/html_templates/admin_lost_pushpull_payloads_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");
                header["connection"] = "<li><a href=\"/admin/logout\">Logout</a></li>";

                qDebug() << "COOKIES : " << event->cookies.value("nodecast");
                QString l_alert;
                if (!check_user_login(event, l_alert)) return;
                if (!l_alert.isEmpty()) header["alert"] = l_alert;
                header["user_login"] = "connected as " + QString::fromStdString(user_bson[event->cookies.value("nodecast")].getField("login").toString(false));


                BSONObj filter;
                if (!user_bson[event->cookies.value("nodecast")].getBoolField("admin"))
                {
                    filter = BSON("user_id" << user_bson[event->cookies.value("nodecast")].getField("_id"));
                }
                QList <BSONObj> lost_pushpull_payloads = mongodb_->FindAll("lost_pushpull_payloads", filter);

                QString output;
                int counter=0;
                foreach (BSONObj lost_pushpull_payload, lost_pushpull_payloads)
                {
                    //BSONObj user_id = BSON("_id" << node.getField("user_id"));
                    //BSONObj user = mongodb_->Find("users", user_id);


                    QString trclass = (counter %2 == 0) ? "odd" : "even";
                    QDateTime timestamp;
                    timestamp.setTime_t(lost_pushpull_payload.getField("timestamp").Number());

                    output.append("<tr class=\"" + trclass +  "\"><td><input type=\"checkbox\" class=\"checkbox\" name=\"id_" + QString::number(counter) + "\" value=\"" + QString::fromStdString(lost_pushpull_payload.getField("_id").OID().str()) + "\"></td><td>" +
                            QString::fromStdString(lost_pushpull_payload.getFieldDotted("data.payload.action").str()) + "</td>" +
                            "<td>" + QString::fromStdString(lost_pushpull_payload.getFieldDotted("data.payload.session_uuid").str()) + "</td>" +
                            "<td>" + QString::fromStdString(lost_pushpull_payload.getField("pushed").boolean()==true? "true" : "false") + "</td>" +
                            "<td>" + timestamp.toString(Qt::SystemLocaleLongDate) + "</td>" +
                            "<td>" + QString::fromStdString(lost_pushpull_payload.getField("worker").str()) + "</td>" +
                            "<td>" + QString::fromStdString(lost_pushpull_payload.getFieldDotted("data.payload.data").jsonString(Strict, false)) + "</td>" +
                            "<td class=\"last\"><a href=\"#\">show</a> | <a href=\"#\">edit</a> | <a href=\"#\">destroy</a></td></tr>");

                    counter++;
                }



                body["data"]=output;
                body["data_count"]=QString::number(counter);




                /*
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());
                */
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           header.render().toUtf8() +
                                           body.render().toUtf8() +
                                           footer.render().toUtf8());


                page->contentType="text/html";
            }

            postEvent(page);


            /* postEvent(new QxtWebPageEvent(event->sessionID,
                                          event->requestID,
                                          index.render().toUtf8()));

            */
}



/********** ADMIN PAGE ************/
void Http_admin::admin_template(QxtWebRequestEvent* event)
{

    /*    QString bodyMessage;
        bodyMessage = buildResponse("error", "ncs version 0.9.1");

        qDebug() << bodyMessage;

      */


    QxtHtmlTemplate header;
    QxtHtmlTemplate body;
    QxtHtmlTemplate footer;
    QxtWebPageEvent *page;

//            if(!body.open("html_templates/admin_users_get.html"))

    if(!body.open(m_basedirectory + "/html_templates/admin_lost_pushpull_payloads_get.html"))
            {
                body["content"]="error 404";
                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                           body.render().toUtf8());
                page->status = 404;
                qDebug() << "error 404";
            }
            else
            {
                header.open(m_basedirectory + "/html_templates/header.html");
                body.open(m_basedirectory + "/html_templates/template.html");
                footer.open(m_basedirectory + "/html_templates/footer.html");

                page = new QxtWebPageEvent(event->sessionID,
                                           event->requestID,
                                       //    header.render().toUtf8() +
                                           body.render().toUtf8());
                                        //   footer.render().toUtf8());


                page->contentType="text/html";
            }
            postEvent(page);
}


void Http_admin::staticfile(QxtWebRequestEvent* event, QString directory, QString filename)
{

    qDebug() << "URL : " <<event->url;

    QxtWebPageEvent *page;
    QString response;

    qDebug() << "STATIC FILE " << directory << " filename " << filename;
    QFile *static_file;
    static_file = new QFile(m_basedirectory + "/html_templates/" + directory + "/" + filename);

    if (!static_file->open(QIODevice::ReadOnly))
    {
        response = "FILE NOT FOUND";
        page = new QxtWebPageEvent(event->sessionID,
                                   event->requestID,
                                   response.toUtf8());
        page->contentType="text/html";
    }
    else
    {
        QFileInfo fi(static_file->fileName());
        QString ext = fi.suffix();
        qDebug() << "EXTENSION : " << ext;

        page = new QxtWebPageEvent(event->sessionID,
                                   event->requestID);
        page->chunked = true;
        page->streaming = false;
        page->dataSource = static_file;

        if (ext == "js")
            page->contentType="text/javascript";
        else if (ext == "css")
            page->contentType="text/css";
        else if (ext == "png")
            page->contentType="image/png";
        else
            page->contentType="text/html";

    }

    postEvent(page);
}


QString Http_admin::errorMessage(QString msg, QString level)
{
    QString error;
    return error = "<div class=\"message " + level + "\"><p>" + msg + "</p></div>";
}

