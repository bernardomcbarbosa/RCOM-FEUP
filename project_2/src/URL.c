#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "URL.h"

void initURL(struct URL *url){
	memset(url->user, 0, sizeof(url_content));
	memset(url->password, 0, sizeof(url_content));
	memset(url->host, 0, sizeof(url_content));
	memset(url->path, 0, sizeof(url_content));
	url->port = 21;
}

const char* regExpression =
		"ftp://([([A-Za-z0-9])*:([A-Za-z0-9])*@])*([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";

const char* regExprAnony = "ftp://([A-Za-z0-9.~-])+/([[A-Za-z0-9/~._-])+";


int parseURL(struct URL *url, const char *urlArg){
	char *activeExpression;
	regex_t* regex;
	size_t nmatch = strlen(urlArg);
	regmatch_t pmatch[nmatch];
	int mode;

  if(urlArg[6] == '['){
    //Normal
		activeExpression = (char *) regExpression;
		mode = NORMAL;
  }
  else{
    //Anonymous
		activeExpression = (char *) regExprAnony;
		mode = ANONYMOUS;
  }

	regex = (regex_t*) malloc(sizeof(regex_t));

	if (regcomp(regex, activeExpression, REG_EXTENDED) != 0) {
		perror("URL format is wrong ");
		return -1;
	}

	if (regexec(regex, urlArg, nmatch, pmatch, REG_EXTENDED) != 0) {
		perror("URL couldn't execute ");
		return -1;
	}

	free(regex);

	const char *startOfHost = strchr(urlArg, ']');
	if (mode == NORMAL){
		parseNormalAuth(url, urlArg);
		startOfHost++; //"ftp://[username:password@]->host"
	}
	else{
		initDefaultAuth(url);
		startOfHost = urlArg + strlen(URL_HEADER); //"ftp://->host"
	}

  char* endOfHost = strchr(startOfHost, '/'); //"ftp://host->/"
  memcpy(url->host, startOfHost, (endOfHost-startOfHost));
  printf("Host : %s\n", url->host);

	endOfHost++; //"ftp://host/->path"
	memcpy(url->path, endOfHost, strlen(endOfHost));
	printf("Path : %s\n", url->path);

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
