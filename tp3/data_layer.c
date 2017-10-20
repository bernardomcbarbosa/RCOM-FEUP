#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>

#include "data_layer.h"

#define BAUDRATE B38400

char BST[2] = {0x7D,0x5E};
data data_layer;
static int c=0; //llwrite / RR / REJ

int llopen(int port, int status){
  int fd;
  struct termios oldtio,newtio;
  unsigned char buffer[5];
  int buffer_length;

  data_layer.status = status;

  switch (port) {
    case 0:
      strcpy(data_layer.port, COM1);
      break;

    case 1:
      strcpy(data_layer.port, COM2);
      break;
    default:
      break;
  }

  fd = open(data_layer.port, O_RDWR | O_NOCTTY );
    if (fd <0) {
		perror(data_layer.port);
		exit(-1);
	}

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    if(!data_layer.status){
      //If TRANSMITTER, send SET and receive UA
      if(send_US(fd,SET) == -1)
        return -1;

      read_buffer(fd,buffer,&buffer_length);
      if(!is_US_UA(buffer))
        return -1;
    }
    else { //IF RECEIVER, read SET and send UA
      read_buffer(fd,buffer,&buffer_length);
      if(!is_US_SET(buffer)){
        return -1;
      }

        if(send_US(fd,UA) == -1)
          return -1;
    }
    return fd;
}

void print_frame(unsigned char *frame,int frame_len){
  int i;
  printf("----------------\n");
  for(i=0;i<frame_len;i++)
    printf("0x%2X\n",frame[i]);
  printf("----------------\n");
}

int is_US_SET(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]==SET && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_US_UA(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]==UA && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_DISC(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == RECEIVE_A && frame[2]== DISC && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_RR(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]== (c << 7 | RR) && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_REJ(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]== (c << 7 | REJ) && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int write_buffer(int fd, unsigned char *buffer, int buffer_length){
  int res_total = 0,res=0;

 while (buffer_length > res_total) { //Make sure all buffer is sent
   res = write(fd, buffer, buffer_length);

   if (res <= 0) {
     return -1;
   }
   res_total += res;
 }
 return res_total;
}

void read_buffer(int fd, unsigned char* buffer, int *buffer_length){
  int i=0,res=0;

  //S1
  while (res==0) {
    res = read(fd,buffer,1);
  }
    i++;

  //S2
  while ((res!=0) && (buffer[0] == FLAG)) {
    res = read(fd,buffer+i,1);
    if(buffer[i] != FLAG)
      break;
  }

  //S3
  i++;
  while ((res!=0)) {
    res = read(fd,buffer+i,1);
    if(buffer[i] == FLAG)
      break;
    i++;
  }
  i++;
  *buffer_length = i;
}

int send_US(int fd,int control) {
  unsigned char* US_msg;
  int attempts = 0;
  static int c = 0;

  US_msg = (unsigned char *) malloc(5);

  US_msg[0] = FLAG;

  if(data_layer.status == 0){
    if(control == SET || control == DISC)
      US_msg[1] = SEND_A;
    else
      US_msg[1] = RECEIVE_A;
  }
  else{
    if(control == UA || control == REJ || control == RR)
      US_msg[1] = SEND_A;
    else
      US_msg[1] = RECEIVE_A;
  }

  if (control == REJ || control == RR) {
    US_msg[2] = c << 7 | control;
    c = !c;
  } else
    US_msg[2] = control;

  US_msg[3] = US_msg[1] ^ US_msg[2];
  US_msg[4] = FLAG;

  // while (attempts <= data_layer.numTransmissions + 1) {
    if (write_buffer(fd, US_msg, 5) == -1) {
      printf("Error writing US frame!\n");
      return -1;
    }
  free(US_msg);

    // alarm(data_layer.timeout);
    //
    // alarm(0);
    // if (attempts > 0 && attempts < data_layer.numTransmissions)
    //   printf("Connection failed! Retrying %d out of %d...\n",attempts, data_layer.numTransmissions);
  // }

  return 0;
}

void llclose(int fd){
  close(fd);
}

unsigned char* read_byte_destuffing(unsigned char* buff, int *buff_length){
  int i,destuff=0,buff_destuffed_len;
  unsigned char* buff_destuffed;

  buff_destuffed_len = (*buff_length);
  buff_destuffed = (unsigned char *)malloc(buff_destuffed_len);

  for(i=0;i<(*buff_length);i++){
    if(i == (*buff_length)-1){
      buff_destuffed[destuff] = buff[i];
    }
    else{
      if(buff[i] == BST[0] && buff[i+1] == BST[1]){
        buff_destuffed[destuff] = FLAG;
        destuff++;
        i++;
        buff_destuffed_len--;
        buff_destuffed = (unsigned char *)realloc(buff_destuffed,buff_destuffed_len);
      }
      else{
        buff_destuffed[destuff] = buff[i];
        destuff++;
      }
    }
  }
  (*buff_length) = buff_destuffed_len;
  return buff_destuffed;
}

unsigned char* write_byte_stuffing(unsigned char* buff, int *buff_length){
  int i,stuff=0,buff_stuffed_len;
  unsigned char *buff_stuffed;

  buff_stuffed_len = (*buff_length);
  buff_stuffed = (unsigned char *)malloc(buff_stuffed_len);

  for(i=0;i<(*buff_length);i++){
    if(buff[i] == FLAG){
    buff_stuffed[stuff] = BST[0];
    stuff++;
    buff_stuffed[stuff] = BST[1];
    stuff++;
    buff_stuffed_len++;
    buff_stuffed = (unsigned char *)realloc(buff_stuffed,buff_stuffed_len);
    }
    else{
      buff_stuffed[stuff] = buff[i];
      stuff++;
    }
  }
  (*buff_length) = buff_stuffed_len;
  return buff_stuffed;
}

unsigned char get_bcc2(unsigned char *pack,int pack_len){
  int i=0;
  char c = pack[i];
  for(i=1;i<pack_len;i++)
    c ^= pack[i];
  return c;
}

int send_I(int fd,unsigned char *buffer, int length){
  unsigned char bcc2;
  unsigned char *buffer_stuffed;
  int buf_len,final_len;
  static int c=1;
  c = !c;

  //get bbc2
  bcc2 = get_bcc2(buffer,length);
  buf_len = length+1;
  buffer = (unsigned char *)realloc(buffer,buf_len);
  buffer[buf_len-1] = bcc2;

  //stuff buffer
  buffer_stuffed = write_byte_stuffing(buffer,&buf_len);

  //header
  final_len = 4 + buf_len + 1;
  unsigned char* final_buff = (unsigned char *)malloc(final_len);
  final_buff[0] = FLAG;
  final_buff[1] = SEND_A;
  final_buff[2] = c << 6;
  final_buff[3] = final_buff[1] ^ final_buff[2];
  memcpy(final_buff + 4, buffer_stuffed, buf_len);
  free(buffer_stuffed);
  final_buff[final_len-1] = FLAG;

  return write_buffer(fd,final_buff,final_len);
}

int llwrite(int fd, unsigned char* buffer, int length){
    unsigned char buff[5];
    int buff_len,ok=1;
    while(ok){ //timeout connections
      send_I(fd,buffer,length);
      read_buffer(fd,buff,&buff_len);
      if(is_RR(buff)){
        c = !c;
        ok = 0;
      }
      // else if(is_US_REJ){
      //
      // }
    }

      return 0;
  }

int llread(int fd,unsigned char* buffer, int *buffer_len){
  unsigned char buff[512],bcc2;
  unsigned char* buffer_destuffed;
  static int c = 0;
  int buff_len;
  int finish=0;


  while(!finish){
  //read buffer from tty
  read_buffer(fd,buff,&buff_len);

  //check header
   if (is_DISC(buff))
    llclose(fd);

   if(!(buff[0] == FLAG && buff[1] == SEND_A && buff[3] == (buff[1] ^ buff[2]))){
     return -1;
   }
   else{
     buff_len -= 5; // 5 == FLAG + A + C1 + BCC1 + FLAG
   }

   memcpy(buffer,buff+4,buff_len);

   //destuff buffer
   buffer_destuffed = read_byte_destuffing(buffer,&buff_len);

   //check bbc2 of buffer
   bcc2 = buffer_destuffed[buff_len-1];

   if(!( bcc2 == get_bcc2(buffer_destuffed,buff_len-1) )) {

     if(buff[2] != (c<<6)){
       //duplicate frame
       //send RR
       //ask again
       printf("frame duplicado\n");
       send_US(fd,RR);
       return -1;
     }
     else{
       printf("bcc2 errado\n");
       send_US(fd,REJ);
       //REJ
     }

   }
   else{

     if(buff[2] != (c<<6)){
       //duplicate frame
       printf("frame duplicado e bcc2 certo\n");
       return -1;
     }
     else{
       //correct frame
       //printf("frame correto\n");
       send_US(fd,RR);
       c = !c;
     }
     finish = !finish;
   }
 }

  *buffer_len = buff_len-1;
  memcpy(buffer,buffer_destuffed,buff_len-1);
  free(buffer_destuffed);
  return 0;
}
