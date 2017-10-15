#include "serial.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>


#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define START_END 0x7E
#define C1_0 0x00
#define C1_1 0x02
#define C2_START 2
#define C2_END 3
#define k(l1,l2) (256*l2+l1)
#define n(x) (x%256)
#define c1_I(x) (x%2)

volatile int STOP=FALSE;

char set[5] = {0x7E,0x03,0x03,0x00,0x7E};
char UA[5] = {0x7E,0x03,0x07,0x04,0x7E};
char BST[2] = {0x7D,0x5E};
int mode;
int fd,fi,size,atual;

int llopen(char* port){
  struct termios oldtio,newtio;

  fd = open(port, O_RDWR | O_NOCTTY );
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

int main(int argc, char** argv)
{
	char buf[256];
	int res,i;
  struct stat st;

    if ( (argc < 3) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 pinguim.gif\n");
      exit(1);
    }

    if (llopen(argv[1]) == -1){
      printf("Can't open %s\n",argv[1]);
      exit(1);
    }

	fi = open(argv[2],O_RDONLY);
	// fseek(fi, 0, SEEK_END); // seek to end of file
	// size = ftell(fi); // get current file pointer
	// fseek(f, 0, SEEK_SET); // seek back to beginning of file
  fstat(fi, &st);
  size = st.st_size;
  atual = 0;
	mode = 1;

    //
        llwrite(set);
		    //   llread();
    llclose();
    return 0;
  }
