//
// Created by johann on 30.09.16.
//

/* server.c */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <cmath>
#include <ldap.h>

#define BUF 1024
#define PORT 6543

#define DIRARRSIZE 1000
#define LDAP_HOST "ldap.technikum-wien.at"
#define LDAP_PORT 389
#define SEARCHBASE "dc=technikum-wien,dc=at"
#define SCOPE LDAP_SCOPE_SUBTREE


int main(int argc, char **argv) {
    int create_socket, new_socket;
    socklen_t addrlen;
    char buffer[BUF];
    int size;
    struct sockaddr_in address, cliaddress;
    DIR *pDir;
    struct dirent *pDirent;
    char dirArray[DIRARRSIZE][256];

    create_socket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) != 0) {
        perror("Bind error.");
        return EXIT_FAILURE;
    }

    listen(create_socket, 5);

    addrlen = sizeof(struct sockaddr_in);

    while (1) {
        printf("Waiting for connections...\n");
        new_socket = accept(create_socket, (struct sockaddr *) &cliaddress, &addrlen);
        if (new_socket > 0) {
            printf("Client connected from %s:%d...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));
            strcpy(buffer, "Welcome to myserver. ");
            send(new_socket, buffer, strlen(buffer), 0);
        }

        int loginOk = 0;
        int loginTries = 0;
        do {

            //receives login-information:
            size = recv(new_socket, buffer, BUF - 1, 0);

            if (size > 0) {
                buffer[size] = '\0';

                int nrOfWhitespaces = 0;
                int index = 0;

                while (buffer[index]!='\0'){
                    if (buffer[index] == ' '){
                        nrOfWhitespaces++;
                    }
                    index++;

                }

                if (strncmp(buffer, "LOGIN", 4) == 0 && strlen(buffer) > 4 && nrOfWhitespaces >= 2) {

                    char usernamePw[BUF] = {0};
                    char username[BUF] = {0};
                    char password[BUF] = {0};

                    strcpy(usernamePw, buffer + 6);
                    char * parts = strtok(usernamePw, " ");
                    strcpy(username, parts);

                    parts = strtok(NULL, " ");
                    strcpy(password, parts);

                    /* setup LDAP connection */
                    LDAP *ld;			/* LDAP resource handle */
                    LDAPMessage *result, *e;	/* LDAP result handle */
                    BerElement *ber;		/* array of attributes */
                    char *attribute;
                    char **vals;

                    int i,rc=0;

                    char *attribs[3];		/* attribute array for search */

                    attribs[0]=strdup("uid");		/* return uid and cn of entries */
                    attribs[1]=strdup("cn");
                    attribs[2]=NULL;		/* array must be NULL terminated */


                    if ((ld=ldap_init(LDAP_HOST, LDAP_PORT)) == NULL)
                    {
                        perror("ldap_init failed");
                        return EXIT_FAILURE;
                    }

                    printf("connected to LDAP server %s on port %d\n",LDAP_HOST,LDAP_PORT);

                    //1. Schritt
                    /* anonymous bind */
                    rc = ldap_simple_bind_s(ld,NULL,NULL);

                    if (rc != LDAP_SUCCESS)
                    {
                        fprintf(stderr,"LDAP error: %s\n",ldap_err2string(rc));
                        loginOk = -1;
                    }
                    else
                    {
                        printf("bind successful\n");

                        //2. Schritt, im LDAP Verzeichnis wird nach dem User gesucht
                        /* perform ldap search */
                        char FILTER[BUF] = {};
                        strcat(FILTER, "(uid=");
                        strcat(FILTER, username);
                        strcat(FILTER, ")");
                        printf("searching in LDAP.\n");
                        rc = ldap_search_s(ld, SEARCHBASE, SCOPE, FILTER, attribs, 0, &result);

                        if (rc != LDAP_SUCCESS)
                        {
                            fprintf(stderr,"LDAP search error: %s\n",ldap_err2string(rc));
                            loginOk = -1;
                        } else {


                            int nrEntries = ldap_count_entries(ld, result);
                            printf("Total results: %d\n", nrEntries);

                            // If a record is found, try to authenticate with given credentials
                            if (nrEntries > 0) {
                                rc = 0;
                                e = ldap_first_entry(ld, result);
                                char *dn;
                                if ((dn = ldap_get_dn(ld, e)) != NULL) {
                                    printf("dn: %s\n", dn);
                                    // Bind again with given credentials
                                    ldap_unbind(ld);
                                    printf("Attempting rebind.\n");
                                    ld = ldap_init(LDAP_HOST, LDAP_PORT);
                                    rc = ldap_simple_bind_s(ld, dn, password);
                                    if (rc != 0) {
                                        loginOk = -1;
                                        printf("LDAP Authentication failed!\n");
                                    } else {
                                        printf("User %s login ok.\n", username);

                                    }
                                    ldap_unbind(ld);
                                    ldap_memfree(dn);
                                }
                            } else {
                                printf("Username not found!\n");
                                loginOk = -1;
                            }
                        }

                        /* free memory used for result */
                        ldap_msgfree(result);
                        free(attribs[0]);
                        free(attribs[1]);
                        printf("LDAP search finished\n");
                    }


                } else {
                    loginOk = -1;
                    printf("LOGIN incorrect (wrong command, no username and/or pw, not enough whitespaces).\n");
                }
            } else {
                loginOk = -1;
                printf("No input.\n");
            }

            if (loginOk == 0){
                strcpy(buffer, "LOGIN OK");
            } else {
                strcpy(buffer, "LOGIN ERROR");
                loginTries++;
                if (loginTries == 3){
                    close(new_socket);
                    break;
                }
            }
            send(new_socket, buffer, strlen(buffer), 0);
        } while (strcmp(buffer, "LOGIN OK") != 0);

        if (loginOk == 0) {

            do {

                //receives command:
                size = recv(new_socket, buffer, BUF - 1, 0);
                if (size > 0) {
                    buffer[size] = '\0';
                    printf("Command received: %s\n", buffer);//Debug-Meldung

                    //Protokoll bei LIST: Server schickt zu allererst Anzahl der zu übertragenden Einträge.
                    if (strncmp(buffer, "LIST", 4) == 0) {

                        //pDir = opendir(con->getDownloadDir().c_str());
                        //Punkt steht für aktuelles VZ
                        pDir = opendir(".");

                        if (pDir == NULL) {
                            printf("pDir is NULL.\n");
                        }

                        int i = 0;

                        while ((pDirent = readdir(pDir)) != NULL) {
                            printf("[%s]\n", pDirent->d_name);
                            strcpy(dirArray[i], pDirent->d_name);
                            i++;
                        }

                        printf("Number of directory entries: %d\n", i);
                        char nrOfEntries[10];
                        snprintf(nrOfEntries, 10, "%d", i);
                        strcpy(buffer, nrOfEntries);
                        send(new_socket, buffer, BUF - 1, 0); // 3. S a

                        //Übertragen der VZ-Inhalte
                        for (int j = 0; j < i; j++) {
                            strcpy(buffer, dirArray[j]);
                            send(new_socket, buffer, BUF - 1, 0);
                        }
                        printf("Finished transferring dir-contents. Closing directory.\n");
                        closedir(pDir);
                        printf("LIST finished.\n");

                    } else if (strncmp(buffer, "GET", 3) == 0 && strlen(buffer) > 4) {

                        char filedirectory[BUF] = {0};

                        strcpy(filedirectory, buffer + 4);

                        /*
                        char *partDirectory = (char *) malloc(strlen(filedirectory));
                        strcpy(partDirectory, filedirectory);
                        char *pSlash = strrchr(partDirectory, '/');
                        if (pSlash != nullptr) {
                            *pSlash = '\0';
                            char *partFilename = strdup(pSlash + 1);
                            strcpy(filedirectory, partFilename);
                        }
                         */

                        FILE *file = fopen(filedirectory, "rb");
                        if (file == NULL) {
                            printf("Error opening file\n");
                            strcpy(buffer,"ERR Error opening file.\n" );
                            send(new_socket, buffer, BUF - 1, 0);

                        } else {

                            strcpy(buffer, "ACK File opened.\n");
                            send(new_socket, buffer, BUF - 1, 0);

                            struct stat finfo;
                            stat(filedirectory, &finfo);
                            int fileSize = finfo.st_size;

                            sprintf(buffer, "%d", fileSize);
                            //Dateigröße senden
                            send(new_socket, buffer, BUF - 1, 0); // 3. S a



                            bool sendError = false;
                            //Server liest blockweise
                            while (!sendError) {
                                size_t bytesRead = fread(buffer, sizeof(char), BUF - 1, file);
                                buffer[bytesRead] = '\0';

                                if (bytesRead > 0) {

                                    //printf("%d bytes gelesen\n", bytesRead);
                                    int bytesGesendet = 0;

                                    while (bytesGesendet < bytesRead) {

                                        bytesGesendet += send(new_socket, buffer, bytesRead, 0);
                                        //printf("bytesGesendet... %d\n", bytesGesendet);

                                        if (bytesGesendet == -1) {
                                            printf("Couldn't send...\n");
                                            sendError = true;
                                            break;
                                        }

                                        usleep(1 * 1000); // 50 ms
                                    }

                                } else {
                                    printf("Finished reading.\n");
                                    break;
                                }
                            }

                            fclose(file);
                            printf("File closed.\n");
                        }


                    } else if (strncmp(buffer, "PUT", 3) == 0 && strlen(buffer) > 4) {
                        char sendCommand[BUF];
                        char filename[BUF] = {0};
                        strcpy(filename, buffer + 4);

                        printf("received PUT for file \"%s\"\n", filename);

                        size = recv(new_socket, buffer, BUF - 1, 0);
                        buffer[size] = '\0';

                        if (strncmp(buffer, "ERR", 3) == 0) {
                            printf("%s", buffer);
                        } else {
                            //Wenn Datei am client geöffnet:
                            FILE *file = fopen(filename, "wb");
                            if (file == NULL) {
                                printf("Error opening file\n");
                            } else {

                                printf("File am Server geöffnet.\n");//debug

                                //Erhalt der filesize
                                size = recv(new_socket, buffer, BUF - 1, 0);
                                buffer[size] = '\0';

                                //Wandelt Größe als String in Größe als int um
                                int filesize = atoi(buffer);

                                printf("client says they be sending %d bytes.\n", filesize);

                                //Client receivt blockweise und schreibt blockweise ins file
                                if (filesize > 0) {

                                    int bytesGelesen = 0;

                                    strcpy(buffer, "ACK Server ready to receive.\n");
                                    send(new_socket, buffer, BUF - 1, 0);

                                    while (bytesGelesen < filesize) {

                                        // receiving the actual file contents
                                        size = recv(new_socket, buffer, BUF - 1, 0);

                                        bytesGelesen += size;

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

                                    printf("\nput done.\n");
                                }
                                fclose(file);
                                printf("File closed.\n");
                            }


                        }


                        /*hier beginnt put alt


                        char sendCommand[BUF];
                        char filename[BUF] = {0};
                        strcpy(filename, buffer + 4);

                        printf("received PUT for file \"%s\"\n", filename);

                        if (strlen(filename) > 0) {
                            printf("now waiting for ACK ...\n");
                            // Receivt ACK oder ERR, je nachdem, ob Datei am Client geöffnet werden konnte oder nicht:
                            size = recv(new_socket, buffer, BUF - 1, 0);
                            buffer[size] = '\0';

                            printf("%d bytes received: Status from client: \"%s\"\n", size, buffer);

                            if (strncmp(buffer, "ERR", 3) == 0) {
                                printf("An error occurred: %s\n", buffer);

                            } else {
                                printf("Didn't receive ERR so it must be ACK.\n");

                                // Wenn Datei am client geöffnet, datei zum schreiben am server oeffnen
                                FILE *file = fopen(filename, "wb");
                                if (file == NULL) {
                                    printf("Error opening file\n");
                                }

                                //Erhalt der filesize
                                size = recv(new_socket, buffer, BUF - 1, 0);
                                buffer[size] = '\0';

                                //Wandelt Größe als String in Größe als int um
                                int filesize = atoi(buffer);

                                printf("client says they be sending %d bytes.\n", filesize);

                                if (filesize > 0) {
                                    int bytesGelesen = 0;

                                    while (bytesGelesen < filesize) {
                                        printf("now waiting for data ... %d < %d\n", bytesGelesen, filesize);
                                        // receiving the actual file contents
                                        size = recv(new_socket, buffer, BUF, 0);
                                        bytesGelesen += size;
                                        printf("Bytes received: %d\n", size);

                                        if (size > 0) {

                                            int bytesWrite = fwrite(buffer, sizeof(char), size, file);

                                            if (bytesWrite > 0) {
                                                printf("%d bytes written.\n", bytesWrite);
                                            } else {
                                                printf("Couldn't write.\n");
                                            }
                                        } else {
                                            printf("Error receiving.\n");
                                            break;
                                        }
                                    }

                                    printf("Put done.\n");
                                } else {
                                    printf("Error receiving size.\n");
                                }
                                fclose(file);
                                printf("File closed.\n");
                            }

                        } else {
                            printf("Error receiving name.\n");
                        }


                    hier endet put
                    */
                    } else if (strncmp(buffer, "QUIT", 4) == 0) {
                        //damit nicht nochmal unkown command hingeschrieben wird, while-Schleife (unten) beendet das Ganze
                    } else {
                        strcpy(buffer, "Unknown command.\n");
                        send(new_socket, buffer, BUF - 1, 0);
                    }
                } else if (size == 0) {
                    printf("Client closed remote socket.\n");
                    break;
                } else {
                    perror("Recv error.\n");
                    return EXIT_FAILURE;
                }
            } while (strncmp(buffer, "QUIT", 4) != 0);
            close(new_socket);
        } else {
            printf("Too many failed logins. Bye.\n");
        }
    }
    close(create_socket);
    return EXIT_SUCCESS;
}
