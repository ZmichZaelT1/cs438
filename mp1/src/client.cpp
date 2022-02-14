/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <regex.h> 
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <bits/stdc++.h> 
#include <vector>
using namespace std;

// using std::string;

#define PORT "1234" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
// ./http_client http://hostname[:port]/path/to/file
vector<string> parse_arg(char* argv) {
	string tmp = argv;
	// size_t url_length = tmp.length();

	size_t minus_http_i = tmp.find("//") + 2;
	string minus_http = tmp.substr(minus_http_i);

	size_t domain_i, port_i;
	string domain, port, request;
	if (minus_http.find(":") != string::npos ) {
		domain_i = minus_http.find(":");
		domain = minus_http.substr(0,domain_i);
		port_i = minus_http.find("/");
		port = minus_http.substr(domain_i + 1, port_i - domain_i - 1);
		request = minus_http.substr(port_i);
	} else {
		domain_i = minus_http.find("/");
		domain = minus_http.substr(0,domain_i);
		port = "80";
		request = minus_http.substr(domain_i);
	}

	return {domain, port, request};
}

//	"GET /server.c HTTP/1.1\r\n\r\n";
string get_request(string request) { //request = "/server.c"
	string ret = "GET " + request + " HTTP/1.1\r\n\r\n";
	// cout << ret;
	return ret;
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	vector<string> vec = parse_arg(argv[1]);

	const char* domain_c = vec[0].c_str();
	const char* port_c = vec[1].c_str();
	string a = get_request(vec[2]);
	const char* request_c = a.c_str();

	printf("domain: %s\n", domain_c);
	printf("port: %s\n", port_c);
	printf("request_c: %s\n", request_c);

	if ((rv = getaddrinfo(domain_c, port_c, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}
//
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	write(sockfd, request_c, strlen(request_c));

	FILE *fd = fopen("output", "w+");
	while(1) {
		ssize_t bytes_recd = read(sockfd, buf, sizeof(buf));
		if (bytes_recd > 0) {
			// write(1, buf, bytes_recd);
			fprintf(fd, buf);
			memset(&buf, 0, sizeof buf);
		} else {
			break;
		}
	}

	fclose(fd);
	close(sockfd);

	return 0;
}

