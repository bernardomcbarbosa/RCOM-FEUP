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
      resetSettings(fd);
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
        resetSettings(fd);
        free(frame);
        return -1;
      }

      alarm(data_layer.timeout);

      if(read_frame(fd, frame_rsp, &frame_rsp_length)==0){
        attempts = 0;
        alarm(0);
        if(is_frame_UA(frame_rsp)){
          free(frame);
          return fd;
        }
      }

      if(attempts>0 && attempts<data_layer.numTransmissions){
        printf("Connection failed. Retrying %d out of %d...\n",
        attempts, data_layer.numTransmissions);
      }
    }
    printf("Connection timed out\n");
    resetSettings(fd);
    free(frame);
    return -1;
  }
  else{
    fprintf(stderr, "Invalid mode\n");
    exit(1);
  }
}

int llwrite(int fd, unsigned char* data_packet, unsigned int data_packet_length){
  unsigned int frame_length, frame_rsp_length;
  unsigned char *frame, frame_rsp[255];

  frame = create_I_frame(&frame_length, data_packet, data_packet_length);
  while(attempts < data_layer.numTransmissions){
    if(write_frame(fd, frame, frame_length) == -1){
      fprintf (stderr, "Error writing US frame!\n");
      resetSettings(fd);
      free(frame);
      return -1;
    }

    alarm(data_layer.timeout);

    if(read_frame(fd, frame_rsp, &frame_rsp_length)==0){
      attempts = 0;
      alarm(0);
      if(is_frame_RR(frame_rsp)){
        c = !c;
        free(frame);
        return 0; //num caracteres escritos
      }
      //If we get a REJ frame it will just resend the I frame
    }

    alarm(0);
    if(attempts>0 && attempts<data_layer.numTransmissions){
      printf("Connection failed. Retrying %d out of %d...\n",
      attempts, data_layer.numTransmissions);
    }
  }
  printf("Connection timed out\n");
  resetSettings(fd);
  free(frame);
  return -1;
}

int llread(int fd, unsigned char *data_packet, unsigned int *data_packet_length){
  unsigned char frame_rsp[512], expected_bcc2;
  unsigned char* data_packet_destuffed, *frame;
  unsigned int frame_rsp_length, frame_length, data_packet_destuffed_length;
  int read_succesful=0, sig=0;
  static int c = 0;

  while(!read_succesful){
    read_frame(fd, frame_rsp, &frame_rsp_length);

    if (is_frame_DISC(frame_rsp)){
      llclose(fd);
    }

    if(!is_I_frame_header_valid(frame_rsp, frame_rsp_length)){
      printf("Invalid frame header. Rejecting frame..\n");
      frame = create_US_frame(&frame_length, REJ);
    }
    else{
      frame_rsp_length -= I_FRAME_HEADER_SIZE;

      memcpy(data_packet, frame_rsp+4, frame_rsp_length);

      data_packet_destuffed_length = frame_rsp_length;
      data_packet_destuffed = read_byte_destuffing(data_packet, &data_packet_destuffed_length);
      //check bbc2 of destuffed data packet
      expected_bcc2 = data_packet_destuffed[data_packet_destuffed_length-1];

      if((expected_bcc2 == get_bcc2(data_packet_destuffed, data_packet_destuffed_length-1))){
        //Valid bcc2 - might still be a duplicate frame
        frame = create_US_frame(&frame_length, RR);

        if(is_I_frame_sequence_number_valid(frame_rsp[2], c)){
          //Correct frame
          c = !c;
        }
        else{
          printf("Found duplicate frame with correct bcc2. Discarding...\n");
          sig = 1;
        }
        read_succesful = 1;
      }
      else{
        //Invalid bcc2
        if(is_I_frame_sequence_number_valid(frame_rsp[2], c)){
          frame = create_US_frame(&frame_length, REJ);
          printf("Found new incorrect frame. Rejecting...\n");
        }
        else{
          frame = create_US_frame(&frame_length, RR);
          printf("Found duplicate frame. Discarding...\n");
          sig = 1;
          read_succesful = 1;
        }
      }
    }

    if (write_frame(fd, frame, frame_length) == -1){
      fprintf (stderr, "Error writing US frame!\n");
      resetSettings(fd);
      free(frame);
      return -1;
    }
  }
  free(frame);

  if (!sig){
    *data_packet_length = data_packet_destuffed_length-1;
    memcpy(data_packet, data_packet_destuffed, data_packet_destuffed_length-1);
  }
  else{
    *data_packet_length = 0;
  }

  free(data_packet_destuffed);
  return 0;
}

int llclose(int fd){
  unsigned char *frame, frame_rsp[255];
  unsigned int frame_length, frame_rsp_length;
  attempts = 0;

  if(data_layer.mode == RECEIVER){
    do{
      read_frame(fd, frame_rsp, &frame_rsp_length);
    }while (!is_frame_DISC(frame_rsp));

    frame = create_US_frame(&frame_length, DISC);
    if (write_frame(fd, frame, frame_length) == -1){
      fprintf (stderr, "Error writing US frame!\n");
      resetSettings(fd);
      free(frame);
      return -1;
    }
    resetSettings(fd);
    free(frame);

    printf ("Receiver finished \n");
    return 1;
  }
  else if(data_layer.mode == TRANSMITTER){
    frame = create_US_frame(&frame_length, DISC);
    while (attempts < data_layer.numTransmissions){
      if (write_frame(fd, frame, frame_length) == -1){
        fprintf (stderr, "Error writing US frame!\n");
        resetSettings(fd);
        free(frame);
        return -1;
      }

      alarm(data_layer.timeout);

      if(read_frame(fd, frame_rsp, &frame_rsp_length)==0){
        attempts = 0;
        alarm(0);
        if(is_frame_DISC(frame_rsp)){
          //Received DISC so we send UA
          frame = create_US_frame(&frame_length, UA);
          if (write_frame(fd, frame, frame_length) == -1){
            fprintf (stderr, "Error writing US frame!\n");
            resetSettings(fd);
            free(frame);
            return -1;
          }
          free(frame);
          resetSettings(fd);
          printf ("Transmitter finished \n");
          return 1;
        }
      }

      alarm(0);
      if(attempts>0 && attempts<data_layer.numTransmissions){
        printf("Connection failed. Retrying %d out of %d...\n",
        attempts, data_layer.numTransmissions);
      }
    }
    printf("Connection timed out\n");
    resetSettings(fd);
    free(frame);
    return -1;
  }
  else{
    fprintf(stderr, "Invalid mode\n");
    exit(1);
  }

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

int is_I_frame_header_valid(unsigned char *frame, unsigned int frame_len){
  if (frame_len < 6){
      return 0;
  }

  return (frame[0] == FLAG && frame[1] == SEND_A &&
         frame[3] == (frame[1] ^ frame[2]));
}

int is_I_frame_sequence_number_valid(unsigned char control_byte, int c){
  return (control_byte == (c << 6));
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

unsigned char *create_I_frame(unsigned int *frame_length, unsigned char *data_packet, unsigned int data_packet_length){
  unsigned char bcc2;
  unsigned int bcc_length;
  unsigned char *stuffed_bcc, *stuffed_data_packet;
  unsigned char *frame;
  static int c=1;
  c = !c;

  bcc2 = get_bcc2(data_packet, data_packet_length);
  bcc_length = 1;
  stuffed_bcc = write_byte_stuffing (&bcc2, &bcc_length);
  stuffed_data_packet = write_byte_stuffing (data_packet, &data_packet_length);

  (*frame_length) = 5 + data_packet_length + bcc_length;
  frame = malloc (*frame_length * sizeof(char));

  frame[0] = FLAG;
  frame[1] = SEND_A;
  frame[2] = c<<6;
  frame[3] = frame[1]^frame[2];

  memcpy (frame + 4, stuffed_data_packet, data_packet_length);
  memcpy (frame + data_packet_length + 4, stuffed_bcc, bcc_length);

  frame[*frame_length-1] = FLAG;
  free(stuffed_data_packet);
  free(stuffed_bcc);

  return frame;
}

unsigned char* read_byte_destuffing(unsigned char* buff, unsigned int *buff_length){
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

unsigned char* write_byte_stuffing(unsigned char* buff, unsigned int *buff_length){
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

unsigned char get_bcc2(unsigned char *pack, unsigned int pack_len){
  int i=0;
  char c = pack[i];
  for(i=1;i<pack_len;i++)
    c ^= pack[i];
  return c;
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

int resetSettings(int fd) {
  if (sigaction(SIGALRM, &data_layer.old_action, NULL) == -1){
    printf("Error setting SIGALRM handler to original.\n");
  }

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
    printf("Error settings old port settings.\n");
  }

  if (close(fd)) {
    printf("Error closing terminal file descriptor.\n");
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
}
