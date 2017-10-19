#ifndef APP_LAYER_H
#define APP_LAYER_H

#define FILE_SIZE 0x00
#define FILE_NAME 1

#define DATA_C2 1
#define START_C2 2
#define END_C2 3

#define k(l1,l2) (256*l2+l1)
#define n(x) (x%256)

typedef struct applicationLayer {
       int fileDescriptor; /*Descritor correspondente à porta série*/
       int status;  /*TRANSMITTER | RECEIVER*/
} app;

int connection(const char *port, int status);

int send_file(char* filename);

int receive_file();

#endif
