import json
import pymongo
import datetime

db_url = "mongodb://localhost:27017"
db_name = "db"

with pymongo.MongoClient(db_url) as client:

    db = client[db_name]

    def pipeline(granularity, format, output):
        return [
            {
                "$project": {
                    "aid": 1,
                    "date": { "$dateToString": { "format": format, "date": { "$toDate": { "$toLong": "$timestamp" }}}},
                    "readOrNot": { "$toInt": "$readOrNot" }
                }
            },
            {
                "$group": {
                    "_id": { "date": "$date", "aid": "$aid" },
                    "count": { "$sum": "$readOrNot" }
                }
            },
            { "$sort": { "_id.date": 1, "count": -1, "_id.aid": 1 }},
            {
                "$group": {
                    "_id": "$_id.date",
                    "top": { "$push": { "aid": "$_id.aid", "count" : "$count" }}
                }
            },
            { "$project": { "articleAidList": { "$slice": [ "$top", 5 ] }}},
            { "$addFields": { "temporalGranularity": granularity }},
            { "$out": output }
        ]

    print("Generating 'popular-rank-daily' collection...")
    db["read"].aggregate(pipeline("daily", "%Y-%m-%d (day)", "popular-rank-daily"), allowDiskUse=True)
    print("Inserted %d entries into '%s'" % (db['popular-rank-daily'].count(), db['popular-rank-daily'].full_name))

    print("Generating 'popular-rank-weekly' collection...")
    db["read"].aggregate(pipeline("weekly" , "%Y-%V (week)", "popular-rank-weekly"), allowDiskUse=True)
    print("Inserted %d entries into '%s'" % (db['popular-rank-weekly'].count(), db['popular-rank-weekly'].full_name))

    print("Generating 'popular-rank-monthly' collection...")
    db["read"].aggregate(pipeline("monthly", "%Y-%m (month)", "popular-rank-monthly"), allowDiskUse=True)
    print("Inserted %d entries into '%s'" % (db['popular-rank-monthly'].count(), db['popular-rank-monthly'].full_name))

    print("Generating 'popular-rank' collection...")

    for d in db["popular-rank-daily"].find():
        db["popular-rank"].insert(d)
    db["popular-rank-daily"].drop()

    for d in db["popular-rank-weekly"].find():
        db["popular-rank"].insert(d)
    db["popular-rank-weekly"].drop()

    for d in db["popular-rank-monthly"].find():
        db["popular-rank"].insert(d)
    db["popular-rank-monthly"].drop()

    print("Inserted %d entries into '%s'" % (db['popular-rank'].count(), db['popular-rank'].full_name))