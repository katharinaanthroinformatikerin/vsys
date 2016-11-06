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
#include <include/common.h>
#include <include/config.h>
#include <sys/stat.h>
#include "../include/get.h"
#include "../include/list.h"
#include "../include/put.h"
#include <cmath>
#include <ldap.h>

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
    char filename[BUF] = {0};
    char filenameGet[BUF] = {0};

    create_socket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) != 0) {
        perror("bind error");
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
                printf("Login-data received: %s\n", buffer);//Debug-Meldung

                int nrOfWhitespaces = 0;
                int index = 0;

                while (buffer[index]!='\0'){
                    if (buffer[index] == ' '){
                        nrOfWhitespaces++;
                    }
                    index++;

                }
                printf("Number of Leerzeichn: %d\n", nrOfWhitespaces);

                if (strncmp(buffer, "LOGIN", 4) == 0 && strlen(buffer) > 4 && nrOfWhitespaces >= 2) {
                    printf("Loginprozess startet.\n");


                    char usernamePw[BUF] = {0};
                    char username[BUF] = {0};
                    char password[BUF] = {0};

                    strcpy(usernamePw, buffer + 6);
                    char * parts = strtok(usernamePw, " ");
                    strcpy(username, parts);
                    printf("username: %s\n", username);

                    parts = strtok(NULL, " ");
                    strcpy(password, parts);
                    printf("password: %s\n", password);



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
                    printf("LOGIN Befehl nicht korrekt oder keine username&PW-Eingabe oder zu wenig Leerzeichen.\n");
                }
            } else {
                loginOk = -1;
                printf("Keine Eingabe.\n");
            }
            printf("Login ok: %d.\n", loginOk);
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
                        printf("verzeichnisauslese gewünscht.\n");

                        // Das Downloadverzeichnis des aktuellen Users wird automatisch gesucht und festgelegt
                        //auto con = new config();

                        //pDir = opendir(con->getDownloadDir().c_str());
                        //Punkt steht für aktuelles VZ
                        pDir = opendir(".");

                        if (pDir == NULL) {
                            printf("pDir ist NULL\n");
                        }

                        int i = 0;

                        while ((pDirent = readdir(pDir)) != NULL) {
                            printf("[%s]\n", pDirent->d_name);
                            strcpy(dirArray[i], pDirent->d_name);
                            i++;
                        }
                        //strcpy(buffer, dirArray);
                        printf("Anzahl der VZ Einträge: %d\n", i);
                        char iSend[10];
                        snprintf(iSend, 10, "%d", i);
                        strcpy(buffer, iSend);
                        send(new_socket, buffer, BUF - 1, 0); // 3. S a

                        printf("Übertrage VZ-INhalte\n");

                        for (int j = 0; j < i; j++) {
                            strcpy(buffer, dirArray[j]);
                            send(new_socket, buffer, BUF - 1, 0);
                        }
                        printf("Übertragung der VZ INhalte beendet. Schließe dir \n");
                        closedir(pDir);
                        printf("LIST abgeschlossen\n");


                        // printf("Inhalt von listFile: %s", pDirent->d_name);
                        //strcpy(buffer, pDirent->d_name);

                    } else if (strncmp(buffer, "GET", 3) == 0 && strlen(buffer) > 4) {

                        char filedirectory[BUF] = {0};

                        strcpy(filedirectory, buffer + 4);
                        printf("filedirectory: %s\n", filedirectory);

                        //strcpy(buffer, "ACK: GET wurde gestartet\n");
                        //send(new_socket, buffer, BUF - 1, 0);
                        printf("ACK: %s wurde gestartet\n", buffer);

                        char *partDirectory = (char *) malloc(strlen(filedirectory));
                        strcpy(partDirectory, filedirectory);
                        char *pSlash = strrchr(partDirectory, '/');
                        if (pSlash != nullptr) {
                            *pSlash = '\0';
                            char *partFilename = strdup(pSlash + 1);
                            printf("partFilname: %s\n", partFilename);
                            strcpy(filedirectory, partFilename);
                        }


                        FILE *file = fopen(filedirectory, "rb");
                        if (file == NULL) {
                            printf("Error opening file\n");
                            strcpy(buffer,"ERR Error opening file.\n" );
                            send(new_socket, buffer, BUF - 1, 0);

                        } else {

                            strcpy(buffer, "ACK Die Datei wurde am Server geöffnet.\n");
                            send(new_socket, buffer, BUF - 1, 0);

                            struct stat finfo;
                            stat(filedirectory, &finfo);
                            int fileSize = finfo.st_size;

                            printf("filesize %d\n", fileSize);

                            sprintf(buffer, "%d", fileSize);
                            //Dateigröße senden
                            printf("Dateigröße im buffer vor dem senden zum Server: %s\n", buffer);
                            send(new_socket, buffer, BUF - 1, 0); // 3. S a

                            //receiven Dateigroesse erhalten
                            size = recv(new_socket, buffer, BUF - 1, 0);
                            strtok(buffer, "\n");
                            buffer[size] = '\0';
                            printf("receiven des ok vom client, dass er Dateigrösse erhalten hat: %s\n", buffer);

                            bool sendError = false;
                            //Server liest blockweise
                            while (!sendError) {
                                size_t bytesRead = fread(buffer, sizeof(char), BUF - 1, file);
                                buffer[bytesRead] = '\0';

                                if (bytesRead > 0) {

                                    printf("%d bytes gelesen\n", bytesRead);
                                    int bytesGesendet = 0;

                                    while (bytesGesendet < bytesRead) {

                                        bytesGesendet += send(new_socket, buffer, bytesRead, 0);
                                        printf("bytesGesendet... %d\n", bytesGesendet);

                                        if (bytesGesendet == -1) {
                                            printf("konnte nicht senden...\n");
                                            sendError = true;
                                            break;
                                        }

                                        // zum testen
                                        usleep(1 * 1000); // 50 ms
                                    }

                                } else {
                                    printf("Konnte nicht mehr lesen.\n");
                                    break;
                                }
                            }

                            fclose(file);
                            printf("File closed.\n");
                        }


                    } else if (strncmp(buffer, "PUT", 3) == 0 && strlen(buffer) > 4) {
                        //bspw PUT clientfile.txt
                        char sendCommand[BUF];

                        char filename[BUF] = {0};
                        strcpy(filename, buffer + 4);


                        strcpy(buffer, "ACK: PUT wurde gestartet\n");
                        send(new_socket, buffer, BUF - 1, 0);
                        printf("ACK: PUT wurde gestartet %s.\n", filename);
                        /*if (strlen(sendCommand) > 4) {
                            for (int i = 0; i < strlen(sendCommand)-4; i++) {
                                filename[i] = sendCommand[i+4];
                            }
                            filename[strlen(sendCommand) - 5] = '\0';
                        }*/
                        /*
                        // Das Downloadverzeichnis des aktuellen Users wird automatisch gesucht und festgelegt
                        auto con = new config();

                        //size = recv(new_socket, buffer, BUF - 1, 0);//receivt filenamen
                        //buffer[size]='\0';
                        strcpy(filename, sendCommand);


                        printf("Ausgabe des filenamens %s\n", filename);
                        //strlen(directory) + strlen (filename) + 1
                        char filedirectory[BUF] = {0};
                        strcat(filedirectory, con->getDownloadDir().c_str());
                        strcat(filedirectory, "/");
                        strcat(filedirectory, filename);
                        */

                        printf("nach Zusammensetzen dir und filename steht im filedirectory: %s\n",
                               filename);//Debugging

                        if (strlen(filename) > 0) {
                            //buffer[size] = '\0';
                            //strtok(buffer, "\n");
                            //printf("im buffer steht name: %s\n", buffer);
                            //std::string oName(buffer);
                            //std::string fname = oName + "_new";

                            FILE *file = fopen(filename, "wb");
                            if (file == NULL) {
                                printf("Error opening file\n");
                            }

                            strcpy(buffer, "Die Datei wurde am Server geöffnet.\n");
                            send(new_socket, buffer, BUF - 1, 0);


                            // receiving the file size in bytes
                            //size = recv(new_socket, buffer, BUF - 1, 0); // 8.R
                            //printf("receivt filesize in bytes %d\n", buffer);

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
                                        //buffer[size] = '\0';
                                        //printf("bufferinhalt: %s\n", buffer);
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
                        strcpy(buffer, "unknown command\n");
                        send(new_socket, buffer, BUF - 1, 0);
                    }
                } else if (size == 0) {
                    printf("Client closed remote socket\n");
                    break;
                } else {
                    perror("recv error\n");
                    return EXIT_FAILURE;
                }
            } while (strncmp(buffer, "QUIT", 4) != 0);
            close(new_socket);
        } else {
            printf("zu viele falsche Logins, Befehlsannahme verweigert.\n");
        }
    }
    close(create_socket);
    return EXIT_SUCCESS;
}