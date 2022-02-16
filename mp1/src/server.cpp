/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <bits/stdc++.h> 
#include <vector>
using namespace std;

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXDATASIZE 1024 // max number of bytes we can get at once 

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//GET /test.txt HTTP/1.1
string parse_request(char* buf) {
	string tmp = buf;
	string request;
	if (tmp.find("GET") == 0) {
		size_t minus_get_i = tmp.find(" ");
		string minus_get = tmp.substr(minus_get_i+1);

		if (minus_get.find(" ") == string::npos ) {
			minus_get[minus_get.length() - 2] = '\0';
			return "."+minus_get;
		}

		size_t request_i = minus_get.find(" ");
		request = minus_get.substr(0, request_i);
		// request[request.length() - 2] = 0;
	} else {
		return "";
	}
	return "."+request;
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);
		
		char buffer[MAXDATASIZE];
		const char* request;
		string req;
		// ssize_t len;
		// while (1) {
		// 	ssize_t len = recv(new_fd, buffer, MAXDATASIZE, 0);
		// 	if (len <= 0) break;
		// 	r += string(buffer);
		// }
		ssize_t len = recv(new_fd, buffer, MAXDATASIZE, 0);
		
		// req = parse_request(buffer);
		
		
		if (len > 0) {
			buffer[len] = 0;
			printf("read %d chars\n", (int)len);
			printf("=========\n");
			printf("%s\n", buffer);
			req = parse_request(buffer);
			// request = req.c_str();
		}

		cout<<req<<endl;
		request = req.c_str();
		printf("request: %s\n", request);
		// request[strlen(request) - 2] = 0;
		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			if (!strcmp(request, "")) { //bad request
				if(send(new_fd, "400 Bad Request\r\n\r\n", 19, 0) == -1) 
					perror("send");
			} else {
				FILE *fd = fopen(request, "rb");
				if (!fd) {
					if(send(new_fd, "HTTP/1.1 404 Not Found\r\n\r\n", 26, 0) == -1) 
						perror("send");
				} else {
					if(send(new_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, 0) == -1) 
						perror("send");

					char buf[1024];
					size_t nread;
					if (buf == NULL) {
						exit(1);
					}

					while ((nread = fread(buf, 1, 1024, fd)) > 0) 
						if(send(new_fd, buf, nread, 0) == -1) 
							perror("send");
					fclose(fd);
				}
			}
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

