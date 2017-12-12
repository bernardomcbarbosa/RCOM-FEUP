#include <stdio.h>
#include <string.h>
#include <netdb.h>  //gethostbyname
#include <sys/socket.h> //inet_ntoa
#include <netinet/in.h> //inet_ntoa
#include <arpa/inet.h> //inet_ntoa
#include <errno.h>

#include "URL.h"
#include "ftp.h"

int getIp(struct URL *url){
  struct hostent* h;

  if ((h = gethostbyname(url->host)) == NULL) {
		herror("Error, could not retrieve host information.\n");
		return 1;
	}

  printf("Host name  : %s\n", h->h_name);

	char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
	strcpy(url->ip, ip);

  printf("IP Address : %s\n", url->ip);

  url->port = FTP_PORT;

  return 0;
}

int connect_to (const char *adress, const int port){
  int sockfd;
  struct sockaddr_in server_addr;

  // server address handling
  bzero((char*) &server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(adress); /*32 bit Internet address network byte ordered*/
  server_addr.sin_port = htons(port); /*server TCP port must be network byte ordered */

  // open an TCP socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "socket(): %s", strerror(errno));
    return -1;
  }

  // connect to the server
  if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "connect(): %s", strerror(errno));
    return -1;
  }

  return sockfd;
}

int ftpRead (struct FTP *connection){
  FILE* fp = fdopen(connection->control_socket_fd, "r");
}
