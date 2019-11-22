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
#define TOWERBOTTOM 25     //ȭ�鿡���� ���� �Ʒ� �� y����ġ
#define MAXVIEWEDBLOCKS 8  //������ ȭ�鿡 ������ ���� ����


char* borderary[TOWERBOTTOM + 2] = {
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
double arrCenterX[100];     	//���� �����߽� x��ǥ���� �迭(�ε����¸���� ������)
int arrBlockPosition[100];  	//���� ���ʳ� x��ǥ���� �迭(�ε����� ���° ������)
int numStackedBlocks = 0;
int down_block_cnt = 0;
int user_cnt=0;

typedef struct user{
    char username[100];
    int score;
}user;

user user_arr[11];

void sig_handler();
int set_ticker(int n_msecs);
void set_cr_noecho_mode();
void initial_screen();		// �ʱ���� ����ȭ�� ���
void stack_tower();		// ž�� ������ ���̴� ����
int can_stack(double);          //ž�� ���������ʰ� ���� ���� �� �ִ��� üũ�ϴ� �Լ�(���� �� ������ T, ������ F ��ȯ)
void view_stack_cnt();
void move_tower_down();         //ȭ�鿡 ���� ������ ���� ���̸� ž�� �Ʒ��� ������ 
void scretch_bolder();		// ���� â�� ������ ���
void reduce_speed(int*);
void increase_speed();		// ž�� �׿������� ž�ӵ��� �÷���
void game_over_view();      //ž �������� ���� �������� ������ ��
void mode_initialize();     //���� ���� ���� �����ؾ��� �ʱ�ȭ �Լ�
void set_block_position();
int get_ok_char(void);     //�ùٸ� �Է��� �´��� Ȯ���ϴ� �Լ�
void highscore_screen();
void read_userscore();
int check_highscore();
void write_highscore(char[],int);

int main()
{
	char c;
	int reduce_speed_item_cnt = 2;
	int set_item_cnt = 1;
	int block_color = rand() % 6 + 1;    //�����ϰ� �� ���� ������

	mode_initialize();

	while (1) {
		init_pair(numStackedBlocks + 1, block_color, COLOR_BLACK);      //�� ������ ���� ���� ���
		flushinp();
		//c = getchar();
		c = get_ok_char();
		switch (c) {
		case ' ': // spaec bar�� ��������, ����

			set_ticker(2000); // ž�� �� �������� ������ �ð� ����δ°�
			stack_tower();
			if (!can_stack((double)pos))
			{
				game_over_view();
				return 0;
			}

			arrBlockPosition[numStackedBlocks] = pos;	// stack ��ġ���� ����
			if (numStackedBlocks > MAXVIEWEDBLOCKS)
				move_tower_down();
			else
				FLOOR -= 1; // ������ �׿����ϱ�, FLOOR -1�� ��Ų��.

			pos = rand() % (RIGHTEDGE - LEFTEDGE) + LEFTEDGE;
			increase_speed();
			flags = TRUE;
			block_color = rand() % 6 + 1;
			break;

		case 'q':
			endwin();
			return 0;

		case 'r': // r�� ������ ���ӵ��� �پ��� �Ѱ��Ӵ� 2������ ����
			if (flags) { // ���� ����߸������� r�� �ι������� �� , ������ ������ ������� ���� �����ϱ� ���� flags
				reduce_speed(&reduce_speed_item_cnt);
			}
			break;
		case 's':
			set_block_position(&set_item_cnt);
			break;
		}
	}
}

void set_block_position(int *item) {

	int first_down_block_position = arrBlockPosition[1];
	int other_down_block_position, row, i;
	row = TOWERBOTTOM;

	if (*item != 0) {

		for (i = down_block_cnt + 1; i <= numStackedBlocks; i++) {
			other_down_block_position = arrBlockPosition[i];

			mvaddstr(row, other_down_block_position, blank);
			mvaddstr(row, other_down_block_position - 2, blank);

			attron(A_STANDOUT | COLOR_PAIR(i));
			mvaddstr(row--, first_down_block_position, blank);
			attroff(A_STANDOUT | COLOR_PAIR(i));
		}


		for (i = 1; i <= numStackedBlocks; i++) {
			arrBlockPosition[i] = first_down_block_position;
			arrCenterX[i] = arrCenterX[1];
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
	mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "Press Button !!");
	attroff(A_BLINK);
	mvaddstr(TOWERBOTTOM / 2 - 1, LEFTEDGE + 25, "Game start  : 1");
	mvaddstr(TOWERBOTTOM / 2, LEFTEDGE + 25, "Help        : 2");
	mvaddstr(TOWERBOTTOM / 2 + 1, LEFTEDGE + 25, "Score Record: 3");
	mvaddstr(TOWERBOTTOM / 2 + 2, LEFTEDGE + 25, "Quit        : q");
	curs_set(0);

	refresh();

	while (1) {
		control = getchar();
		switch (control) {
		case '1':
			clear();
			return;
		case '2':
			break;
		case '3':
			highscore_screen();
			break;
		case 'q':
			endwin();
			exit(0);
		}
	}
}

void highscore_screen() {

	char username[10][10];
    char userscore[10];
	char c;
	int size,cnt = 0;

	FILE *fp;

    clear();
	scretch_bolder();
	fp = fopen("highscore.txt", "r");

	attron(A_BLINK);
	mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "< High Score >");
	attroff(A_BLINK);

	mvaddstr(TOWERBOTTOM / 2-1, LEFTEDGE + 25, "User name      Score");

    for(size=0;size<5;size++){

        if(fscanf(fp,"%s",username[cnt])!=-1){
            if(fscanf(fp,"%s",userscore)!=-1){    
	        	mvaddstr(TOWERBOTTOM / 2 +cnt, LEFTEDGE + 25,username[cnt]);
                mvaddstr(TOWERBOTTOM / 2+cnt,LEFTEDGE + 45,userscore);
	        	cnt++;
            }
        }

    }

	curs_set(0);
	refresh();
    fclose(fp);

	while (c = getchar()) {
		if (c == 'q') {
			clear();
            break;
		}
	}
}

void mode_initialize()
{
	initscr();
	set_cr_noecho_mode();
	clear();

	if (has_colors())
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

	for (row = FLOOR, idx = numStackedBlocks; row <= TOWERBOTTOM; row++, idx--)
		mvaddstr(row, arrBlockPosition[idx], blank);

	for (row = FLOOR + 1, idx = numStackedBlocks; row <= TOWERBOTTOM; row++, idx--)
	{
		attron(A_STANDOUT | COLOR_PAIR(idx));
		mvaddstr(row, arrBlockPosition[idx], blank);
		attroff(A_STANDOUT | COLOR_PAIR(idx));
	}

	down_block_cnt++;
	refresh();
}

void reduce_speed(int *item_cnt) {

	int move_speed = 70;

	if (*item_cnt>0) {
		(*item_cnt)--;
		set_ticker(move_speed);
		flags = FALSE;
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

void sig_handler() // ���� �¿�� �����̴� ����
{
	move(1, pos);
	addstr(blank);
	pos += dir;
	if (pos >= RIGHTEDGE)
		dir = -1;
	if (pos <= LEFTEDGE)
		dir = +1;
	move(1, pos);
	attron(A_STANDOUT | COLOR_PAIR(numStackedBlocks + 1));
	addstr(blank);
	attroff(A_STANDOUT | COLOR_PAIR(numStackedBlocks + 1));

	view_stack_cnt();
	curs_set(0);
	refresh();
}

void view_stack_cnt() {
	char stack_cnt_string[100];
	sprintf(stack_cnt_string, "Stacked block : %d ", numStackedBlocks);
	mvaddstr(30, RIGHTEDGE + 10, stack_cnt_string);
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

void set_echo_mode(){
    
    struct termios ttystate;

    tcgetattr(0,&ttystate);
    ttystate.c_lflag |= ECHO;
    tcsetattr(0,TCSANOW,&ttystate);
}

int get_ok_char(void)
{
	int c;
	while ((c = getchar()) != EOF && strchr("qQrRsS ", c) == NULL);
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
		attron(A_STANDOUT | COLOR_PAIR(numStackedBlocks + 1));
		addstr(blank);
		attroff(A_STANDOUT | COLOR_PAIR(numStackedBlocks + 1));
		curs_set(0);
		refresh();
		usleep(50000);		// 1�� �̸� �����ٶ� ���

		if (row_pos == FLOOR)
			break;
	}
}

void game_over_view()
{
	char collapsed[] = "Tower is collapsed!!!!!!!!";
	char collapsed2[35]=" ";
	char collapsed3[] = "Press any key to exit";
    char question[] = "Do you want record your score ?( y or n )  ";
    char question2[] = "Write your name : ";
    char select;
    char answer[20];
    int insert_index=0;

	signal(SIGALRM, SIG_IGN); // ���õǸ�, ���̻� ���� �������� �ʰ� �ȴ�.
	clear(); // ȭ�� ���ֱ�
	scretch_bolder();
	sprintf(collapsed2, "Stacked block : %d  ", numStackedBlocks);
	attron(A_BLINK);
	mvaddstr(TOWERBOTTOM / 2 - 3, (RIGHTEDGE - strlen(collapsed)) / 2 + 14, collapsed);
	attroff(A_BLINK);
	mvaddstr(TOWERBOTTOM / 2 + 2, (RIGHTEDGE - strlen(collapsed)) / 2 + 18, collapsed2);
    refresh();

    
    if((insert_index=check_highscore())!=-1){
        mvaddstr(TOWERBOTTOM / 2 +6,(RIGHTEDGE - strlen(collapsed))/2+ 14,question);
        refresh();
        fflush(stdin); 
        while((select=getch())!=-1){
            
            if(select=='y'){
                 mvaddstr(TOWERBOTTOM / 2 +8,(RIGHTEDGE - strlen(collapsed))/2+ 14,question2);
                 refresh();
                 set_echo_mode();
                 fflush(stdin);
                 scanf("%s",answer);
                 write_highscore(answer,insert_index);
                 set_cr_noecho_mode();
                 highscore_screen();
                 endwin();
            }
            else if(select=='n'){
                endwin();
            }

            exit(1);
        
        }

    }
    else{
    	mvaddstr(TOWERBOTTOM / 2 + 3, (RIGHTEDGE - strlen(collapsed)) / 2 + 16, collapsed3);
        refresh();
    	getch();
        endwin();
    }

}

void write_highscore(char name[],int insert_index){

    FILE *fp;
    int i,idex=0;
    fp=fopen("highscore.txt","w+");

    for(i=0;i<=user_cnt;i++){
    
        if(i==insert_index){
            fprintf(fp,"%s %d\n",name,numStackedBlocks);
        }
        else{
            fprintf(fp,"%s %d\n",user_arr[idex].username,user_arr[idex].score);
            idex++;
        }
    
    }
    fclose(fp);

}

int check_highscore(){
    
    int insert_index=0;
    int find_flag=0;
    int i=0;

    read_userscore();

    for(i=0;i<5;i++){
        if(numStackedBlocks>user_arr[i].score){
            find_flag=1;
            insert_index=i;
            break;
        }
    }

    if(user_cnt==0 || find_flag==1){     
        return insert_index;
    }
    else
        return -1;

}

void read_userscore(){

    FILE *fp;
    fp=fopen("highscore.txt","r");

    while(fscanf(fp,"%s",user_arr[user_cnt].username)!=-1){

        fscanf(fp,"%d",&user_arr[user_cnt].score);
        user_cnt++;
    }


    fclose(fp);

}

void scretch_bolder() {
	int i;
	int bolder = 200;

	for (i = 0; i<TOWERBOTTOM + 2; i++) {
		move(i, LEFTEDGE - 1);
		addstr(borderary[i]);
		refresh();
	}
}

void increase_speed()
{
	if (numStackedBlocks % 5 == 0 && TIMEVAL >10)
		TIMEVAL -= 10;

	set_ticker(TIMEVAL);
}
