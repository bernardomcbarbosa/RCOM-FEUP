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
  int fd; //Serial port file descriptor
  char port[20]; //Serial port device
  int mode; //TRANSMITTER 0 / RECEIVER 1
  struct sigaction new_action; //timer "start"
  struct sigaction old_action; //timer "stopped"
  unsigned int timeout; //Time to timeout
  unsigned int numTransmissions; //Maximum number of retries
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

/*
int llwrite(int fd, char * buffer, int length)
argumentos
– fd: identificador da ligação de dados
– buffer: array de caracteres a transmitir
– length: comprimento do array de caracteres
retorno
– número de caracteres escritos
– valor negativo em caso de erro
*/
int llwrite(int fd, unsigned char* data_packet, unsigned int data_packet_length);

/*
int llread(int fd, char * buffer)
argumentos
– fd: identificador da ligação de dados
– buffer: array de caracteres recebidos
retorno
– número de caracteres escritos
– valor negativo em caso de erro
*/
int llread(int fd, unsigned char *data_packet, unsigned int *data_packet_length);

/*
int llclose(int fd)
argumentos
– fd: identificador da ligação de dados
retorno
– valor positivo em caso de sucesso
– valor negativo em caso de erro
*/
int llclose(int fd);

int is_frame_SET(unsigned char* frame);
int is_frame_UA(unsigned char* frame);
int is_frame_DISC(unsigned char* frame);
int is_frame_RR(unsigned char* frame);
int is_frame_REJ(unsigned char* frame);
int is_I_frame_header_valid(unsigned char *frame, unsigned int frame_len);
int is_I_frame_sequence_number_valid(unsigned char control_byte, int c);

int write_frame(int fd, unsigned char *frame, unsigned int frame_length);
int read_frame (int fd, unsigned char* frame, unsigned int *frame_length);

unsigned char *create_US_frame(unsigned int *frame_length, int control_byte);
unsigned char *create_I_frame(unsigned int *frame_length, unsigned char *data_packet, unsigned int data_packet_length);

unsigned char* read_byte_destuffing(unsigned char* buff, unsigned int *buff_length);
unsigned char* write_byte_stuffing(unsigned char* buff, unsigned int *buff_length);

unsigned char get_bcc2(unsigned char *pack, unsigned int pack_len);

int setTerminalAttributes(int fd);
int resetSettings(int fd);

void setTimeOutSettings(unsigned int timeout, unsigned int retries);
void timeout_handler(int signum);

#endif
