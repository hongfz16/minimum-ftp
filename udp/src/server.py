import socket

size = 8192

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', 9876))

counter = 1

try:
  while True:
    data, address = sock.recvfrom(size)
    sock.sendto(str(counter) + ' ' + data.upper(), address)
    counter += 1
finally:
  sock.close()