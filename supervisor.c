#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/time.h>
#include <semaphore.h>
#include <string.h>
 #include <sys/time.h>

#define SHM_SIZE sizeof(int)+sizeof(char)+sizeof(long)+sizeof(char)+sizeof(long)+sizeof(char)+sizeof(char)+1

struct clientEst{
    char clientID[64];
    char bestStime[64];
    struct clientEst *next;
    int numberOfServers;
};
struct clientEst *cEst=NULL;
struct timeval te;
int signalCount=0;
long int sigT;
int killing=0;
int serverNum;
int *pidServer;

void createTable(FILE *fp){
    struct clientEst *tmp;
    tmp=cEst;
    fprintf(fp,"----------------------------------------------------------------------\n");
    fprintf(fp,"| %20s | %20s | %20s |\n","bestEstimate","clientID","numberOfServers");
    while(tmp!=NULL){
        fprintf(fp,"----------------------------------------------------------------------\n");
        fprintf(fp,"| %20s | %20s | %20d |\n", tmp->bestStime, tmp->clientID, tmp->numberOfServers);
        tmp=tmp->next;
    }
    fprintf(fp,"----------------------------------------------------------------------\n");
}

void rmList(struct clientEst **h){
    if((*h)==NULL);
    else{
        rmList(&(**h).next);
        free(*h);
    }
}

int initializeServerNum(int);
void sigHandler(int signum);

void addEl(struct clientEst **h, struct clientEst *nEl){
    if(strcmp(nEl->bestStime,"0")<=0) return;
    if((*h)==NULL){
        (*h)=nEl;
        (*h)->next=NULL;
        (*h)->numberOfServers=1;   
    } else if(strcmp((*h)->clientID, nEl->clientID)==0){
        (*h)->numberOfServers++;
        if(strlen(nEl->bestStime)<strlen((*h)->bestStime)){
            strcpy((*h)->bestStime, nEl->bestStime);
        } else if (strlen(nEl->bestStime)==strlen((*h)->bestStime)){
            if(strcmp(nEl->bestStime, (*h)->bestStime)<0){
                strcpy((*h)->bestStime, nEl->bestStime);
            }
        }
    } else addEl(&((*h)->next), nEl);
}

int main(int argc, char **argv){
    if(argc!=2){
        fprintf(stderr,"ERR: Too few arguments. Usage: '%s [serverNumber]'\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int serverNumber=atoi(argv[1]);
    fprintf(stdout,"SUPERVISOR: STARTING %d SERVER\n", serverNumber);
    initializeServerNum(serverNumber);
    signal(SIGINT, sigHandler);

    pidServer=(int*)malloc(sizeof(int)*serverNum);

    int shmSupervisor;
    if((shmSupervisor=shm_open("/shmSupervisor", O_CREAT | O_RDWR, 0666))<0){
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if(ftruncate(shmSupervisor, SHM_SIZE) == -1){
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    char *shmData=(char*)mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmSupervisor, 0);
    if(!shmData){
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    sem_t *semSupervisor=sem_open("/semSupervisor", O_CREAT | O_RDWR, 0666, 0);
    sem_t *semServers=sem_open("/semServers", O_CREAT | O_RDWR, 0666, 1);
    if(semSupervisor==SEM_FAILED){
        perror("sem_open: semSupervisor");
        exit(EXIT_FAILURE);
    }
    if(semServers==SEM_FAILED){
        perror("sem_open: semServers");
        exit(EXIT_FAILURE);
    }    

    int pidSupervisor=getpid();
    int serverId=0;
    char *token=(char*)malloc(SHM_SIZE);
    for(int i=0;i<serverNumber;i++){
        if(getpid()==pidSupervisor){
            serverId=i+1;
            if((pidServer[i]=fork())==-1){
                perror("fork");
                exit(EXIT_FAILURE);
            } 
        }       
    }
    int change;
    char serverID[64];
    if(getpid()==pidSupervisor){
        while(1){
            change=0;
            sem_wait(semSupervisor);
            strcpy(token, shmData);
            struct clientEst *c=(struct clientEst*)malloc(sizeof(struct clientEst));
            strcpy(serverID, strtok(token, "."));
            strcpy(c->clientID, strtok(NULL, "."));
            strcpy(c->bestStime, strtok(NULL, "."));
            addEl(&cEst, c);
            fprintf(stdout,"SUPERVISOR: ESTIMATE %s FOR %s FROM %s\n", c->bestStime, c->clientID, serverID);
            memset(shmData, 0, SHM_SIZE);
            memset(serverID, 0, 64);
            sem_post(semServers);
        }
    } else {
        char id[10];
        sprintf(id, "%d", serverId);
        execl("./server", "server", id, NULL);
    }
    return 0;
}

int initializeServerNum(int SN){
    serverNum=SN;
}

void sigHandler(int signum){
    if(signum==SIGINT){
        signalCount++;    
        gettimeofday(&te, NULL);
        createTable(stderr);
        if(signalCount==1){
            sigT=te.tv_sec*1000L;
        } 
        else{
            if(te.tv_sec*1000L+te.tv_usec/1000-sigT <= 1000) killing=1;
            else  sigT=te.tv_sec*1000L;
            if(killing==1){
                createTable(stdout);
                fprintf(stdout,"SUPERVISOR EXITING\n");
                for(int i=0;i<serverNum;i++){
                    kill(pidServer[i], SIGINT);
                }
                sem_unlink("/semSupervisor");
                shm_unlink("/semServers");
                rmList(&cEst);
                free(pidServer);
                exit(0);
            }
        }   
    } 
}