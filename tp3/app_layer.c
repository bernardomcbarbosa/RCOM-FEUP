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

int send_file(char* filename){
  int fi,i,st_packet_length;
  struct stat st;
  unsigned char fileSize[4];

  fi = open(filename,O_RDONLY);
  if(fi < 0){
    printf("Can't open %s\n",filename);
    return -1;
  }

  fstat(fi, &st);
  off_t size = st.st_size;

  //START PACKET
  st_packet_length = 7 + (1+1+strlen(filename));
  unsigned char* start_packet = (unsigned char *)malloc(st_packet_length);
  start_packet[0] = START_C2;
  start_packet[1] = FILE_SIZE;
  start_packet[2] = 0x04;
  for (i = 0; i < 4; i++) {
        fileSize[i] = (unsigned char) (size>>(8*i) % 256);
  }
  strcat((char *) start_packet+3, (char *) fileSize);
  start_packet[7]= FILE_NAME;
  start_packet[8] = (unsigned char) strlen(filename);
  strcat((char *) start_packet+9, (char *) filename);

  start_packet[11] = FLAG;

  llwrite(serial.fileDescriptor, start_packet, st_packet_length);

  //DATA


  //END PACKET


  llclose(serial.fileDescriptor);
  return 0;
}

int receive_file(){
  unsigned char buffer[256];
  int buffer_len;
  llread(serial.fileDescriptor,buffer,&buffer_len);

  return 0;
}
