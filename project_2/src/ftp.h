#ifndef FTP_H
#define FTP_H

#define FTP_PORT 21

#define CODE_LENGTH 4
#define CODE_READY_NEW_USER "220"
#define CODE_READY_FOR_PW "331"
#define CODE_LOGGED_IN "230"

struct FTP{
  int control_socket_fd; // file descriptor to control socket
  int data_socket_fd; // file descriptor to data socket
};

int getIp(struct URL *url);
int connect_to (const char *adress, const int port);

int ftpLogin (const struct FTP *connection, const struct URL *url);
int ftpPasv (struct FTP *connection);

int ftpWrite(const struct FTP *connection, const char *frame);
int ftpReadCode(const struct FTP *connection, char *frame, char *code);
int ftpRead(const struct FTP *connection, char *frame, size_t frame_length);


#endif
