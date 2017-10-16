#include "data_layer.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#define BAUDRATE B38400

char BST[2] = {0x7D,0x5E};
data data_layer;

int is_US_SET(char* frame);
int is_US_UA(char* frame);
int write_buffer(int fd, char *buffer, int buffer_length);
void read_buffer(int fd, char* buffer, int *buffer_length);
int send_US(int fd,int control);
void print_US_frame(char* frame);

int llopen(int port, int status){
  int fd;
  struct termios oldtio,newtio;
  char buffer[5];
  int buffer_length;

  data_layer.status = status;

  switch (port) {
    case 0:
      strcpy(data_layer.port, COM1);
      break;

    case 1:
      strcpy(data_layer.port, COM2);
      break;
    default:
      break;
  }

  fd = open(data_layer.port, O_RDWR | O_NOCTTY );
    if (fd <0) {
		perror(data_layer.port);
		exit(-1);
	}

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

      if(!data_layer.status){ //If TRANSMITTER, send SET and receive UA
    if(send_US(fd,SET) == -1)
      return -1;

      read_buffer(fd,buffer,&buffer_length);
      if(!is_US_UA(buffer))
        return -1;
    }
    else { //IF RECEIVER, read SET and send UA
      read_buffer(fd,buffer,&buffer_length);
      if(!is_US_SET(buffer)){
        print_US_frame(buffer);
        return -1;
      }

        if(send_US(fd,UA) == -1)
          return -1;
    }
    return fd;
}

void print_US_frame(char *frame){
  int i;
  for(i=0;i<5;i++)
    printf(" %2X ",frame[i]);
  printf("\n");
}

int is_US_SET(char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]==SET && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_US_UA(char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]==UA && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int write_buffer(int fd, char *buffer, int buffer_length){
  int res_total = 0,res=0;

 while (buffer_length > res_total) { //Make sure all buffer is sent
   res = write(fd, buffer, buffer_length);

   if (res <= 0) {
     return -1;
   }
   res_total += res;
 }
 return 0;
}

void read_buffer(int fd, char* buffer, int *buffer_length){
  int i=0,res=0;

  //S1
  while (res==0) {
    res = read(fd,buffer,1);
  }
    i++;

  //S2
  while ((res!=0) && (buffer[0] == FLAG)) {
    res = read(fd,buffer+i,1);
    if(buffer[i] != FLAG)
      break;
  }

  //S3
  i++;
  while ((res!=0) && (buffer[i] != FLAG)) {
    res = read(fd,buffer+i,1);
    i++;
  }

  *buffer_length = i-1;
  print_US_frame(buffer);
}

int send_US(int fd,int control) {
  char* US_msg;
  int attempts = 0;

  US_msg = (char *) malloc(5);

  US_msg[0] = FLAG;

  if(data_layer.status == 0){
    if(control == SET || control == DISC)
      US_msg[1] = SEND_A;
    else
      US_msg[1] = RECEIVE_A;
  }
  else{
    if(control == UA) // || control == REJ || control == RR)
      US_msg[1] = SEND_A;
    else
      US_msg[1] = RECEIVE_A;
  }


  US_msg[2] = control;
  US_msg[3] = US_msg[1] ^ US_msg[2];
  US_msg[4] = FLAG;

  // while (attempts <= data_layer.numTransmissions + 1) {
    if (write_buffer(fd, US_msg, 5)) {
      printf("Error writing US frame!\n");
      return -1;
    }

    // alarm(data_layer.timeout);
    //
    // alarm(0);
    // if (attempts > 0 && attempts < data_layer.numTransmissions)
    //   printf("Connection failed! Retrying %d out of %d...\n",attempts, data_layer.numTransmissions);
  // }

  return 0;
}

void llclose(int fd){
  close(fd);
}

void addChar(char *str, char c){
  int i = 0;
  str[strlen(str)+1] = '\0';
  str[strlen(str)] = c;
  for(i=strlen(str)-1;i>=1;i--)
      str[i] = str[i-1];
  str[0] = c;
}

void removeChar(char *str, char c) {
int i = 0;

for(i=0;i<strlen(str)-1;i++)
    str[i] = str[i+1];
str[i] = '\0';
}

void write_byte_stuffing(char* buff){
  char *r;
  r = strchr(buff,FLAG);
  while( r != NULL){
    r[0] = BST[0];
    r++;
    addChar(r,BST[1]);
    r = strchr(buff,FLAG);
  }
}

char* get_package(int c2){
    char* buff;
    // int res,i=4;
    //
    buff = (char *) malloc(504);
    // res = read(fi,buff+i,251);
  return buff;
}

char get_bcc2(char *pack){
  int i=0;
  char c = pack[i];
  for(i=1;i<strlen(pack);i++)
    c ^= pack[i];
  return c;
}

void llwrite(char* msg){
    // int res,i;
    // char* buff;
    //
    // buff = get_package();
    //
    // for(i=0;i<size;i++){
    //   res = write(fd,buff+i,i);
    //   printf("%d\n",i);
    // }
    //
    // free(buff);
  }

void llread(int fd){
  int i,res=0;
  char buffer[5],conf;

  //S1
  i=0;
  while (res==0) {
    res = read(fd,buffer,1);
    }
    i++;
//S2
while ((res!=0) && (buffer[0] == 0x7E)) {
    res = read(fd,buffer+i,1);
	if(buffer[i] != 0x7E)
		break;
    }
//S3
i++;
while ((res!=0) && (buffer[i] != 0x7E)) {
    res = read(fd,buffer+i,1);
	i++;
    }
}
