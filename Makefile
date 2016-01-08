CFLAGS=-Os -march=native -ggdb -Wall -Wextra

all : tcp_send_splice tcp_recv_splice

tcp_send_splice : tcp_send_splice.o
	$(CC) -o $@ $^

tcp_recv_splice : tcp_recv_splice.o
	$(CC) -o $@ $^

.PHONY : clean install
clean :
	rm -f *.o *.d *~ *.bak tcp_send_splice tcp_recv_splice

install : tcp_send_splice
	install -m755 -o0 -g0 tcp_send_splice /usr/local/bin
	install -m755 -o0 -g0 tcp_recv_splice /usr/local/bin
