#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <QString>
#include <QHash>
#include <QUrl>
#include <QSet>


long strtolong(const std::string& str);
long long strtolonglong(const std::string& str);
std::string inttostr(int i);
std::string hex_decode(const std::string &in);
std::string hextostr(const std::string &in);
int timeval_subtract (timeval* result, timeval* x, timeval* y);


typedef struct {
    int userid;
    std::string peer_id;
    std::string user_agent;
    std::string ip_port;
    std::string ip;
    unsigned int port;
    long long uploaded;
    long long downloaded;
    qint64 left;
    time_t last_announced;
    time_t first_announced;
    unsigned int announces;
} bt_peer;

typedef QHash<QString, bt_peer> peer_list;

enum freetype { NORMAL, FREE, NEUTRAL };

typedef struct {
    int id;
    time_t last_seeded;
    long long balance;
    int completed;
    freetype free_torrent;
    QHash<QString, bt_peer> seeders;
    QHash<QString, bt_peer> leechers;
    std::string last_selected_seeder;
    QSet<int> tokened_users;
    time_t last_flushed;
} bt_torrent;

typedef struct {
    int id;
    bool can_leech;
} bt_user;


typedef QHash<QString, bt_torrent> torrent_list;
typedef QHash<QString, bt_user> user_list;


QString getkey(QUrl url, QString key, bool &error, bool fixed_size=false);


#endif // UTIL_H
