#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>  //gethostbyname
#include <sys/socket.h> //inet_ntoa
#include <netinet/in.h> //inet_ntoa
#include <arpa/inet.h> //inet_ntoa
#include <unistd.h> //write
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

int connect_to(const char *adress, const int port){
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

int ftpLogin(const struct FTP *connection, const struct URL *url){
  char code[CODE_LENGTH];
  char frame[1024];

  char *username = malloc(sizeof(url->user) + 5 * sizeof(char));
  sprintf(username, "user %s\r\n", url->user);

  if (ftpWrite(connection, username) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    free(username);
    return -1;
  }
  if(ftpRead(connection, frame, strlen(frame)) != 0){
    fprintf(stderr, "Error: ftpReadCode()");
    free(username);
    return -1;
  }
/*
  if(ftpReadCode(connection, code, CODE_READY_FOR_PW) != 0){
    fprintf(stderr, "Error: ftpReadCode()");
    free(username);
    return -1;
  }
  */
  free(username);

  char *password = malloc(sizeof(url->password) + 5 * sizeof(char));
	sprintf(password, "pass %s\r\n", url->password);

  if (ftpWrite(connection, password) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    free(password);
    return -1;
  }
  if(ftpRead(connection, frame, strlen(frame)) != 0){
    fprintf(stderr, "Error: ftpReadCode()");
    free(password);
    return -1;
  }
  /*
  if(ftpRead(connection, code, CODE_LOGGED_IN) != 0){
    fprintf(stderr, "Error: ftpReadCode()");
    free(password);
    return -1;
  }
  */
  free(password);

  return 0;
}

int ftpWrite(const struct FTP *connection, const char *frame){

    if (write(connection->control_socket_fd, frame, strlen(frame)) != strlen(frame)){
      return -1;
    }
    return 0;
}

int ftpReadCode(const struct FTP *connection, char *code, char *exp_code){
  FILE* fp = fdopen(connection->control_socket_fd, "r");

  do{
    memset(code, 0, CODE_LENGTH);
    code = fgets(code, CODE_LENGTH, fp);
    printf("%s\n", code);
  } while(!('1' <= code[0] && code[0] <= '5'));

  if(strncmp(code, exp_code, CODE_LENGTH) != 0){
    fprintf(stderr, "Error: Wrong code received\n");

    return -1;
  }

  return 0;
}


int ftpRead(const struct FTP *connection, char *frame, size_t frame_length){
  FILE* fp = fdopen(connection->control_socket_fd, "r");

  do {
    memset(frame, 0, frame_length);
    frame = fgets(frame, frame_length, fp);
    printf("%s\n", frame);
  } while(!('1' <= frame[0] && frame[0] <= '5')|| frame[3] != ' ');

  return 0;
}
