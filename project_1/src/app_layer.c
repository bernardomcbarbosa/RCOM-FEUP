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

applicationLayer serial; //see header file

int connection(const char *port, int mode){
  int serial_port;
  serial.mode = mode;

  if (strcmp(port, COM1) == 0)
    serial_port = 0;
  else if(strcmp(port, COM2) == 0)
    serial_port = 1;
  else{
    fprintf(stderr, "Invalid Port!");
    exit(1);
  }

  if ((serial.fileDescriptor = llopen(serial_port, serial.mode)) == -1) {
    fprintf(stderr, "Error opening port %s\n", port);
    exit(1);
  }

  return serial.fileDescriptor;
}

int send_file(char* filename){
  int fi, read_bytes=0, send_bytes=0, sequenceN=0, send_buff_len, filesize;
  struct stat st;
  unsigned char file[252];
  unsigned char* send_buff;
  unsigned int st_packet_length;

  if ((fi = open(filename,O_RDONLY)) == -1){
    fprintf(stderr, "Can't open %s\n", filename);
    exit(1);
  }

  fstat(fi, &st);
  off_t file_size = st.st_size;
  mode_t file_mode = st.st_mode;

  //START PACKET
  st_packet_length = 7 + sizeof(mode_t) + (sizeof(st.st_size) + strlen(filename));
  unsigned char* start_packet = (unsigned char *)malloc(sizeof(char) * st_packet_length);
  start_packet[0] = START_C2;
  start_packet[1] = FILE_PERMISSIONS;
  start_packet[2] = sizeof(mode_t);
  *((mode_t *)(start_packet + 3)) = file_mode;
  start_packet[3 + sizeof(mode_t)] = FILE_SIZE;
  start_packet[4 + sizeof(mode_t)] = sizeof(st.st_size);
  *((off_t *)(start_packet + 5 + sizeof(mode_t))) = file_size;
  start_packet[5 + sizeof(mode_t) + sizeof(st.st_size)] = FILE_NAME;
  start_packet[6 + sizeof(mode_t) + sizeof(st.st_size)] = strlen(filename);
  strcat((char*) start_packet + 7 + sizeof(mode_t) + sizeof(st.st_size), filename);

  if (llwrite(serial.fileDescriptor, start_packet, st_packet_length) < 0){
    fprintf(stderr, "Error llwrite ()\n");
    exit(-1);
  }

  printf ("Sending data . . .\n");

  //DATA
  filesize = (int) file_size;
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
    if (llwrite(serial.fileDescriptor, send_buff, send_buff_len) < 0){
      fprintf(stderr, "Error llwrite ()\n");
      exit(-1);
    }


    send_bytes += read_bytes;
    sequenceN++;
    printf("Sent : %d ; Out of :%d\n",send_bytes,filesize);
  }
  close(fi);
  //END PACKET
  start_packet[0] = END_C2;

  if (llwrite(serial.fileDescriptor, start_packet, st_packet_length) < 0){
    fprintf(stderr, "Error llwrite ()\n");
    exit(-1);
  }
  if(llclose(serial.fileDescriptor) < 0){
    fprintf(stderr, "Error llclose()");
    return -1;
  }
  return 0;
}

int receive_file(){
  unsigned char buffer[256], read_bytes=0, sequenceN=0;
  int fd;
  unsigned buffer_len;

  //START PACKET
  do{
    if (llread(serial.fileDescriptor,buffer,&buffer_len) < 0){
      fprintf(stderr, "Error llread()\n");
      exit(-1);
    }
  } while (buffer_len == 0 || buffer[0] != START_C2);

  printf ("Receiving data . . .\n");

  off_t fileSize = get_file_size(buffer, buffer_len);
  char *filename = get_file_name(buffer, buffer_len);
  mode_t file_mode = get_file_permissions(buffer, buffer_len);

  //DATA PACKETS

  if((fd=open((char *) filename,O_TRUNC | O_CREAT |  O_WRONLY))==-1){
    fprintf(stderr, "Error opening %s\n",filename);
    exit(2);
  }

  ssize_t write_total = 0;
  if (llread(serial.fileDescriptor, buffer, &buffer_len) != 0) {
    printf("Error llread() in function receive_data().\n");
    close(fd);
    exit(-1);
  }

  while (buffer_len == 0 || buffer[0] != END_C2){
    if (buffer_len > 0 && buffer[1] == n(sequenceN)){
      read_bytes = buffer_len - 4;
      write_total += write(fd,buffer+4,read_bytes);
      sequenceN++;

      printf("Received : %ld ; Expected : %ld\n",write_total,fileSize);
    }

    if (llread(serial.fileDescriptor, buffer, &buffer_len) != 0) {
      printf("Error llread() in function receive_data().\n");
      close(fd);
      exit(-1);
    }
  }


  close(fd);

  if(llclose(serial.fileDescriptor) < 0){
    fprintf(stderr, "Error llclose()");
    return -1;
  }

  chmod(filename, file_mode);
  return 0;
}

off_t get_file_size(unsigned char *buffer, int buffer_len){
  int i = 1;
  while (i < buffer_len) {
    if (buffer[i] == FILE_SIZE)
      return *((off_t *)(buffer + i + 2));

    i += 2 + buffer[i + 1];
  }

  return 0;
}

char *get_file_name(unsigned char *buffer, int buffer_len){
  int i = 1;
  while (i < buffer_len) {
    if (buffer[i] == FILE_NAME) {
      char *file_name = (char *)malloc((buffer[i + 1] + 1) * sizeof(char));
      memcpy(file_name, buffer + i + 2, buffer[i + 1]);
      file_name[(buffer[i + 1] + 1)] = 0;
      return file_name;
    }

    i += 2 + buffer[i + 1];
  }

  return NULL;
}

mode_t get_file_permissions(unsigned char *buffer, int buffer_len){
  int i = 1;
  while (i < buffer_len) {
    if (buffer[i] == FILE_PERMISSIONS){
      return *((mode_t *)(buffer + i + 2));
    }

    i += 2 + buffer[i + 1];
  }

  return -1;
}
