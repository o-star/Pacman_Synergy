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
    int sock_fd[10];
    char ip[20];
	int user_cnt;
}client;

client room_data[FDCNT]={0};

int client_cnt=0;
int turn_cnt=0;
int room_cnt=0;
int sockfd_connet[FDCNT];

int main(int ac, char *av[])
{
    struct sockaddr_in client_addr;	/* build our address here	*/
    struct sockaddr_in server_addr;
    struct hostent *hp;
    pthread_t ptid[FDCNT]={0}; //FDCNT는 방 갯수

    char hostname[HOSTLEN];		/* address			*/
    int sock_id,tempfd,i;
    int client_addr_size;
	int write_message;
	int read_message;
	int limit_user_cnt;

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

    if(listen(sock_id, 10) != 0)
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

		if(client_cnt==0){
			  write_message=-2;
			  write(tempfd,&write_message,sizeof(write_message));
			  read(tempfd,&read_message,sizeof(read_message));
			  limit_user_cnt=read_message;
		}
		else{
			write_message=-3;
			write(tempfd,&write_message,sizeof(write_message));
		}

	
		write_message=0;
		write(tempfd,&write_message,sizeof(write_message));
		

		for(j=0;j<limit_user_cnt;j++){
			if(room_data[room_cnt].sock_fd[j]==0){
				room_data[room_cnt].sock_fd[j]=tempfd;
				//strcpy(room_data[room_cnt].ip,inet_ntoa(client_addr.sin_addr));

				printf("Accept sucess\n");
				printf("accpeted sockfd %d ,client count %d\n",client_data[i].sock_fd,++client_cnt);

				if(j==limit_user_cnt-1){
					 pthread_create(ptid+i,NULL,(void*)room_handler,room_data+i);
					 room_data[room_cnt].user_cnt=limit_user_cnt;
					 room_cnt++;
					 printf("Make Room.\n");
				}
				else
					break;
			}

		}
	  
    }
         
 }
    close(sock_id);

}


void* room_handler(void* room){

    struct client* ptr = room;

    int write_message=0;
    int read_message=-1;
   
    int i=0;

    while(1){
		for(i=0;i<ptr->user_cnt;i++){
			if(ptr->sock_fd[i]==ptr->sock_fd[turn_cnt]){
				write_message=1;
				write(ptr->sock_fd[i],&write_message,sizeof(write_message));
			}
			else{
				write_message=2;
				write(ptr->sock_fd[i],&write_message,sizeof(write_message));
			}
		}

		read(ptr->sock_fd[turn_cnt],&read_message,sizeof(read_message)); 

		 if(read_message==-1)
                break;
    
		for(i=0;i<ptr->user_cnt;i++){
			if(ptr->sock_fd[i]!=ptr->sock_fd[turn_cnt]){
				write(ptr->sock_fd[i],&read_message,sizeof(read_message));
				 printf("[%d] send pos : %d\n",ptr->sock_fd,read_message);
			}
		}

            if(turn_cnt+1==ptr->user_cnt){
                turn_cnt=0;
            }
            else
                turn_cnt++;
        } 
    }

	for(i=0;i<ptr->user_cnt;i++)
		close(ptr->sock_fd[i]);

    client_cnt--;
    return NULL;
}
