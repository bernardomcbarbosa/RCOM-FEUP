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

data serial;

int llopen(int port, int status){
  int fd;
  struct termios oldtio,newtio;

  switch (port) {
    case 0:
      strcpy(serial.port, COM1_PORT);
      break;

    case 1:
      strcpy(serial.port, COM2_PORT);
      break;
    default:
      break;
  }

  fd = open(serial.port, O_RDWR | O_NOCTTY );
    if (fd <0) {
		perror(port);
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

  return 0;
}

void llclose(){
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
  r = strchr(buff,START_END);
  while( r != NULL){
    r[0] = BST[0];
    r++;
    addChar(r,BST[1]);
    r = strchr(buff,START_END);
  }
}

char* get_package(int c2){
    char* buff[];
    int res,i=4;

    buff = (char *) malloc(504);
    res = read(fi,buff+i,251);
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
    int res,i;
    char* buff;

    buff = get_package();

    for(i=0;i<size;i++){
      res = write(fd,buff+i,i);
      printf("%d\n",i);
    }

    free(buff);
  }

void llread(){
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

//READER MSG
	for(i=0;i<5;i++)
		printf("0x%x |",buffer[i]);
	printf("\n");

	if(!mode){
		conf = buffer[1]^buffer[2];
		if(conf == buffer[3])
	  		llwrite(UA);
		else
			printf("Message Invalid\n");
	}
	else{
		conf = buffer[1]^buffer[2];
		if(conf == buffer[3])
			printf("Sucess\n");
		else
			printf("Message Invalid\n");
	}
}
