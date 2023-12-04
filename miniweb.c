#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include "base64.h"
#include <stdlib.h> 

int fromHex(char ch) {
  if(ch >= '0' && ch <= '9')
    return (int) ch - '0';
  return (int) ch - 'A' + 10;
}

void decodeURL(char* src,char* dest) {
  while(*src != '\0') {
    if(*src == '%') {
      ++src;
      int n1 = fromHex(*src++);
      int n2 = fromHex(*src++);
      *dest++ = (char) n1*16+n2;
    } else {
      *dest++ = *src++;
    }
  }
  *dest = '\0';
}

void send404Response(int fd) {
    char buffer[1024];

    // Open the 404 error response file
    int f404 = open("404Response.txt", O_RDONLY);

    if (f404 == -1) {
        // Handle file open error
        perror("Error opening 404Response.txt");
        return;
    }

    // Read the contents of the 404 error response file
    int readSize = read(f404, buffer, sizeof(buffer));

    if (readSize == -1) {
        // Handle file read error
        perror("Error reading 404Response.txt");
        close(f404);
        return;
    }

    // Close the file
    close(f404);

    // Send the 404 error response to the client
    write(fd, buffer, readSize);
}


void serveRequest(int fd) {
  // Read the request
  char buffer[1024];
	int bytesRead = read(fd,buffer,1023);
  buffer[bytesRead] = '\0';

  // Grab the method and URL
  char method[16];
  char url[128];
  sscanf(buffer,"%s %s",method,url);

  // Check the HTTP method (GET or POST)
  if (strcmp(method, "POST") == 0) {
    // Handle POST request
    // Extract the body of the POST request
    // strstr returns a pointer to the start of the pattern
    char *bodyStart = strstr(buffer, "\r\n\r\nurl=");

    // Move past the "url=" part to get the URL
    bodyStart += strlen("\r\n\r\nurl=");
    char userUrl[1024];
    strcpy(userUrl, bodyStart);

    
    // Decode the URL
    char decodedUrl[128];  // Declare a character array
  
    //The decodeURL() function takes two parameters, a pointer to a source string 
    //containing an encoded URL and a pointer to a character array where you want the decoded URL to be written.
    decodeURL(userUrl, decodedUrl);

    // Open the text file for appending
    // wa = writing and append mode
    FILE *urlFile = fopen("urls.txt", "wa");

    // Determine the current file position using ftell
    long filePosition = ftell(urlFile);

    // Append the URL to the file with a newline character
    fprintf(urlFile, "%s\n", decodedUrl);

    // Close the file
    fclose(urlFile);

    char shortUrl[128];
    encode(filePosition, shortUrl);

    //construct response for the POST
    int f200 = open("200Response.txt", O_RDONLY);
    int readSize = read(f200, buffer, 1023); /**/
    buffer[readSize] = '\0';
    close(f200);

    //locate the xxxxxx pattern
    char *shortUrlStart = strstr(buffer, "XXXXXX");

    //replace the Xs with the short Url
    size_t urlLength = strlen(shortUrl);
    size_t patternLength = strlen("XXXXXX");

    for(int i=0; i<6; i++){
      *shortUrlStart = shortUrl[i];
      shortUrlStart++;
    }

    //send the response
    write(fd, buffer, readSize);

  } else if (strcmp(method, "GET") == 0) {
      // Handle GET request
      char *urlStart = strstr(buffer, "GET /s/");
      if (urlStart == NULL) {
          // Request does not start with /s/
          // Return a 404 response
          send404Response(fd);
          return;
      }

      // Move the pointer to the start of the code sequence (after "/s/")
      urlStart += strlen("GET /s/");

      // Extract the short Url
      char codeSequence[7]; 
      strcpy(codeSequence, urlStart);
      codeSequence[6] = '\0';

      int position = decode(codeSequence);

      FILE *urlFile = fopen("urls.txt", "r");
      if (urlFile == NULL) {
          // Handle file open error
          perror("Error opening URL file");
          send404Response(fd);
          return;
      }

      // Move to the specified position in the file
      fseek(urlFile, position, SEEK_SET);

      char originalUrl[256];
      if (fgets(originalUrl, sizeof(originalUrl), urlFile) == NULL) {
          // Handle read error
          perror("Error reading URL from file");
          fclose(urlFile);
          send404Response(fd);
          return;
      }

      fclose(urlFile);

      char response[512]; 

      // Construct the response
      int f301 = open("301Response.txt", O_RDONLY);
      int readSize = read(f301, buffer, 1023); /**/
      buffer[readSize] = '\0';
      close(f301);
      char *responseUrlStart = strstr(buffer, "Location: ");
      // Move the pointer to where i am going to add the original url
      responseUrlStart += strlen("Location: ");

      memcpy(responseUrlStart, originalUrl, strlen(originalUrl));

      // Send the response to the client
      write(fd, response, strlen(response));
      
      //its a convention to always end the response sent to the client with "\r\n\r\n"
      write(fd, "\r\n\r\n", strlen("\r\n\r\n") );
      close(fd);
  } 


}

void* workerThread(void *arg) {
  queue* q = (queue*) arg;
  while(1) {
    int fd = dequeue(q);
    serveRequest(fd);
  }
  return NULL;
}

int main() {
  // Set up the queue
  queue* q = queueCreate();

  // Set up the worker threads
  pthread_t w1,w2;
  pthread_create(&w1,NULL,workerThread,q);
  pthread_create(&w2,NULL,workerThread,q);

  // Create the socket
  int server_socket = socket(AF_INET , SOCK_STREAM , 0);
  if (server_socket == -1) {
    printf("Could not create socket.\n");
    return 1;
  }

  // Set the 'reuse address' socket option
  int on = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  //Prepare the sockaddr_in structure
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( 8888 );

  // Bind to the port we want to use
  if(bind(server_socket,(struct sockaddr *)&server , sizeof(server)) < 0) {
    printf("Bind failed\n");
    return 1;
  }
  printf("Bind done\n");

  // Mark the socket as a passive socket
  listen(server_socket , 3);

  // Accept incoming connections
  printf("Waiting for incoming connections...\n");
  while(1) {
    struct sockaddr_in client;
    int new_socket , c = sizeof(struct sockaddr_in);
    new_socket = accept(server_socket, (struct sockaddr *) &client, (socklen_t*)&c);
    if(new_socket != -1) {
      enqueue(q,new_socket);
    }
  }

  return 0;
}
