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
#include <locale.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define HOST "ip-172-31-36-126"
#define TRUE 1
#define FALSE 0
#define LEFTEDGE 20
#define RIGHTEDGE 80
#define TOWERBOTTOM 25     //화면에서의 제일 아래 블럭 y축위치
#define MAXVIEWEDBLOCKS 8  //게임중 화면에 보여질 블럭의 개수
#define INITIALTIMEVAL 40
#define oops(msg)	{ perror(msg); exit(1); }


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

int TIMEVAL = INITIALTIMEVAL;
int dir = 1;
int pos;
int FLOOR = TOWERBOTTOM;
int flags = TRUE;
char blank[] = "        ";
double arrCenterX[100];        //블럭의 무게중심 x좌표들의 배열(인덱스는몇번쨰 블럭인지)
int arrBlockPosition[100];     //블럭의 왼쪽끝 x좌표들의 배열(인덱스는 몇번째 블럭인지))
int numStackedBlocks = 0;
int down_block_cnt = 0;
int user_cnt=0;

typedef struct user{
    char username[100];
    int score;
}user;

user user_arr[100];

void view_game_explanation();
void sig_handler();
int set_ticker(int n_msecs);
void set_cr_noecho_mode();
void set_echo_and_canon_mode();
void initial_screen();        // 초기게임 시작화면 출력
void stack_tower();        // 탑이 밑으로 쌓이는 과정
int can_stack(double);          //탑이 무너지지않게 블럭을 쌓을 수 있는지 체크하는 함수(쌓을 수 있으면 T, 없으면 F 반환)
void view_stack_and_item_cnt(int, int);          
void move_tower_down();         //화면에 일정 개수의 블럭이 쌓이면 탑을 아래로 내려줌 
void scretch_bolder();		// 게임 창의 테투리 출력
void reduce_speed(int*);
void increase_speed();		// 탑이 쌓여갈수록 탑속도를 늘려줌
int game_over_view();      //탑 무너져서 게임 끝났을때 나오는 뷰
void set_block_position();
int get_ok_char(void);     //올바른 입력이 맞는지 확인하는 함수
void highscore_screen();
void read_userscore();
int check_highscore();
void write_highscore(char[],int);
void game_view();
int show_restart_comment();
void show_game_over_comment();
void multi_gameversion();	// 멀티버전 게임 


int main()
{
    setlocale(LC_ALL, "ko_KR.utf8");
    setlocale(LC_CTYPE, "ko_KR.utf8");	
    /* 한글출력 깨짐 방지 */

    initscr();
    set_cr_noecho_mode();
    clear();

    if (has_colors())
        start_color();

    initial_screen();
    endwin();
    return 0;
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

    while(1){
        scretch_bolder();
        attron(A_BLINK);
        mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "Press Button !!");
        attroff(A_BLINK);
        mvaddstr(TOWERBOTTOM / 2 - 1, LEFTEDGE + 25, "Game start   : 1");
        mvaddstr(TOWERBOTTOM / 2    , LEFTEDGE + 25, "Multi Game   : 2");
        mvaddstr(TOWERBOTTOM / 2 + 1, LEFTEDGE + 25, "Help         : 3");
        mvaddstr(TOWERBOTTOM / 2 + 2, LEFTEDGE + 25, "Score Record : 4");
        mvaddstr(TOWERBOTTOM / 2 + 3, LEFTEDGE + 25, "Quit         : Q");
        curs_set(0);

        refresh();

        while (1) {
            control = getchar();

            if(control == '1'){
                game_view();
                break;
            }
            if(control == '2'){
                multi_gameversion();
                break;
            }
            if(control == '3'){
                view_game_explanation();
                break;
            }
            if(control == '4'){
                highscore_screen();
                break;
            }
            if(control == 'q')
                return;
        }
    }
}

void game_mode_initialize(int *item1, int *item2)
{
    clear();
    scretch_bolder();
    TIMEVAL = INITIALTIMEVAL;
    signal(SIGALRM, sig_handler);
    if (set_ticker(TIMEVAL) == -1)
        perror("set_ticker");

    FLOOR = TOWERBOTTOM;
    srand(time(NULL));
    pos = rand() % (RIGHTEDGE - LEFTEDGE) + LEFTEDGE;
    numStackedBlocks = 0;
    down_block_cnt = 0;

    *item1 = 2, *item2 = 1;
}

void multi_gameversion()
{
    struct sockaddr_in servadd;
    struct hostent *hp;
    int sock_id;
    int read_message;
    int write_message;
    int now_player = 0;	// 선공 체크

    char c;
    int reduce_speed_item_cnt;
    int set_item_cnt;
    int block_color = rand() % 6 + 1;    //랜덤하게 블럭 색깔 정해줌

    clear();
    scretch_bolder();

    sock_id = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_id == -1)
        oops("socket");
    bzero(&servadd, sizeof(servadd));
    hp = gethostbyname(HOST);
    if( hp == NULL)
        oops("hostname");
    bcopy(hp->h_addr, (struct sockaddr*) &servadd.sin_addr, hp->h_length);
    servadd.sin_port = htons(13000);
    servadd.sin_family = AF_INET;

    if(connect(sock_id, (struct sockaddr *)&servadd, sizeof(servadd)) != 0)
        oops("bind");

    read(sock_id, &read_message, BUFSIZ);

    if(read_message==-2){
        int player_num;
        char str[] = "How many players do you want to play with including you?: ";
        char remove[] = "                                                            ";
        mvaddstr(TOWERBOTTOM/2-3, LEFTEDGE + 5, str);
        refresh();
        curs_set(1);
        fflush(stdin);
        mvscanw(TOWERBOTTOM/2-3, LEFTEDGE+5+strlen(str), "%d", &player_num);
        mvaddstr(TOWERBOTTOM/2-3, LEFTEDGE+5, remove);
        refresh();
        curs_set(0);
        write_message=player_num; // 인원설정
        write(sock_id, &write_message, sizeof(write_message));
    }

    read(sock_id, &read_message, BUFSIZ);
    if(read_message == -1){
        attron(A_BLINK);
        mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "Wait Your Partner");
        attroff(A_BLINK);
        refresh();
        read(sock_id, &read_message, BUFSIZ);
    } // 대전상대 기다리기


    game_mode_initialize(&reduce_speed_item_cnt, &set_item_cnt);
    while (1) {
        init_pair(numStackedBlocks + 1, block_color, COLOR_BLACK);      //각 블럭마다 색깔 정보 등록
        flushinp();

        if(read_message==1)
        {
            now_player=1;
            set_ticker(TIMEVAL);
            signal(SIGALRM, sig_handler);
        }
        else if(read_message==2)
        {
            now_player=2;
            signal(SIGALRM, SIG_IGN);
        }

        if(now_player==2){
            attron(A_BLINK);
            mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "Partner Turn! Wait!");
            attroff(A_BLINK);
            refresh();

            read(sock_id, &read_message, BUFSIZ);
            pos = read_message;
            mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "                   ");

            set_ticker(2000); // 탑이 다 떨어질때 까지의 시간 멈춰두는것
            stack_tower();

            if (!can_stack((double)pos))
            {
                write_message = -1;
                write(sock_id, &write_message, sizeof(write_message));

                mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "You Win!");
                refresh();

                sleep(2);

                game_mode_initialize(&reduce_speed_item_cnt, &set_item_cnt);
                signal(SIGALRM, SIG_IGN);
                clear();
                return;
            }

            arrBlockPosition[numStackedBlocks] = pos;    // stack 위치정보 저장
            if (numStackedBlocks > MAXVIEWEDBLOCKS)
                move_tower_down();
            else
                FLOOR -= 1; // 한층이 쌓였으니깐, FLOOR -1을 시킨다.

            pos = rand() % (RIGHTEDGE - LEFTEDGE) + LEFTEDGE + 1;
            increase_speed();
            flags = TRUE;
            block_color = rand() % 6 + 1;

            // 다음 순서 지정
        }

        else{
            while((c = getchar()) != EOF && strchr("qQ ", c) == NULL);

            switch (c) {
                case ' ': // spaec bar를 눌렸을때, 실행
                    write(sock_id, &pos, sizeof(pos));

                    set_ticker(2000); // 탑이 다 떨어질때 까지의 시간 멈춰두는것
                    stack_tower();

                    if (!can_stack((double)pos))
                    {
                        write_message = -1;
                        write(sock_id, &write_message, sizeof(write_message));

                        mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "You Lose");
                        refresh();

                        sleep(2);

                        game_mode_initialize(&reduce_speed_item_cnt, &set_item_cnt);
                        signal(SIGALRM, SIG_IGN);
                        clear();
                        return;
                    }

                    arrBlockPosition[numStackedBlocks] = pos;    // stack 위치정보 저장
                    if (numStackedBlocks > MAXVIEWEDBLOCKS)
                        move_tower_down();
                    else
                        FLOOR -= 1; // 한층이 쌓였으니깐, FLOOR -1을 시킨다.

                    pos = rand() % (RIGHTEDGE - LEFTEDGE) + LEFTEDGE - 1;
                    increase_speed();
                    flags = TRUE;
                    block_color = rand() % 6 + 1;
                    break;

                case 'q':
                    signal(SIGALRM, SIG_IGN);
                    clear();
                    return ;
            }
        }

        read(sock_id,&read_message,sizeof(read_message));
    }
}

void game_view()
{
    char c;
    int reduce_speed_item_cnt;
    int set_item_cnt;
    int block_color = rand() % 6 + 1;    //랜덤하게 블럭 색깔 정해줌

    game_mode_initialize(&reduce_speed_item_cnt, &set_item_cnt);

    while (1) {
        view_stack_and_item_cnt(reduce_speed_item_cnt, set_item_cnt);
        init_pair(numStackedBlocks + 1, block_color, COLOR_BLACK);      //각 블럭마다 색깔 정보 등록
        flushinp();
        c = get_ok_char();
        switch (c) {
            case ' ': // spaec bar를 눌렸을때, 실행

                set_ticker(2000); // 탑이 다 떨어질때 까지의 시간 멈춰두는것
                stack_tower();
                if (!can_stack((double)pos))
                {
                    if(game_over_view())
                    {
                        game_mode_initialize(&reduce_speed_item_cnt, &set_item_cnt);
                        break;
                    }
                    else
                        return;
                }

                arrBlockPosition[numStackedBlocks] = pos;    // stack 위치정보 저장
                if (numStackedBlocks > MAXVIEWEDBLOCKS)
                    move_tower_down();
                else
                    FLOOR -= 1; // 한층이 쌓였으니깐, FLOOR -1을 시킨다.

                pos = rand() % (RIGHTEDGE - LEFTEDGE) + LEFTEDGE;
                increase_speed();
                flags = TRUE;
                block_color = rand() % 6 + 1;
                break;

            case 'q':
                signal(SIGALRM, SIG_IGN);
                clear();
                return ;

            case 'r': // r을 누르면 블럭속도가 줄어든다 한게임당 2번까지 가능
                if(flags){ // 블럭을 떨어뜨리기전에 r을 두번눌렀을 때 , 아이템 갯수가 사라지는 것을 방지하기 위한 flags
                    reduce_speed(&reduce_speed_item_cnt);
                }
                break;
            case 's':
                if(numStackedBlocks > 1)
                    set_block_position(&set_item_cnt);
                break;
        }
    }
}

void view_game_explanation()
{
    char c;

    clear();
    scretch_bolder();

    mvaddstr(4, LEFTEDGE + 2, "게임설명 : ");
    mvaddstr(5, LEFTEDGE + 2, "탑이 무너지지 않게 균형을 맞추어 쌓아가는 게임입니다.");
    mvaddstr(6, LEFTEDGE + 2, "탑이 많이 쌓여갈수록 탑이 움직이는 속도는 증가합니다.");
    mvaddstr(7, LEFTEDGE + 2, "탑을 최대한 많이 쌓아 최고기록을 갱신해 보세요 !!");

    mvaddstr(10, LEFTEDGE + 2, "조작키 설명 : ");
    mvaddstr(11, LEFTEDGE + 2, "Space bar : 탑 떨어뜨리기");
    mvaddstr(12, LEFTEDGE + 2, "R : 탑이 좌우로 움직이는 속도를 일시적으로 늦춰주는 아이템 효과");
    mvaddstr(13, LEFTEDGE + 2, "    본 아이템은 게임당 2회 사용가능");
    mvaddstr(14,LEFTEDGE + 2, "S : 쌓여있는 탑들을 게임당 1회 재정렬 해주는 아이템 효과");

    mvaddstr(TOWERBOTTOM, RIGHTEDGE - 20, "Press key [Q] <- 뒤로가기");

    curs_set(0);
    refresh();

    while(1){
        c=getchar();

        if(c == 'q'){
            clear();
            break;
        }
    }
}

void highscore_screen() {

    char c;
    int size,cnt = 0;
    char line[30];
    char *score;

    FILE *fp;

    clear();
    scretch_bolder();
    fp = fopen("highscore.txt", "r");

    if(fp == NULL)
        fp = fopen("highscore.txt", "w");

    mvaddstr(TOWERBOTTOM / 2 - 3, LEFTEDGE + 25, "< High Score >");
    mvaddstr(TOWERBOTTOM / 2-1, LEFTEDGE + 25, "User name      Score");

    for(size = 0; size < 5; size++)
    {
        if(!fgets(line, sizeof(line), fp)) break;
        line[strlen(line)-1] = '\0';    //개행 문자를 널로 바꿔줌
        for(int i = strlen(line)-1; i >= 0; i--)
        {
            if(line[i] == ' ')
            {
                line[i] = '\0';
                score = &line[i+1];
                break;
            }
        }
        mvaddstr(TOWERBOTTOM / 2 + cnt, LEFTEDGE + 25, line);
        mvaddstr(TOWERBOTTOM / 2 + cnt, LEFTEDGE + 42, score);
        cnt++;
    }

    mvaddstr(TOWERBOTTOM, RIGHTEDGE - 20, "Press key [Q] <- 뒤로가기");

    curs_set(0);
    refresh();
    fclose(fp);

    while(1){
        c=getchar();

        if(c == 'q')
            return;
    }
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
    attron(A_STANDOUT | COLOR_PAIR(numStackedBlocks + 1));
    addstr(blank);
    attroff(A_STANDOUT | COLOR_PAIR(numStackedBlocks + 1));

    curs_set(0);
    refresh();
}

void view_stack_and_item_cnt(int numItem1, int numItem2) {
    char stack_cnt_string[100];
    char *sub_border[8] = {
        "===============================",
        "|                             |",
        "|                             |",
        "|                             |",
        "|                             |",
        "|                             |",
        "|                             |",
        "==============================="
    };

    for(int i = 0; i < 8; i++)
        mvaddstr(2+i, RIGHTEDGE + 10, sub_border[i]);

    sprintf(stack_cnt_string, "Stacked block : %d ", numStackedBlocks);
    mvaddstr(4, RIGHTEDGE + 14, stack_cnt_string);
    mvprintw(6, RIGHTEDGE + 14, "reduce speed item[R]: %d", numItem1);
    mvprintw(7, RIGHTEDGE + 14, "block set item[S]: %d", numItem2);
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

void set_echo_and_canon_mode(){

    struct termios ttystate;

    tcgetattr(0,&ttystate);
    ttystate.c_lflag |= ECHO;
    ttystate.c_lflag |= ICANON;
    tcsetattr(0,TCSANOW,&ttystate);
}

int get_ok_char(void)
{
    int c;
    while ((c = getchar()) != EOF && strchr("qQrRsSyYnN ", c) == NULL);
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
        usleep(50000);        // 1초 미만 쉬어줄때 사용

        if (row_pos == FLOOR)
            break;
    }
}

void show_game_over_comment()
{
    char collapsed[] = "Tower is collapsed!!!!!!!!";
    char collapsed2[35]=" ";

    scretch_bolder();
    sprintf(collapsed2, "Stacked block : %d  ", numStackedBlocks);
    attron(A_BLINK);
    mvaddstr(TOWERBOTTOM / 2 - 3, (RIGHTEDGE - strlen(collapsed)) / 2 + 14, collapsed);
    attroff(A_BLINK);
    mvaddstr(TOWERBOTTOM / 2, (RIGHTEDGE - strlen(collapsed)) / 2 + 18, collapsed2);
    refresh();
}

int game_over_view()
{
    char collapsed3[] = "Press [Q] to quit";
    char question[] = "Do you want to record your score?( y or n )";
    char question2[] = "Write your name : ";
    char select, c;
    char answer[20];
    int insert_index=0;
    int restart_flag;
    char remove[50] = "                                               ";

    signal(SIGALRM, SIG_IGN); // 무시되면, 더이상 블럭이 움직이지 않게 된다..
    clear(); // 화면 없애기
    show_game_over_comment();

    if((insert_index=check_highscore())!=-1){       //top5안에 드는지 체크
        mvaddstr(TOWERBOTTOM / 2 +5,(RIGHTEDGE - strlen(question))/2 + 14,question);
        refresh();
        fflush(stdin);
        while((select=get_ok_char())!=-1){

            if(select=='y'){
                mvaddstr(TOWERBOTTOM / 2 + 7,(RIGHTEDGE - strlen(question))/2 + 14,question2);
                curs_set(1);
                refresh();
                set_echo_and_canon_mode();
                fflush(stdin);
                getnstr(answer, 12);   //최대 12문자까지 받을 수 있음
                curs_set(0);
                write_highscore(answer,insert_index);
                set_cr_noecho_mode();
                highscore_screen();
                show_game_over_comment();
                break;
            }
            else if(select=='n')
            {
                mvaddstr(TOWERBOTTOM / 2 + 5, (RIGHTEDGE - strlen(question))/2 + 14, remove);
                break;
            }
        }
    }

    restart_flag = show_restart_comment();
    return restart_flag;
}

int show_restart_comment(void)
{
    char c;
    char question[] = "Do you want to restart a game? ( y or n )";

    mvaddstr(TOWERBOTTOM / 2 + 5, (RIGHTEDGE - strlen(question))/2 + 14, question);
    refresh();
    while(1)
    {
        c = getchar();
        if(c == 'y') return TRUE;
        else if(c == 'n') return FALSE;
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
    char *line;
    int score;

    fp=fopen("highscore.txt","r+");

    if(fp == NULL)
        fp=fopen("highscore.txt","w+");

    user_cnt = 0;
    while(1)
    {
        line = user_arr[user_cnt].username;
        if(fgets(line, 100, fp) == NULL) break;
        for(int i = strlen(line) - 1; i >= 0; i--)
        {
            if(line[i] == ' ')
            {
                line[i] = '\0';
                score = atoi(&line[i+1]); 
                break;
            }
        }
        user_arr[user_cnt].score = score;
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
