#include "recobop.h"

#include "reconos_thread.h"
#include "reconos_calls.h"
#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <errno.h>

#define HEADER_COUNT_MAX 2048
#define INDEX_COUNT_MAX  20480
#define BUF_COUNT_MAX    2048


THREAD_ENTRY() {
	int server_sock, client_sock;
	struct sockaddr_in server_addr;

	char header[HEADER_COUNT_MAX];
	ssize_t header_count;

	char buf[BUF_COUNT_MAX];
	ssize_t buf_count;

	char index[INDEX_COUNT_MAX];
	ssize_t index_count;

	struct recobop_info *rb_info;

	THREAD_INIT();
	rb_info = (struct recobop_info *)GET_INIT_DATA();

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(12345);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock < 0) {
		panic("Failed to open socket");
	}

	if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in))) {
		panic("Failed to bind socket %d\n", errno);

		close(server_sock);
	}

	listen(server_sock, 1);


	while (1) {
		FILE *fp = fopen("d3.html", "r");
		if (!fp) {
			panic("Failed to open source");
		}
		index_count = fread(index, sizeof(char), INDEX_COUNT_MAX, fp);
		index[index_count] = '\0';
		fclose(fp);

		client_sock = accept(server_sock, 0, 0);
		if (client_sock < 0) {
			panic("Failed to accept socket");

			close(server_sock);
		}

		header_count = recv(client_sock, header, HEADER_COUNT_MAX, 0);
		header[header_count] = '\0';

		send(client_sock, "HTTP/1.1 200 OK\n\n", 17, 0);
		if (!strncmp(header, "GET / ", 6)) {
			send(client_sock, index, index_count, 0);
		} else if (!strncmp(header, "GET /saw/pos/x ", 15)) {
			buf_count = snprintf(buf, 1024, "%d", rbi_saw_pos_x(rb_info));
			send(client_sock, buf, buf_count, 0);
		} else if (!strncmp(header, "GET /saw/pos/y ", 15)) {
			buf_count = snprintf(buf, 1024, "%d", rbi_saw_pos_y(rb_info));
			send(client_sock, buf, buf_count, 0);
		} else if (!strncmp(header, "GET /saw/vsense ", 16)) {
			buf_count = snprintf(buf, 1024, "%f", rbi_saw_power(rb_info));
			send(client_sock, buf, buf_count, 0);
		} else if (!strncmp(header, "GET /perf/touch ", 16)) {
			buf_count = snprintf(buf, 1024, "%f", rbi_perf_touch(rb_info));
			send(client_sock, buf, buf_count, 0);
		} else if (!strncmp(header, "GET /perf/control ", 18)) {
			buf_count = snprintf(buf, 1024, "%f", rbi_perf_control(rb_info));
			send(client_sock, buf, buf_count, 0);
		} else if (!strncmp(header, "GET /perf/inverse ", 18)) {
			buf_count = snprintf(buf, 1024, "%f", rbi_perf_inverse(rb_info));
			send(client_sock, buf, buf_count, 0);
		} else if (!strncmp(header, "GET /perf/overhead ", 19)) {
			buf_count = snprintf(buf, 1024, "%f", rbi_perf_overhead(rb_info));
			send(client_sock, buf, buf_count, 0);
		}

		close(client_sock);
	}

	close(server_sock);

	THREAD_EXIT();
}