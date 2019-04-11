import hdfs
import pymongo

hdfs_url = "http://localhost:50070"
db_url   = "mongodb://localhost:27017"
db_name  = "db"

with pymongo.MongoClient(db_url) as mongo_client:

    db = mongo_client[db_name]

    hdfs_client = hdfs.InsecureClient(hdfs_url, user='root')

    hdfs_client.makedirs('/data/texts')
    hdfs_client.makedirs('/data/images')
    hdfs_client.makedirs('/data/videos')

    hdfs_client.upload('/data/texts/84393add8c', './data/texts/84393add8c.txt')
    hdfs_client.upload('/data/images/77af778b51', './data/images/77af778b51.jpg')
    hdfs_client.upload('/data/videos/92a15e5a53', './data/videos/92a15e5a53.mp4')

    db.article.update_many({}, { "$set" : {
        "text" : "84393add8c",
        "image": "77af778b51",
        "video": "92a15e5a53"
    }})