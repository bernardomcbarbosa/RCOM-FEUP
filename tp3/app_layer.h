#ifndef APP_LAYER_H
#define APP_LAYER_H

#include "data_layer.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>

#define FILE_SIZE 0
#define FILE_NAME 1

#define DATA_C2 1
#define START_C2 2
#define END_C2 3

typedef struct applicationLayer {
       int fileDescriptor; /*Descritor correspondente à porta série*/
       int status;  /*TRANSMITTER | RECEIVER*/
} app;

int connection(const char *port, int status);
void send_file(const char *port,char* filename);
#endif
