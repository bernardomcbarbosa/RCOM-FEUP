#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "app_layer.h"
#include "data_layer.h"

#define MODEMDEVICE "/dev/ttyS1"

int main(int argc, char** argv)
{
	int mode=-1,fd;

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

		printf("TRANSMITTER(0) or RECEIVER(1): ");
		scanf("%d",&mode);

    while (mode != 0 && mode != 1) {
        printf("Invalid mode!\nTry Again!\n");
				printf("TRANSMITTER(0) or RECEIVER(1): ");
				scanf("%d",&mode);
      }

      // TRANSMITTER -> 0
      // RECEIVER -> 1
    if(!mode){
      char filename[50] = "pinguim.gif";
			fflush(stdin);
      // printf("File: ");
      // fgets(filename,50,stdin);
      // filename[strlen(filename)-1] = '\0';

      // TRANSMITTER
      fd = connection(argv[1], mode);

      if(fd == -1)
        return -1;

      send_file(filename);
    }
    else if(mode){
      // RECEIVER
      fd = connection(argv[1], mode);

      if(fd == -1)
        return -1;

      receive_file();
    }

    return 0;
  }
