#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define WIDTH 20
#define HEIGHT 20
#define ARRAY_GRIDSIZE HEIGHT*(WIDTH+1)

//globals:
bool EXIT_FLAG = 0;

int PLAYER_X = 10;
int PLAYER_Y = 10;
int PLAYER_DIR = 0; //N,E,S,W

int SWORD_X = 0;
int SWORD_Y = 0;
int SWORD_DIR = 0;
char SWORD_CHAR = '^';

char GRID[ARRAY_GRIDSIZE];
char MESSAGE[50];

enum {
	CF_BLOCK = 1 << 0,
	CF_ENEMY = 1 << 1,
	CF_ITEM  = 1 << 2
};

typedef struct { int x, y, d; } PointDir;

/// credit: https://stackoverflow.com/questions/421860/capture-characters-from-standard-input-without-waiting-for-enter-to-be-pressed
char getch() {
	char buf = 0;
	struct termios old = {0};
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr ICANON");
	if (read(0, &buf, 1) < 0)
		perror ("read()");
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
		perror("tcsetattr ~ICANON");
	return (buf);
}

int coordtoindex(int x, int y)
{
	return y*(WIDTH+1)+x;
}

int printgrid(char grid[])
{
	char *tg=malloc(sizeof(char) * ARRAY_GRIDSIZE);
	memcpy(tg, grid, sizeof(char) * ARRAY_GRIDSIZE);	
	
	//add player to grid:
	int coord = coordtoindex(PLAYER_X,PLAYER_Y);
	tg[coord] = '@';

	int scoord = coordtoindex(SWORD_X,SWORD_Y);
	tg[scoord] = SWORD_CHAR;

	printf(tg);

	free(tg);
	return 0;
}

/// modulo an integer with no negatives
int modc(int in, int mod)
{
	int res = in % mod;
	if(res < 0)
		res = mod + res;

	return res;
}

int setgridch(char *grid, char ch, int x, int y)
{
	int mx = modc(x, WIDTH);
	int my = modc(y, HEIGHT);
	grid[coordtoindex(mx,my)]=ch;
	return 0;
}

char getgridch(char *grid, int x, int y)
{
	int mx = modc(x, WIDTH);
	int my = modc(y, HEIGHT);
	return grid[coordtoindex(mx,my)];
}

int initgrid(char *grid, int W, int H)
{
	int i = 0;
	for(int y = 0; y < H; y++)
	{
		for(int x = 0; x < W; x++)
		{
			i = (y*(W+1))+x;
			grid[i]='.';
		}
		i = (y*(W+1))+W;
		grid[i]='\n';
	}
	setgridch(grid, '#', 2, 2);
	setgridch(grid, 'G', 4, 4);
	return 0;
}

int setmessage(char *message)
{
	snprintf(MESSAGE, sizeof(MESSAGE), "%s", message);
	return 0;
}

int collision(char *grid, int x, int y, int d)
{
	char c = getgridch(grid, x, y);
	int res = 0;

	res |= (c == '#') ? CF_BLOCK : 0;
	res |= (isupper(c)) ? (CF_ENEMY | CF_BLOCK) : 0;
	
	return res;
}

PointDir getswordpos(int x, int y, int dir)
{
	int dx = 0;
	int dy = 0;
	PointDir p;
	p.x = x;
	p.y = y;

	if(dir == 0)
	{
		dy = -1;
	}
	else if(dir == 1)
	{
		dx = 1;
	}
	else if (dir == 2)
	{
		dy = 1;
	}
	else if(dir == 3)
	{
		dx = -1;
	}
	p.x += dx;
	p.y += dy;
	p.d = dir;

	return p;
}

int updateswordchar(int dir)
{
	if(dir == 0)
	{
		SWORD_CHAR='^';
	}
	else if(dir == 1)
	{
		SWORD_CHAR='>';
	}
	else if (dir == 2)
	{
		SWORD_CHAR='v';
	}
	else if(dir == 3)
	{
		SWORD_CHAR='<';
	}
	return 0;
}

int process(char input)
{
	int dx=0;
	int dy=0;
	int tpd=PLAYER_DIR;
	if(input == 'q')
	{
		EXIT_FLAG = 1;
	}

	if(input == 'h')
	{
		dx = -1;
		tpd = 3;
	}	
	else if(input == 'j')
	{
		dy = -1;
		tpd = 0;
	}	
	else if(input == 'k')
	{
		dy = 1;
		tpd = 2;
	}	
	else if(input == 'l')
	{
		dx = 1;
		tpd = 1;
	}
		
	int tpx = modc(PLAYER_X + dx, WIDTH);
	int tpy = modc(PLAYER_Y + dy, HEIGHT);

	int colres = collision(GRID, tpx,tpy,tpd);

	setmessage("...");
	
	bool validMove = true;

	if (colres & CF_BLOCK)
	{
		setmessage("Blocked!");
		validMove = false;
	}

	if (colres & CF_ENEMY)
	{
		setmessage("Owch!");
		validMove = false;
	}

	//sword collision
	PointDir swordpos = getswordpos(tpx,tpy,tpd);
	int scolres = collision(GRID, swordpos.x, swordpos.y, swordpos.d);
	if(scolres & CF_BLOCK)
	{
		validMove = false;
	}

	if(validMove)
	{
		PLAYER_X = tpx;
		PLAYER_Y = tpy;
		PLAYER_DIR = tpd;
		SWORD_X = swordpos.x;
		SWORD_Y = swordpos.y;
		SWORD_DIR = swordpos.d;
		updateswordchar(SWORD_DIR);
	}

	return 0;
}

int main(void)
{
	char input;
	initgrid(GRID, WIDTH, HEIGHT);

	while(!EXIT_FLAG)
	{
		system("clear");

		printgrid(GRID);
		printf("%s\n", MESSAGE);
		printf("You pressed: %c \n", input);
		input = getch();
		process(input);
	}

	system("clear");
	printf("Game Over!\n");
	return 0;
}
