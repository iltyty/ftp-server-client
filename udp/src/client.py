import socket

size = 8192

msgs_num = 51


def gen_msgs(num):
    msgs =[]
    for i in range(num):
        msgs.append('this is message number {:}'.format(i))
    return msgs


try:
    msgs = gen_msgs(msgs_num)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    for msg in msgs:
        sock.sendto(msg.encode(), ('localhost', 9876))
        resp = sock.recvfrom(size)
    sock.close()
except:
    print("cannot reach the server")
