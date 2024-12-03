#include "../include/utils.h"
#include "../include/server.h"
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <stdint.h>

//TODO: Declare a global variable to hold the file descriptor for the server socket
int master_fd;

//TODO: Declare a global variable to hold the mutex lock for the server socket
pthread_mutex_t lock;
//pthread_mutex_init(&lock, NULL) not sure if we need to initialize this somewhere? 

//TODO: Declare a gloabl socket address struct to hold the address of the server
// do we need this? we declare them within the funcitons.

/*
################################################
##############Server Functions##################
################################################
*/

/**********************************************
 * init
   - port is the number of the port you want the server to be
     started on
   - initializes the connection acception/handling system
   - if init encounters any errors, it will call exit().
************************************************/
void init(int port) {
   //TODO: create an int to hold the socket file descriptor
  int sd;

   //TODO: create a sockaddr_in struct to hold the address of the server
  struct sockaddr_in server_addr;

   /**********************************************
    * IMPORTANT!
    * ALL TODOS FOR THIS FUNCTION MUST BE COMPLETED FOR THE INTERIM SUBMISSION!!!!
    **********************************************/
  
   // TODO: Create a socket and save the file descriptor to sd (declared above)
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    perror("Error in socket call");
    exit(EXIT_FAILURE);
  }

   // TODO: Change the socket options to be reusable using setsockopt(). 
  int opt = 1;
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("Error when changing options");
    close(sd);
    exit(EXIT_FAILURE);
  }
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

   // TODO: Bind the socket to the provided port.
  if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error in bind()");
    close(sd);
    exit(EXIT_FAILURE);
  }

   // TODO: Mark the socket as a pasive socket. (ie: a socket that will be used to receive connections)
  if (listen(sd, SOMAXCONN) < 0) {
    perror("Error in listen call");
    close(sd);
    exit(EXIT_FAILURE);
  }
   
   
   
   // We save the file descriptor to a global variable so that we can use it in accept_connection().
   // TODO: Save the file descriptor to the global variable master_fd
  master_fd = sd;
  printf("UTILS.O: Server Started on Port %d\n",port);
  fflush(stdout);
}


/**********************************************
 * accept_connection - takes no parameters
   - returns a file descriptor for further request processing.
   - if the return value is negative, the thread calling
     accept_connection must should ignore request.
***********************************************/
int accept_connection(void) {

   //TODO: create a sockaddr_in struct to hold the address of the new connection
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);


   /**********************************************
    * IMPORTANT!
    * ALL TODOS FOR THIS FUNCTION MUST BE COMPLETED FOR THE INTERIM SUBMISSION!!!!
    **********************************************/
   
   
   
   // TODO: Aquire the mutex lock
  pthread_mutex_lock(&lock);
   // TODO: Accept a new connection on the passive socket and save the fd to newsock
  int newsock = accept(master_fd, (struct sockaddr *)&client_addr, &client_len);
  if (newsock < 0) {
    perror("Error accepting connection");
    pthread_mutex_unlock(&lock);
    return -1;
  }

   // TODO: Release the mutex lock
  pthread_mutex_unlock(&lock);

   // TODO: Return the file descriptor for the new client connection
  return newsock;
}


/**********************************************
 * send_file_to_client
   - socket is the file descriptor for the socket
   - buffer is the file data you want to send
   - size is the size of the file you want to send
   - returns 0 on success, -1 on failure 
************************************************/
int send_file_to_client(int socket, char * buffer, int size) 
{
  //TODO: create a packet_t to hold the packet data
  packet_t packet;

  //TODO: send the file size packet
  packet.size = size;
  if (write(socket, &packet, sizeof(packet_t)) < 0) {
    perror("Write error in sending file size to client");
    return -1;
  }

    //TODO: send the file data
  int bytes_sent = 0;
  while (bytes_sent < size) {
    int chunk_size = size - bytes_sent;
    if (write(socket, buffer + bytes_sent, chunk_size) < 0) {
      perror("Write error in sending file data to client");
      return -1;
    }
    bytes_sent += chunk_size;
  }
  //TODO: return 0 on success, -1 on failure
  return 0;
}


/**********************************************
 * get_request_server
   - fd is the file descriptor for the socket
   - filelength is a pointer to a size_t variable that will be set to the length of the file
   - returns a pointer to the file data
************************************************/
char * get_request_server(int fd, size_t *filelength){
  //TODO: create a packet_t to hold the packet data
  packet_t packet;

  //TODO: receive the response packet
  if (read(fd, &packet, sizeof(packet_t)) < 0) {
    perror("Error receiving file size");
    return NULL;
  }

  //TODO: get the size of the image from the packet
  *filelength = packet.size;
  
  //TODO: recieve the file data and save into a buffer variable.
  char *buffer = malloc(*filelength);
  size_t bytes_received = 0;
  while (bytes_received < *filelength) {
    ssize_t chunk_size = read(fd, buffer + bytes_received, *filelength - bytes_received);
    if (chunk_size < 0) {
      perror("Error receiving file data");
      free(buffer);
      return NULL;
    }
    bytes_received += chunk_size;
  }

  //TODO: return the buffer
  return buffer;

}


/*
################################################
##############Client Functions##################
################################################
*/

/**********************************************
 * setup_connection
   - port is the number of the port you want the client to connect to
   - initializes the connection to the server
   - if setup_connection encounters any errors, it will call exit().
************************************************/
int setup_connection(int port)
{
  //TODO: create a sockaddr_in struct to hold the address of the server   
  struct sockaddr_in server_addr;

  //TODO: create a socket and save the file descriptor to sockfd
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Error creating socket in setup_connection");
    exit(EXIT_FAILURE);
  }
  //TODO: assign IP, PORT to the sockaddr_in struct
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  //TODO: connect to the server
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error connecting to server");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  //TODO: return the file descriptor for the socket
  return sockfd;
}


/**********************************************
 * send_file_to_server
   - socket is the file descriptor for the socket
   - file is the file pointer to the file you want to send
   - size is the size of the file you want to send
   - returns 0 on success, -1 on failure
************************************************/
int send_file_to_server(int socket, FILE *file, int size) 
{
  //TODO: send the file size packet
  packet_t packet;
  packet.size = size;
  if (write(socket, &packet, sizeof(packet_t)) < 0) {
    perror("Error sending file size packet");
    return -1;
  }

  //TODO: send the file data
  char buffer[1024]; 
  int bytes_read, bytes_sent = 0;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    if (write(socket, buffer, bytes_read) < 0) {
      perror("Error sending file data");
      return -1;
    }
    bytes_sent += bytes_read;
  }

  // TODO: return 0 on success, -1 on failure
  if (ferror(file)) { // check to make sure file was read correctly
    perror("Error reading file");
    return -1;
  }
  if (bytes_sent != size) { // check to make sure write size matches file size
    fprintf(stderr, "Error: Sent bytes (%d) do not match file size (%d)\n", bytes_sent, size);
    return -1;
  }
  return 0;

}

/**********************************************
 * receive_file_from_server
   - socket is the file descriptor for the socket
   - filename is the name of the file you want to save
   - returns 0 on success, -1 on failure
************************************************/
int receive_file_from_server(int socket, const char *filename) 
{
  //TODO: create a buffer to hold the file data
  char buffer[1024];  

  //TODO: open the file for writing binary data
  FILE *file = fopen(filename, "wb");
  if (!file) {
    perror("Error opening file when receiving from server");
    return -1;
  }
    
  //TODO: create a packet_t to hold the packet data
  packet_t packet;

  //TODO: receive the response packet
  if (read(socket, &packet, sizeof(packet_t)) < 0) {
    perror("Error receiving file size packet");
    fclose(file);
    return -1;
  }

  //TODO: get the size of the image from the packet
  size_t file_size = packet.size;

  //TODO: recieve the file data and write it to the file
  size_t bytes_received = 0;
  while (bytes_received < file_size) {
    ssize_t chunk_size = read(socket, buffer, sizeof(buffer));
    if (chunk_size < 0) {
      perror("Error receiving file data");
      fclose(file);
      return -1;
    }
    if (fwrite(buffer, 1, chunk_size, file) != (size_t)chunk_size) {
      perror("Error writing to file");
      fclose(file);
      return -1;
    }
    bytes_received += chunk_size;
  }
  fclose(file);

  //TODO: return 0 on success, -1 on failure
  return 0;
}

