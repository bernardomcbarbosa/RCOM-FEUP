#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

char buf[5];
int fd;

int llopen(char* port){
  int fd;
  struct termios oldtio,newtio;

  fd = open(port, O_RDWR | O_NOCTTY );
  if (fd <0){
    perror(port);
    return -1;
  }

  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    return -1;
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
    return -1;
  }
  return 0;
}

void llclose(){
  close(fd);
}

void llread(){
  int i,res;
  char buffer[5],c;

  //S1
  i=0;
  while (STOP==FALSE) {
    res = read(fd,c,1);
    if(res>0){
      if (c !=0x7E){
       STOP=TRUE;
      }
      else
        TA[i] = c;
    }
  }

  STOP = FALSE;
  TA[1] = c;
  i = 2;
  while (STOP==FALSE) {
    res = read(fd,TA+i,1);
    if(res>0){
      if (TA[i]==0x7E){
       STOP=TRUE;
      }
     i++;
    }
  }
  llwrite(TA);
}

void llwrite(){
  int res;

  buf[0] = 0x7E;
  buf[1] = 0x03;
  buf[2] = 0x03;
  buf[3] = 0x00;
  buf[4] = 0x7E;

  res = write(fd,buf,strlen(buf)+1);
  sleep(3);
}

int main(int argc, char** argv)
{
    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    if ((fd = llopen(argv[1])) == -1){
      printf("Can't open %s\n",argv[1]);
      exit(1);
    }

        llread(fd);
        return 0;
  }
