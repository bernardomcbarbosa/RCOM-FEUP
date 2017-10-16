#include "app_layer.h"
#include <errno.h>

app serial;

int connection(char *port, int status){
  int serial_port;
  serial.status = status;

  if (strcmp(port, COM1) == 0)
    serial_port = 0;
  else if(strcmp(port, COM2) == 0)
    serial_port = 1;
  else{
    printf("Invalid Port!\n");
    return -1;
  }

  if ((serial.fileDescriptor = llopen(serial_port, serial.status)) == -1) {
    printf("Can't open %s\n",port);
    return -1;
  }

  return serial.fileDescriptor;
}

void send_file(char* filename){
  int fi,size;
  fi = open(argv[2],O_RDONLY);
  if(fi < 0){
    printf("Can't open %s\n",filename);
  }

  fseek(fi, 0, SEEK_END); // seek to end of file
	size = ftell(fi); // get current file pointer
	fseek(f, 0, SEEK_SET); // seek back to beginning of file
  fstat(fi, &st);
  size = st.st_size;

  //START PACKET
  char* start_packet = (char *)malloc()
}
