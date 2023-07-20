#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <fcntl.h> 

/* This is a reference socket server implementation that prints out the messages
 * received from clients. */

#define MAX_PENDING 50
#define MAX_LINE 100
#define CLIENT_CONNECTIONS 100

void handle_first_shake(int socket);
void handle_second_shake(int socket);

int main(int argc, char *argv[]) {
  char* host_addr = "127.0.0.1";
  int port = atoi(argv[1]);

  /*setup passive open*/
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
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

  /* Bind the socket to the address */
  if((bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0) {
    perror("simplex-talk: bind");
    exit(1);
  }

  // connections can be pending if many concurrent client requests
  listen(s, MAX_PENDING);  
  int new_s;
  socklen_t len = sizeof(sin);

  // declar a client structure
  struct clientState{
    int fd, state, handshake;
  };

  // create client-state array to track each client, and initialize it
  struct clientState clientStateArray[CLIENT_CONNECTIONS];

  for (int i = 0; i < CLIENT_CONNECTIONS; i++){
    struct clientState eachClient;

    eachClient.fd = -1; 
    eachClient.state = -1;
    eachClient.handshake = -1;
    clientStateArray[i] = eachClient;
  }

  // create and initialize master_set and ready_set
  int fd_max = s;
  fd_set master_set;
  fd_set ready_set;

  // set s to non-blocking using fcntl and add s to master_set
  if(fcntl(s, F_SETFL, O_NONBLOCK) < 0){
    perror("Setting s to non-blocking error");
    exit(1);
  }

  FD_SET(s, &master_set);

  while(1) {
    // initialize ready_set to 0 and copy master_set to ready_set
    FD_ZERO(&ready_set);
    ready_set = master_set;

    // use select() on ready_set
    if((pselect(fd_max + 1, &ready_set, NULL, NULL, NULL, NULL)) < 0){
      perror("Selecting error");
      exit(1);
    }
    else if((pselect(fd_max + 1, &ready_set, NULL, NULL, NULL, NULL)) == 0){
      continue;
    }

    for(int i = 0; i <= fd_max; i++){
      //check if this socket is a listening socket
      if FD_ISSET(i, &ready_set){

        if(i == s){
          if((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0){
          perror("simplex-talk: accept");
          exit(1);
          }
        
          // add it to master_set
          FD_SET(new_s, &master_set);

          // add new_s to client_state_array
          for (int j = 0; j < CLIENT_CONNECTIONS; j++){
            struct clientState currentClient = clientStateArray[j];

            if(currentClient.fd == -1){
              clientStateArray[j].fd = new_s;
              clientStateArray[j].state = 1;
              clientStateArray[j].handshake = s;
              break;
            }
          }

          // update new_s, if new_s > fd_max
          if(new_s > fd_max){
            fd_max = new_s;
          }
        }
        else{ 
          // iterate through client_state_array and check if existing file descriptors is present
          for (int j = 0; j < CLIENT_CONNECTIONS; j++){
            struct clientState currentClient = clientStateArray[j];

            // check if the status of file-descriptor showing first handshake then update it
            if(currentClient.fd == i && currentClient.state == 1){
              handle_first_shake(i);
              clientStateArray[j].state = 2;
            }

            else if(currentClient.fd == i && currentClient.state == 2){
              handle_second_shake(i);
              close(i);

              // remove i from master_set and re-initialize struct entry
              FD_CLR(i, &master_set);
              
              clientStateArray[j].fd = -1;
              clientStateArray[j].state = -1;
              clientStateArray[j].handshake = -1;
            }
          }
        }
      }  
    }
  }  
  return 0;
}

// handle the first handshake
void handle_first_shake(int socket){
  int new_s = socket;
  int len;
  char buf[MAX_LINE];

  //clear the buffer before each recv() system call 
  memset(buf, '\0', MAX_LINE); 

// receive message X from client and print the message
  if((len = recv(new_s, buf, sizeof(buf), 0)) < 0){
    perror("Receiving message error \n");
    exit(1);
  }
  else{
    fputs(buf, stdout);
    fputc('\n', stdout); 
    fflush(stdout);
  }

  // separate Hello and sequence number
  char* token = strtok(buf, " "); 
  while(token != NULL){
    token = strtok(NULL, " ");
    break;
  }

  int sequence_num = atoi(token);
  sequence_num ++;

  char string[10];
  sprintf(string, "%d", sequence_num);
  strcat(buf, " ");
  strcat(buf, string);

  // send message Y back to client
  int length = strlen(buf) + 1;
  if(send(new_s, buf, length, 0) < 0){
    perror("Sending message error \n");
  }
}  

// handle the second handshake
void handle_second_shake(int socket){
  int new_s = socket;
  int len;
  char buf[MAX_LINE];

  //clear the buffer before each recv() system call 
  memset(buf, '\0', MAX_LINE); 

  //receive message Z from client and print the message
  if((len = recv(new_s, buf, sizeof(buf), 0)) < 0){
    perror("Receiving message error \n");
  }
  else{
    fputs(buf, stdout);
    fputc('\n', stdout);  
    fflush(stdout);
  }

  // separate Hello and sequence number
  char* token = strtok(buf, " "); 
  while(token != NULL){
    token = strtok(NULL, " ");
    break;
  }

  int sequence_num2 = atoi(token);
  if(sequence_num2 != socket + 1){
    perror("Error in sequence Z");
    close(new_s);
  }
}