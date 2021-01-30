from pwn import *
import json

r = remote('127.0.0.1', 39079)

def send_packet(d):
    d = '\x01' + json.dumps(d)
    r.sendline(d)

def get_packet():
    v = r.readuntil('\n')
    print v


send_packet(dict(
    topic='user.auth',
    password='',
    name='',
    version='v0.19'))
    
get_packet()

send_packet(dict(topic='server.room.join', name="asdf"))

raw_input()
send_packet(dict(topic='server.room.join', name="asdf"))
raw_input()
send_packet(dict(topic='server.room.join', name="asdf"))
raw_input()
send_packet(dict(topic='server.room.join', name="asdf"))


while True:
    get_packet()
