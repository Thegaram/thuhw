import json
import pymongo
import datetime

db_url = "mongodb://localhost:27017"
db_name = "db"
batch_size = 100000

def insert_batch(collection, batch):
    if (len(batch) > 0):
        x = collection.insert_many(batch)
        print("Inserted %d entries into '%s'" % (len(x.inserted_ids), collection.full_name))

def import_file(collection, path):
    print("Importing '%s' from {%s}..." % (collection.full_name, path))
    batch = []
    with open(path) as f:
        for line in f:
            batch.append(json.loads(line))
            if len(batch) == batch_size:
                insert_batch(collection, batch)
                batch = []
    insert_batch(collection, batch)

with pymongo.MongoClient(db_url) as client:

    db = client[db_name]

    import_file(db["user"]   , 'data/user.dat')
    import_file(db["article"], 'data/article.dat')
    import_file(db["read"]   , 'data/read.dat')
