#ifndef APP_LAYER_H
#define APP_LAYER_H

#define FILE_SIZE 0
#define FILE_NAME 1
#define FILE_PERMISSIONS 2

#define DATA_C2 1
#define START_C2 2
#define END_C2 3

#define MILLISECONDS_PER_SECOND 1000
#define NANOSECONDS_PER_MILLISECOND 1000000

#define k(l1,l2) (256*l2+l1)
#define n(x) (x%256)

typedef struct applicationLayer{
       int fileDescriptor; //Serial port file descriptor
       int mode;  //TRANSMITTER 0 / RECEIVER 1
} applicationLayer;

int connection(const char *port, int mode);

int send_file(char* filename);

int receive_file();

off_t get_file_size(unsigned char *buffer, int buffer_len);
char *get_file_name(unsigned char *buffer, int buffer_len);
mode_t get_file_permissions(unsigned char *buffer, int buffer_len);

#endif
