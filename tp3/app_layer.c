#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>

#include "data_layer.h"
#include "app_layer.h"

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
  int fi,i,st_packet_length,read_bytes=0,send_bytes=0,sequenceN=0,send_buff_len,filesize;
  struct stat st;
  unsigned char fileSize[4];
  unsigned char file[252];
  unsigned char* send_buff;

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

  // start_packet[11] = FLAG;

  llwrite(serial.fileDescriptor, start_packet, st_packet_length);

  //DATA
  filesize = (int) size;
  send_buff = (unsigned char *)malloc(256);
  while(send_bytes < filesize){
    read_bytes = read(fi, file, 252); // 256 - (C2+N+L1+L2)
    if(read_bytes < 0)
      printf("Error read %s\n",filename);

    send_buff[0] = DATA_C2;
    send_buff[1] = (unsigned char) n(sequenceN);
    //K = 256*L1 + L2
    send_buff[2] = read_bytes / 256; //L1
    send_buff[3] = read_bytes % 256; //L2
    memcpy(send_buff+4, file, read_bytes); //data packet

    send_buff_len = read_bytes + 4;
    llwrite(serial.fileDescriptor, send_buff, send_buff_len);
    //free(send_buff);
    send_bytes += read_bytes;
    sequenceN++;
    printf("%d / %d\n",send_bytes,filesize);
  }
  //END PACKET
  start_packet[0] = END_C2;
  llwrite(serial.fileDescriptor, start_packet, st_packet_length);

  llclose(serial.fileDescriptor);
  return 0;
}

int receive_file(){
  unsigned char buffer[256],filename[50],read_bytes=0,sequenceN=0,read_total;
  int buffer_len,res,filesize=0,i,fi;

  //START PACKET
  res = llread(serial.fileDescriptor,buffer,&buffer_len);
  while (res != 0 || buffer[0] != START_C2){
    res = llread(serial.fileDescriptor,buffer,&buffer_len);
  }
  //filesize
  for (i = 3; i < 7; i++) {
        filesize += (int) (buffer[i]<<(8*(i-3)));
  }
  //filename
  for (i = 9; i < buffer_len; i++) {
    filename[i-9] = buffer[i];
  }
  filename[(int) buffer[8]] = '\0';

  //DATA PACKETS
  fi = open((char *) filename,O_TRUNC | O_CREAT |  O_WRONLY);
  if(fi < 0){
    printf("Error open %s\n",filename);
    return -1;
  }

  read_total = 0;
  while (res != 0 || buffer[0] != END_C2){
    res = llread(serial.fileDescriptor,buffer,&buffer_len);
    if(res != 0)
      continue;
    if((int) buffer[1] == n(sequenceN)){
      read_bytes = buffer_len - 4;
      //if(read_bytes == (int)buffer[2]*256 + (int)buffer[3])
      write(fi,buffer+4,read_bytes);
      read_total += read_bytes;
      sequenceN++;

      printf("%d / %d\n",read_total,filesize);
    }
  }

  close(fi);
  if(read_total == filesize)
    printf("Sucess\n");
  llclose(serial.fileDescriptor);
  return 0;
}
