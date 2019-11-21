#include <stdio.h>
#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/time.h>
#include <math.h>

#define TRUE 1
#define FALSE 0
#define LEFTEDGE 20
#define RIGHTEDGE 80
#define TOWERBOTTOM 25     //화면에서의 제일 아래 블럭 y축위치
#define MAXVIEWEDBLOCKS 8  //게임중 화면에 보여질 블럭의 개수


char* borderary[TOWERBOTTOM+2] = {
" ==================================================================== ",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
"|                                                                    |",
" ==================================================================== "
};

int TIMEVAL = 40;
int dir = 1;
int pos;
int FLOOR = TOWERBOTTOM;
int flags = TRUE;
char blank[] = "        ";
double arrCenterX[100];     	//블럭의 무게중심 x좌표들의 배열(인덱스는몇번쨰 블럭인지)
int arrBlockPosition[100];  	//블럭의 왼쪽끝 x좌표들의 배열(인덱스는 몇번째 블럭인지)
int numStackedBlocks = 0;
int down_block_cnt=0;

void sig_handler();
int set_ticker(int n_msecs);
void set_cr_noecho_mode();
void initial_screen();		// 초기게임 시작화면 출력
void stack_tower();		// 탑이 밑으로 쌓이는 과정
int can_stack(double);          //탑이 무너지지않게 블럭을 쌓을 수 있는지 체크하는 함수(쌓을 수 있으면 T, 없으면 F 반환)
void view_stack_cnt();          
void move_tower_down();         //화면에 일정 개수의 블럭이 쌓이면 탑을 아래로 내려줌 
void scretch_bolder();		// 게임 창의 테투리 출력
void reduce_speed(int*);
void increase_speed();		// 탑이 쌓여갈수록 탑속도를 늘려줌
void game_over_view();      //탑 무너져서 게임 끝났을때 나오는 뷰
void mode_initialize();     //게임 시작 전에 세팅해야할 초기화 함수
void set_block_position();
int get_ok_char(void);     //올바른 입력이 맞는지 확인하는 함수


int main()
{
	char c;
    int reduce_speed_item_cnt=2;
	int set_item_cnt=1;
    int block_color = rand() % 6 + 1;    //랜덤하게 블럭 색깔 정해줌

    mode_initialize();

	while (1) {
        init_pair(numStackedBlocks + 1, block_color, COLOR_BLACK);      //각 블럭마다 색깔 정보 등록
		flushinp();
		//c = getchar();
        c = get_ok_char();
		switch (c) {
		case ' ': // spaec bar를 눌렸을때, 실행

			set_ticker(2000); // 탑이 다 떨어질때 까지의 시간 멈춰두는것
			stack_tower();
			if (!can_stack((double)pos))
            {
                game_over_view();
                return 0;
            }

            arrBlockPosition[numStackedBlocks] = pos;	// stack 위치정보 저장
            if(numStackedBlocks > MAXVIEWEDBLOCKS)
                move_tower_down();
            else
			    FLOOR -= 1; // 한층이 쌓였으니깐, FLOOR -1을 시킨다.

			pos = rand() % (RIGHTEDGE - LEFTEDGE) + LEFTEDGE;
			increase_speed();
			flags=TRUE;
            block_color = rand() % 6 + 1;
			break;

		case 'q':
			endwin();
            return 0;

		case 'r': // r을 누르면 블럭속도가 줄어든다 한게임당 2번까지 가능
            if(flags){ // 블럭을 떨어뜨리기전에 r을 두번눌렀을 때 , 아이템 갯수가 사라지는 것을 방지하기 위한 flags
             reduce_speed(&reduce_speed_item_cnt);
            }
            break;
		case 's':
			set_block_position(&set_item_cnt);
			break;
		}
    }
}

void set_block_position(int *item){

	int first_down_block_position=arrBlockPosition[1];
	int other_down_block_position,row,i;
	row = TOWERBOTTOM;

	if(*item !=0){

		for(i=down_block_cnt+1;i<=numStackedBlocks;i++){
			other_down_block_position = arrBlockPosition[i];
			
            mvaddstr(row,other_down_block_position,blank);
            mvaddstr(row,other_down_block_position-2, blank);

			attron(A_STANDOUT | COLOR_PAIR(i));
			mvaddstr(row--,first_down_block_position,blank);
            attroff(A_STANDOUT | COLOR_PAIR(i));
		}

        
        for(i=1;i<=numStackedBlocks;i++){
            arrBlockPosition[i]=first_down_block_position;
            arrCenterX[i]=arrCenterX[1];
        }

		(*item)--;
		refresh();

	}

}



void initial_screen()
{
	char control;

    scretch_bolder();
	attron(A_BLINK);
	mvaddstr(TOWERBOTTOM/2-3, LEFTEDGE+25, "Press Button !!");
	attroff(A_BLINK);
	mvaddstr(TOWERBOTTOM/2-1, LEFTEDGE+25, "Game start  : 1");
	mvaddstr(TOWERBOTTOM/2  , LEFTEDGE+25, "Help        : 2");
	mvaddstr(TOWERBOTTOM/2+1, LEFTEDGE+25, "Score Record: 3");
	mvaddstr(TOWERBOTTOM/2+2, LEFTEDGE+25, "Quit        : q");
	curs_set(0);
	
	refresh();

	while(1){
		control = getchar();
		switch(control){
			case '1':
				clear();
				return;
			case '2':
				break;
			case '3':
				break;
			case 'q':
				endwin();
				exit(0);
		}
	}
}
void mode_initialize()
{
	initscr();
	set_cr_noecho_mode();
	clear();

    if(has_colors())
        start_color();

    initial_screen();
	scretch_bolder();

	signal(SIGALRM, sig_handler);
	if (set_ticker(TIMEVAL) == -1)
		perror("set_ticker");

	srand(time(NULL));
    pos = rand() % (RIGHTEDGE - LEFTEDGE) + LEFTEDGE;
}

void move_tower_down(void)
{
    int row, idx;
    
    for(row = FLOOR, idx = numStackedBlocks; row <= TOWERBOTTOM; row++, idx--)
        mvaddstr(row, arrBlockPosition[idx], blank);
    
    for(row = FLOOR+1, idx = numStackedBlocks; row <= TOWERBOTTOM; row++, idx--)
    {
        attron(A_STANDOUT | COLOR_PAIR(idx));
        mvaddstr(row, arrBlockPosition[idx], blank);
        attroff(A_STANDOUT | COLOR_PAIR(idx));
    }

    down_block_cnt++;
    refresh();
}

void reduce_speed(int *item_cnt){

    int move_speed=70;

    if(*item_cnt>0){
        (*item_cnt)--;
        set_ticker(move_speed);
        flags=FALSE;
    }
}

int can_stack(double leftX)
{
	double curCenterX, centerX;

	curCenterX = leftX + (double)strlen(blank) / 2;
	centerX = 0;

	for (int i = numStackedBlocks; i > 0; i--)
	{
		double tempCenterX;
		if (i == numStackedBlocks)
			tempCenterX = ((numStackedBlocks - i) * centerX + curCenterX) / (numStackedBlocks - i + 1);
		else
			tempCenterX = ((numStackedBlocks - i) * centerX + arrCenterX[i + 1]) / (numStackedBlocks - i + 1);

		if (fabs(tempCenterX - arrCenterX[i]) > strlen(blank) / 2)
			return FALSE;

		centerX = tempCenterX;
	}
	numStackedBlocks++;
	arrCenterX[numStackedBlocks] = curCenterX;
	return TRUE;
}

void sig_handler() // 블럭이 좌우로 움직이는 구간
{
	move(1, pos);
	addstr(blank);
	pos += dir;
	if (pos >= RIGHTEDGE)
		dir = -1;
	if (pos <= LEFTEDGE)
		dir = +1;
	move(1, pos);
    attron(A_STANDOUT | COLOR_PAIR(numStackedBlocks+1));
	addstr(blank);
    attroff(A_STANDOUT | COLOR_PAIR(numStackedBlocks+1));

	view_stack_cnt();
	curs_set(0);
	refresh();
}

void view_stack_cnt() {
	char stack_cnt_string[100];
	sprintf(stack_cnt_string, "Stacked block : %d ", numStackedBlocks);
	mvaddstr(30, RIGHTEDGE+10, stack_cnt_string);
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

int get_ok_char(void)
{
    int c;
    while((c = getchar()) != EOF && strchr("qQrRsS ", c) == NULL);
    return c;
}

void stack_tower()
{
	int row_pos = 1;

	while (1) {
		move(row_pos, pos);
		addstr(blank);
		row_pos += 1;

		move(row_pos, pos);
        attron(A_STANDOUT | COLOR_PAIR(numStackedBlocks+1));
		addstr(blank);
        attroff(A_STANDOUT | COLOR_PAIR(numStackedBlocks+1));
		curs_set(0);
		refresh();
		usleep(50000);		// 1초 미만 쉬어줄때 사용

		if (row_pos == FLOOR)
			break;
	}
}

void game_over_view()
{
	char collapsed[] = "Tower is collapsed!!!!!!!!";
    char collapsed2[30];
    char collapsed3[] = "Press any key to exit";

    signal(SIGALRM, SIG_IGN); // 무시되면, 더이상 블럭이 움직이지 않게 된다.
    clear(); // 화면 없애기
    scretch_bolder();
    sprintf(collapsed2,"Stacked block : %d",numStackedBlocks);
    attron(A_BLINK);
	mvaddstr(TOWERBOTTOM / 2-3, (RIGHTEDGE - strlen(collapsed)) / 2 + 14, collapsed);
    attroff(A_BLINK);
    mvaddstr(TOWERBOTTOM/2+1,(RIGHTEDGE-strlen(collapsed))/2 + 18,collapsed2);
    mvaddstr(TOWERBOTTOM/2+3, (RIGHTEDGE - strlen(collapsed)) / 2 + 16, collapsed3);
    refresh();
    getch();
    endwin();
}

void scretch_bolder(){
	int i;
	int bolder = 200;

	for(i=0; i<TOWERBOTTOM+2; i++){
		move(i,LEFTEDGE-1);
		addstr(borderary[i]);
		refresh();
	}
}

void increase_speed()
{
	if(numStackedBlocks % 5  == 0) 
		TIMEVAL -= 10;
		
	set_ticker(TIMEVAL);
}
