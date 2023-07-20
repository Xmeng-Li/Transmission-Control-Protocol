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

#define MAX_PENDING 50
#define MAX_LINE 100

void *mythread(void *arg);

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
  pthread_t tids[105];
  int i;

  while(1) {
    for( i = 0; i < 105; i++){
      if((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0){
        perror("simplex-talk: accept");
        exit(1);
      }

      // Spawn threads, and pass a random value to each thread
      if(pthread_create(&tids[i], NULL, mythread, (void*)(intptr_t)new_s) < 0){
        perror("Creating thread error \n");
        exit(1);     
      };
    }
  }  
  return 0;
}

// Describe the behavior of a thread - main part
void *mythread(void *arg) {

  /* wait for connection, then receive and print text */
  int new_s = (intptr_t)arg;
  char buf[MAX_LINE];
  int len;

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

  int sequence_num2 = atoi(token);
  if(sequence_num2 != sequence_num + 1){
    perror("Error in sequence Z");
    close(new_s);
  }  

  // Exit - the current thread terminates
  pthread_exit(NULL);
}