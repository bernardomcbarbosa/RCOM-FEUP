#include <stdio.h>

#include "URL.h"
#include "main.h"

int main(int argc, char** argv){
  if(argc != 2){
    fprintf(stderr, "WARNING: Wrong number of arguments.\n");
    printUsage(argv[0]);
    return 1;
  }

  struct URL url;
  
  initURL(&url);

  return 0;
}

void printUsage(char* argv0){
  printf("Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv0);
}
