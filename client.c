#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <endian.h>

#define PORT 9000+i+1
#define IP "127.0.0.1"

int main(int argc, char **argv){
    if(argc!=4){
        fprintf(stderr,"ERR: Too few arguments. Usage: '%s [serverNumber, serverToConnect, messageNumber]'\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int serverNumber=atoi(argv[1]);
    int serverToConnect=atoi(argv[2]);
    if(serverToConnect<1 || serverToConnect>serverNumber){
        fprintf(stderr,"ERR: Server to connect must be between 1 and %d\n", serverNumber);
        exit(EXIT_FAILURE);
    }
    int messageNumber=atoi(argv[3]);
    if(messageNumber<=(3*serverToConnect)){
        fprintf(stderr,"ERR: Message number must be greater than 3*k\n");
        exit(EXIT_FAILURE);
    }
    //creo il secret e l'id
    srand(time(NULL));
    int secret=rand()%3000+1;
    long int id=rand();
    fprintf(stdout,"CLIENT ID: %lx SECRET: %d\n", id, secret);
    
    //scelgo i server a cui collegarmi
    int *totalServer=calloc(serverNumber, sizeof(int));
    int randomN, selected=0;

    while(selected<serverToConnect){
        randomN=rand()%serverNumber;
        if(totalServer[randomN]==0){
            totalServer[randomN]=1;
            selected++;
        }
    }
    //connessione
    int sockDesc[serverToConnect], tmp=0;
    struct sockaddr_in server[serverToConnect];

    for(int i=0;i<serverNumber;i++){
        if(totalServer[i]==1){
            if((sockDesc[tmp]=socket(AF_INET, SOCK_STREAM, 0))<0){
                perror("socket");
                exit(EXIT_FAILURE);
            }
            server[tmp].sin_family=AF_INET;
            server[tmp].sin_port=htons(PORT);
            server[tmp].sin_addr.s_addr=inet_addr(IP);
            if(connect(sockDesc[tmp], (struct sockaddr *)&server[tmp], sizeof(server[tmp]))<0){
                perror("connect");
                exit(EXIT_FAILURE);
            }
            fprintf(stdout,"[CLIENT %lx] CONNECTED TO SERVER %d\n",id,i+1);
            tmp++;
        }
    }
    //mando il messaggio
    long int IDnbo=htobe64(id);
    struct timespec wait;
    wait.tv_sec=secret/1000;
    wait.tv_nsec=secret%1000*1000000;
    int mexSent=0;

    while(mexSent<messageNumber){
        randomN=rand()%serverToConnect;
        if(send(sockDesc[randomN], &IDnbo, sizeof(IDnbo), 0)<0){
            perror("send");
            exit(EXIT_FAILURE);
        }
        fprintf(stdout,"[CLIENT %lx] SENDING MESSAGE %lx TO SERVER %d\n",id,IDnbo,ntohs(server[randomN].sin_port)-9000);
        mexSent++;
        nanosleep(&wait, NULL);
    }
    //chiudo le socket e libero la memoria di totalServer
    for(int i=0;i<serverToConnect;i++) close(sockDesc[i]);
    fprintf(stdout,"CLIENT %lx DONE\n", id);
    free(totalServer);
    return 0;
}