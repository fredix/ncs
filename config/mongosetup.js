// SET A TTL
// all publish payloads are store into pubsub_payloads collection. You must have to set a ttl. In this example mongodb flush payloads created after 172800 seconds (48 hours).
db.createCollection("pubsub_payloads");
db.pubsub_payloads.ensureIndex( { "ttl": 1 }, { expireAfterSeconds: 172800 } )
// http connections in the admin web interface will expire after 24 hours
db.createCollection("http_sessions_payloads");
db.http_sessions_payloads.ensureIndex( { "ttl": 1 }, { expireAfterSeconds: 86400 } )
db.createCollection("lock_collections");
db.lock_collections.ensureIndex( { "ttl": 1 }, { expireAfterSeconds: 120 } )

db.permissions.save({name: "ftp", staff: false, values: ["site_torrents_comment","site_torrents_search","site_torrents_upload","site_torrents_view","site_torrents_download","site_users_view","site_kb_search","site_kb_view"]});


// SET permission about bittorrent tracker
//db.bt_permissions.save({id: 2, name: "User", staff: false, values: ["site_torrents_comment","site_torrents_search","site_torrents_upload","site_torrents_view","site_torrents_download","site_users_view","site_kb_search","site_kb_view"]});
//db.bt_permissions.save({id: 3, name: "Member", staff: false, values: ["site_torrents_comment","site_torrents_search","site_torrents_upload","site_torrents_view","site_torrents_download","site_users_view","site_kb_search","site_kb_view"]});
//db.bt_permissions.save({id: 5, name: "Elite", staff: false, values: ["site_torrents_comment","site_torrents_search","site_torrents_upload","site_torrents_view","site_torrents_download","site_torrents_tags_delete","site_torrents_tags_add","site_users_view","site_kb_search","site_kb_view"]});
//db.bt_permissions.save({id: 11, name: "Moderator", staff: true, values: ["site_torrents_comment","site_torrents_search","site_torrents_upload","site_torrents_view","site_torrents_download","site_torrents_tags_delete","site_torrents_tags_add","site_users_view","site_kb_search","site_kb_view","site_kb_new","site_kb_edit","site_kb_delete"]});
//db.bt_permissions.save({id: 1, name: "Admin", staff: true, values: ["site_torrents_comment","site_torrents_search","site_torrents_upload","site_torrents_view","site_torrents_download","site_torrents_tags_delete","site_torrents_tags_add","site_users_view","site_kb_search","site_kb_view","site_kb_new","site_kb_edit","site_kb_delete","site_users_edit"]});


