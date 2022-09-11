
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

struct client {
  int fd;   
  int x;
};

#define MAX_PENDING 100  // max # of clients the server can receive
#define MAX_LINE 100  // max length of the message 

void handle_first_shake(struct client *ptr);
void handle_second_shake(struct client *ptr);
void resetBuffer(char *buf, char *newString);

int main(int argc, char *argv[]) {
  char* host_addr = "127.0.0.1";
  int port = atoi(argv[1]);  // 8080

  /* setup passive open */   
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

  // Now, the server is ready to listen to pending calls from clients
  listen(s, MAX_PENDING);  

  int new_s;
  socklen_t len = sizeof(sin);
  int array_length = 0;
  struct client client_state_array[MAX_PENDING];  
  memset(client_state_array, 0, sizeof(client_state_array));  // clear client_state_array all to 0

  // set up master rfds: master has all fds  
  fd_set rfds_master;  
  FD_ZERO(&rfds_master);
  // for the server socket fd: s into the master rfds
  fcntl(s, F_SETFL, O_NONBLOCK);   
  FD_SET(s, &rfds_master);  

  while (1) {
    // set up temp rfds: temp only has the fds for this select() call in this one loop
    fd_set rfds_temp;
    FD_ZERO(&rfds_temp);
    // get the fds from the master
    rfds_temp = rfds_master;

    // find maximum fd number for the select() statement
    int max_fd = s;  // already include s in max_fd
    for (int i = 0; i < array_length; i++)  {  
      if (client_state_array[i].fd > max_fd) {
        max_fd = client_state_array[i].fd;
      }
    }

    int res_select = pselect(max_fd+1, &rfds_temp, NULL, NULL, NULL, NULL); // note pslelct() only keeps changed fds in rfds_temp

    if (res_select < 0) {
			perror("pselect");
			exit(1);
		}
    else if (res_select == 0)
			continue;
		
    // if s has changed:
    if ( FD_ISSET(s, &rfds_temp) ) {  
      if ( (new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0 ) {   
      perror("simplex-talk: accept");
      exit(1);
      }
      
      // add new_s into client_state_array if it's not in there already. (tested in temp.c)
      int pos = -1;
      for (int i = 0; i < array_length; i++) { // loop through client_state_array
        if (client_state_array[i].fd == new_s) {
          pos = i;
          break;
        }
      }
      if (pos == -1) {
        struct client new_client = {new_s, -1};
        client_state_array[array_length] = new_client;
        array_length++;
      }

      // add new_s into rfds_master
      fcntl(new_s, F_SETFL, O_NONBLOCK);   
      FD_SET(new_s, &rfds_master);  

    }  
    
    // if any client fds have changed:
    for (int i = 0; i < array_length; i++) { // loop through client_state_array
      if (FD_ISSET(client_state_array[i].fd, &rfds_temp)) {  
        if (client_state_array[i].x == -1) {   
          handle_first_shake(&client_state_array[i]);
        } else {    
          handle_second_shake(&client_state_array[i]);
          // clear fd from rfds_master.
          FD_CLR(client_state_array[i].fd, &rfds_master);   
          // remove this struct from client_state_array.
          for (int j = i; j < array_length - 1; j++) {
            client_state_array[j] = client_state_array[j+1];
          }
          array_length--;
        }
      }
    }
    
  }  /* END while(1) loop */

  return 0;
}


void handle_first_shake(struct client *ptr) {
  char buf[MAX_LINE];
  memset(buf, '\0', MAX_LINE*sizeof(buf[0])); //clear buffer
  char string_y[10];

  recv(ptr->fd, buf, sizeof(buf), 0);  // receive "HELLO X" 
  fputs(buf, stdout);  // print out "HELLO X"

  int x = atoi(buf+6);
  int y = x + 1;    
  sprintf(string_y, "%d", y); 
  resetBuffer(buf, string_y);

  int len = strlen(buf) + 1;
  send(ptr->fd, buf, len, 0);  // send "HELLO Y"

  ptr->x = x;  // put x value into the struct
}


void handle_second_shake(struct client *ptr) {
  char buf[MAX_LINE];
  memset(buf, '\0', MAX_LINE*sizeof(buf[0]));  // clear buffer
  recv(ptr->fd, buf, sizeof(buf), 0);  // receive "HELLO Z" 
  fputs(buf, stdout);  // print out "HELLO Z"
  
  int z = atoi(buf+6);
  int x = ptr->x;
  if ( z != (x+2) ) {
    perror("ERROR");
    close(ptr->fd);
    exit(1);
  }
  close(ptr->fd);
}


void resetBuffer(char *buf, char *newString) {
    memset(buf, '\0', MAX_LINE*sizeof(buf[0]));
    strcpy(buf,  "HELLO ");
    strcat(buf, newString);
    strcat(buf, "\n");
}