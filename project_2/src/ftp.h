#ifndef FTP_H
#define FTP_H

struct FTP{
  int control_socket_fd; // file descriptor to control socket
  int data_socket_fd; // file descriptor to data socket
};

int getIp(struct URL *url);

#endif
