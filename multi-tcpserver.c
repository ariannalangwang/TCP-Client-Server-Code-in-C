
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>

/* This is a reference socket server implementation that prints out the messages
 * received from clients. */

#define MAX_PENDING 200  // max # of clients the server can receive
#define MAX_LINE 100  // max length of the message 

void *handshake(void *ptr);
void resetBuffer(char *buf, char *newString);

int main(int argc, char *argv[]) {
  // char* host_addr = argv[1];  // 127.0.0.1
  char* host_addr = "127.0.0.1";
  int port = atoi(argv[1]);  // 8080

  /* setup passive open */  // open a socket
  int s;
  if ( (s = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("simplex-talk: socket");
    exit(1);
  }

  /* Config the server address */  // boiler-plate
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;  // protocol
  sin.sin_addr.s_addr = inet_addr(host_addr);  // address
  sin.sin_port = htons(port);  // port
  // Set all bits of the padding field to 0
  memset(sin.sin_zero, '\0', sizeof(sin.sin_zero));

  /* Bind the socket to the address */
  if ( (bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0 ) {
    perror("simplex-talk: bind");
    exit(1);
  }

  // connections can be pending if many concurrent client requests
  listen(s, MAX_PENDING);  // this regulates how many clients the socket can listen to concurrently

  /* wait for connection, then receive and print text */
  int new_s;
  socklen_t len = sizeof(sin);

  while(1) {
    if ( (new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0 ) {  // each client coming in gets a new socket (new_s)
      perror("simplex-talk: accept");
      exit(1);
    }

    // spawn off a new thread here to handle the handshake
    // so that the main thread can keep getting new client requests in.
    pthread_t id;
    pthread_create(&id, NULL, handshake, (void*) &new_s);
  }

  return 0;
}

void *handshake(void *ptr) {
  char buf[MAX_LINE];
  int len;
  int x, y, z;
  char string_y[10];
  int new_s = *((int*)ptr);

  recv(new_s, buf, sizeof(buf), 0);  // receive "HELLO X" 
  fputs(buf, stdout);  // print out "HELLO X"

  x = atoi(buf+6);
  y = x + 1;    
  sprintf(string_y, "%d", y); 
  resetBuffer(buf, string_y);
  len = strlen(buf) + 1;
  send(new_s, buf, len, 0);  // send "HELLO Y"

  recv(new_s, buf, sizeof(buf), 0);  // receive "HELLO Z" 
  fputs(buf, stdout);  // print out "HELLO Z"
  
  z = atoi(buf+6);
  if (z != (y+1)) {
    perror("ERROR");
    close(new_s);
    return 0;
  }

  close(new_s);
  pthread_exit(NULL); 
}


void resetBuffer(char *buf, char *newString) {
    memset(buf, '\0', MAX_LINE*sizeof(buf[0]));
    strcpy(buf,  "HELLO ");
    strcat(buf, newString);
    strcat(buf, "\n");
}