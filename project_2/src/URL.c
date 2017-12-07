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

  const char *startOfHost = strchr(urlArg, ']');
  if(urlArg[6] == '['){
    //Normal
    parseNormalAuth(url, urlArg);
    startOfHost++; //"ftp://[username:password@]->host"
  }
  else{
    //Anonymous
    initDefaultAuth(url);
    startOfHost = urlArg + strlen(URL_HEADER); //"ftp://->host"
  }

  char* endOfHost = strchr(startOfHost, '/');
  memcpy(url->host, startOfHost, (endOfHost-startOfHost));
  printf("Host : %s\n", url->host);

  return 0;
}

int parseNormalAuth(struct URL *url, const char *urlArg){
  //We already verified "ftp://"

  char* username = strchr(urlArg, '/'); //"ftp:->/"
  username += 3; //"ftp://[->username"

  char* password = strchr(username, ':');
  if(password == NULL){
    fprintf(stderr, "Your link must contain a ':' separating the username and password!'\n");
    return 1;
  }
  memcpy(url->user, username, (password-username));
  printf("Username : %s\n", url->user);

  password++; //"ftp://[username:->password@]"

  char* endOfPassword = strrchr(urlArg, '@');
  if(endOfPassword == NULL){
    fprintf(stderr, "Your link must contain a '@' delimiting the password");
    return 1;
  }

  memcpy(url->password, password, (endOfPassword-password));
  printf("Password : %s\n", url->password);

  return 0;
}

void initDefaultAuth(struct URL *url){
  memcpy(url->user, DEFAULT_USER, strlen(DEFAULT_USER)+1);
  printf("Username : %s\n", url->user);

  memcpy(url->password, DEFAULT_PASSWORD, strlen(DEFAULT_PASSWORD)+1);
  printf("Password : %s\n", url->password);
}
