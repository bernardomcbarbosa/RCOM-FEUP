#include <stdio.h>
#include <stdlib.h>

#include "URL.h"
#include "ftp.h"
#include "main.h"

int main(int argc, char** argv){
  char code[CODE_LENGTH];
  char frame[FRAME_SIZE];

  if(argc != 2){
    fprintf(stderr, "WARNING: Wrong number of arguments.\n");
    printUsage(argv[0]);
    return 1;
  }

  struct URL url;
  initURL(&url);

  if(parseURL(&url, argv[1]) != 0){
    fprintf(stderr, "Invalid URL\n");
    return -1;
  }

  if (getIp(&url) != 0){
    fprintf(stderr, "ERROR: Cannot find ip to hostname %s\n", url.host);
    return -1;
  }

  struct FTP connection;

  if((connection.control_socket_fd = connect_to(url.ip, url.port)) < 0){
    fprintf(stderr, "ERROR: Cannot connect socket.\n");
    return -1;
  }

  if(ftpRead(&connection, frame, FRAME_SIZE) != 0){
    fprintf(stderr, "Error: ftpReadCode()");
    return -1;
  }

  /*
  if (ftpReadCode(&connection, code, CODE_READY_NEW_USER)){
    fprintf(stderr, "ERROR: ftpRead().\n");
    return -1;
  }*/

  if (ftpLogin(&connection, &url) != 0){
    fprintf(stderr, "Error: Couldn't Login.\n");
    return -1;
  }

  if (ftpPasv(&connection) != 0){
    fprintf(stderr, "Error: Couldn't enter Passive Mode.\n");
    return -1;
  }

  //Maybe connect here to host

  if (ftpRetr(&connection, &url) != 0){
    fprintf(stderr, "Error: .\n");
    return -1;
  }

  if (ftpDownload(&connection, &url) != 0){
    fprintf(stderr, "Error: Cannot download file from host.\n");
    return -1;
  }

  if (disconnect_from(&connection, &url) != 0){
    fprintf(stderr, "Error: Couldn't disconnect from host.\n");
    return -1;
  }

  return 0;
}

void printUsage(char* argv0){
  printf("\n\n");
  printf("Usage:\n");
  printf("Normal - %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv0);
  printf("Anonymous - %s ftp://<host>/<url-path>\n", argv0);
}
