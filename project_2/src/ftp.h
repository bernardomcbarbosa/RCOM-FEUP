#ifndef FTP_H
#define FTP_H

#define FTP_PORT 21

struct FTP{
  int control_socket_fd; // file descriptor to control socket
  int data_socket_fd; // file descriptor to data socket
};

int getIp(struct URL *url);
int connect_to (const char *adress, const int port);

//int ftpWrite ();
int ftpRead (struct FTP *connection);


#endif
