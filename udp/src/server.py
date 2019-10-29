import socket

size = 8192
seq_id = -1

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', 9876))

def add_seq(data):
    global seq_id
    seq_id += 1
    return '{:} {:}'.format(seq_id, data)

try:
  while True:
    data, address = sock.recvfrom(size)
    data = add_seq(data.decode())
    sock.sendto(data.encode(), address)
finally:
  sock.close()
