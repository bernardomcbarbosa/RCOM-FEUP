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

int main(int argc, char** argv)
{
	int mode,fd;
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

	if (mode){
		// RECEIVER
		fd = connection(argv[1], mode);
		receive_file();
	}
	else{
    // TRANSMITTER
    fd = connection(argv[1], mode);
    send_file(filename);
	}

  return 0;
}
