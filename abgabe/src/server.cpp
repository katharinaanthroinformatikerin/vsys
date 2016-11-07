//
// Created by Katharina Schallerl, if15b054
// on 30.09.16.
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
    char *downloaddir;
    int port = 0;

    if (argc < 3) {
        printf("Usage: %s Port Downloaddirectory\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    downloaddir = argv[2];
    printf("Downloaddir set to %s\n", downloaddir);
    port = atoi(argv[1]);

    create_socket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) != 0) {
        perror("Bind error.");
        return EXIT_FAILURE;
    }

    listen(create_socket, 5);

    addrlen = sizeof(struct sockaddr_in);

    //while(pid != 0); wenn es kindprozess ist, is es null, er soll nicht in diese schleife

    while (1) {
        printf("Waiting for connections on port %d...\n", port);
        new_socket = accept(create_socket, (struct sockaddr *) &cliaddress, &addrlen);
        if (new_socket > 0) {
            printf("Client connected from %s:%d...\n", inet_ntoa(cliaddress.sin_addr), ntohs(cliaddress.sin_port));
            strcpy(buffer, "Welcome to myserver. ");
            send(new_socket, buffer, strlen(buffer), 0);
            pid_t pid = fork();
            if(pid == -1) {//elternprozess
                printf("error forking, closing new_socket!\n");
                close(new_socket);
                exit(EXIT_FAILURE);
            } else if(pid == 0) {//childprocess == 0, just give a debug message
                close(create_socket);
                printf("childprocess, handle connections\n");
            } else {
                printf("parentprocess continued\n"); //parentprocess pid!=0 close new_socket, it is owned by childprocess
                close(new_socket);
                continue;//don't do any of the following code, jump to start of while(1)
            }
        }

        int loginOk = 0;
        int loginTries = 0;
        do {

            //receives login-information
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
                                        loginOk = 0;
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
                        //ev. ldap_unbind(ld) statt weiter oben.
                    }

                } else {
                    loginOk = -1;
                    printf("LOGIN incorrect (wrong command, no username and/or pw, not enough whitespaces).\n");
                }
            } else {
                loginOk = -1;
                printf("No input.\n");
            }
            printf("loginOk: %d\n", loginOk);
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
                        pDir = opendir(downloaddir);

                        if (pDir == NULL) {
                            printf("pDir is NULL.\n");
                        }

                        int i = 0;

                        while ((pDirent = readdir(pDir)) != NULL) {
                            if(strcmp(pDirent->d_name, ".")!=0 && strcmp(pDirent->d_name, "..")!=0) {
                                printf("[%s]\n", pDirent->d_name);
                                struct stat fileInfo; //struct to hold fileinfo
                                char filePath[BUF] = {0}; //full path to file
                                strcat(filePath, downloaddir);
                                strcat(filePath, pDirent->d_name);
                                stat(filePath, &fileInfo); //read fileinfo/filestats into fileInfo struct
                                printf("filesize %d\n", fileInfo.st_size);
                                char fileNameAndSize[256] = {0}; //holds filename + filesize
                                strcat(fileNameAndSize, pDirent->d_name);
                                strcat(fileNameAndSize, " size: ");
                                char fileSizeString[30];
                                sprintf(fileSizeString, "%d", fileInfo.st_size); //convert fileSize into string
                                strcat(fileNameAndSize, fileSizeString);
                                strcpy(dirArray[i], fileNameAndSize); //put full fileinfo (name and size) into array
                                i++;
                            }
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

                        strcat(filedirectory, downloaddir);
                        strcat(filedirectory, buffer + 4);
                        printf("filedirectory für get: %s\n", filedirectory);
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
                        strcat(filename, downloaddir);
                        strcat(filename, buffer + 4);


                        strcpy(buffer, "ACK: PUT wurde gestartet\n");
                        send(new_socket, buffer, BUF - 1, 0);
                        printf("ACK: PUT wurde gestartet %s.\n", filename);


                        if (strlen(filename) > 0) {

                            FILE *file = fopen(filename, "wb");
                            if (file == NULL) {
                                printf("Error opening file\n");
                            }

                            strcpy(buffer, "Die Datei wurde am Server geöffnet.\n");
                            send(new_socket, buffer, BUF - 1, 0);

                            //Erhalt der filesize
                            size = recv(new_socket, buffer, BUF - 1, 0);
                            buffer[size] = '\0';
                            printf("receivt filesize vom client: %s\n", buffer);
                            //Wandelt Größe als String in Größe als int um
                            int filesize = atoi(buffer);

                            strcpy(buffer, "Filesize erhalten, Datentransfer kann beginnen.\n");
                            send(new_socket, buffer, BUF - 1, 0);


                            if (filesize > 0) {

                                int bytesGelesen = 0;

                                while (bytesGelesen < filesize) {
                                    printf("in receiveschleife.\n");

                                    // receiving the actual file contents
                                    size = recv(new_socket, buffer, BUF - 1, 0);
                                    bytesGelesen += size;
                                    printf("Bytes received: %d\n", size);
                                    if (size > 0) {

                                        int bytesWrite = fwrite(buffer, sizeof(char), size, file);

                                        if (bytesWrite > 0) {
                                            printf("%d bytes geschrieben\n", bytesWrite);
                                        } else {
                                            printf("Konnte nicht schreiben.\n");
                                        }
                                    } else {
                                        printf("error receiving\n");
                                        break;
                                    }
                                }

                                printf("put done.\n");
                            } else {
                                printf("Error receiving size\n");
                            }
                            fclose(file);
                            printf("File closed.\n");

                        } else {
                            printf("Error receiving name.\n");
                        }

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
            close(create_socket);
            close(new_socket);
            break;
        }

    } //end of while(1)

    return EXIT_SUCCESS;
}
