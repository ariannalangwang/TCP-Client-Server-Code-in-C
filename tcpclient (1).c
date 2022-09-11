#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_LINE 100  // max length of the message

void resetBuffer(char *buf, char *newString);


int main (int argc, char *argv[]) {
  char *host_addr = argv[1];
  int port = atoi(argv[2]);
  char *string_x = argv[3];
  int x = atoi(argv[3]);

  /* Open a socket */
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) <0) {
    perror("simplex-talk: socket");
    exit(1);
  }

  /* Config the server address */
  struct sockaddr_in sin;
  sin.sin_family = AF_INET; 
  sin.sin_addr.s_addr = inet_addr(host_addr);
  sin.sin_port = htons(port);
  // Set all bits of the padding field to 0
  memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));

  /* Connect to the server */  // connect the socket to the server
  if ( connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0 ) {  // binding the socket to this particular IP address
    perror("simplex-talk: connect");
    close(s);
    exit(1);
  }

  /* main code: send and receive lines of text */
  char buf[MAX_LINE];
  int y, z;
  char string_z[10];

  resetBuffer(buf, string_x);
  int len = strlen(buf) + 1;
  send(s, buf, len, 0);  // send "HELLO X"
  
  recv(s, buf, sizeof(buf), 0);  // receive "HELLO Y" 
  fputs(buf, stdout);  // print out "HELLO Y"

  y = atoi(buf+6);
  if ( y == (x+1) ) {
    z = y + 1;    
    sprintf(string_z, "%d", z); 
    resetBuffer(buf, string_z);
    len = strlen(buf) + 1;
    send(s, buf, len, 0);  // send "HELLO Z"
    close(s);
  } else {
    perror("ERROR");
    close(s);
  }

  return 0;
}


void resetBuffer(char *buf, char *newString) {
    memset(buf, '\0', MAX_LINE*sizeof(buf[0]));
    strcpy(buf,  "HELLO ");
    strcat(buf, newString);
    strcat(buf, "\n");
}