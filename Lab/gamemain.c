/* 
 * Brian Chrzanowski's Terminal Pong
 * Fri Dec 02, 2016 17:00
 */

//#include <ncurses.h>
//#include <string.h>
//#include <unistd.h>
#include <stm32f4xx.h>
#include "GLCD.h"
#include "JOY.h"

#define DELAY 30000

typedef struct paddle {
	/* paddle variables */
	int x;
	int y;    /* y is the 'top' of the paddle */
	int len;
	int score;
} paddle_t;

typedef struct ball {
	/* ball variables */
	int x;
	int y;
	int next_x;
	int next_y;
	int x_vel;
	int y_vel;
} ball_t;

typedef struct dimensions {
	int x;
	int y;
} dimensions_t;

void draw_ball(ball_t *input);
void draw_paddle(paddle_t *paddle);
void draw_score(paddle_t *inpt_paddle, dimensions_t *wall);
void paddle_collisions(ball_t *inpt_ball, paddle_t *inpt_paddle);
void paddle_pos(paddle_t *pddl, dimensions_t *wall, char dir);

int wall_collisions(ball_t *usr_ball, dimensions_t *walls);
//int kbdhit();

#define USERBUT ((GPIOG->IDR & (1 << 15))==0)
#define WAKEUPBUT ((GPIOA->IDR & (1 << 0))==1)
#define TAMPERBUT ((GPIOC->IDR & (1 << 13))==0)
char getch(void)  //return button or joystick
{
	static int userbut=0;  // not pressed
	static int wakeupbut=0;
	static int tamperbut=0;
	static int joy_center_state=0;
	
	if (userbut==0 && USERBUT) {  //pause
		userbut=1;     // pressed
	  } else if (userbut==1 && !USERBUT) {
		userbut=0;     // not pressed
		return 'p';
	}
	if (wakeupbut==0 && WAKEUPBUT) {  //pause
		wakeupbut=1;     // pressed
	} else if (wakeupbut==1 && !WAKEUPBUT) {
		wakeupbut=0;     // not pressed
		return 'p';
	}
	if (tamperbut==0 && TAMPERBUT) {  //pause
		tamperbut=1;     // pressed
	} else if (tamperbut==1 && !TAMPERBUT) {
		tamperbut=0;     // not pressed
		return 'p';
	}
	if (JOY_GetKeys() == JOY_LEFT) {
		return 'j';
	}
	if(JOY_GetKeys() == JOY_RIGHT) {
		return 'k';
	}
	
	return 0;
}
/*
* function : kbdhit
* purpose  : find out if we've got something in the input buffer
* input    : void
* output   : 0 on none, 1 on we have a key
*/
int kbdhit()
{
	int key = getch();

	if (key != 0) {
		//ungetch(key);
		return 0;
	} else {
		return 1;
	}
}

int gamemain(int argc, char **argv)
{
	int i;
	char buf[128];
	dimensions_t walls = { 0 };
	paddle_t usr_paddle = { 0 };
	ball_t usr_ball = { 0 };
	/* we actually have to store the user's keypress somewhere... */
	int keypress = 0;
	int run = 1;
	/* initialize curses */
	//initscr();
	//noecho();
	//curs_set(0);
	GLCD_SetTextColor(Black); 
    GLCD_SetBackColor(White);
    GLCD_Clear(White);

	//getmaxyx(stdscr, walls.y, walls.x); /* get dimensions */
	walls.y = 30;
    walls.x = 53;

	/* set the paddle variables */
	usr_paddle.x = 5;
	usr_paddle.y = 11;
	usr_paddle.len = walls.y / 4; usr_paddle.score = 0; 

	/* set the ball */
	usr_ball.x = walls.x / 2;
	usr_ball.y = walls.y / 2;
	usr_ball.next_x = 0;
	usr_ball.next_y = 0;
	usr_ball.x_vel = 1;
	usr_ball.y_vel = 1;

	//nodelay(stdscr, TRUE);
	//scrollok(stdscr, TRUE);
	
	/* SysTick timer interrupt */
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 20);
	
	while (run) {
		//getmaxyx(stdscr, walls.y, walls.x);
		walls.y = 30;
		walls.x = 53;
		//clear();
		/* clear screen of all printed chars */
		GLCD_Clear(White);

		draw_ball(&usr_ball);
		draw_paddle(&usr_paddle);
		draw_score(&usr_paddle, &walls);
		
		//refresh(); /* draw to term */
		//usleep(DELAY);
		for(i=0; i<DELAY*3; i++) ;

		/* set next positions */
		usr_ball.next_x = usr_ball.x + usr_ball.x_vel;
		usr_ball.next_y = usr_ball.y + usr_ball.y_vel;

		/* check for collisions */
		paddle_collisions(&usr_ball, &usr_paddle);
		if (wall_collisions(&usr_ball, &walls)) {
			run = 0;
			break;
		}
		
		/* we fell out, get the key press */
		keypress = getch();

		switch (keypress) {
		case 'j':
		case 'k':
			paddle_pos(&usr_paddle, &walls, keypress);
			break;

		case 'p': /* pause functionality, because why not */
			//mvprintw(1, 0, "PAUSE - press any key to resume");
			sprintf(buf,"PAUSE - press any key to resume");
			GLCD_DisplayString(1,0,0,buf);
			while (getch() == 0) {
				//usleep(DELAY * 7);
				for(i=0; i<DELAY * 7; i++) ;
			}
			break;

		case 'q':
			run = 0;
			break;

		}
	}
	
	//endwin();

	//printf("GAME OVER\nFinal Score: %d\n", usr_paddle.score);
	sprintf(buf,"GAME OVER");
	GLCD_DisplayString(1,0,0,buf);
	sprintf(buf,"Final Score: %d", usr_paddle.score);
	GLCD_DisplayString(2,0,0,buf);

	return 0;
}

/*
 * function : paddle_pos
 * purpose  : have a function that will return a proper 'y' value for the paddle
 * input    : paddle_t *inpt_paddle, dimensions_t *wall, char dir
 * output   : void
 */
void paddle_pos(paddle_t *pddl, dimensions_t *wall, char dir)
{
	if (dir == 'k') {  /* moving down */
		if (pddl->y + pddl->len + 1 <= wall->y)
			pddl->y++;
	} else if (dir == 'j'){  /* moving up */
		if (pddl->y - 1 >= 0)
			pddl->y--;
	}
	return;
}

/*
 * function : wall_collisions
 * purpose  : to check for collisions on the terminal walls
 * input    : ball_t *, dimensions_t *
 * output   : nothing (stored within the structs)
 */
int wall_collisions(ball_t *usr_ball, dimensions_t *walls)
{
	/* check if we're supposed to leave quick */
	if (usr_ball->next_x < 0) {
		return 1;
	}

	/* check for X */
	if (usr_ball->next_x >= walls->x) {
		usr_ball->x_vel *= -1;
	} else {
		usr_ball->x += usr_ball->x_vel;
	}

	/* check for Y */
	if (usr_ball->next_y >= walls->y || usr_ball->next_y < 0) {
		usr_ball->y_vel *= -1;
	} else {
		usr_ball->y += usr_ball->y_vel;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */

void paddle_collisions(ball_t *inpt_ball, paddle_t *inpt_paddle)
{
	/* 
	* simply check if next_% (because we set the next_x && next_y first) 
	* is within the bounds of the paddle's CURRENT position
	*/

	if (inpt_ball->next_x == inpt_paddle->x) {
		if (inpt_paddle->y <= inpt_ball->y &&
			inpt_ball->y <= 
			inpt_paddle->y + inpt_paddle->len) {

			inpt_paddle->score++;
			inpt_ball->x_vel *= -1;
		}
	}

	return;
}

/* -------------------------------------------------------------------------- */

/*
 * functions : draw_ball && draw_paddle
 * purpose   : condense the drawing functions to functions
 * input     : ball_t * && paddle_t *
 * output    : void
 */
void draw_ball(ball_t *input)
{
	char buf[128];
	//mvprintw(input->y, input->x, "O");
	sprintf(buf,"O");
	GLCD_DisplayString(input->y,input->x,0,buf);
	return;
}

void draw_paddle(paddle_t *paddle)
{
	int i;
	char buf[128];

	for (i = 0; i < paddle->len; i++) {
		//mvprintw(paddle->y + i, paddle->x, "|");
    sprintf(buf,"|");
    GLCD_DisplayString(paddle->y + i,paddle->x,0,buf);
	}
	return;
}

void draw_score(paddle_t *inpt_paddle, dimensions_t *wall)
{
	char buf[128];
	//mvprintw(0, wall->x / 2 - 7, "Score: %d", inpt_paddle->score);
	sprintf(buf,"Score: %d", inpt_paddle->score);
	GLCD_DisplayString(0,wall->x / 2 - 7,0,buf);
	return;
}
/* -------------------------------------------------------------------------- */

/* SysTick function */
void SysTick_Handler (void) {
	static int flag = 0;
	static int x=0;
	char buf[128];
	if (flag == 0) {
		sprintf(buf," EWHA ");
    GLCD_DisplayString(29,x++,0,buf);
		flag = 1;
	} else {
		flag = 0;
	}
}