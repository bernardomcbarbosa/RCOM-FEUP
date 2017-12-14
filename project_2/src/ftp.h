#ifndef FTP_H
#define FTP_H

#define FTP_PORT 21

#define CODE_LENGTH 3
#define CODE_READY_NEW_USER "220"
#define CODE_READY_FOR_PW "331"
#define CODE_LOGGED_IN "230"
#define CODE_PASSIVE_MODE "227"
#define CODE_FILE_OKAY "150"

struct FTP{
  int control_socket_fd; // file descriptor to control socket
  int data_socket_fd; // file descriptor to data socket
};

int getIp(struct URL *url);
int connect_to (const char *adress, const int port);
int disconnect_from (const struct FTP *connection, const struct URL *url);

int ftpLogin (const struct FTP *connection, const struct URL *url);
int ftpPasv (struct FTP *connection, char *pasvIP, int *pasvPort);
int ftpRetr (const struct FTP *connection, const struct URL *url);

int ftpDownload (const struct FTP *connection, const struct URL *url);

int ftpWrite(const struct FTP *connection, const char *frame);
int ftpRead(const struct FTP *connection, char *frame, size_t frame_length, char *exp_code);

#endif
