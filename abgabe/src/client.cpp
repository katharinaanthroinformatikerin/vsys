//
// Created by Katharina Schallerl, if15b054
// on 30.09.16.
//

/* client.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <string>

#define BUF 1024

//Taken from open source https://gist.github.com/sinannar/2175646
//to implement missing hidden input from terminal window.
int mygetch( )
{
    struct termios oldt, newt;
    int ch;

    tcgetattr( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt );
    ch = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
    return ch;
}


int main(int argc, char **argv) {
    int csocket;
    char buffer[BUF];
    struct sockaddr_in address;
    int size;

    if (argc < 3) {
        printf("Usage: %s ServerAdresse Port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((csocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(atoi (argv[2]));//Port wird übergeben
    inet_aton(argv[1], &address.sin_addr);

    if (connect(csocket, (struct sockaddr *) &address, sizeof(address)) == 0) {
        printf("Connection with server (%s) established\n", inet_ntoa(address.sin_addr));
        size = recv(csocket, buffer, BUF - 1, 0);
        //receivt welcome to myserver

        if (size > 0) {
            buffer[size] = '\0';
            printf("%s", buffer);
        }
    } else {
        perror("Connect error - no server available");
        return EXIT_FAILURE;
    }

    int quit = 0;

    do {
        char inputUsername[BUF] = {0};
        char inputPassword[BUF] = {0};

        printf("Please log in: \n");

        printf("Username: ");
        fgets(inputUsername, BUF, stdin);
        strtok(inputUsername, "\n");

        printf("\nPassword: ");
        //fgets(inputPassword, BUF, stdin);
        //strtok(inputPassword, "\n");
        char c;
        int p=0;
        // masking input for password
        do
        {
            c = mygetch();
            if(c != '\n')
            {
                inputPassword[p]=c;
                p++;
                putchar('*');
            }
        } while(c != '\n');
        strtok(inputPassword, "\n");
        printf("\n");
        char loginString[BUF] = {0};
        strcat(loginString, "LOGIN ");
        strcat(loginString, inputUsername);
        strcat(loginString, " ");
        strcat(loginString, inputPassword);

        strcpy(buffer, loginString);

        send(csocket, buffer, strlen(buffer), 0); // Client sendet Login Infos

        //recv. LOGIN ok oder error:
        size = recv(csocket, buffer, BUF - 1, 0);
        if(size>0) {
            buffer[size] = '\0';
        } else {
            printf("error reading from server");
            return -1;
        }

    } while(strcmp(buffer, "LOGIN OK") != 0);

    printf("Login successful.\n");

    do {
        printf("Please enter your command: \n");
        fgets(buffer, BUF, stdin);
        strtok(buffer, "\n");
        send(csocket, buffer, strlen(buffer), 0); // Client sendet den Befehl

        char sendCommand[BUF];
        strcpy(sendCommand, buffer);

        if (size > 0) {
            buffer[size] = '\0';

            if (strncmp(sendCommand, "LIST", 4) == 0) {
                size = recv(csocket, buffer, BUF - 1, 0);
                buffer[size] = '\0';

                int numberOfEntries = atoi(buffer);
                printf("Number of directory entries: %d\n", numberOfEntries);
                printf("Contents:\n");
                for (int j = 0; j < numberOfEntries; j++) {
                    size = recv(csocket, buffer, BUF - 1, 0);
                    if (size > 0) {
                        printf("%s\n", buffer);
                    }
                }
                printf("Finished.\n");


            } else if (strncmp(sendCommand, "GET", 3) == 0 && strlen(sendCommand) > 4) {

                char filename[BUF] = {0};
                strcpy(filename, sendCommand + 4);

                //Receivt ACK oder ERR, je nachdem, ob Datei am Server geöffnet werden konnte oder nicht:
                size = recv(csocket, buffer, BUF - 1, 0);
                buffer[size] = '\0';

                if (strncmp(buffer, "ERR", 3) == 0) {
                    printf("%s", buffer);
                } else {
                    //Wenn Datei am server geöffnet:

                    FILE *file = fopen(filename, "wb");
                    if (file == NULL) {
                        printf("Error opening file.\n");
                        continue;
                    }

                    //Erhalt der filesize vom Server
                    size = recv(csocket, buffer, BUF - 1, 0);
                    buffer[size] = '\0';
                    int filesize = atoi(buffer);

                    //Client receivt blockweise und schreibt blockweise ins file
                    if (filesize > 0) {

                        int bytesGelesen = 0;

                        while (bytesGelesen < filesize) {
                            //printf("in receiveschleife.\n");

                            // receiving the actual file contents
                            size = recv(csocket, buffer, BUF - 1, 0);

                            bytesGelesen += size;
                            //printf("Bytes received: %d\n", size);
                            printf("Fortschrott: %d%%\r", bytesGelesen * 100 / filesize);
                            fflush(stdout);

                            if (size > 0) {
                                buffer[size] = '\0';

                                int bytesWrite = fwrite(buffer, sizeof(char), size, file);

                                if (bytesWrite > 0) {
                                    //printf("%d bytes geschrieben\n", bytesWrite);

                                } else {
                                    printf("Konnte nicht schreiben.\n");
                                }
                            } else {
                                printf("error receiving\n");
                                break;
                            }

                        }

                        printf("\nget done.\n");
                    } else {
                        printf("Error receiving size\n");
                    }
                    fclose(file);
                    printf("File closed.\n");
                }

            } else if (strncmp(sendCommand, "PUT", 3) == 0 && strlen(sendCommand) > 4) {

                char filedirectory[BUF] = {0};

                size = recv(csocket, buffer, BUF - 1, 0);
                buffer[size] = '\0';
                printf("ACK vom Server, dass PUT gestartet: %s", buffer);//receiven des ACKs vom Server

                strcpy(filedirectory, sendCommand+4);


                //receive the server's ready
                size = recv(csocket, buffer, BUF - 1, 0);
                printf("Receivt datei am server geöffnet: %s\n", buffer);


                printf("Ausgabe dateipfad vor öffnen: %s\n", filedirectory);
                FILE *file = fopen(filedirectory, "rb");
                if (file == NULL) {
                    printf("Error opening file.\n");
                } else {
                    printf("file opened.\n");

                    //fseek(file, 0L, SEEK_END);
                    struct stat finfo;
                    stat(filedirectory, &finfo);
                    int fileSize = finfo.st_size;

                    printf("filesize %d\n", fileSize);

                    sprintf(buffer, "%d", fileSize);
                    //Dateigröße senden
                    printf("Dateigröße im buffer vor dem senden zum Server: %s\n", buffer);
                    send(csocket, buffer, strlen(buffer), 0);

                    //receiven Dateigroesse erhalten
                    size = recv(csocket, buffer, BUF - 1, 0);
                    printf("receiven des ok vom server, dass er Dateigrösse erhalten hat: %s\n", buffer);

                    //rewind(file);

                    while (1) {
                        size_t bytesRead = fread(buffer, sizeof(char), BUF - 1, file);

                        if (bytesRead > 0) {

                            printf("%d bytes gelesen\n", bytesRead);
                            int bytesGesendet = 0;

                            while (bytesGesendet < bytesRead) {

                                bytesGesendet += send(csocket, buffer, bytesRead, 0);

                                printf("bytesGesendet... %d\n", bytesGesendet);
                            }


                        } else {
                            printf("Konnte nicht mehr lesen.\n");
                            break;
                        }

                    }
                    printf("Schließe file. \n");
                    memset(&buffer, 0, sizeof(buffer));
                    fclose(file);
                }

                /*
                char filedirectory[BUF] = {0};

                strcpy(filedirectory, sendCommand + 4);

                printf("filedirectory: %s\n", filedirectory);

                FILE *file = fopen(filedirectory, "rb");
                if (file == NULL) {
                    printf("Error opening file.\n");
                    strcpy(buffer, "ERR Error opening file.");
                    send(csocket, buffer, strlen(buffer), 0);

                } else {
                    // file konnte geoeffnet werden, also senden
                    strcpy(buffer, "ACK File opened.");
                    send(csocket, buffer, strlen(buffer), 0);

                    struct stat finfo;
                    stat(filedirectory, &finfo);
                    int fileSize = finfo.st_size;

                    printf("File opened, sending %d bytes.\n", fileSize);

                    // Dateigröße senden
                    sprintf(buffer, "%d", fileSize);
                    send(csocket, buffer, strlen(buffer), 0);

                    //Client receives ACK from Server
                    size = recv(csocket, buffer, BUF - 1, 0);
                    buffer[size] = '\0';

                    bool sendError = false;
                    //Server liest blockweise
                    while (!sendError) {
                        size_t bytesRead = fread(buffer, sizeof(char), BUF - 1, file);
                        buffer[bytesRead] = '\0';

                        if (bytesRead > 0) {

                            //printf("%d bytes gelesen\n", bytesRead);
                            int bytesGesendet = 0;

                            while (bytesGesendet < bytesRead) {

                                bytesGesendet += send(csocket, buffer, bytesRead, 0);
                                //printf("bytesGesendet... %d\n", bytesGesendet);

                                if (bytesGesendet == -1) {
                                    printf("Couldn't send...\n");
                                    sendError = true;
                                    break;
                                }

                                //usleep(1 * 1000); // 50 ms
                            }

                        } else {
                            printf("Finished reading.\n");
                            break;
                        }
                    }
                    fclose(file);
                    printf("File closed.\n");
                }
                */
                /* anfang put alt
                char filedirectory[BUF] = {0};

                strcpy(filedirectory, sendCommand + 4);

                printf("filedirectory: %s\n", filedirectory);

                FILE *file = fopen(filedirectory, "rb");
                if (file == NULL) {
                    printf("Error opening file.\n");
                    strcpy(buffer, "ERR Error opening file.");
                    send(csocket, buffer, strlen(buffer), 0);

                } else {
                    // file konnte geoeffnet werden, also senden
                    strcpy(buffer, "ACK File opened.");
                    send(csocket, buffer, strlen(buffer), 0);

                    struct stat finfo;
                    stat(filedirectory, &finfo);
                    int fileSize = finfo.st_size;

                    printf("File opened, sending %d bytes.\n", fileSize);

                    // Dateigröße senden
                    sprintf(buffer, "%d", fileSize);
                    send(csocket, buffer, strlen(buffer), 0);

                    while (1) {
                        size_t bytesRead = fread(buffer, sizeof(char), BUF, file);

                        if (bytesRead > 0) {
                            int bytesGesendet = 0;

                            while (bytesGesendet < bytesRead) {
                                bytesGesendet += send(csocket, buffer, bytesRead, 0);
                                printf("%d bytes sent\n", bytesGesendet);
                            }

                        } else {
                            printf("Finished reading.\n");
                            break;
                        }
                    }

                    printf("Closing file. \n");
                    memset(&buffer, 0, sizeof(buffer));
                    fclose(file);
                }
                ende put*/

            } else if (strncmp(sendCommand, "QUIT", 4) == 0) {
                quit = 1;

            } else {
                //Im Falle eines unknown commands:
                size = recv(csocket, buffer, BUF - 1, 0);
                printf("%s", buffer);
            }
        }

    } while (quit != 1);
    printf("bye!");
    close(csocket);
    return EXIT_SUCCESS;
}


