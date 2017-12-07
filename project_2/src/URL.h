#ifndef URL_H
#define URL_H

typedef char url_content[256];

struct URL {
	url_content user; // string to user
	url_content password; // string to password
	url_content host; // string to host
	url_content ip; // string to IP
	url_content path; // string to path
	url_content filename; // string to filename
	int port; // integer to port
};

void initURL(struct URL *url);

#endif
