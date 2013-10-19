import socket
import sys

server_address = sys.argv[1]
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
try:
    sock.connect(server_address)
    data = sys.stdin.read()
    size = '0x%06x' % (len(data))
    sock.sendall(size + data)
    resp = sock.makefile()
    data = resp.read(8)
    size = int(data, 0)
    data = resp.read(size)
    print data
except IOError, e:
    print str(e)
finally:
    sock.close()
