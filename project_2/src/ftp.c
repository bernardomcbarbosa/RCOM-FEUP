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
#include "main.h"

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
    fprintf(stderr, "socket(): %s.\n", strerror(errno));
    return -1;
  }

  // connect to the server
  if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "connect(): %s.\n", strerror(errno));
    return -1;
  }

  return sockfd;
}

int ftpLogin(const struct FTP *connection, const struct URL *url){
  char code[CODE_LENGTH];
  char frame[FRAME_SIZE];

  char *username = malloc(sizeof(url->user) + 5 * sizeof(char));
  sprintf(username, "USER %s\r\n", url->user);
  printf("%s\n", username);

  if (ftpWrite(connection, username) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    free(username);
    return -1;
  }
  if(ftpRead(connection, frame, FRAME_SIZE) != 0){
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
	sprintf(password, "PASS %s\r\n", url->password);
  printf("%s\n", password);

  if (ftpWrite(connection, password) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    free(password);
    return -1;
  }

  if(ftpRead(connection, frame, FRAME_SIZE) != 0){
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

int ftpPasv (struct FTP *connection){
  char frame[FRAME_SIZE];
  char pasvIP[16];
  int pasvPort;

  char * pasv = malloc(7 * sizeof(char));
	sprintf(pasv, "PASV \r\n");

  if (ftpWrite(connection, pasv) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    free(pasv);
    return -1;
  }

  if(ftpRead(connection, frame, FRAME_SIZE) != 0){
    fprintf(stderr, "Error: Didn't receive passive mode information.\n");
    free(pasv);
    return -1;
  }

  free(pasv);

  // starting process information
  int ip[4];
  int port[2];

	if ((sscanf(frame, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip[0],&ip[1], &ip[2], &ip[3], &port[0], &port[1])) < 0){
		fprintf(stderr, "ERROR: Cannot process information to calculating port.\n");
		return -1;
	}

  if ((sprintf(pasvIP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3])) < 0){
		fprintf(stderr, "ERROR: Cannot form ip address.\n");
		return 1;
	}

  pasvPort = port[0]*256 + port[1];

  printf("IP: %s\n", pasvIP);
	printf("PORT: %d\n", pasvPort);

  if ((connection->data_socket_fd = connect_to(pasvIP, pasvPort))<0) {
		printf("ERROR: Incorrect file descriptor associated to ftp data socket fd.\n");
		return 1;
	}

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
