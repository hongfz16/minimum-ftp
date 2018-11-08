CC = gcc
CC_FLAG = -lpthread
prog = server
deps = socket_utils.h server_handler.h
obj = server.o socket_utils.o server_handler.o

all: $(prog)
	./server -port 8000 -root /tmp -host 192.168.0.106

$(prog): $(obj)
	$(CC) -o $(prog) $(obj) $(CC_FLAG)

%.o: %.c $(deps)
	$(CC) -c $< -o $@

clean:
	rm $(obj) $(prog)
