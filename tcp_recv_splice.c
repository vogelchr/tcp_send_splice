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
	fprintf(stderr, "Usage: %s [options] port|service\n", argv0);
	fprintf(stderr, "Options:\n");
	fprintf(stderr,"\t-h              this help\n");
	exit(1);
}

static int
tcp_listen_accept(char *service)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int lfd=-1, cfd=-1, i;

	memset(&hints, '\0', sizeof(hints));
	hints.ai_family = AF_UNSPEC; /* allow IP4 and IP6 */
	hints.ai_socktype = SOCK_STREAM; /* tcp */
	hints.ai_flags = AI_PASSIVE; /* for listening */

	i = getaddrinfo(NULL, service, &hints, &result);
	if (i != 0) {
		fprintf(stderr, "getaddrinfo(%s): %s\n",
			service, gai_strerror(i));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (lfd == -1) /* address family not supported. */
			continue;
		i = 1;
		if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i))
				== -1) {
			perror("setsockopt(SO_REUSEADDR)");
			close(lfd);
			lfd=-1;
			continue;
		}
		if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break; /* success */
		close(lfd);
		lfd = -1;
	}

	if (!rp) {  /* no accesses succeeded */
		fprintf(stderr, "Error: could not create a listening socket"
		 	" for service \"%s\".\n", service);
		goto err_out;
	}

	if (listen(lfd, 1) == -1) {
		perror("listen()");
		goto err_out;
	}

	cfd = accept(lfd, NULL, NULL);
	if (cfd == -1) {
		perror("accept()");
		goto err_out;
	}

	close(lfd);
	lfd = -1;

	if (-1 == shutdown(cfd, SHUT_WR)) {
		perror("shutdown(SHUT_WR)");
		goto err_out;
	}

	goto out;
err_out:
	if (cfd != -1)
		close(cfd);
	if (lfd != -1)
		close(lfd);
out:
	freeaddrinfo(result);
	return cfd;
}

int
main(int argc, char **argv)
{
	int i, netfd;
	char *service;
	ssize_t ret;

	while ((i = getopt(argc, argv, "hb:")) != -1) {
		switch (i) {
		case 'h':
			usage_exit(argv[0], NULL);
			break;
		}
	}

	if (optind != argc - 1)
		usage_exit(argv[0], "Please specify service and port.");

	service = argv[optind];

	netfd = tcp_listen_accept(service);
	if (netfd == -1)
		exit(1);

	ret = -1;
	while (1) {
		/* copy max. 1M data from the network to stdout */
		ret = splice(netfd, NULL, 1, NULL, 1U<<20, SPLICE_F_MORE);
		if (ret == -1) {
			perror("splice()");
			exit(1);
		}

		if (ret == 0) /* eof */
			break;
	}

	if (-1 == shutdown(netfd, SHUT_RD)) {
		/* to be expected when connection is torn down */
		if (ret != 0) {
			perror("shutdown(SHUT_RD)");
			exit(1);
		}
	}

	close(netfd);
	exit(0);

}
