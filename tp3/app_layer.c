#include "app_layer.h"
#include <errno.h>

app serial;

int connection( const char *port, int status){
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

void send_file(const char *port,char* filename){
  int fi,size,f;
  struct stat st;
  char *fileSize;
  int resto = 256;

  fi = open(port,O_RDONLY);
  if(fi < 0){
    printf("Can't open %s\n",filename);
  }

  fseek(fi, 0, SEEK_END); // seek to end of file
	size = ftell(fi); // get current file pointer
	fseek(f, 0, SEEK_SET); // seek back to beginning of file
  fstat(fi, &st);
  size = st.st_size;

  //START PACKET
  char* start_packet = (char *)malloc(3;
  start_packet[0] = START_C2;
  start_packet[1] = FILE_SIZE;
  start_packet[2] = 0x04;

  fileSize =  (char *)malloc(size);

for (size_t i = 0; i < 4; i++) {
    fileSize[i] = size % resto;
    resto = resto % 256;
}
  printf("%d\n",size);

start_packet = (char *) realloc(start_packet, sizeof(start_packet) + sizeof(fileSize)+ );

strcat(start_packet, fileSize);

start_packet[7]= FILE_NAME;

start_packet[8] = strlen(filename);




}
