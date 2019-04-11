import json
import ijson


# Discussion: My best result is a simple heuristic: we assume
# that authors with the same name and same organization are the
# same people. For authors without an organization we make an
# arbitrary choice.

# More advanced approaches would implement some kind of clus-
# tering with features such as institute name, common co-authors,
# common keywords from abstracts, etc.

# Implementation detail: To prevent loading the whole data set
# into memory, I use ijson for streaming json and only keeping
# the relevant parts.


# source: https://stackoverflow.com/a/3229493
def pretty(d, indent=0):
   for key, value in d.items():
        print('\t' * indent + str(key))
        if isinstance(value, dict):
            pretty(value, indent+1)
        else:
            print('\t' * (indent+1) + str(value))

def name_from_key(key):
    parts = key.split('_')
    parts = map(lambda p: p.capitalize() + ('.' if len(p) == 1 else ''), parts)
    return ' '.join(parts)

key = ''
name = ''

inst_to_id = {}
results = {}

last_org = ''
keep_next_org = False

# FILENAME = './pubs_train.json'
FILENAME = './pubs_validate.json'
# FILENAME = './pubs_test.json'

parser = ijson.parse(open(FILENAME))

for prefix, event, value in parser:
    # new name entry
    if (prefix, event) == ('', 'map_key'):
        key = value
        name = name_from_key(key)
        inst_to_id = {}

    # relevant author entry
    elif (prefix, event, value) == (f'{key}.item.authors.item.name', 'string', name):
        keep_next_org = True

    # relevant organization
    elif (prefix, event) == (f'{key}.item.authors.item.org', 'string') and keep_next_org:
        keep_next_org = False
        org = last_org if value == '' else value
        last_org = value or last_org

    # article id
    elif (prefix, event) == (f'{key}.item.id', 'string'):
        id = value
        if org not in inst_to_id: inst_to_id[org] = []
        inst_to_id[org].append(id)

    # end of name entry
    elif (prefix, event) == (key, 'end_array'):
        results[key] = []
        for inst, id_list in inst_to_id.items():
            results[key].append(id_list)

print(json.dumps(results, indent=4))
