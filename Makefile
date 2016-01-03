CFLAGS=-Os -march=native -ggdb -Wall -Wextra

tcp_send_splice : tcp_send_splice.o
	$(CC) -o $@ $^

.PHONY : clean install
clean :
	rm -f *.o *.d *~ *.bak tcp_send_splice

install : tcp_send_splice
	install -m755 -o0 -g0 tcp_send_splice /usr/local/bin
