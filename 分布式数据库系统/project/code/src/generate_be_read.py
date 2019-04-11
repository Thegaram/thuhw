import json
import pymongo
import datetime

db_url = "mongodb://localhost:27017"
db_name = "db"

with pymongo.MongoClient(db_url) as client:

    db = client[db_name]

    pipeline = [
        {
            "$group": {
                "_id": "$aid",
                "aid": { "$first": "$aid" },
                "category": { "$first": "$category" },
                "readNum"   : { "$sum": { "$toInt": "$readOrNot" }},
                "commentNum": { "$sum": { "$toInt": "$commentOrNot" }},
                "agreeNum"  : { "$sum": { "$toInt": "$agreeOrNot" }},
                "shareNum"  : { "$sum": { "$toInt": "$shareOrNot" }},
                "readUidList"   : { "$addToSet": { "$cond": [{ "$eq": ["$readOrNot"   , "1"] }, "$uid", "$noval"] } },
                "commentUidList": { "$addToSet": { "$cond": [{ "$eq": ["$commentOrNot", "1"] }, "$uid", "$noval"] } },
                "agreeUidList"  : { "$addToSet": { "$cond": [{ "$eq": ["$agreeOrNot"  , "1"] }, "$uid", "$noval"] } },
                "shareUidList"  : { "$addToSet": { "$cond": [{ "$eq": ["$shareOrNot"  , "1"] }, "$uid", "$noval"] } }
            }
        },
        { "$out": "be-read-temp" }
    ]

    print("Generating 'be-read-temp' collection...")
    db["read"].aggregate(pipeline, allowDiskUse=True)
    print("Inserted %d entries into '%s'" % (db['be-read-temp'].count(), db['be-read-temp'].full_name))

    print("Generating 'be-read' collection...")
    ii = 0
    count = db["be-read-temp"].count()
    for d in db["be-read-temp"].find():
        db["be-read"].insert(d)
        ii = ii + 1
        if ii % 1000 == 0:
            print("Progress: %.2f%%" % ((ii / count) * 100))
    db["be-read-temp"].drop()

    print("Inserted %d entries into '%s'" % (db['be-read'].count(), db['be-read'].full_name))