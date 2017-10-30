
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

void getParameters(){
	unsigned int timeout=0, retries;

	do{
		printf("Set timeout timer [0,...,5]: ");
		scanf("%u", &timeout);
	}while(timeout<=0 || timeout>5);

	do{
		printf("Number of times to retry on timeout [0,...,5]: ");
		scanf("%u", &retries);
	}while(retries<=0 || retries>5);

	setTimeOutSettings(timeout, retries);
}

int main(int argc, char** argv)
{
	int mode;
	char filename[12] = "pinguim.gif";

  if ( (argc < 2) ||
	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

	do{
		printf("TRANSMITTER(0) or RECEIVER(1): ");
		scanf("%d", &mode);
	}while (mode != 0 && mode != 1);

	getParameters();
	if (mode == RECEIVER){
		connection(argv[1], mode);
		receive_file();
	}
	else if (mode == TRANSMITTER){
    // TRANSMITTER
    connection(argv[1], mode);
    send_file(filename);
	}
	else{
		fprintf(stderr, "Invalid mode\n");
		exit(1);
	}

  return 0;
}
