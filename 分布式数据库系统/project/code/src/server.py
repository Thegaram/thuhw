import flask
import flask_caching
import pymongo
import hdfs
import io

from datetime import datetime

hdfs_url = "http://localhost:50070"
db_url = "mongodb://localhost:27017"
db_name = "db"

hdfs_client = hdfs.InsecureClient(hdfs_url, user='root')
db_client = pymongo.MongoClient(db_url)
db = db_client[db_name]

app = flask.Flask(__name__)
cache = flask_caching.Cache(app, config={'CACHE_TYPE': 'redis'})


@cache.memoize(timeout=60)
def retrieve_user(uid):
    user = db["user"].find_one({ "uid" : str(uid) })
    if user is not None:
        del user["_id"]
    return user

@cache.memoize(timeout=60)
def retrieve_article(aid):
    article = db["article"].find_one({ "aid" : str(aid) })
    if article is not None:
        del article["_id"]
    return article

@cache.memoize(timeout=60)
def retrieve_article_meta(aid):
    meta = db["be-read"].find_one({ "aid" : str(aid) })
    if meta is not None:
        del meta["_id"]
    return meta

@cache.memoize(timeout=60)
def retrieve_articles_read(db, uid):
    pipeline = [
        { "$match"  : { "uid" : str(uid) }},
        { "$group"  : { "_id": "$uid", "aids" : { "$addToSet" : "$aid" } }}
    ]
    try:
        return db["read"].aggregate(pipeline).next()["aids"]
    except StopIteration:
        return None

@cache.memoize(timeout=60)
def retrieve_top(key):
    top = db["popular-rank"].find_one({ "_id" : key })
    if top is not None:
        top = top["articleAidList"]
    return top

@cache.memoize(timeout=60)
def retrieve_hdfs(path):
    try:
        with hdfs_client.read(path) as reader:
            content = reader.read()
            return content
    except:
        return None

def parse_date(date):
    try:
        return datetime.strptime(date, '%Y-%m-%d')
    except:
        return None

def get_format(granularity):
    if granularity == "daily"  : return "%Y-%m-%d (day)"
    if granularity == "weekly" : return "%Y-%V (week)"
    if granularity == "monthly": return "%Y-%m (month)"
    return None


@app.route('/user/<int:uid>', methods=['GET'])
def user(uid):
    user = retrieve_user(uid)
    return flask.jsonify(user) if user is not None else ("User not found", 404)

@app.route('/user/<int:uid>/articles_read', methods=['GET'])
def articles_read(uid):
    res = retrieve_articles_read(db, uid) or []
    return flask.jsonify(res)

@app.route('/article/<int:uid>', methods=['GET'])
def article(uid):
    article = retrieve_article(uid)
    return flask.jsonify(article) if article is not None else ("Article not found", 404)

@app.route('/article/<int:uid>/meta', methods=['GET'])
def article_meta(uid):
    meta = retrieve_article_meta(uid) or {}
    return flask.jsonify(meta)

@app.route('/text/<string:id>', methods=['GET'])
def text(id):
    text = retrieve_hdfs('/data/texts/%s' % (id))
    return (text, 200) if text is not None else ("Text not found", 404)

@app.route('/image/<string:id>', methods=['GET'])
def image(id):
    image = retrieve_hdfs('/data/images/%s' % (id))
    return flask.send_file(io.BytesIO(image), mimetype='image/png') if image is not None else ("Image not found", 404)

@app.route('/video/<string:id>', methods=['GET'])
def video(id):
    video = retrieve_hdfs('/data/videos/%s' % (id))
    return flask.send_file(io.BytesIO(video), mimetype='video/mp4') if video is not None else ("Vide not found", 404)

@app.route('/top/<string:date>/<string:granularity>', methods=['GET'])
def top(date, granularity):
    date = parse_date(date)
    format = get_format(granularity)
    if date is None: return "Unsupported date format, please use the format \"2017-10-11\"", 400
    if format is None: return "Granularity must be one of: daily, weekly, monthly", 400

    key = date.strftime(format)
    top = retrieve_top(key) or []
    return flask.jsonify(top)

@app.route('/')
def index():
    return "Hello world!", 200

app.run(debug=True)