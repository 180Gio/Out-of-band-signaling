#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <endian.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>

#define PORT 9000+serverID
#define MAX_NUMBER_CONNECTIONS 3
#define SIZE_MEX sizeof(int)+sizeof(char)+sizeof(long)+sizeof(char)+sizeof(long)+sizeof(char)+sizeof(char)

int serverID;

sem_t *semSupervisor;
sem_t *semServers;

fd_set sockets,readSockets;
int sharedSD=-1;
sem_t mutex,newConn;
pthread_mutex_t thread=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t threadComm=PTHREAD_MUTEX_INITIALIZER;

pthread_t threadID,commsID[MAX_NUMBER_CONNECTIONS];

void sigHandKilledBySupervisor(int sig){
    fprintf(stdout, "SERVER %d EXITING\n", serverID);
    sem_unlink("/semSupervisor");
    sem_unlink("/semServers");
    shm_unlink("/shmSupervisor");
    exit(0);
}

void *communicationsThread(void *args){
    int sdCopy;
    char *mex=(char*)malloc(SIZE_MEX);
    long int clientID;
    long int bestTime=-1;
    int bytesRead;
    long int *buffer=(long int*)malloc(sizeof(long int));
    struct timespec start;
    int shmDesc=shm_open("/shmSupervisor", O_RDWR, 0666);
    long int elapsedS;
    if(shmDesc==-1){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    char *shm=(char*)mmap(0, SIZE_MEX, PROT_READ|PROT_WRITE, MAP_SHARED, shmDesc, 0);
    if(shm==MAP_FAILED){
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    close(shmDesc);

    while(1){
        sem_wait(&mutex);
        pthread_mutex_lock(&thread);
        sdCopy=sharedSD;
        sharedSD=-1;
        pthread_mutex_unlock(&thread);
        sem_post(&newConn);

        bestTime=-1;
        elapsedS=-1;
        while((bytesRead=read(sdCopy, buffer, sizeof(long int)))>0){                         
            if(bytesRead==-1){
                perror("read");
                exit(EXIT_FAILURE);
            } else {
                clientID=be64toh(*buffer);
                clock_gettime(CLOCK_MONOTONIC, &start);
                if(elapsedS==-1) elapsedS= start.tv_sec *1000 + (int)start.tv_nsec/1000000;
                else{
                    elapsedS = start.tv_sec *1000 + (int)start.tv_nsec/1000000 - elapsedS;
                    if(elapsedS<bestTime || bestTime==-1) bestTime=elapsedS;
                    elapsedS = start.tv_sec *1000 + (int)start.tv_nsec/1000000;
                } 
                fprintf(stdout,"SERVER %d: INCOMING FROM: %lx @ %ld\n", serverID, clientID, start.tv_sec + (int)start.tv_nsec/1000000);
            }
        }
        fprintf(stdout,"SERVER %d: CLOSING %lx ESTIMATE %ld\n", serverID, clientID, bestTime);
        sprintf(mex, "%d.%lx.%ld.", serverID, clientID, bestTime);
        pthread_mutex_lock(&threadComm);
        sem_wait(semServers);
        memcpy(shm, mex, SIZE_MEX);
        sem_post(semSupervisor);
        close(sdCopy);
        memset(&start, 0, sizeof(struct timespec));
        pthread_mutex_unlock(&threadComm);
        
    }
}

void *connectionThread(void *args){
    struct sockaddr_in server;
    server.sin_family=AF_INET; //dominio del socket
    server.sin_addr.s_addr=htonl(INADDR_ANY); //da quale indirizzo accettare le connessioni
    server.sin_port=htons(PORT); //porta del socket

    int sDesc, maxSet;
    
    if((sDesc=socket(AF_INET, SOCK_STREAM, 0))==-1){
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    setsockopt(sDesc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); //altrimenti devo aspettare x tempo per riutilizzare l'indirizzo (tempo del so per "unbindare" la socket)
    setsockopt(sDesc, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)); 
    
    if(bind(sDesc, (struct sockaddr*)&server, sizeof(server))==-1){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    listen(sDesc, MAX_NUMBER_CONNECTIONS);

    FD_ZERO(&sockets);
    FD_SET(sDesc, &sockets);
    maxSet=sDesc;

    int client;
    
    while(1){
        readSockets=sockets;
        if(select(maxSet+1, &readSockets, NULL, NULL, NULL)==-1){
            perror("select");
            exit(EXIT_FAILURE);
        }
        for(int sd=0;sd<maxSet+1;sd++){
            if(FD_ISSET(sd,&readSockets)){
                if(sd==sDesc){
                    if((client=accept(sDesc, NULL, 0))==-1){
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stdout,"SERVER %d: CONNECT FROM CLIENT\n", serverID);
                    FD_SET(client, &sockets);
                    if(client>maxSet) maxSet=client;
                } else {
                    sharedSD=sd;
                    sem_post(&mutex);                    
                    sem_wait(&newConn);
                    
                    while(!FD_ISSET(maxSet,&sockets)) maxSet--;
                    FD_CLR(sd, &sockets);
                }
            }
        }
    }

}


int main(int argc, char *argv[]) {
    if(argc!=2){
        fprintf(stderr,"ERR: Wrong number of arguments. Usage: '%s [numberOfServers]'\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, sigHandKilledBySupervisor);
    
    serverID=atoi(argv[1]);
    fprintf(stdout,"SERVER %d: ACTIVE\n", serverID);
    
    sem_init(&mutex, 0, 0);
    sem_init(&newConn, 0, 0);
    
    semSupervisor=sem_open("/semSupervisor",  0, 0666);
    if(semSupervisor==SEM_FAILED){
        perror("sem_open (semSupervisor)");
        exit(EXIT_FAILURE);
    }
    semServers=sem_open("/semServers", 0, 0666);
    if(semServers==SEM_FAILED){
        perror("sem_open (semServers)");
        exit(EXIT_FAILURE);
    }

    pthread_create(&threadID, NULL, connectionThread, NULL);
    for(int i=0;i<MAX_NUMBER_CONNECTIONS;i++) pthread_create(&commsID[i], NULL, communicationsThread, NULL);
    pthread_join(threadID, NULL);
    
    return 0;
}
