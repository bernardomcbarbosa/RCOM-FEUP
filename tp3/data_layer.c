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

char BST[2] = {0x7D,0x5E};
linkLayer data_layer;
static int c=0; //llwrite / RR / REJ
struct termios oldtio;
unsigned int attempts = 0;

int llopen(int port, int mode){
  int fd;
  //struct termios newtio;
  unsigned char *frame, frame_rsp[255];
  unsigned int frame_length, frame_rsp_length;
  attempts = 0;

  data_layer.mode = mode;

  switch (port) {
    case 0:
    {
      strcpy(data_layer.port, COM1);
    }
    break;
    case 1:
    {
      strcpy(data_layer.port, COM2);
    }
    break;
    default:
    {
      return -1;
    }
    break;
  }

  fd = open(data_layer.port, O_RDWR | O_NOCTTY );
  if (fd <0) {
    perror(data_layer.port);
    return -1;
  }

  if (setTerminalAttributes(fd) != 0) {
    fprintf(stderr, "Error setTerminalAttributes().\n");
    return -1;
  }

  if (sigaction(SIGALRM, &data_layer.new_action, &data_layer.old_action) == -1){
    fprintf(stderr, "Error installing SIGALRM handler\n");
    return -1;
  }

  if (data_layer.mode == RECEIVER){
    do{
      read_frame(fd, frame_rsp, &frame_rsp_length);
    }while (!is_frame_SET(frame_rsp));

    frame = create_US_frame(&frame_length, UA);
    if (write_frame(fd, frame, frame_length) == -1){
      fprintf (stderr, "Error writing US frame!\n");
      free(frame);
      return -1;
    }
    free(frame);
    return fd;
  }
  else if (data_layer.mode == TRANSMITTER){
    frame = create_US_frame(&frame_length, SET);
    while (attempts < data_layer.numTransmissions){
      if (write_frame(fd, frame, frame_length) == -1){
        fprintf (stderr, "Error writing US frame!\n");
        free(frame);
        return -1;
      }

      alarm(data_layer.timeout);

      if(read_frame(fd, frame_rsp, &frame_rsp_length)==0){
        attempts = 0;
        alarm(0);
        if(is_frame_UA(frame_rsp)){
          return fd;
        }
      }

      if(attempts>0 && attempts>data_layer.numTransmissions){
        printf("Connection failed. Retrying %d out of %d...\n",
        attempts, data_layer.numTransmissions);
      }
    }
    free(frame);
    return -1;
  }
  else{
    fprintf(stderr, "Invalid mode\n");
    exit(1);
  }
}

int llwrite(int fd, unsigned char* buffer, int length){
    unsigned char buff[5];
    int buff_len,ok=1;
    while(ok){ //timeout connections
      send_I(fd,buffer,length);
      read_buffer(fd,buff,&buff_len);
      if(is_frame_RR(buff)){
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
   if (is_frame_DISC(buff))
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
       send_US_frame(fd,RR);
       return -1;
     }
     else{
       printf("bcc2 errado\n");
       send_US_frame(fd,REJ);
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
       send_US_frame(fd,RR);
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

int llclose(int fd){
  int buff_len;
  unsigned char buff[5];

  //RECEIVER
  if(data_layer.mode){
    read_buffer(fd, buff, &buff_len);
    if(!is_frame_DISC(buff))
      return -1;
    else
        printf("DISC received");

    send_US_frame(fd,DISC);
  }
  else{
    send_US_frame(fd,DISC);

    read_buffer(fd, buff, &buff_len);
    if(!is_frame_DISC(buff))
      return -1;
    else
      printf("DISC received");

    send_US_frame(fd,UA);
  }

  close(fd);
  return 0;
}

int is_frame_SET(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]==SET && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_frame_UA(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]==UA && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_frame_DISC(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]== DISC && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_frame_RR(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]== (c << 7 | RR) && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int is_frame_REJ(unsigned char* frame){
  if(frame[0] == FLAG && frame[1] == SEND_A && frame[2]== (c << 7 | REJ) && ((frame[1] ^ frame[2]) == frame[3]) && frame[4] == FLAG)
    return 1;
  else
    return 0;
}

int write_frame(int fd, unsigned char *frame, unsigned int frame_length){
  int bytes_sent, bytes_sent_total = 0;

  while (bytes_sent_total < frame_length){
    bytes_sent = write(fd, frame, frame_length);

    if (bytes_sent <= 0){
      return -1;
    }
    bytes_sent_total += bytes_sent;
  }
  return bytes_sent_total;
}

int read_frame (int fd, unsigned char *frame, unsigned int *frame_length){
  unsigned int first_flag=0, end_of_frame=0;
  char buf;
  *frame_length = 0;

  while (!end_of_frame){
    if (read(fd, &buf, 1)>0){
      if (buf == FLAG){
        if(!first_flag){
          //if the first flag hasn't been read yet
          first_flag = 1;
        }
        else if (first_flag){
          //reading the second flag
          end_of_frame = 1;
        }

        frame[*frame_length] = buf;
        (*frame_length)++;
      }
      else{
        //If the char is not a flag and
        //the final flag has not been found
        //then add it to the frame.
        if(first_flag){
          frame[*frame_length]=buf;
          (*frame_length)++;
        }
      }
    }
    else{
      printf("timeout?");
      return -1; //usually caused by a timeout
    }
  }

  return 0;
}

unsigned char *create_US_frame(unsigned int *frame_length, int control_byte){
  static int c = 0;
  unsigned char *US_frame = (unsigned char *) malloc(US_FRAME_LENGTH * sizeof(char));

  US_frame[0] = FLAG;

  if(data_layer.mode == RECEIVER){
    if(control_byte == UA || control_byte == REJ || control_byte == RR || control_byte == DISC)
      US_frame[1] = SEND_A;
    else
      US_frame[1] = RECEIVE_A;
  }
  else if (data_layer.mode == TRANSMITTER){
    if(control_byte == SET || control_byte == DISC)
      US_frame[1] = SEND_A;
    else
      US_frame[1] = RECEIVE_A;
  }
  else{
    fprintf(stderr, "Invalid mode\n");
    exit(1);
  }

  if (control_byte == REJ || control_byte == RR) {
    US_frame[2] = c << 7 | control_byte;
    c = !c;
  } else
    US_frame[2] = control_byte;

  US_frame[3] = US_frame[1] ^ US_frame[2];
  US_frame[4] = FLAG;

  *frame_length = US_FRAME_LENGTH;
  return US_frame;
}



/* Make sure all of the buffer is sent*/
int write_buffer(int fd, unsigned char *buffer, int buffer_size){
  int bytes_sent, bytes_sent_total = 0;

  while (bytes_sent_total < buffer_size){
    bytes_sent = write(fd, buffer, buffer_size);

    if (bytes_sent <= 0){
      return -1;
    }
    bytes_sent_total += bytes_sent;
  }
  return bytes_sent_total;
}


void read_buffer(int fd, unsigned char* buffer, int *buffer_length){
  int i=0,res;

  //S1
  do{
    res = read(fd,buffer,1);
  }while (res==0);
  i++;

  //S2
  while ((res!=0) && (buffer[0] == FLAG)) {
    res = read(fd,buffer+i,1);
    if(buffer[i] != FLAG)
      break;
  }
  i++;

  //S3
  while ((res!=0)) {
    res = read(fd,buffer+i,1);
    if(buffer[i] == FLAG)
      break;
    i++;
  }
  i++;
  *buffer_length = i;
}

int send_US_frame(int fd,int control) {
  unsigned char* US_msg;
  static int c = 0;

  US_msg = (unsigned char *) malloc( sizeof(char)*US_FRAME_LENGTH);

  US_msg[0] = FLAG;

  if(data_layer.mode){
    //RECEIVER
    if(control == UA || control == REJ || control == RR || control == DISC)
      US_msg[1] = SEND_A;
    else
      US_msg[1] = RECEIVE_A;
  }
  else{
    //TRANSMITTER
    if(control == SET || control == DISC)
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
  if (write_buffer(fd, US_msg, US_FRAME_LENGTH) == -1) {
    printf("Error writing US frame!\n");
    return -1;
  }

  free(US_msg);

  // alarm(data_layer.timeout);
  //
  // alarm(0);
  // if (attempts > 0 && attempts < data_layer.numTransmissions)
  //   printf("Connection failed! Retrying %d out of %d...\n",attempts, data_layer.numTransmissions);
  // }asd

  return 0;
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

int setTerminalAttributes(int fd) {
  struct termios newtio;

  if (tcgetattr(fd, &oldtio) == -1) {
    /* save current port settings */
    perror("tcgetattr");
    close(fd);
    return -1;
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    close (fd);
    return -1;
  }

  return 0;
}

void setTimeOutSettings(unsigned int timeout, unsigned int retries){
  data_layer.timeout = timeout;
  data_layer.numTransmissions = retries;

  data_layer.new_action.sa_handler = timeout_handler;
  data_layer.new_action.sa_flags &= !SA_RESTART; //stop the primitives functions (like read or write)
}

void timeout_handler(int signum){
  attempts++;
  printf ("%d", attempts);
  printf ("entrou no handler\n");
}
