//
// Created by kathi on 30.09.16.
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
#include <include/common.h>
#include <cmath>
#include <sys/stat.h>

#include <string>

int main(int argc, char **argv) {
    int create_socket;
    char buffer[BUF];
    struct sockaddr_in address;
    int size;
    // char filename[50];
    char extractedFilename[BUF] = {0};


    if (argc < 2) {
        printf("Usage: %s ServerAdresse\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_aton(argv[1], &address.sin_addr);

    if (connect(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0) {
        printf("Connection with server (%s) established\n", inet_ntoa(address.sin_addr));
        size = recv(create_socket, buffer, BUF - 1, 0);
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

        char temp;
        int pwIndex = 0;

        printf("Please log in: \n");
        printf("Username: ");
        fgets(inputUsername, BUF, stdin);
        strtok(inputUsername, "\n");

        //system("ssty -icanon");
        printf("\nPassword: ");


        fgets(inputPassword, BUF, stdin);
        //system("ssty -icanon");

        strtok(inputPassword, "\n");
        char loginString[BUF] = {0};
        strcat(loginString, "LOGIN ");
        strcat(loginString, inputUsername);
        strcat(loginString, " ");
        strcat(loginString, inputPassword);
        printf("LoginString: %s\n", loginString);//Debugmeldung löschen!

        strcpy(buffer, loginString);


        send(create_socket, buffer, strlen(buffer), 0); // Client sendet Login Infos
        printf("Login-Daten an server gesendet.\n"); //debug meldung

        //recv. LOGIN ok oder error:
        size = recv(create_socket, buffer, BUF - 1, 0);
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
        send(create_socket, buffer, strlen(buffer), 0); // Client sendet den Befehl
        printf("Command an server gesendet.\n"); //debug meldung

        char sendCommand[BUF];
        strcpy(sendCommand, buffer);

        if (size > 0) {
            buffer[size] = '\0';
            printf("Command: %s\n", buffer);
            if (strncmp(sendCommand, "LIST", 4) == 0) {
                size = recv(create_socket, buffer, BUF - 1, 0);
                buffer[size] = '\0';

                int numberOfEntries = atoi(buffer);
                printf("Zahl der Einträge %d\n", numberOfEntries);
                printf("VZ Inhalt Start:\n");
                for (int j = 0; j < numberOfEntries; j++) {
                    size = recv(create_socket, buffer, BUF - 1, 0);
                    if (size > 0) {
                        printf("%s\n", buffer);
                    }
                }
                printf("VZ Inhalt Ende\n");


            } else if (strncmp(sendCommand, "GET", 3) == 0 && strlen(sendCommand) > 4) {

                char filename[BUF] = {0};
                strcpy(filename, sendCommand + 4);

                char * partDirectory = (char *) malloc (strlen(filename));
                strcpy(partDirectory, filename);
                char * pSlash = strrchr(partDirectory, '/');
                if(pSlash != nullptr) {
                    *pSlash = '\0';
                    char *partFilename = strdup(pSlash + 1);
                    printf("partFilname: %s\n", partFilename);
                    strcpy(filename, partFilename);
                }

                size = recv(create_socket, buffer, BUF - 1, 0);
                buffer[size] = '\0';
                printf("ACK vom Server, dass GET gestartet: %s", buffer);

                size = recv(create_socket, buffer, BUF - 1, 0);
                buffer[size] = '\0';
                printf("Receivt datei am server geöffnet: %s\n", buffer);

                FILE *file = fopen(filename, "wb");
                if (file == NULL) {
                    printf("Error opening file.\n");
                }

                //Erhalt der filesize
                size = recv(create_socket, buffer, BUF - 1, 0);
                buffer[size] = '\0';
                printf("receivt filesize vom server: %s\n", buffer);
                int filesize = atoi(buffer);

                strcpy(buffer, "Filesize erhalten, Datentransfer kann beginnen.\n");
                send(create_socket, buffer, strlen(buffer), 0);

                //Client receivt blockweise und schreibt blockweise ins file

                if (filesize > 0) {

                    int bytesGelesen = 0;
                    float progress = 0;
                    float filesizeprogress = filesize;
                    float percent = 0;

                    while (bytesGelesen < filesize) {
                        //printf("in receiveschleife.\n");

                        // receiving the actual file contents
                        size = recv(create_socket, buffer, BUF - 1, 0);

                        bytesGelesen += size;
                        //printf("Bytes received: %d\n", size);
                        // printf("\r");
                        printf("Fortschrott: %d%%\r", bytesGelesen*100/filesize);
                        fflush(stdout);

                        if (size > 0) {
                            buffer[size] = '\0';
                            //printf("bufferinhalt: %s\n", buffer);
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

            } else if (strncmp(sendCommand, "PUT", 3) == 0 && strlen(sendCommand) > 4) {
                char filedirectory[BUF] = {0};

                size = recv(create_socket, buffer, BUF - 1, 0);
                buffer[size] = '\0';
                printf("ACK vom Server, dass PUT gestartet: %s", buffer);//receiven des ACKs vom Server

                strcpy(filedirectory, sendCommand+4);

                //printf("Bitte geben Sie den Verzeichnispfad und Dateinamen der zu übertragenden Datei an:\n");
                //fgets(buffer, BUF, stdin);
                //Um das \n aus der stdin Eingabe rauszukriegen, überschreibe ich es mit \0.
                //strtok(buffer, "\n");
                //strcpy(filedirectory, buffer);

                //markiert letzten /, um filename zu extrahieren
                //wenn zum put der gesamte dateipfad zum file eingegeben wird, muss ich dateinamen extrahieren:
                /*for (int i = strlen(buffer) - 1; i >= 0; i--) {
                    if (buffer[i] == '/') {
                        int l = 0;
                        for (int k = i + 1; k < strlen(buffer); k++) {
                            extractedFilename[l] = buffer[k];
                            l++;
                        }
                        extractedFilename[l] = '\0';
                        break;
                    }
                }
                strcpy(buffer, extractedFilename);
                send(create_socket, buffer, strlen(buffer), 0);
                printf("Der extracted Filename auf Clientseite nach dem Schicken: %s\n", extractedFilename);
                */

                //printf("Geben Sie den Pfad und dateinamen ein: \n");
                //fgets(buffer, BUF, stdin);
                //strtok(buffer, "\n");
                //std::string fname(buffer);
                //sendet Pfad der zu übermittelnden Datei
                //
                //send(create_socket, buffer, strlen(buffer), 0); // 5. S - Dateipfad senden

                //printf("Ausgabe Bufferinhalt der pfad sein sollt: %s\n", buffer);
                //sendet jetzt nur, damits mitn receiven übereinstimmt.


                /*
                char directory[] = "/home/katharina/";
                printf("Ausgabe des filenamens %s", filename);
                //strlen(directory) + strlen (filename) + 1
                char filedirectory[BUF] = {0};
                strcat(filedirectory, directory);
                strcat(filedirectory, filename);
                */


                //receive the server's ready
                size = recv(create_socket, buffer, BUF - 1, 0);
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
                    send(create_socket, buffer, strlen(buffer), 0);

                    //receiven Dateigroesse erhalten
                    size = recv(create_socket, buffer, BUF - 1, 0);
                    printf("receiven des ok vom server, dass er Dateigrösse erhalten hat: %s\n", buffer);

                    //rewind(file);

                    while (1) {
                        size_t bytesRead = fread(buffer, sizeof(char), BUF - 1, file);

                        if (bytesRead > 0) {

                            printf("%d bytes gelesen\n", bytesRead);
                            int bytesGesendet = 0;

                            while (bytesGesendet < bytesRead) {

                                bytesGesendet += send(create_socket, buffer, bytesRead, 0);

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
            } else if (strncmp(sendCommand, "QUIT", 4) == 0) {
                quit = 1;
            }else {
                size = recv(create_socket, buffer, BUF - 1, 0);
                printf("Sollte unknown conmmand vom server receiven: %s", buffer);
            }
        }

    } while (quit != 1);
    printf("bye!");
    close(create_socket);
    return EXIT_SUCCESS;
}