#ifndef DATA_LAYER_H
#define DATA_LAYER_H

#define COM1 "/dev/ttyS0"
#define COM2 "/dev/ttyS1"

#define FLAG 0x7E

#define SEND_A 0x03
#define RECEIVE_A 0x01

#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define REJ 0x01
#define RR 0x05

typedef struct linkLayer {
int fd;
char port[20];
int status;
unsigned int timeout; //temporizador
unsigned int numTransmissions; //Nº tentativas caso falhe
} data;

int llopen(int port, int status);

void llclose(int fd);

int llwrite(int fd, unsigned char* buffer, int length);

int llread(int fd,unsigned char* buffer, int *buffer_len);

#endif
