#include <stdio.h>
#include <string.h>

#include "URL.h"

void initURL(struct URL *url){
	memset(url->user, 0, sizeof(url_content));
	memset(url->password, 0, sizeof(url_content));
	memset(url->host, 0, sizeof(url_content));
	memset(url->path, 0, sizeof(url_content));
	memset(url->filename, 0, sizeof(url_content));
	url->port = 21;
}


int parseURL(struct URL *url, const char *urlArg){
  if(strncmp(urlArg, URL_HEADER, strlen(URL_HEADER)) != 0){
    fprintf(stderr, "Your link must begin with 'ftp://'\n");
    return 1;
  }

  if(urlArg[6] == '['){
    //Normal
    parseNormalAuth(url, urlArg);
  }
  else{
    //Anonymous
    initDefaultAuth(url);
  }

  return 0;
}

int parseNormalAuth(struct URL *url, const char *urlArg){

  return 0;
}

void initDefaultAuth(struct URL *url){
  memcpy(url->user, DEFAULT_USER, strlen(DEFAULT_USER)+1);
  memcpy(url->password, DEFAULT_PASSWORD, strlen(DEFAULT_PASSWORD)+1);
}
