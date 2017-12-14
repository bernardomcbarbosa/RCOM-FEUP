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
		return -1;
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

int disconnect_from (const struct FTP *connection, const struct URL *url){
  char *quitMsg = malloc(6 * sizeof(char));
  sprintf(quitMsg, "quit\r\n");

  if (ftpWrite(connection, quitMsg) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    return -1;
  }

  close(connection->control_socket_fd);

  return 0;
}

int ftpLogin(const struct FTP *connection, const struct URL *url){
  char frame[FRAME_SIZE];

  char *username = malloc(sizeof(url->user) + 5 * sizeof(char));
  sprintf(username, "USER %s\r\n", url->user);

  if (ftpWrite(connection, username) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    free(username);
    return -1;
  }

  if(ftpRead(connection, frame, FRAME_SIZE, CODE_READY_FOR_PW) != 0){
    fprintf(stderr, "Error: Couldn't receive message from host.\n");
    free(username);
    return -1;
  }

  free(username);

  char *password = malloc(sizeof(url->password) + 5 * sizeof(char));
	sprintf(password, "PASS %s\r\n", url->password);

  if (ftpWrite(connection, password) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    free(password);
    return -1;
  }

  if(ftpRead(connection, frame, FRAME_SIZE, CODE_LOGGED_IN) != 0){
    fprintf(stderr, "Error: Couldn't receive message from host.\n");
    free(password);
    return -1;
  }

  free(password);

  return 0;
}

int ftpPasv (struct FTP *connection, char *pasvIP, int *pasvPort){
  char frame[FRAME_SIZE];

  char * pasv = malloc(7 * sizeof(char));
	sprintf(pasv, "PASV \r\n");

  if (ftpWrite(connection, pasv) != 0){
    fprintf(stderr, "Error: Couldn't send message to host.\n");
    free(pasv);
    return -1;
  }

  if(ftpRead(connection, frame, FRAME_SIZE, CODE_PASSIVE_MODE) != 0){
    fprintf(stderr, "Error: Didn't receive passive mode information.\n");
    free(pasv);
    return -1;
  }

  free(pasv);

  // starting process information
  int ip[4];
  int port[2];

	if ((sscanf(frame, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &ip[0],&ip[1], &ip[2], &ip[3], &port[0], &port[1])) < 0){
		fprintf(stderr, "Error: Cannot process passive mode information.\n");
		return -1;
	}

  if ((sprintf(pasvIP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3])) < 0){
		fprintf(stderr, "Error: Cannot form ip address.\n");
		return -1;
	}

  *pasvPort = port[0]*256 + port[1];

  return 0;
}

int ftpRetr (const struct FTP *connection, const struct URL *url){
  char frame[FRAME_SIZE];

  sprintf(frame, "RETR %s/%s\r\n", url->path, url->filename);
  printf("%s\n", frame);

  if (ftpWrite(connection, frame) !=0){
    fprintf(stderr, "Error: Couldn't send path to host.\n");
    return -1;
  }

  if (ftpRead(connection, frame, FRAME_SIZE, CODE_FILE_OKAY) != 0){
    fprintf(stderr, "Error: Couldn't receive message from host.\n");
    return -1;
  }

  return 0;
}

int ftpDownload (const struct FTP *connection, const struct URL *url){
  FILE *f;
  char frame[FRAME_SIZE];
  int read_bytes;

  if ((f = fopen(url->filename, "w")) == NULL){
    fprintf(stderr, "Error: Couldn't create/open %s file\n", url->filename);
    return -1;
  }

  while((read_bytes = read(connection->data_socket_fd, frame, FRAME_SIZE))){
    if (read_bytes == -1){
      fprintf(stderr, "Error: Nothing was received through the data socket.\n");
      return -1;
    }
    if(fwrite(frame, read_bytes, 1, f) < 0){
      fprintf(stderr, "Error: Cannot write data to file %s.\n", url->filename);
      return -1;
    }
  }

  close(connection->data_socket_fd);
  fclose(f);

  return 0;
}

int ftpWrite(const struct FTP *connection, const char *frame){

    if (write(connection->control_socket_fd, frame, strlen(frame)) != strlen(frame)){
      return -1;
    }
    return 0;
}


int ftpRead(const struct FTP *connection, char *frame, size_t frame_length, char *exp_code){
  FILE* fp = fdopen(connection->control_socket_fd, "r");

  do {
    memset(frame, 0, frame_length);
    frame = fgets(frame, frame_length, fp);
    printf("%s", frame);
  } while(!('1' <= frame[0] && frame[0] <= '5')|| frame[3] != ' ');

  if(strncmp(frame, exp_code, CODE_LENGTH) != 0){
    fprintf (stderr, "Error: Wrong code received.\n");
    return -1;
  }

  return 0;
}
