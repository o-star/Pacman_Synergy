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

#define PORTNUM 13000 
#define HOSTLEN 256
#define oops(msg)	{ perror(msg); exit(1); }


int main(int ac, char *av[])
{
	struct sockaddr_in saddr;	/* build our address here	*/
	struct hostent *hp;		/* this part of our		*/
	char hostname[HOSTLEN];		/* address			*/
	int sock_id, sock_fd, sock_fd1;		/* line id, file descriptor	*/

	int message = -1;			// 커뮤니케이션 할 메세지

	sock_id = socket( PF_INET, SOCK_STREAM, 0);
	if(sock_id == -1)
		oops("socket");
	bzero((void*)&saddr, sizeof(saddr));

	gethostname(hostname, HOSTLEN);
	hp = gethostbyname(hostname);

	bcopy((void*) hp->h_addr, (void*) &saddr.sin_addr, hp->h_length);
	saddr.sin_port = htons(PORTNUM);
	saddr.sin_family = AF_INET;

	if(bind(sock_id, (struct sockaddr*) &saddr, sizeof(saddr)) != 0)
		oops("bind");

	if(listen(sock_id, 2) != 0)
		oops("listen");	
	

	sock_fd = accept(sock_id, NULL, NULL);
	printf("Wow! got a call\n");
	if(sock_fd == -1)
		oops("accept");		
	write(sock_fd, &message, sizeof(message));	// 선접속 유저  게임 대기

	sock_fd1 = accept(sock_id, NULL, NULL);
        printf("Wow! got a call\n");
        if(sock_fd1 == -1)
                oops("accept");

	message = 0;
	write(sock_fd, &message, sizeof(message));	// 게임시작
	write(sock_fd1, &message, sizeof(message));	// 게임시작
	
	
	while(1){
		read(sock_fd, &message, BUFSIZ);
		if(message == -1)
			break;
		write(sock_fd1, &message, sizeof(message));
		read(sock_fd1, &message, BUFSIZ);
		if(message == -1)
			break;
		write(sock_fd, &message, sizeof(message));
	}

	close(sock_fd);
	close(sock_fd1);
}
