#include "app_layer.h"
#include <errno.h>

app serial;

int connection(char *port, int status){

  serial.status = status;

  if (strcmp(port, COM1) == 0)
    port = 0;
  else if(strcmp(port, COM2) == 0)
    port = 1;
  else{
    printf("Invalid Port!\n");
    return -1;
  }

  if ((serial.fileDescriptor = llopen(port, serial.status)) == -1) {
    printf("Can't open %s\n",port);
    return -1;
  }

  return serial.fileDescriptor;
}
