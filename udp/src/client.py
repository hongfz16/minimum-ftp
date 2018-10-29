import socket
 
size = 8192

counter = 0

try:
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  for i in range(51):
    msg = 'Message No.' + str(counter)
    counter += 1
    sock.sendto(msg, ('localhost', 9876))
    print sock.recv(size)
  sock.close()
 
except:
  print "cannot reach the server"