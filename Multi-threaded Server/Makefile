all: tcpclient multi-tcpserver

tcpclient: tcpclient.c
	gcc -o tcpclient tcpclient.c 

multi-tcpserver: multi-tcpserver.c	
	gcc -pthread -o multi-tcpserver multi-tcpserver.c

clean:
	rm -f *.o tcpclient multi-tcpserver