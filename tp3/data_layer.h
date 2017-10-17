#ifndef DATA_LAYER_H
#define DATA_LAYER_H

#define FLAG 0x7E

#define SEND_A 0x03
#define RECEIVE_A 0x01

#define SET 0x03
#define DISC 0x0B
#define UA 0x07
// #define RR 0x05
// #define REJ 0x01

#define COM1 "/dev/ttyS0"
#define COM2 "/dev/ttyS1"

// #define US_FRAME_LENGTH 5
// #define I_FRAME_HEADER_SIZE 6

#define k(l1,l2) (256*l2+l1)
#define n(x) (x%256)
#define c1_I(x) (x%2)

typedef struct linkLayer {
int fd;
char port[20];
int baudRate;
int status;
unsigned int sequenceNumber;
unsigned int timeout; //temporizador
unsigned int numTransmissions; //Nº tentativas caso falhe
} data;

int llopen(int port, int status);

void llclose(int fd);

#endif