#ifndef DATA_LAYER_H
#define DATA_LAYER_H

#include <signal.h> //sigaction

#define COM1 "/dev/ttyS0"
#define COM2 "/dev/ttyS1"

#define US_FRAME_LENGTH 5
#define I_FRAME_HEADER_SIZE 5

#define TRANSMITTER 0
#define RECEIVER 1

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
  struct sigaction new_action; //timer "start"
  struct sigaction old_action; //timer "stopped"
  unsigned int timeout; //temporizador
  unsigned int numTransmissions; //Nº tentativas caso falhe
} linkLayer;

/*
int llopen(int port, int mode)
argumentos
– port: COM1, COM2, ...
– mode: TRANSMITTER / RECEIVER
retorno
– identificador da ligação de dados
– valor negativo em caso de erro
*/
int llopen(int port, int mode);


int is_frame_SET(unsigned char* frame);
int is_frame_UA(unsigned char* frame);
int is_frame_DISC(unsigned char* frame);
int is_frame_RR(unsigned char* frame);
int is_frame_REJ(unsigned char* frame);

int write_frame(int fd, unsigned char *frame, unsigned int frame_length);
int read_frame (int fd, unsigned char* frame, unsigned int *frame_length);

unsigned char *create_US_frame(unsigned int *frame_length, int control_byte);

int write_buffer(int fd, unsigned char *buffer, int buffer_size);
void read_buffer(int fd, unsigned char* buffer, int *buffer_length);

int send_US_frame(int fd,int control);

int llclose(int fd);

unsigned char* read_byte_destuffing(unsigned char* buff, int *buff_length);
unsigned char* write_byte_stuffing(unsigned char* buff, int *buff_length);

unsigned char get_bcc2(unsigned char *pack,int pack_len);

int send_I(int fd,unsigned char *buffer, int length);

int llwrite(int fd, unsigned char* buffer, int length);
int llread(int fd,unsigned char* buffer, int *buffer_len);

int setTerminalAttributes(int fd);

void setTimeOutSettings(unsigned int timeout, unsigned int retries);
void timeout_handler(int signum);

#endif
