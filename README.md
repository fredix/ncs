# NCS

ncs is the nodecast server. Go to http://nodecast.github.com/ncs/


## INSTALL

first, install some debs :


```bash
sudo apt-get install g++ make autoconf scons libboost-all-dev libqt4-dev libavahi-compat-libdnssd-dev
```

next, get submodule libraries :

```bash
git submodule update --init
```

make the zeromq lib :

```bash
cd externals/zeromq
./configure
make
sudo make install
```

make the mongodb client :

```bash
cd externals/mongo-cxx-driver/
scons mongoclient
```

make the qxmpp client :

```bash
cd externals/qxmpp
qmake
make
sudo make install
```

make the libqxt library

```bash
cd externals/libqxt
./configure
make
sudo make install
```

make the nodecast server :

```bash
qmake
make
```

set the ncs's directory


```bash
mkdir $HOME/bin
cp ncs $HOME/bin
sudo mkdir /var/lib/ncs
sudo chown $USER:$USER /var/lib/ncs
cp -r html_templates /var/lib/ncs/
```


### MONGODB
edit config/ncssetup.js and run :
mongo nodecast config/ncssetup.js


### Launch NCS

```bash
ncs --ncs-base-directory=/var/lib/ncs --mongodb-ip=127.0.0.1 --mongodb-base=nodecast_prod --domain-name=localhost --xmpp-client-port=6222 --xmpp-server-port=6269 --smtp-hostname="your.server.mail" --smtp-username="your-user-account" --smtp-password="your-password" --smtp-sender="your-email-sender" --smtp-recipient="your-email-recipient"
```

to connect to a mongodb replica set, use :

```bash
 --mongodb-ip="yourreplicasetname/ip1,ip2,ip3"
 ```

### WEB ADMIN

go with your browser to http://localhost:2501/

First, create an admin user. Then you can create users, nodes and workflows, or use the HTTP API.


### API USE

1. create a user

```bash
curl -H "X-user-token: admin-token" -X POST -d '{ "email": "user@email.com", "password": "password", "ftp": true, "tracker": false, "xmpp": false, "api": false}' http://127.0.0.1:2502/user
return user token {"token":"8c70645c-b12b-4b5e-b998-e08158d09bdd"}%
```

2. create a node

```bash
curl -H "X-user-token: your-token" -X POST http://127.0.0.1:2502/node/nodename
return node auth : {"node_password":"e6cc13a3-1236-46cb-b40f-a66650ab5eef","node_uuid":"2d0a7780-e8fe-4e0a-89c6-a5a2737b095a"} 
```

3. create a workflow

```bash
curl -H "X-user-token: user-token" -X POST -d '{ "worker1": 1, "worker2": 2 }' http://127.0.0.1:2502/workflow/workflowname
return a workflow id : {"uuid":"0ebcdab6-0263-42d3-be7d-9602fa15f68c"}
```

4. SEND DATA

4a. push binary data

```bash
curl -H "X-node-uuid: 2d0a7780-e8fe-4e0a-89c6-a5a2737b095a" -H "X-node-password: 2d0a7780-e8fe-4e0a-89c6-a5a2737b095a" -H "X-workflow-uuid: 0ebcdab6-0263-42d3-be7d-9602fa15f68c" -H "X-payload-filename: filename" -H "X-payload-type: filetype" -X POST --data-binary @filename http://127.0.0.1:2502/payload/push
```

4b. push json data

```bash
curl -H "X-node-uuid: 2d0a7780-e8fe-4e0a-89c6-a5a2737b095a" -H "X-node-password: 2d0a7780-e8fe-4e0a-89c6-a5a2737b095a" -H "X-workflow-uuid: 0ebcdab6-0263-42d3-be7d-9602fa15f68c" -d '{ "data1": "mydata", "data2": "mydata" }' http://127.0.0.1:2502/payload/push
```

To send your data to all worker1 of your worflow, use publish instead of push : http://127.0.0.1:2502/payload/publish


return a session uuid : {"uuid":"7b32b9d0-3ea9-4663-9577-4b34192055bf"}


contact : fredix at nodecast dot net
