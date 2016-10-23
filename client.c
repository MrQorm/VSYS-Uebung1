//Client fuer Socketuebung
//Tobias Nemecek/Verena Poetzl

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUF 1024
//#define PORT 6543

int main (int argc, char **argv)
{
  int create_socket;
  char buffer[BUF];
  char filename[BUF];
  char file_size[256];
  struct sockaddr_in address;
  int size, PORT;
  struct stat st;
  unsigned long int remain_data, sent_bytes;
  long offset;
  ssize_t len;

  if( argc < 3 )
  {
     printf("Usage: %s ServerAdresse Port\n", argv[0]);
     exit(EXIT_FAILURE);
  }

  errno = 0;
  PORT = strtol(argv[2], &argv[2], 10);

  if(errno != 0 || *argv[2] != '\0' || PORT > INT_MAX)
  {
       perror("Input error");
       return EXIT_FAILURE;
  }

  if ((create_socket = socket (AF_INET, SOCK_STREAM, 0)) == -1)
  {
     perror("Socket error");
     return EXIT_FAILURE;
  }

  memset(&address,0,sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons (PORT);
  inet_aton (argv[1], &address.sin_addr);

  if (connect ( create_socket, (struct sockaddr *) &address, sizeof (address)) == 0)
  {
     printf ("Connection with server (%s) established\n", inet_ntoa (address.sin_addr));
     size=recv(create_socket,buffer,BUF-1, 0);
     if (size>0)
     {
        buffer[size]= '\0';
        printf("%s",buffer);
     }
  }
  else
  {
     perror("Connect error - no server available");
     return EXIT_FAILURE;
  }

  do
  {
     memset(&buffer[0], 0 , BUF);

     printf ("Send message: ");

     fgets (buffer, BUF, stdin);
     size = send(create_socket, buffer, strlen (buffer), 0);

     if(strncmp(buffer, "list", 4) == 0)
     {
          size = recv(create_socket, buffer, BUF-1, 0);

          if(size > 0)
          {
               buffer[size] ='\0';
               printf("%s", buffer);
          }


     }
     else if(strncmp(buffer, "get ", 4) == 0)
     {
         if(size > 4)
         {
              for(int i = 0; i < size - 4; i++)
              {
                   filename[i] = buffer[i+4];
              }

              printf("%s", filename);
         }
     }
     else if(strncmp(buffer, "put ", 4) == 0)
     {
         if(size > 4)
         {
              for(int i = 4; i < size; i++)
              {
                   filename[i-4] = buffer[i];
              }
              filename[size-5] = '\0';
         }

         size = recv(create_socket, buffer, BUF-1, 0);

         int fd = open(filename, O_RDONLY);

         if(fd == -1)
         {
              printf("Error while opening file\n");
              return EXIT_FAILURE;
         }

         if(fstat(fd, &st) < 0)
         {
              printf("Error fstat\n");
              return EXIT_FAILURE;
         }

         sprintf(file_size, "%li", st.st_size);

         len = send(create_socket, file_size, sizeof(file_size), 0);
         if(len < 0)
         {
              printf("Error while sending filesize\n");
              return EXIT_FAILURE;
         }

         offset = 0;
         sent_bytes = 0;
         remain_data = st.st_size;

         while(((sent_bytes = sendfile(create_socket, fd, &offset, BUF-1)) > 0) && (remain_data > 0))
         {
              remain_data -= sent_bytes;
              printf("Sent %lu bytes of data, offset ist now %li and %lu bytes remain\n", sent_bytes, offset, remain_data);
         }

         printf("Finished sending\n");

         //size = recv(create_socket, buffer, BUF-1, 0);
     }
  }
  while (strcmp (buffer, "quit\n") != 0);
  close (create_socket);
  return EXIT_SUCCESS;
}
