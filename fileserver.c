//Server fuer Socketuebung
//Tobias Nemecek/Verena Poetzl

#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>

#define BUF 1024
//#define PORT 6543

//Notwendig um der Thread-Funktion mehrere Argumente zu übergeben
struct threadArguments {
    int new_socket;
    char directoryPath[256];
};

void listServerFiles(char directoryPath[256], int new_socket)
{
    char buffer[BUF];
    unsigned long int filesize;
    char newline[2] = "\n";
    char temp[256];
    char filepath[256];

     //open server directory
        DIR *dp;
        struct dirent *ep;
        dp = opendir (directoryPath);

        if (dp != NULL)
        {
            int filecounter = 0;
            strcpy(buffer, "\n");

            //all files in directory get copied to buffer
            while((ep = readdir(dp)) != NULL)
            {
                 struct stat st;

                 strcpy(filepath, directoryPath);
                 strcat(filepath, "/");
                 strcat(filepath, ep->d_name);

                 if (stat(filepath, &st) == 0);
                 {
                      if(!(!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, "..")))
                      {
                           filecounter++;
                           strcat(buffer, ep->d_name);
                           strcat(buffer, newline);

                           filesize = st.st_size;

                           sprintf(temp, "%lu", filesize);

                           strcat(buffer, "Size in Bytes: ");
                           strcat(buffer, temp);
                           strcat(buffer, newline);
                      }
                 }
            }

            //filepath and number of files in it get copied to buffer
            dp = opendir(directoryPath);

            sprintf(temp, "%d", filecounter);

            strcat(buffer, newline);
            strcat(buffer, "Content of ");
            strcat(buffer, directoryPath);
            strcat(buffer, ": ");
            strcat(buffer, temp);
            strcat(buffer, " files");
            strcat(buffer, newline);
            strcat(buffer, newline);

            //buffer is sent to client
            send(new_socket, buffer, strlen(buffer), 0);

            (void) closedir (dp);
        }
        else
        {
            perror ("Couldn't open the directory\n");
        }
}

int getFileFromServer(char directoryPath[256], int new_socket, int size, char buffer[BUF])
{
  char filename[BUF-4];
  struct stat st;
  long offset;

  unsigned long int remain_data, sent_bytes;
  unsigned long int len;

  char filepath[256];
  char file_size[256];

  //get the name of the file to be transferred
  if(size > 4)
  {
       for(int i = 4; i < size; i++)
       {
            filename[i-4] = buffer[i];
       }
       filename[size-5] = '\0';
  }

  //open server directory
  strcpy(filepath, directoryPath);
  strcat(filepath, "/");
  strcat(filepath, filename);

  int fd = open(filepath, O_RDONLY);

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

  //size of the file sent to client
 sprintf(file_size, "%li", st.st_size);

  len = send(new_socket, file_size, sizeof(file_size), 0);
  if(len < 0)
  {
       printf("Error while sending filesize\n");
       return EXIT_FAILURE;
  }

  //client sends a ready
  recv(new_socket, buffer, BUF-1, 0);
  if(size > 0)
  {
       buffer[size] ='\0';
       printf("%s\n", buffer);
  }

  offset = 0;
  sent_bytes = 0;
  remain_data = st.st_size;

  //send file to client
  while(((sent_bytes = sendfile(new_socket, fd, &offset, BUF-1)) > 0) && (remain_data > 0))
  {
       remain_data -= sent_bytes;
       printf("Sent %lu bytes of data, offset ist now %li and %lu bytes remain\n", sent_bytes, offset, remain_data);
  }

  printf("Finished sending\n\n");

  close (fd);
  return EXIT_SUCCESS;
}

int putFileToServer(char directoryPath[256], int new_socket, int size, char buffer[BUF])
{
  char filename[BUF-4];
  unsigned long int filesize, remain_data;
  FILE *received_file;
  unsigned long int len;
  char filepath[256];

  //get the name of the file to be transferred
   if(size > 4)
   {
        for(int i = 0; i < size - 4; i++)
        {
             filename[i] = buffer[i+4];
        }
        filename[size-5] = '\0';
   }

   //send ready to client
   send(new_socket, "ready\n", sizeof("ready\n"), 0);

   //receive file size from client
   size = recv(new_socket, buffer, BUF-1, 0);
   buffer[size] = '\0';
   filesize = atoi(buffer);
   printf("%lu bytes\n", filesize);

   //print filepath where file gets stored
   strcpy(filepath, directoryPath);
   strcat(filepath, "/");
   strcat(filepath, filename);

   printf("%s\n", filepath);

   received_file = fopen(filepath, "w");
   if(received_file == NULL)
   {
        printf("Error while creating the file\n");
        return EXIT_FAILURE;
   }
   remain_data = filesize;

   //receive file and print stats
   while(remain_data > 0)
   {
        if((len = recv(new_socket, buffer, BUF-1, 0)) > 0)
        {
             printf("\n%lu bytes received\n", len);
             buffer[len] = '\0';
             fwrite(buffer, sizeof(char), len, received_file);
             remain_data -= len;
             printf("Wrote %lu bytes, %lu bytes remain\n", len, remain_data);

             if (remain_data == 0) {
               printf("Received file\n\n");
             }
        }
   }

   fclose(received_file);
   return EXIT_SUCCESS;
}

//Wird für jeden Thread aufgerufen
void *connection_handler(void *threadArguments)
{
  char directoryPath[256];
  struct threadArguments *threadArgs = threadArguments;
  int new_socket = threadArgs->new_socket;
  strcpy(directoryPath, threadArgs->directoryPath);
  char buffer[BUF];
  int size;

  do
  {
       memset(&buffer[0], 0 , BUF);
       printf("Waiting for message\n");

     size = recv (new_socket, buffer, BUF-1, 0);

     if( size > 0)
     {
        buffer[size] = '\0';
        printf ("Message received: %s\n", buffer);
     }

    if (strncmp(buffer, "list", 4) == 0)
    {
     listServerFiles(directoryPath, new_socket);
    }

    else if(strncmp(buffer, "put ", 4) == 0)
    {
      putFileToServer(directoryPath, new_socket, size, buffer);
    }

    else if(strncmp(buffer, "get ", 4) == 0)
    {
      getFileFromServer(directoryPath, new_socket, size, buffer);
    }

    else if(strncmp(buffer, "quit", size-1) == 0)
    {
         printf("User terminated connection\n");
    }

    else if (size == 0)
    {
    printf("Client closed remote socket\n");
    break;
    }

    else
    {
    printf("Invalid argument. Please try again.\n");
    }

  } while (strncmp (buffer, "quit", 4)  != 0);

  pthread_exit(NULL);
}



int main (int argc, char **argv)
{
  int create_socket, new_socket;
  socklen_t addrlen;
  char buffer[BUF];
  int PORT;
  struct sockaddr_in address, cliaddress;


  if (argc < 3)
  {
       printf("Usage: %s Port Downloadverzeichnis\n", argv[0]);
       exit(EXIT_FAILURE);
  }

  errno = 0;
  PORT = strtol(argv[1], &argv[1], 10);

  if(errno != 0 || *argv[1] != '\0' || PORT > INT_MAX)
  {
       perror("Input error");
       return EXIT_FAILURE;
  }

  create_socket = socket (AF_INET, SOCK_STREAM, 0);

  memset(&address,0,sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons (PORT);

  if (bind ( create_socket, (struct sockaddr *) &address, sizeof (address)) != 0)
  {
     perror("bind error");
     return EXIT_FAILURE;
  }
  listen (create_socket, 5);

  addrlen = sizeof (struct sockaddr_in);


  while (1)
  {
     printf("Waiting for connections...\n");

     while((new_socket = accept ( create_socket, (struct sockaddr *) &cliaddress, &addrlen )))
     {
        printf ("Client connected from %s:%d...\n", inet_ntoa (cliaddress.sin_addr),ntohs(cliaddress.sin_port));
        strcpy(buffer,"Welcome to myserver, Please enter your command:\n");
        send(new_socket, buffer, strlen(buffer),0);

        struct threadArguments threadArgs;
          threadArgs.new_socket = new_socket;
          strcpy(threadArgs.directoryPath, argv[2]);

//NOTE: Speicher für Struct allozieren!
        pthread_t sniffer_thread;
        //new_sock = malloc(1);
        //*new_sock = new_socket;
        //threadArguments = malloc(1);
        //*threadArguments = threadArguments;

//NOTE: Ist es richtig, threadArgs mit & zu übergeben?
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) &threadArgs) < 0)
        {
            perror("could not create thread");
            return 1;
        }

//NOTE: Was macht join thread?
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
      }


     close (new_socket);
  }


  close (create_socket);
  return EXIT_SUCCESS;
}
