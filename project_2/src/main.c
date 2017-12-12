#include <stdio.h>
#include <stdlib.h>

#include "URL.h"
#include "ftp.h"
#include "main.h"

int main(int argc, char** argv){
  if(argc != 2){
    fprintf(stderr, "WARNING: Wrong number of arguments.\n");
    printUsage(argv[0]);
    return 1;
  }

  struct URL url;
  initURL(&url);

  if(parseURL(&url, argv[1]) != 0){
    fprintf(stderr, "Invalid URL\n");
    exit(1);
  }

  if (getIp(&url) != 0){
    fprintf(stderr, "ERROR: Cannot find ip to hostname %s\n", url.host);
    exit(1);
  }

  struct FTP connection;

  if((connection.control_socket_fd = connect_to(url.ip, url.port)) < 0){
    fprintf(stderr, "ERROR: Cannot connect socket.\n");
    exit(1);
  }


  return 0;
}

void printUsage(char* argv0){
  printf("\n\n");
  printf("Usage:\n");
  printf("Normal - %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv0);
  printf("Anonymous - %s ftp://<host>/<url-path>\n", argv0);
}
