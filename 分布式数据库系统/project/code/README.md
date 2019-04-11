# ddbms-hw

## Basic setup

Prerequisites: install docker and python.

Prepare:
```
$ docker pull mongo
$ mkdir db db/conf1 db/conf2 db/dbms11 db/dbms12 db/dbms21 db/dbms22
```

Start clusters:
```
$ docker-compose -f config/cluster-1.yml -f config/cluster-2.yml -f config/hdfs.yml --project-name cluster up -d
```

You can stop the containers using:
```
$ docker-compose -f config/cluster-1.yml -f config/cluster-2.yml -f config/hdfs.yml --project-name cluster down
```

Initialize replica sets:
```
$ docker run -it --net cluster_mongonet mongo mongo mongodb://172.18.0.30:27017
> rs.initiate(
  {
    _id: "config",
    configsvr: true,
    members: [
      { _id : 0, host : "172.18.0.30:27017" },
      { _id : 1, host : "172.18.0.40:27017" }
    ]
  }
)

$ docker run -it --net cluster_mongonet mongo mongo mongodb://172.18.0.31:27017
> rs.initiate()

$ docker run -it --net cluster_mongonet mongo mongo mongodb://172.18.0.41:27017
> rs.initiate()

$ docker run -it --net cluster_mongonet mongo mongo mongodb://172.18.0.32:27017
> rs.initiate(
  {
    _id: "dbms12",
    members: [
      { _id : 0, host : "172.18.0.32:27017" },
      { _id : 1, host : "172.18.0.42:27017" }
    ]
  }
)
```

Initialize sharding:
```
$ docker run -it --net cluster_mongonet mongo mongo mongodb://172.18.0.33:27017
> sh.addShard("dbms1-only/172.18.0.31:27017")
> sh.addShard("dbms2-only/172.18.0.41:27017")
> sh.addShard("dbms12/172.18.0.32:27017")
> sh.status()
  ...
  shards:
        {  "_id" : "dbms1-only",  "host" : "dbms1-only/172.18.0.31:27017",  "state" : 1 }
        {  "_id" : "dbms12",  "host" : "dbms12/172.18.0.32:27017,172.18.0.42:27017",  "state" : 1 }
        {  "_id" : "dbms2-only",  "host" : "dbms2-only/172.18.0.41:27017",  "state" : 1 }
  ...

> sh.enableSharding("db")
```

Shard collection `user`:
```
> sh.shardCollection("db.user", { region : 1 })
> sh.disableBalancing("db.user")

> sh.splitAt("db.user", { region: "Beijing" })
> sh.splitAt("db.user", { region: "Hong Kong" })
> sh.moveChunk("db.user", { region: "Beijing" }, "dbms1-only")
> sh.moveChunk("db.user", { region: "Hong Kong" }, "dbms2-only")
```

Shard collection `article`:
```
> sh.shardCollection("db.article", { category : 1 })
> sh.disableBalancing("db.article")

> sh.splitAt("db.article", { category: "science" })
> sh.splitAt("db.article", { category: "technology" })
> sh.moveChunk("db.article", { category: "science" }, "dbms12")
> sh.moveChunk("db.article", { category: "technology" }, "dbms2-only")
```

Shard collection `read`:
```
> sh.shardCollection("db.read", { region : 1 })
> sh.disableBalancing("db.read")

> sh.splitAt("db.read", { region: "Beijing" })
> sh.splitAt("db.read", { region: "Hong Kong" })
> sh.moveChunk("db.read", { region: "Beijing" }, "dbms1-only")
> sh.moveChunk("db.read", { region: "Hong Kong" }, "dbms2-only")
```

Shard collection `be-read`:
```
> sh.shardCollection("db.be-read", { "category" : 1 })
> sh.disableBalancing("db.be-read")

> sh.splitAt("db.be-read", { "category": "science" })
> sh.splitAt("db.be-read", { "category": "technology" })
> sh.moveChunk("db.be-read", { "category": "science" }, "dbms12")
> sh.moveChunk("db.be-read", { "category": "technology" }, "dbms2-only")
```

Generate data:
```
$ mkdir data
$ cd data
data $ python ../scripts/genTable_433507293.py
```

Import data:
```
$ pip install pymongo hdfs
$ python src/import_collections.py
$ python src/generate_be_read.py
$ python src/generate_popular_rank.py
$ python src/import_assets.py
```

Check and query:
```
$ docker run -it --net cluster_mongonet mongo mongo mongodb://172.18.0.33:27017
> use db
> show collections
> db['be-read'].find()
...
```

Start server:
```
$ docker pull redis
$ docker run --name redis -d -p 6379:6379 redis
$ pip install flask flask_caching redis
$ python src/server.py
```