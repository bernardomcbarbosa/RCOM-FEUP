#ifndef DATA_LAYER_H
#define DATA_LAYER_H

#define COM1 "/dev/ttyS0"
#define COM2 "/dev/ttyS1"

#define US_frame_size 5

#define FLAG 0x7E

#define SEND_A 0x03
#define RECEIVE_A 0x01

#define SET 0x03
#define DISC 0x0B
#define UA 0x07
#define REJ 0x01
#define RR 0x05

#define BAUDRATE B38400

typedef struct linkLayer {
int fd;
char port[20];
int mode; //TRANSMITTER 0 / RECEIVER 1
unsigned int timeout; //temporizador
unsigned int numTransmissions; //Nº tentativas caso falhe
} linkLayer;

int llopen(int port, int status);

void print_frame(unsigned char *frame,int frame_len);

int is_US_SET(unsigned char* frame);
int is_US_UA(unsigned char* frame);
int is_DISC(unsigned char* frame);
int is_RR(unsigned char* frame);
int is_REJ(unsigned char* frame);

int write_buffer(int fd, unsigned char *buffer, int buffer_size);
void read_buffer(int fd, unsigned char* buffer, int *buffer_length);

int send_US_frame(int fd,int control);

void llclose(int fd);

unsigned char* read_byte_destuffing(unsigned char* buff, int *buff_length);
unsigned char* write_byte_stuffing(unsigned char* buff, int *buff_length);

unsigned char get_bcc2(unsigned char *pack,int pack_len);

int send_I(int fd,unsigned char *buffer, int length);

int llwrite(int fd, unsigned char* buffer, int length);
int llread(int fd,unsigned char* buffer, int *buffer_len);

#endif
