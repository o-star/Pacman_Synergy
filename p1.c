#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/time.h>


#define LEFTEDGE 30
#define RIGHTEDGE 60
#define TIMEVAL 40

int dir = 1;
int pos = LEFTEDGE;
int FLOOR = 20;
char blank[] = "     ";

void sig_handler();
int set_ticker(int n_msecs);
void set_cr_noecho_mode();
void stack_tower();	// 탑이 밑으로 쌓이는 과정


int main()
{
	char c;
	initscr();
	set_cr_noecho_mode();
	clear();
	

	signal(SIGALRM, sig_handler);
    srand(time(NULL));
	
	if(set_ticker(TIMEVAL) == -1)
		perror("set_ticker");
	
	
	while(1){
        flushinp();
		c = getchar();
		switch(c){
			case ' ':
				set_ticker(2000); // 탑이 다 떨어질때 까지의 시간 멈춰두는것
				stack_tower();
				FLOOR -= 1;
                pos = rand() % (RIGHTEDGE - LEFTEDGE) + LEFTEDGE;
				set_ticker(TIMEVAL);
				break;
			case 'q':
				endwin();
				return 0;
		}
	}
}


void sig_handler()
{	
	move(0, pos);
	standend();
	addstr(blank);
	pos += dir;
	if(pos>=RIGHTEDGE)
		dir = -1;
	if(pos<=LEFTEDGE)
		dir = +1;
	move(0, pos);
	standout();
	addstr(blank);
    curs_set(0);
	refresh();
}

int set_ticker(int n_msecs)
{
	struct itimerval new_timeset;
	long n_sec, n_usecs;

	n_sec = n_msecs / 1000;
	n_usecs = (n_msecs % 1000) * 1000L;

	new_timeset.it_interval.tv_sec = n_sec;
	new_timeset.it_interval.tv_usec = n_usecs;
	new_timeset.it_value.tv_sec = n_sec;
	new_timeset.it_value.tv_usec = n_usecs;

	return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

void set_cr_noecho_mode()
{
	struct termios ttystate;

	tcgetattr(0, &ttystate);
	ttystate.c_lflag &= ~ICANON;
	ttystate.c_lflag &= ~ECHO;
	ttystate.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &ttystate);
}

void stack_tower()
{
	int row_pos = 0;
	

	while(1){
		move(row_pos,pos);
		standend();
		addstr(blank);
		
		row_pos += 1;

		move(row_pos,pos);
		standout();
		addstr(blank);
        curs_set(0);
		refresh();
		usleep(50000);		// 1초 미만 쉬어줄때 사용

		if(row_pos == FLOOR)
			break;
	}
}
