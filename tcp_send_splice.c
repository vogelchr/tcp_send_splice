#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>

static void
usage_exit(char *argv0, char *error)
{
	if (error)
		fprintf(stderr, "Error: %s\n", error);
	fprintf(stderr, "Usage: %s [options] host|addr port|service\n", argv0);
	fprintf(stderr, "Options:\n");
	fprintf(stderr,"\t-h              this help\n");
	exit(1);
}

static int
tcp_connect(char *host, char *service)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int fd=-1, s;

	memset(&hints, '\0', sizeof(hints));
	hints.ai_family = AF_UNSPEC; /* allow IP4 and IP6 */
	hints.ai_socktype = SOCK_STREAM; /* tcp */

	s = getaddrinfo(host, service, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo(%s, %s): %s\n",
			host, service, gai_strerror(s));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (fd == -1) /* address family not supported. */
			continue;
		if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
			break; /* success */
		close(fd);
		fd = -1;
	}

	if (!rp) {  /* no accesses succeeded */
		fprintf(stderr, "Error: could not connect to %s:%s.\n",
			host, service);
		goto err_out;
	}

	if (-1 == shutdown(fd, SHUT_RD)) {
		perror("shutdown(SHUT_RD)");
		goto err_out;
	}

	goto out;
err_out:
	if (fd != -1)
		close(fd);
out:
	freeaddrinfo(result);
	return fd;
}

int
main(int argc, char **argv)
{
	int i, netfd;
	char *host, *service;


	while ((i = getopt(argc, argv, "hb:")) != -1) {
		switch (i) {
		case 'h':
			usage_exit(argv[0], NULL);
			break;
		}
	}

	if (optind != argc - 2)
		usage_exit(argv[0], "Please specify service and port.");

	host = argv[optind];
	service = argv[optind+1];

	netfd = tcp_connect(host, service);
	if (netfd == -1)
		exit(1);

	while (1) {
		ssize_t ret;

		/* copy max. 1M data from stdin to the network */
		ret = splice(0, NULL, netfd, NULL, 1U<<20, SPLICE_F_MORE);
		if (ret == -1) {
			perror("splice()");
			exit(1);
		}

		if (ret == 0) /* eof */
			break;
	}

	if (-1 == shutdown(netfd, SHUT_WR)) {
		perror("shutdown(SHUT_WR)");
		exit(1);
	}

	close(netfd);
	exit(0);

}
