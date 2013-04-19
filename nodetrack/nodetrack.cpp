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


#include "nodetrack.h"


Nodetrack::Nodetrack(QString basedirectory, QxtAbstractWebSessionManager * sm, QObject * parent): m_basedirectory(basedirectory), QxtWebSlotService(sm,parent)
{
    mongodb_ = Mongodb::getInstance();
    zeromq_ = Zeromq::getInstance ();


    enumToHTTPmethod.insert(QString("GET"), GET);
    enumToHTTPmethod.insert(QString("POST"), POST);
    enumToHTTPmethod.insert(QString("PUT"), PUT);
    enumToHTTPmethod.insert(QString("DELETE"), DELETE);


    enumToTorrentUpdate.insert(QString("change_passkey"), change_passkey);
    enumToTorrentUpdate.insert(QString("add_torrent"), add_torrent);
    enumToTorrentUpdate.insert(QString("update_torrent"), update_torrent);
    enumToTorrentUpdate.insert(QString("add_token"), add_token);
    enumToTorrentUpdate.insert(QString("remove_token"), remove_token);
    enumToTorrentUpdate.insert(QString("delete_torrent"), delete_torrent);
    enumToTorrentUpdate.insert(QString("add_user"), add_user);
    enumToTorrentUpdate.insert(QString("remove_user"), remove_user);
    enumToTorrentUpdate.insert(QString("update_user"), update_user);
    enumToTorrentUpdate.insert(QString("add_whitelist"), add_whitelist);
    enumToTorrentUpdate.insert(QString("remove_whitelist"), remove_whitelist);
    enumToTorrentUpdate.insert(QString("edit_whitelist"), edit_whitelist);
    enumToTorrentUpdate.insert(QString("update_announce_interval"), update_announce_interval);
    enumToTorrentUpdate.insert(QString("info_torrent"), info_torrent);


    z_message = new zmq::message_t(2);
    //m_context = new zmq::context_t(1);
    z_push_api = new zmq::socket_t(*zeromq_->m_context, ZMQ_PUSH);

    int hwm = 50000;
    z_push_api->setsockopt(ZMQ_SNDHWM, &hwm, sizeof (hwm));
    z_push_api->setsockopt(ZMQ_RCVHWM, &hwm, sizeof (hwm));

    QString directory = "ipc://" + m_basedirectory + "/nodetrack";
    z_push_api->bind(directory.toLatin1());
}



Nodetrack::~Nodetrack()
{
    qDebug() << "Nodetrack : close";
    z_push_api->close ();
    delete(z_push_api);
}




//---------- Worker - does stuff with input
/*
Nodetrack::worker(torrent_list &torrents, user_list &users, std::vector<std::string> &_whitelist, config * conf_obj, Mongo * db_obj, site_comm &sc) : torrents_list(torrents), users_list(users), whitelist(_whitelist), conf(conf_obj), db(db_obj), s_comm(sc) {
    status = OPEN;
}
bool worker::signal(int sig) {
    if (status == OPEN) {
        status = CLOSING;
        std::cout << "closing tracker... press Ctrl-C again to terminate" << std::endl;
        return false;
    } else if (status == CLOSING) {
        std::cout << "shutting down uncleanly" << std::endl;
        return true;
    } else {
        return false;
    }
}


std::string Nodetrack::work(std::string &input, std::string &ip) {
    unsigned int input_length = input.length();

    std::cout << "INPUT : " << input << std::endl;

    //---------- Parse request - ugly but fast. Using substr exploded.
    if(input_length < 60) { // Way too short to be anything useful
        return error("GET string too short");
    }

    size_t pos = 5; // skip GET /

    // Get the passkey
    std::string passkey;
    passkey.reserve(32);
    if(input[37] != '/') {
        return error("Malformed announce");
    }

    for(; pos < 37; pos++) {
        passkey.push_back(input[pos]);
    }

    pos = 38;

    // Get the action
    enum action_t {
        INVALID = 0, ANNOUNCE, SCRAPE, UPDATE
    };
    action_t action = INVALID;

    switch(input[pos]) {
        case 'a':
            action = ANNOUNCE;
            pos += 9;
            break;
        case 's':
            action = SCRAPE;
            pos += 7;
            break;
        case 'u':
            action = UPDATE;
            pos += 7;
            break;
    }
    if(action == INVALID) {
        return error("invalid action");
    }

    if ((status != OPEN) && (action != UPDATE)) {
        return error("The tracker is temporarily unavailable.");
    }

    // Parse URL params
    std::list<std::string> infohashes; // For scrape only

    std::map<std::string, std::string> params;
    std::string key;
    std::string value;
    bool parsing_key = true; // true = key, false = value

    for(; pos < input_length; ++pos) {
        if(input[pos] == '=') {
            parsing_key = false;
        } else if(input[pos] == '&' || input[pos] == ' ') {
            parsing_key = true;
            if(action == SCRAPE && key == "info_hash") {
                infohashes.push_back(value);
            } else {

                params[key] = value;
            }
            key.clear();
            value.clear();
            if(input[pos] == ' ') {
                break;
            }
        } else {
            if(parsing_key) {
                key.push_back(input[pos]);
            } else {
                value.push_back(input[pos]);
            }
        }
    }

    pos += 10; // skip HTTP/1.1 - should probably be +=11, but just in case a client doesn't send \r

    // Parse headers
    std::map<std::string, std::string> headers;
    parsing_key = true;
    bool found_data = false;

    for(; pos < input_length; ++pos) {
        if(input[pos] == ':') {
            parsing_key = false;
            ++pos; // skip space after :
        } else if(input[pos] == '\n' || input[pos] == '\r') {
            parsing_key = true;

            if(found_data) {
                found_data = false; // dodge for getting around \r\n or just \n
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                headers[key] = value;
                key.clear();
                value.clear();
            }
        } else {
            found_data = true;
            if(parsing_key) {
                key.push_back(input[pos]);
            } else {
                value.push_back(input[pos]);
            }
        }
    }



    if(action == UPDATE) {
        if(passkey == conf->site_password) {
            return update(params);
        } else {
            return error("Authentication failure");
        }
    }

    // Either a scrape or an announce

    user_list::iterator u = users_list.find(passkey);
    if(u == users_list.end()) {
        return error("passkey not found");
    }

    if(action == ANNOUNCE) {
        boost::mutex::scoped_lock lock(db->torrent_list_mutex);
        // Let's translate the infohash into something nice
        // info_hash is a url encoded (hex) base 20 number
        std::string info_hash_decoded = hex_decode(params["info_hash"]);
        torrent_list::iterator tor = torrents_list.find(info_hash_decoded);

               //DELETE ME
                std::cout << "Info hash is " << info_hash_decoded << std::endl;

        if(tor == torrents_list.end()) {
            return error("unregistered torrent");
        }
        return announce(tor->second, u->second, params, headers, ip);
    } else {
        return scrape(infohashes);
    }
}

std::string Nodetrack::error(std::string err) {
    std::string output = "d14:failure reason";
    output += inttostr(err.length());
    output += ':';
    output += err;
    output += 'e';
    return output;
}
*/


/**** ANNOUNCE

GET /tq27ols4afs3guh89s3t361sizm7bfzs/announce?info_hash=%ed%40%3fIU%eff%a8%d8%9b%df%1d%93%aa%9c%3f%edm%0c%db&peer_id=-TR2610-85aivt2ar96p&port=51413&uploaded=0&downloaded=0&left=1076965&numwant=80&key=236bcff5&compact=1&supportcrypto=1&event=started
 MUST TO CHANGE TO :
GET /u/tq27ols4afs3guh89s3t361sizm7bfzs/announce?info_hash=%ed%40%3fIU%eff%a8%d8%9b%df%1d%93%aa%9c%3f%edm%0c%db&peer_id=-TR2610-85aivt2ar96p&port=51413&uploaded=0&downloaded=0&left=1076965&numwant=80&key=236bcff5&compact=1&supportcrypto=1&event=started

An example of this GET message could be:
http://some.tracker.com:999/announce
?info_hash=12345678901234567890
&peer_id=ABCDEFGHIJKLMNOPQRST
&ip=255.255.255.255
&port=6881
&downloaded=1234
&left=98765
&event=stopped


*/

//void Nodetrack::u(QxtWebRequestEvent* event, QString user_token, QString action, QString info_hash="", QString peer_id="", QString ip="", QString port="", QString uploaded="", QString downloaded="", QString left="", QString numwant="", QString key="", QString compact="", QString supportcrypto="", QString a_event="")
void Nodetrack::u(QxtWebRequestEvent* event, QString user_token, QString action)
{

    qDebug() << "RECEIVE ANNOUNCE : " << "METHOD : " << event->method << " headers : " << event->headers;

    QString bodyMessage="";


//    QHash <QString, QString> params;

    qDebug() << "USER TOKEN : " << user_token;

    qDebug() << "ACTION : " << action;

    switch (enumToHTTPmethod[event->method.toUpper()])
    {
    case GET:
        qDebug() << "HTTP GET";

        if (action == "announce")
        {
            /*
            params["user_token"] = user_token;
            params["action"] = action;
            params["info_hash"] = info_hash;
            params["peer_id"] = peer_id;
            params["ip"] = ip;
            params["port"] = port;
            params["uploaded"] = uploaded;
            params["downloaded"] = downloaded;
            params["left"] = left;
            params["numwant"] = numwant;
            params["key"] = key;
            params["numwant"] = numwant;
            params["compact"] = compact;
            params["supportcrypto"] = supportcrypto;
            params["event"] = a_event;
*/
            get_announce(event, user_token);
            return;
        }
        else bodyMessage = "NOT ANNOUNCE REQUEST";
        break;
    case POST:
        qDebug() << "HTTP POST";
        bodyMessage = "HTTP POST not implemented";

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
        postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 bodyMessage.toUtf8()));



}




/********** INDEX PAGE ************/
void Nodetrack::index(QxtWebRequestEvent* event)
{
/*    QString bodyMessage;
    bodyMessage = buildResponse("error", "ncs version 0.9.1");

    qDebug() << bodyMessage;

  */

    QxtHtmlTemplate index;
    QxtWebPageEvent *page;

        if(!index.open(m_basedirectory + "/html_templates/torrent_index.html"))
        {
            index["content"]="error 404";
            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());
            page->status=404;
            qDebug() << "error 404";
        }
        else
        {
            index["ncs_version"]="0.9.7";

            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());

            page->contentType="text/html";
        }

        postEvent(page);


        /* postEvent(new QxtWebPageEvent(event->sessionID,
                                      event->requestID,
                                      index.render().toUtf8()));

        */
}


void Nodetrack::torrent_post(QxtWebRequestEvent* event, QString user_token)
{
    QString bodyMessage="";

    if (event->content.isNull())
    {
        bodyMessage = buildResponse("error", "data", "empty payload");
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     bodyMessage.toUtf8()));
        return;
    }


    BSONObj query = BSON("tracker.token" << user_token.toStdString());
    BSONObj user = mongodb_->Find("users", query);

    if (user.nFields() == 0)
    {
        user_token = "passkey not found";
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     user_token.toUtf8()));
        return;
    }


    QString s_tags = event->headers.value("X-torrent-tags");
    if (s_tags.isEmpty()) s_tags = "private";
    QStringList list_tags = s_tags.split(",");

    int counter=0;
    BSONObjBuilder b_tags;

    foreach (QString tag, list_tags)
    {
        b_tags.append(QString::number(counter).toStdString(), tag.simplified().toStdString());
        counter++;
    }

    BSONObj tags = b_tags.obj();


    QxtWebContent *myContent = event->content;
    qDebug() << "Bytes to read: " << myContent->unreadBytes();
    myContent->waitForAllContent();

    //QByteArray requestContent = QByteArray::fromBase64(myContent->readAll());
    QByteArray requestContent = myContent->readAll();

    BSONObjBuilder b_torrent;
    b_torrent.genOID();
    b_torrent.append("user_id", user.getField("_id").OID());
    b_torrent.appendArray("tags", tags);
    // mongo::BinDataType(0) == BinDataGeneral
    b_torrent.appendBinData("data", requestContent.size(), BinDataGeneral, requestContent);

    BSONObj torrent = b_torrent.obj();
    mongodb_->Insert("torrents", torrent);

    postEvent(new QxtWebPageEvent(event->sessionID,
                                  event->requestID,
                                  "OK"));

}



void Nodetrack::torrent(QxtWebRequestEvent* event, QString user_token)
{

        qDebug() << "ADMIN : " << "METHOD : " << event->method << " headers : " << event->headers << " user_token " << user_token;

        QString bodyMessage="";


        switch (enumToHTTPmethod[event->method.toUpper()])
        {
        case GET:
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


            /*
            if (action == "login")
            {
                admin_login(event);
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
*/
            break;
        case POST:
            //qDebug() << "HTTP POST not implemented";
            //bodyMessage = buildResponse("error", "HTTP POST not implemented");

                torrent_post(event, user_token);

            break;
        case PUT:
            qDebug() << "HTTP PUT not implemented";
            bodyMessage = buildResponse("error", "HTTP PUT not implemented");

            break;
        case DELETE:
            qDebug() << "HTTP DELETE not implemented";
            bodyMessage = buildResponse("error", "HTTP DELETE not implemented");

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
void Nodetrack::admin(QxtWebRequestEvent* event, QString action)
{
/*    QString bodyMessage;
    bodyMessage = buildResponse("error", "ncs version 0.9.1");

    qDebug() << bodyMessage;

  */

    QxtHtmlTemplate index;
    QxtWebPageEvent *page;

        if(!index.open(m_basedirectory + "/html_templates/admin.html"))
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
            index["content"]="Ncs admin pages";


            QString html;
            html.append("<select>");

            for (int i=0; i<5; i++)
            {
                html.append("<option value=\"volvo\">Volvo</option>");
            }

            html.append("</select>");

            index["prot"]=html;

            page = new QxtWebPageEvent(event->sessionID,
                                       event->requestID,
                                       index.render().toUtf8());

            page->contentType="text/html";
        }

        postEvent(page);


        /* postEvent(new QxtWebPageEvent(event->sessionID,
                                      event->requestID,
                                      index.render().toUtf8()));

        */
}


/**** ANNOUNCE

GET /tq27ols4afs3guh89s3t361sizm7bfzs/announce?info_hash=%ed%40%3fIU%eff%a8%d8%9b%df%1d%93%aa%9c%3f%edm%0c%db&peer_id=-TR2610-85aivt2ar96p&port=51413&uploaded=0&downloaded=0&left=1076965&numwant=80&key=236bcff5&compact=1&supportcrypto=1&event=started
 MUST TO CHANGE TO :
GET /u/tq27ols4afs3guh89s3t361sizm7bfzs/announce?info_hash=%ed%40%3fIU%eff%a8%d8%9b%df%1d%93%aa%9c%3f%edm%0c%db&peer_id=-TR2610-85aivt2ar96p&port=51413&uploaded=0&downloaded=0&left=1076965&numwant=80&key=236bcff5&compact=1&supportcrypto=1&event=started

An example of this GET message could be:
http://some.tracker.com:999/announce
?info_hash=12345678901234567890
&peer_id=ABCDEFGHIJKLMNOPQRST
&ip=255.255.255.255
&port=6881
&downloaded=1234
&left=98765
&event=stopped


http://localhost:6969/u/tq27ols4afs3guh89s3t361sizm7bfzs/announce?info_hash=12345678901234567890&peer_id=ABCDEFGHIJKLMNOPQRST&ip=255.255.255.255&port=6881&downloaded=1234&left=98765&event=stopped&compact=1

****************/

//std::string Nodetrack::get_announce(torrent &tor, user &u, std::map<std::string, std::string> &params, std::map<std::string, std::string> &headers, std::string &ip)
void Nodetrack::get_announce(QxtWebRequestEvent* event, QString user_token)
{

    bool error = false;
    QUrl url = event->url;


    //---------- Parse request - ugly but fast. Using substr exploded.

    if(url.toString().length() < 60) { // Way too short to be anything useful       
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     "GET string too short"));
        return;
    }


    //QString user_key = getkey(url, "user_key", error, true);

   // QString user_token = params["user_token"];

    if (user_token.length() != 32)
    {
        user_token = "Malformed announce";
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     user_token.toUtf8()));
        return;

    }

    BSONObj query = BSON("tracker.token" << user_token.toStdString());
    BSONObj user = mongodb_->Find("users", query);

    if (user.nFields() == 0)
    {
        user_token = "passkey not found";
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     user_token.toUtf8()));
        return;
    }






    QString info_hash = getkey(url, "info_hash", error, true);
    if (error)
    {
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     info_hash.toUtf8()));
        return;
    }

    QString peer_id = getkey(url, "peer_id", error, true);
    if (error)
    {
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     peer_id.toUtf8()));
        return;
    }

    QString port = getkey(url, "port", error);
    if (error)
    {
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     port.toUtf8()));
        return;
    }

    error = true;


    QString uploaded = getkey(url, "uploaded", error);
    QString downloaded = getkey(url, "downloaded", error);
    QString left = getkey(url, "left", error);
    QString numwant = getkey(url, "numwant", error);

    QString compact = getkey(url, "compact", error);
    if (compact != "1")
    {
        compact = "Your client does not support compact announces";

        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     compact.toUtf8()));
        return;
    }

    QString l_event = getkey(url, "event", error);
    QString key = getkey(url, "key", error);




    time_t cur_time = time(NULL);

    qint64 l_left = strtolonglong(left.toStdString());
    qint64 l_uploaded = std::max(0ll, strtolonglong(uploaded.toStdString()));
    qint64 l_downloaded = std::max(0ll, strtolonglong(downloaded.toStdString()));

    bool inserted = false; // If we insert the peer as opposed to update
    bool update_torrent = false; // Whether or not we should update the torrent in the DB
    bool expire_token = false; // Whether or not to expire a token after torrent completion






/*
    string info_hash_decoded = hex_decode(info_hash.toStdString());
    std::cout << "info_hash_decoded : " << info_hash_decoded << std::endl;




    torrent_list::iterator tor = torrents_list.find(info_hash_decoded);

               //DELETE ME
                std::cout << "Info hash is " << info_hash_decoded << std::endl;

        if(tor == torrents_list.end()) {
            return error("unregistered torrent");
        }
        return announce(tor->second, u->second, params, headers, ip);
*/



    /*
    std::map<QString, QString>::const_iterator peer_id_iterator = peer_id;
    if(peer_id_iterator == params.end()) {       
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     "no peer id"));
        return;

    }
    std::string peer_id = peer_id_iterator->second;
    peer_id = hex_decode(peer_id);

    QVector<QString> whitelist;
    mongodb_->bt_load_whitelist(whitelist);


    if(whitelist.size() > 0) {
        bool found = false; // Found client in whitelist?
        for(unsigned int i = 0; i < whitelist.size(); i++) {
            if(peer_id.find(whitelist[i]) == 0) {
                found = true;
                break;
            }
        }

        if(!found) {
            postEvent(new QxtWebPageEvent(event->sessionID,
                                         event->requestID,
                                         "Your client is not on the whitelist"));
            return;
        }
    }

    peer * p;
    peer_list::iterator i;
    // Insert/find the peer in the torrent list
    if(l_left > 0 || params["event"] == "completed") {
        if(u.can_leech == false) {
            return error("Access denied, leeching forbidden");
        }

        i = tor.leechers.find(peer_id);
        if(i == tor.leechers.end()) {
            peer new_peer;
            std::pair<peer_list::iterator, bool> insert
            = tor.leechers.insert(std::pair<std::string, peer>(peer_id, new_peer));

            p = &(insert.first->second);
            inserted = true;
        } else {
            p = &i->second;
        }
    } else {
        i = tor.seeders.find(peer_id);
        if(i == tor.seeders.end()) {
            peer new_peer;
            std::pair<peer_list::iterator, bool> insert
            = tor.seeders.insert(std::pair<std::string, peer>(peer_id, new_peer));

            p = &(insert.first->second);
            inserted = true;
        } else {
            p = &i->second;
        }

        tor.last_seeded = cur_time;
    }

    // Update the peer
    p->left = l_left;
    long long upspeed = 0;
    long long downspeed = 0;
    long long real_uploaded_change = 0;
    long long real_downloaded_change = 0;

    if(inserted || params["event"] == "started" || uploaded < p->uploaded || downloaded < p->downloaded) {
        //New peer on this torrent
        update_torrent = true;
        p->userid = u.id;
        p->peer_id = peer_id;
        p->user_agent = headers["user-agent"];
        p->first_announced = cur_time;
        p->last_announced = 0;
        p->uploaded = uploaded;
        p->downloaded = downloaded;
        p->announces = 1;
    } else {
        long long uploaded_change = 0;
        long long downloaded_change = 0;
        p->announces++;

        if(uploaded != p->uploaded) {
            uploaded_change = uploaded - p->uploaded;
            real_uploaded_change = uploaded_change;
            p->uploaded = uploaded;
        }
        if(downloaded != p->downloaded) {
            downloaded_change = downloaded - p->downloaded;
            real_downloaded_change = downloaded_change;
            p->downloaded = downloaded;
        }
        if(uploaded_change || downloaded_change) {
            long corrupt = strtolong(params["corrupt"]);
            tor.balance += uploaded_change;
            tor.balance -= downloaded_change;
            tor.balance -= corrupt;
            update_torrent = true;

            if(cur_time > p->last_announced) {
                upspeed = uploaded_change / (cur_time - p->last_announced);
                downspeed = downloaded_change / (cur_time - p->last_announced);
            }
            std::set<int>::iterator sit = tor.tokened_users.find(u.id);
            if (tor.free_torrent == NEUTRAL) {
                downloaded_change = 0;
                uploaded_change = 0;
            } else if(tor.free_torrent == FREE || sit != tor.tokened_users.end()) {
                if(sit != tor.tokened_users.end()) {
                    expire_token = true;
                    db->record_token(u.id, tor.id, static_cast<long long>(downloaded_change));
                }
                downloaded_change = 0;
            }

            if(uploaded_change || downloaded_change) {
                mongodb_->bt_record_user(u.id, uploaded_change, downloaded_change);
            }
        }
    }
    p->last_announced = cur_time;

    std::map<std::string, std::string>::const_iterator param_ip = params.find("ip");
    if(param_ip != params.end()) {
        ip = param_ip->second;
    } else {
        param_ip = params.find("ipv4");
        if(param_ip != params.end()) {
            ip = param_ip->second;
        }
    }

    unsigned int port = strtolong(params["port"]);
    // Generate compact ip/port string
    if(inserted || port != p->port || ip != p->ip) {
        p->port = port;
        p->ip = ip;
        p->ip_port = "";
        char x = 0;
        for(size_t pos = 0, end = ip.length(); pos < end; pos++) {
            if(ip[pos] == '.') {
                p->ip_port.push_back(x);
                x = 0;
                continue;
            } else if(!isdigit(ip[pos])) {
                return error("Unexpected character in IP address. Only IPv4 is currently supported");
            }
            x = x * 10 + ip[pos] - '0';
        }
        p->ip_port.push_back(x);
        p->ip_port.push_back(port >> 8);
        p->ip_port.push_back(port & 0xFF);
        if(p->ip_port.length() != 6) {
            return error("Specified IP address is of a bad length");
        }
    }

    // Select peers!
    unsigned int numwant;
    std::map<std::string, std::string>::const_iterator param_numwant = params.find("numwant");
    if(param_numwant == params.end()) {
        numwant = 50;
    } else {
        numwant = std::min(50l, strtolong(param_numwant->second));
    }

    int snatches = 0;
    int active = 1;
    if(params["event"] == "stopped") {
        update_torrent = true;
        active = 0;
        numwant = 0;

        if(l_left > 0) {
            if(tor.leechers.erase(peer_id) == 0) {
                std::cout << "Tried and failed to remove seeder from torrent " << tor.id << std::endl;
            }
        } else {
            if(tor.seeders.erase(peer_id) == 0) {
                std::cout << "Tried and failed to remove leecher from torrent " << tor.id << std::endl;
            }
        }
    } else if(params["event"] == "completed") {
        snatches = 1;
        update_torrent = true;
        tor.completed++;

        db->record_snatch(u.id, tor.id, cur_time, ip);

        // User is a seeder now!
        tor.seeders.insert(std::pair<std::string, peer>(peer_id, *p));
        tor.leechers.erase(peer_id);
        if(expire_token) {
            (&s_comm)->expire_token(tor.id, u.id);
            tor.tokened_users.erase(u.id);
        }
        // do cache expire
    }

    std::string peers;
    if(numwant > 0) {
        peers.reserve(300);
        unsigned int found_peers = 0;
        if(l_left > 0) { // Show seeders to leechers first
            if(tor.seeders.size() > 0) {
                // We do this complicated stuff to cycle through the seeder list, so all seeders will get shown to leechers

                // Find out where to begin in the seeder list
                peer_list::const_iterator i;
                if(tor.last_selected_seeder == "") {
                    i = tor.seeders.begin();
                } else {
                    i = tor.seeders.find(tor.last_selected_seeder);
                    i++;
                    if(i == tor.seeders.end()) {
                        i = tor.seeders.begin();
                    }
                }

                // Find out where to end in the seeder list
                peer_list::const_iterator end;
                if(i == tor.seeders.begin()) {
                    end = tor.seeders.end();
                } else {
                    end = i;
                    end--;
                }

                // Add seeders
                while(i != end && found_peers < numwant) {
                    if(i == tor.seeders.end()) {
                        i = tor.seeders.begin();
                    }
                    peers.append(i->second.ip_port);
                    found_peers++;
                    tor.last_selected_seeder = i->second.peer_id;
                    i++;
                }
            }

            if(found_peers < numwant && tor.leechers.size() > 1) {
                for(peer_list::const_iterator i = tor.leechers.begin(); i != tor.leechers.end() && found_peers < numwant; i++) {
                    if(i->second.ip_port == p->ip_port) { // Don't show leechers themselves
                        continue;
                    }
                    found_peers++;
                    peers.append(i->second.ip_port);
                }

            }
        } else if(tor.leechers.size() > 0) { // User is a seeder, and we have leechers!
            for(peer_list::const_iterator i = tor.leechers.begin(); i != tor.leechers.end() && found_peers < numwant; i++) {
                found_peers++;
                peers.append(i->second.ip_port);
            }
        }
    }

    if(update_torrent || tor.last_flushed + 3600 < cur_time) {
        tor.last_flushed = cur_time;

        db->record_torrent(tor.id, tor.seeders.size(), tor.leechers.size(), snatches, tor.balance);
    }

    db->record_peer(u.id, tor.id, active, peer_id, headers["user-agent"], ip, uploaded, downloaded, upspeed, downspeed, left, (cur_time - p->first_announced), p->announces);

    if (real_uploaded_change > 0 || real_downloaded_change > 0) {
        db->record_peer_hist(u.id, static_cast<long long>(downloaded), static_cast<long long>(left), static_cast<long long>(l_uploaded), static_cast<long long>(upspeed), static_cast<long long>(downspeed), static_cast<long long>((cur_time - p->first_announced)), peer_id, tor.id);
    }

    std::string response = "d8:intervali";
    response.reserve(350);
    response += inttostr(conf->announce_interval+std::min((size_t)600, tor.seeders.size())); // ensure a more even distribution of announces/second
    response += "e12:min intervali";
    response += inttostr(conf->announce_interval);
    response += "e5:peers";
    if(peers.length() == 0) {
        response += "0:";
    } else {
        response += inttostr(peers.length());
        response += ":";
        response += peers;
    }
    */
    QString response = "d8:intervali";
    response += "8:completei";
    //response += inttostr(tor.seeders.size());
    response += "e10:incompletei";
   // response += inttostr(tor.leechers.size());
    response += "e10:downloadedi";
    //response += inttostr(tor.completed);
    response += "ee";

    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 response.toUtf8()));

}

//std::string Nodetrack::scrape(const std::list<std::string> &infohashes) {
void Nodetrack::scrape(QxtWebRequestEvent* event, QString key)
{
    /*
    bool error = false;
    QUrl url = event->url;
    QString output = "d5:filesd";

    QString info_hash = getkey(url, "info_hash", error, true);
    if (error)
    {
        postEvent(new QxtWebPageEvent(event->sessionID,
                                     event->requestID,
                                     info_hash.toUtf8()));
        return;
    }


    QStringList list_info_hash = url.allQueryItemValues("info_hash");
    QStringListIterator hash;

    for (hash = list_info_hash.begin(); hash != list_info_hash.end(); ++hash)
    {
        std::string infohash = *hash;
        infohash = hex_decode(infohash);

        std::unordered_map<std::string, bt_torrent> torrents_list;
        mongodb_->bt_load_torrents(torrents_list);


        torrent_list::iterator tor = torrents_list.find(infohash);
        if(tor == torrents_list.end()) {
            continue;
        }
        torrent *t = &(tor->second);

        output += inttostr(infohash.length());
        output += ':';
        output += infohash;
        output += "d8:completei";
        output += inttostr(t->seeders.size());
        output += "e10:incompletei";
        output += inttostr(t->leechers.size());
        output += "e10:downloadedi";
        output += inttostr(t->completed);
        output += "ee";
    }
    output+="ee";
    postEvent(new QxtWebPageEvent(event->sessionID,
                             event->requestID,
                             output.toUtf8()));
*/
}

//TODO: Restrict to local IPs
//std::string Nodetrack::update(std::map<std::string, std::string> &params)
void Nodetrack::update(QxtWebRequestEvent* event, QString action)
{
/*
    switch (enumToTorrentUpdate[action])
    {
    case change_passkey:
        std::string oldpasskey = params["oldpasskey"];
        std::string newpasskey = params["newpasskey"];
        user_list::iterator i = users_list.find(oldpasskey);
        if (i == users_list.end())
        {
            std::cout << "No user with passkey " << oldpasskey << " exists when attempting to change passkey to " << newpasskey << std::endl;
        }
        else
        {
            users_list[newpasskey] = i->second;;
            users_list.erase(oldpasskey);
            std::cout << "changed passkey from " << oldpasskey << " to " << newpasskey << " for user " << i->second.id << std::endl;
        }
        break;

    case add_torrent:
        torrent t;
        t.id = strtolong(params["id"]);
        std::string info_hash = params["info_hash"];
        info_hash = hex_decode(info_hash);
        if(params["freetorrent"] == "0")
        {
            t.free_torrent = NORMAL;
        }
        else if(params["freetorrent"] == "1")
        {
            t.free_torrent = FREE;
        }
        else
        {
            t.free_torrent = NEUTRAL;
        }
        t.balance = 0;
        t.completed = 0;
        t.last_selected_seeder = "";
        torrents_list[info_hash] = t;
        std::cout << "Added torrent " << t.id<< ". FL: " << t.free_torrent << " " << params["freetorrent"] << std::endl;

        break;

    case update_torrent:
        std::string info_hash = params["info_hash"];
        info_hash = hex_decode(info_hash);
        freetype fl;
        if(params["freetorrent"] == "0")
        {
            fl = NORMAL;
        }
        else if(params["freetorrent"] == "1")
        {
            fl = FREE;
        }
        else
        {
            fl = NEUTRAL;
        }
        auto torrent_it = torrents_list.find(info_hash);
        if (torrent_it != torrents_list.end())
        {
            torrent_it->second.free_torrent = fl;
            std::cout << "Updated torrent " << torrent_it->second.id << " to FL " << fl << std::endl;
        }
        else
        {
            std::cout << "Failed to find torrent " << info_hash << " to FL " << fl << std::endl;
        }
        break;

    case update_torrents:
        // Each decoded infohash is exactly 20 characters long.
        std::string info_hashes = params["info_hashes"];
        info_hashes = hex_decode(info_hashes);
        freetype fl;
        if(params["freetorrent"] == "0")
        {
            fl = NORMAL;
        }
        else if(params["freetorrent"] == "1")
        {
            fl = FREE;
        }
        else
        {
            fl = NEUTRAL;
        }
        for(unsigned int pos = 0; pos < info_hashes.length(); pos += 20) {
            std::string info_hash = info_hashes.substr(pos, 20);
            auto torrent_it = torrents_list.find(info_hash);
            if (torrent_it != torrents_list.end()) {
                torrent_it->second.free_torrent = fl;
                std::cout << "Updated torrent " << torrent_it->second.id << " to FL " << fl << std::endl;
            } else {
                std::cout << "Failed to find torrent " << info_hash << " to FL " << fl << std::endl;
            }
        }
        break;

    case add_token:
        std::string info_hash = hex_decode(params["info_hash"]);
        int user_id = atoi(params["userid"].c_str());
        auto torrent_it = torrents_list.find(info_hash);
        if (torrent_it != torrents_list.end())
        {
            torrent_it->second.tokened_users.insert(user_id);
        }
        else
        {
            std::cout << "Failed to find torrent to add a token for user " << user_id << std::endl;
        }

        break;

    case remove_token:
        std::string info_hash = hex_decode(params["info_hash"]);
        int user_id = atoi(params["userid"].c_str());
        auto torrent_it = torrents_list.find(info_hash);
        if (torrent_it != torrents_list.end())
        {
            torrent_it->second.tokened_users.erase(user_id);
        }
        else
        {
            std::cout << "Failed to find torrent " << info_hash << " to remove token for user " << user_id << std::endl;
        }

        break;

    case delete_torrent:
        std::string info_hash = params["info_hash"];
        info_hash = hex_decode(info_hash);
        auto torrent_it = torrents_list.find(info_hash);
        if (torrent_it != torrents_list.end())
        {
            std::cout << "Deleting torrent " << torrent_it->second.id << std::endl;
            torrents_list.erase(torrent_it);
        }
        else
        {
            std::cout << "Failed to find torrent " << info_hash << " to delete " << std::endl;
        }

        break;

    case add_user:
        std::string passkey = params["passkey"];
        unsigned int id = strtolong(params["id"]);
        user u;
        u.id = id;
        u.can_leech = 1;
        users_list[passkey] = u;
        std::cout << "Added user " << id << std::endl;

        break;

    case remove_user:
        std::string passkey = params["passkey"];
        users_list.erase(passkey);
        std::cout << "Removed user " << passkey << std::endl;

        break;

    case remove_users:
        // Each passkey is exactly 32 characters long.
        std::string passkeys = params["passkeys"];
        for(unsigned int pos = 0; pos < passkeys.length(); pos += 32)
        {
            std::string passkey = passkeys.substr(pos, 32);
            users_list.erase(passkey);
            std::cout << "Removed user " << passkey << std::endl;
        }
        break;

    case update_user:
        std::string passkey = params["passkey"];
        bool can_leech = true;
        if(params["can_leech"] == "0") {
            can_leech = false;
        }
        user_list::iterator i = users_list.find(passkey);
        if (i == users_list.end())
        {
            std::cout << "No user with passkey " << passkey << " found when attempting to change leeching status!" << std::endl;
        }
        else
        {
            users_list[passkey].can_leech = can_leech;
            std::cout << "Updated user " << passkey << std::endl;
        }

        break;
    case add_whitelist:
        std::string peer_id = params["peer_id"];
        whitelist.push_back(peer_id);
        std::cout << "Whitelisted " << peer_id << std::endl;

        break;

    case remove_whitelist:
        std::string peer_id = params["peer_id"];
        for(unsigned int i = 0; i < whitelist.size(); i++)
        {
            if(whitelist[i].compare(peer_id) == 0)
            {
                whitelist.erase(whitelist.begin() + i);
                break;
            }
        }
        std::cout << "De-whitelisted " << peer_id << std::endl;

        break;

    case edit_whitelist:
        std::string new_peer_id = params["new_peer_id"];
        std::string old_peer_id = params["old_peer_id"];
        for(unsigned int i = 0; i < whitelist.size(); i++)
        {
            if(whitelist[i].compare(old_peer_id) == 0)
            {
                whitelist.erase(whitelist.begin() + i);
                break;
            }
        }
        whitelist.push_back(new_peer_id);
        std::cout << "Edited whitelist item from " << old_peer_id << " to " << new_peer_id << std::endl;


        break;

    case update_announce_interval:
        unsigned int interval = strtolong(params["new_announce_interval"]);
        conf->announce_interval = interval;
        std::cout << "Edited announce interval to " << interval << std::endl;

        break;

    case info_torrent:
        std::string info_hash_hex = params["info_hash"];
        std::string info_hash = hex_decode(info_hash_hex);
        std::cout << "Info for torrent '" << info_hash_hex << "'" << std::endl;
        auto torrent_it = torrents_list.find(info_hash);
        if (torrent_it != torrents_list.end())
        {
            std::cout << "Torrent " << torrent_it->second.id
                << ", freetorrent = " << torrent_it->second.free_torrent << std::endl;
        }
        else
        {
            std::cout << "Failed to find torrent " << info_hash_hex << std::endl;
        }

        break;
    }
*/

    postEvent(new QxtWebPageEvent(event->sessionID,
                                 event->requestID,
                                 "success"));
}

void Nodetrack::reap_peers() {
    std::cout << "started reaper" << std::endl;
   // boost::thread thread(&worker::do_reap_peers, this);
}

void Nodetrack::do_reap_peers() {
    /*
    db->logger_ptr->log("Began worker::do_reap_peers()");
    time_t cur_time = time(NULL);
    unsigned int reaped = 0;
    std::unordered_map<std::string, torrent>::iterator i = torrents_list.begin();
    for(; i != torrents_list.end(); i++) {
        std::map<std::string, peer>::iterator p = i->second.leechers.begin();
        std::map<std::string, peer>::iterator del_p;
        while(p != i->second.leechers.end()) {
            if(p->second.last_announced + conf->peers_timeout < cur_time) {
                del_p = p;
                p++;
                boost::mutex::scoped_lock lock(db->torrent_list_mutex);
                i->second.leechers.erase(del_p);
                reaped++;
            } else {
                p++;
            }
        }
        p = i->second.seeders.begin();
        while(p != i->second.seeders.end()) {
            if(p->second.last_announced + conf->peers_timeout < cur_time) {
                del_p = p;
                p++;
                boost::mutex::scoped_lock lock(db->torrent_list_mutex);
                i->second.seeders.erase(del_p);
                reaped++;
            } else {
                p++;
            }
        }
    }
    std::cout << "Reaped " << reaped << " peers" << std::endl;
    db->logger_ptr->log("Completed worker::do_reap_peers()");
    */
}





QString Nodetrack::buildResponse(QString action, QString data1, QString data2)
{
    QVariantMap data;
    QString body;


    if (action == "update")
    {
        data.insert("uuid", data1);
    }
    else if (action == "register")
    {
        data.insert("node_uuid", data1);
        data.insert("node_password", data2);
    }
    else if (action == "push" || action == "publish" || action == "create")
    {
        data.insert("uuid", data1);
    }
    else if (action == "file")
    {
        data.insert("gridfs", data1);
    }
    else if (action == "error")
    {
        data.insert("error", data1);
        data.insert("code", data2);
    }
    else if (action == "status")
    {
        data.insert("status", data1);
    }
    body = QxtJSON::stringify(data);

    return body;
}
