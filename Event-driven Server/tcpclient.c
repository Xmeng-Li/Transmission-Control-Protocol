#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define MAX_LINE 100

int main (int argc, char *argv[]) {
  char* host_addr = argv[1];
  int port = atoi(argv[2]);
  char *sequence_num = argv[3];

  /* Open a socket */
  int s;
  if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
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

  /* Connect to the server */
  if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    perror("simplex-talk: connect");
    close(s);
    exit(1);
  }

  //get message from command line and send lines of text */
  char buf[MAX_LINE];
 
  memset(buf, '\0', MAX_LINE); 
  strcpy(buf, "HELLO ");
  strcat(buf, sequence_num);

  int length = strlen(buf)+1;
  if(send(s, buf, length, 0) < 0){
    perror("Sending message error \n");
    close(s);
    exit(1);
  }

  // clear the buffer before each recv() system call
  memset(buf, '\0', MAX_LINE); 

  // receive message Y from server and print the message
  if(recv(s, buf, sizeof(buf), 0) < 0){
    perror("Receiving message error \n");
    close(s);
    exit(1);
  }
  
  fputs(buf, stdout);
  fputc('\n', stdout);
  fflush(stdout);

  // separate Hello and sequence number
  char* token = strtok(buf, " "); 
  while(token != NULL){
    token = strtok(NULL, " ");
    break;
  }

  int sequence_num1 = atoi(token);

  // add 1 to sequence number and send it back to server
  if(sequence_num1 == atoi(sequence_num) + 1){
    sequence_num1 ++;

    char string[10];
    sprintf(string, "%d", sequence_num1);
    strcat(buf, " ");
    strcat(buf, string);
    
    send(s, buf, length, 0);
    close(s);
  }
  else{
    perror("Error message \n");
    close(s);
    exit(1);
  }
  return 0;
}