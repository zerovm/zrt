import socket
import sys
import struct
import ctypes

FILTER=''.join([(len(repr(chr(x)))==3) and chr(x) or '.' for x in range(256)])

def dump(src, length=8):
    N=0; result=''
    while src:
       s,src = src[:length],src[length:]
       hexa = ' '.join(["%02X"%ord(x) for x in s])
       s = s.translate(FILTER)
       result += "%04X   %-*s   %s\n" % (N, length*3, hexa, s)
       N+=length
    return result

peers = int(sys.argv[1])
port = 0
if len(sys.argv) > 2 and sys.argv[2]:
    port = int(sys.argv[2])
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(('', port))
print s.getsockname()[1]
bind_map = {}
conn_map = {}
peer_map = {}
while 1:
    try:
        message, address = s.recvfrom(65535)
        print dump(message)
        offset = 0
        alias = struct.unpack_from('!I', message, offset)[0]
        print '%08x = %s:%d' % (alias, address[0], address[1])
        offset += 4
        count = struct.unpack_from('!I', message, offset)[0]
        offset += 4
        for i in range(count):
            h, _junk, port = struct.unpack_from('!IIH', message, offset)[0:3]
            bind_map.setdefault(alias, {})[h] = port
            print '%08x:%d <- %08x' % (alias, port, h)
            offset += 10
        conn_map[alias] = ctypes.create_string_buffer(message[offset:])
        peer_map.setdefault(alias, {})[0] = address[0]
        peer_map.setdefault(alias, {})[1] = address[1]

        if len(peer_map) == peers:
            for src in peer_map.iterkeys():
                reply = conn_map[src]
                offset = 0
                count = struct.unpack_from('!I', reply, offset)[0]
                offset += 4
                log = ''
                for i in range(count):
                    h = struct.unpack_from('!I', reply, offset)[0]
                    port = bind_map[h][src]
                    struct.pack_into('!4sH', reply, offset + 4, socket.inet_pton(socket.AF_INET, peer_map[h][0]), port)
                    log += str([peer_map[h][0], port])
                    offset += 10
                s.sendto(reply, (peer_map[src][0], peer_map[src][1]))
                print ['sending to: ', peer_map[src][0], peer_map[src][1], log]
    except (KeyboardInterrupt, SystemExit):
        exit(1)

