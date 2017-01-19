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
#include <ctype.h>
#include <dirent.h>

#include <pthread.h>
#include <termios.h>

#define BUF 1024
//#define PORT 6543

#define gotoxy(x,y) printf("\033[%d;%dH", (x), (y))

int main (int argc, char **argv)
{
  int create_socket;
  char buffer[BUF];
  char filename[BUF];
  char file_size[256];
  char filepath[256];
  struct sockaddr_in address;
  int size, PORT;
  struct stat st;
  unsigned long int remain_data, sent_bytes, filesize;
  long offset;
  ssize_t len;
  int LogInStat = 0;

  FILE *received_file;

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
     send(create_socket, "ready for authetication", BUF-1,0);

     //Username-Eingabe
     recv(create_socket, buffer, BUF-1, 0);

     printf("%s", buffer);
     scanf("%s", buffer);

     send(create_socket, buffer, BUF-1, 0);

     //Passwort-Eingabe
     recv(create_socket, buffer, BUF-1, 0);

     printf("%s", buffer);

     struct termios oldt, newt;
     tcgetattr(STDIN_FILENO, &oldt);
     newt = oldt;

     newt.c_lflag &= ~(ECHO);
     //disable ECHO
     tcsetattr(STDIN_FILENO, TCSANOW, &newt);

     scanf("%s", buffer);

     tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

     send(create_socket, buffer, BUF-1, 0);

     recv(create_socket, buffer, BUF-1, 0);

     if(strncmp(buffer, "success", 7) == 0)
     {
          LogInStat = 1;
     }
     else if(strncmp(buffer, "lockout", 7) == 0)
     {
          LogInStat = -1;
     }
     else
     {
          LogInStat = 0;
     }
} while(LogInStat == 0);


  //printf("%s\n", buffer);

  if(LogInStat == 1)
  {
       printf("Authentification successful!\n");

       do
      {
         memset(&buffer[0], 0 , BUF);

         printf ("Send message: ");

         fgets (buffer, BUF, stdin);
         size = send(create_socket, buffer, strlen (buffer), 0);


    //LIST BEFEHL
         if(strncmp(buffer, "list", 4) == 0)
         {
              size = recv(create_socket, buffer, BUF-1, 0);

              if(size > 0)
              {
                   buffer[size] ='\0';
                   printf("%s", buffer);
              }


         }

    //GET BEFEHL
         else if(strncmp(buffer, "get ", 4) == 0)
         {
           //get the name of the file to be transferred
           if(size > 4)
           {
                for(int i = 0; i < size - 4; i++)
                {
                     filename[i] = buffer[i+4];
                }
                filename[size-5] = '\0';
           }

        //receive file size
          size = recv(create_socket, buffer, BUF-1, 0);

           buffer[size] = '\0';
           filesize = atoi(buffer);
           printf("%lu bytes\n", filesize);


    //files are going to be stored in current directory
           strcpy(filepath, "./");
           strcat(filepath, filename);

           printf("%s\n", filepath);

           received_file = fopen(filepath, "w");
           if(received_file == NULL)
           {
                printf("Error while creating the file\n");
                return EXIT_FAILURE;
           }
           remain_data = filesize;
           int received = 0;
           double progress;

    //sends a ready message to the server
           send(create_socket, "ready", sizeof("ready"), 0);

    //receives file, prints remaining data to be received
         system("clear");
         printf("[          ]");
         int progress_counter = 1;

           while(remain_data > 0)
           {
                if((len = recv(create_socket, buffer, BUF-1, 0)) > 0)
                {
                     //printf("\n%lu bytes received\n", len);
                     buffer[len] = '\0';
                     fwrite(buffer, sizeof(char), len, received_file);
                     remain_data -= len;
                     received += len;
                     //printf("Wrote %lu bytes, %lu bytes remain\n", len, remain_data);
                     progress = (double) received/filesize * 100;

                     if(received > filesize/10 * progress_counter)
                     {
                          gotoxy(1, progress_counter+2);
                          printf("\b");
                          printf("*");
                          progress_counter++;
                     }

                     gotoxy(1, 15);
                     printf(" % 3.0f%c", progress, '%');

                     if (remain_data == 0)
                     {
                          printf("\nReceived file\n\n");
                     }
                }
           }

           fclose(received_file);

         }

    //PUT BEFEHL

         else if(strncmp(buffer, "put ", 4) == 0)
         {
           //get the name of the file to be transferred
             if(size > 4)
             {
                  for(int i = 4; i < size; i++)
                  {
                       filename[i-4] = buffer[i];
                  }
                  filename[size-5] = '\0';
             }

             //receive the server's ready and print it
             size = recv(create_socket, buffer, BUF-1, 0);

             if(size > 0)
             {
                  buffer[size] ='\0';
                  printf("%s", buffer);
             }

    //open the file which is to be sent to the server
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

    //size of the file is sent to server
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

    //sending file to server
             while(((sent_bytes = sendfile(create_socket, fd, &offset, BUF-1)) > 0) && (remain_data > 0))
             {
                  remain_data -= sent_bytes;
                  printf("Sent %lu bytes of data, offset ist now %li and %lu bytes remain\n", sent_bytes, offset, remain_data);
             }

             printf("Finished sending\n\n");

             close (fd);
         }
      }
      while (strcmp (buffer, "quit\n") != 0);
    //if the user enters the quit command, the loop is ended and the socket gets closed

 }
 else if(LogInStat == -1)
 {
      printf("Locked out from Server!\n");
}


  close (create_socket);
  return EXIT_SUCCESS;
}
