/* timeserv.c - a socket-based time of day server
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>

#define FDCNT 2
#define PORTNUM 13000 
#define HOSTLEN 256
#define oops(msg)	{ perror(msg); exit(1); }

void* client_handler(void *);

typedef struct client{
    int sock_fd;
    char ip[20];
}client;

client client_data[FDCNT]={0};
int client_cnt=0;
int turn_cnt=0;
int sockfd_connet[FDCNT];

int main(int ac, char *av[])
{
    struct sockaddr_in client_addr;	/* build our address here	*/
    struct sockaddr_in server_addr;
    struct hostent *hp;
    int pthread_create();
    pthread_t ptid[FDCNT]={0};

    char hostname[HOSTLEN];		/* address			*/
    int sock_id,tempfd,i;
    int client_addr_size;

    sock_id = socket( PF_INET, SOCK_STREAM, 0);
    if(sock_id == -1)
        oops("socket");
    bzero((void*)&server_addr, sizeof(server_addr));
    bzero((void*)&client_addr,sizeof(client_addr));

    gethostname(hostname, HOSTLEN);
    hp = gethostbyname(hostname);

    bcopy((void*) hp->h_addr, (void*) &server_addr.sin_addr, hp->h_length);
    server_addr.sin_port = htons(PORTNUM);
    server_addr.sin_family = AF_INET;

    if(bind(sock_id, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in)) != 0)
        oops("bind");

    if(listen(sock_id, 5) != 0)
        oops("listen");	

    client_addr_size = sizeof(client_addr);
    while(1){
        tempfd=accept(sock_id,(struct sockaddr*)&client_addr,&client_addr_size);

        if(client_cnt==FDCNT){
            printf("socket full\n");
            close(tempfd);
            continue;
        }

        if(tempfd<0){
            printf("accpet fail\n");
            close(tempfd);
            continue;
        }

        for(i=0;i<FDCNT;i++){
            if(client_data[i].sock_fd==0){
                client_data[i].sock_fd=tempfd;
                printf("Accept sucess");
                break;
            }   

        }

        strcpy(client_data[i].ip,inet_ntoa(client_addr.sin_addr));

        pthread_create(ptid+i,NULL,(void*)client_handler,client_data+i);

        printf("accpeted sockfd %d ,client count %d\n",client_data[i].sock_fd,client_cnt);
        client_cnt++;
    }
    close(sock_id);

}


void* client_handler(void* client){

    struct client* ptr = client;

    int write_message=0;
    int read_message=-1;
    int send_flag1=1;
    int send_flag2=1;
    int check=0;

    while(1){

        if(client_cnt!=FDCNT){  // 아직 참가자가 가득차지않음
            write_message=-1;
            if(send_flag1==1){
            write(ptr->sock_fd,&write_message,sizeof(write_message));
            send_flag1=0;
            }
        }
        else if(client_cnt==FDCNT){ //게임시작

            if(client_data[turn_cnt].sock_fd==ptr->sock_fd){
                write_message=1;
            }
            else
                write_message=2;

            if(send_flag2==1)
             write(ptr->sock_fd,&write_message,sizeof(write_message)); // 순서 전달

            printf("%d\n",write_message);
            printf("%d\n",write_message);

            read(ptr->sock_fd,&read_message,sizeof(read_message));

            if(read_message==0)
                break;

            if(write_message==2){ // other turn.
                write(ptr->sock_fd,&read_message,sizeof(read_message));
            }

            if(turn_cnt+1==client_cnt){
                turn_cnt=0;
            }
            else
                turn_cnt++;
        } 
    }

    close(ptr->sock_fd);
    client_cnt--;
    ptr->sock_fd=0;

    return NULL;
}
