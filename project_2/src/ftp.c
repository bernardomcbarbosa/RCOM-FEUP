int getIp(struct URL *url){
  struct hostent* h;

  if ((h = gethostbyname(url->host)) == NULL) {
		herror("gethostbyname");
		return 1;
	}


  printf("Host name  : %s\n", h->h_name);
  printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *) h->h_addr)));


	char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
	strcpy(url->ip, ip);

  return 0;
}
