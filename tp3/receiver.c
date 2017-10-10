#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

char set[5] = {0x7E,0x03,0x03,0x00,0x7E};
char UA[5] = {0x7E,0x03,0x07,0x04,0x7E};
int mode;
int fd,fi,size;

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

void llwrite(char* msg){
    int res,i;
	for(i=0;i<5;i++)
    	res = write(fd,msg+i,1);
    sleep(3);
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
	//int j;
  int res;
  char buff[12000];

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    if (llopen(argv[1]) == -1){
      printf("Can't open %s\n",argv[1]);
      exit(1);
    }
		mode = 0;
        size = 10968;
        //llread();

        fi = open(argv[2],O_CREAT | O_RDWR);

        res = read(fd,buff,size);

        res = write(fi,buff,size);


	//for(j=0;j<5;j++)
		//printf("0x%2x\n",set[j]);
	return 0;
  }
