/**
 * Meesterproef
 *
 * Applicatie ter afronding van het vak 'Computer Graphics'
 * Academie voor ICT & Management, Avans Hogeschool
 *
 * maart/april 2004
 * Stefan Verhoeff (VTI7-A4)
 */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <time.h>
#include <math.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef LINUX
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <string.h>

Display *dpy;
Window   win;
static Cursor pointer = None;
#endif

#define PADDLE1		1
#define PADDLE2		2
#define BAL			3
#define PLAYFIELD	4
#define LIGHT		5

// data structuren voor
// paddle en bal

typedef struct {
	GLfloat position[3]; // x, y, z
	GLfloat size[3]; // width, height, thickness
	GLfloat color[3]; // red, green, blue
	GLint score;
} paddle_t;

typedef struct {
	GLfloat position[3]; // x, y, z
	GLfloat orientation[3]; // rotation just for fun
	GLfloat delta[6]; // dx, dy, dz
	GLfloat radius;
	GLfloat color[3]; // red, green, blue
} ball_t;

// hoekpunten nummeren en coordinaten vastleggen
static GLfloat vertices[][3] =	{
				{-1.0,-1.0,-1.0},{1.0,-1.0,-1.0},
				{1.0,1.0,-1.0},{-1.0,1.0,-1.0},
				{-1.0,-1.0,1.0},{1.0,-1.0,1.0},
				{1.0,1.0,1.0},{-1.0,1.0,1.0}
				};

static GLfloat eye[] = {0.0, 0.0, 7.0};
static GLfloat schil[] = {15.0, 85.0, 0.0};

static paddle_t paddle1 = {{-2.0, 0.0, 0.0}, {0.3, 0.3, 0.05}, {0.0, 1.0, 0.0}, 0};
static paddle_t paddle2 = {{2.0, 0.0, -.0}, {0.3, 0.3, 0.05}, {0.0, 0.0, 1.0}, 0};
static paddle_t *winner = NULL;
static ball_t bal = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.02, 0.006, 0.01, 3.14, 14.3, 31.4}, 0.05, {1.0, 1.0, 1.0}};
static GLfloat rocketRide = 0.0;

static char paused = 0;
static char wireframe = 0;
static char alpha = 1;
static char light = 1;
static int frameInterval = (CLOCKS_PER_SEC / 100);

GLfloat rnd()
{
	return (rand() % 255) / 255.0;
}

void relaunchBal()
{
	if (winner == &paddle1) {
		bal.position[0] = paddle1.position[0] + paddle1.size[2];
		bal.position[1] = paddle1.position[1];
		bal.position[2] = paddle1.position[2];
		
		bal.delta[0] = rnd() / 20.0 + 0.005;
		bal.delta[1] = rnd() / 20.0 + 0.005;
		bal.delta[2] = rnd() / 20.0 + 0.005;

		if (bal.delta[0] < 0.0)
			bal.delta[0] = -bal.delta[0];
		if (rand() & 1)
			bal.delta[1] = -bal.delta[1];
		if (rand() & 1)
			bal.delta[2] = -bal.delta[2];
	} else {
		bal.position[0] = paddle2.position[0] - paddle2.size[2];
		bal.position[1] = paddle2.position[1];
		bal.position[2] = paddle2.position[2];
		
		bal.delta[0] = rnd() / 20.0 + 0.01;
		bal.delta[1] = rnd() / 20.0 + 0.005;
		bal.delta[2] = rnd() / 20.0 + 0.005;

		if (bal.delta[0] > 0.0)
			bal.delta[0] = -bal.delta[0];
		if (rand() & 1)
			bal.delta[1] = -bal.delta[1];
		if (rand() & 1)
			bal.delta[2] = -bal.delta[2];
	}
	
	bal.color[0] = 1.0;
	bal.color[1] = 1.0;
	bal.color[2] = 1.0;
}

void opponentMove()
{
	// far leet AI stuff
	static GLfloat dy = 0.03, dz = 0.03;

	if (bal.delta[0] < 0.0)
		return;
	
	if (paddle2.position[1] < bal.position[1] && paddle2.position[1] + paddle2.size[1] + dy <= 1.0)
		paddle2.position[1] += dy;
	else if (paddle2.position[1] > bal.position[1] && paddle2.position[1] - paddle2.size[1] - dy >= -1.0)
		paddle2.position[1] -= dy;
	
	if (paddle2.position[2] < bal.position[2] && paddle2.position[2] + paddle2.size[0] + dz <= 1.0)
		paddle2.position[2] += dz;
	else if (paddle2.position[2] > bal.position[2] && paddle2.position[2] - paddle2.size[0] - dz >= -1.0)
		paddle2.position[2] -= dz;
}

void moveBal(void)
{
	static clock_t nextTime = 0;
	clock_t currentTime = clock();

	if (nextTime >= currentTime || paused)
		return;

	opponentMove();

	bal.position[0] += bal.delta[0];
	bal.position[1] += bal.delta[1];
	bal.position[2] += bal.delta[2];

	// Bounce
	if (bal.position[1] >= 1.0 - bal.radius || bal.position[1] <= -1.0 + bal.radius)
		bal.delta[1] = -bal.delta[1];
	if (bal.position[2] >= 1.0 - bal.radius || bal.position[2] <= -1.0 + bal.radius)
		bal.delta[2] = -bal.delta[2];

	// Lame collision detection for the paddles
	// The ball bounced on the paddle
	if (bal.position[0] < -2.0 + bal.radius + paddle1.size[2]
		&& bal.delta[0] < 0.0
		&& bal.position[1] >= paddle1.position[1] - paddle1.size[1]
		&& bal.position[1] <= paddle1.position[1] + paddle1.size[1]
		&& bal.position[2] >= paddle1.position[2] - paddle1.size[0]
		&& bal.position[2] <= paddle1.position[2] + paddle1.size[0]
	) {
		// paddle1 bounces ball
		bal.delta[0] = -bal.delta[0];

		bal.color[0] = paddle1.color[0];
		bal.color[1] = paddle1.color[1];
		bal.color[2] = paddle1.color[2];

		if (bal.position[1] > paddle1.position[1] + paddle1.size[1] / 2.0
			&& bal.position[1] <= paddle1.position[1] + paddle1.size[1] / 2.0
			&& bal.position[2] >= paddle1.position[2] - paddle1.size[0] / 2.0
	                && bal.position[2] <= paddle1.position[2] + paddle1.size[0] / 2.0) {
			// Accelerate forward
			bal.delta[0] *= 1.5;
		} else {
			// Accelerate sideways
			if (bal.delta[1] > 0.0) {
				if (bal.position[1] > paddle1.position[1])
					bal.delta[1] += 0.005;
				else
					bal.delta[1] = -bal.delta[1] + 0.005;
			} else {
				if (bal.position[1] < paddle1.position[1])
					bal.delta[1] -= 0.005;
				else
					bal.delta[1] = -bal.delta[1] - 0.005;
			}
			
			if (bal.delta[2] > 0.0) {
				if (bal.position[2] > paddle1.position[2])
					bal.delta[2] += 0.005;
				else
					bal.delta[2] = -bal.delta[2] + 0.005;
			} else {
				if (bal.position[2] < paddle1.position[2])
					bal.delta[2] -= 0.005;
				else
					bal.delta[2] = -bal.delta[2] - 0.005;
			}
		}
	}

	// Copy 'n paste evilness
	if (bal.position[0] > 2.0 - bal.radius - paddle2.size[2]
		&& bal.delta[0] > 0.0
		&& bal.position[1] >= paddle2.position[1] - paddle2.size[1]
		&& bal.position[1] <= paddle2.position[1] + paddle2.size[1]
		&& bal.position[2] >= paddle2.position[2] - paddle2.size[0]
		&& bal.position[2] <= paddle2.position[2] + paddle2.size[0]
	) {
		// paddle2 bounces ball
		bal.delta[0] = -bal.delta[0];

		bal.color[0] = paddle2.color[0];
		bal.color[1] = paddle2.color[1];
		bal.color[2] = paddle2.color[2];

		if (bal.position[1] > paddle2.position[1] + paddle2.size[1] / 2.0
			&& bal.position[1] <= paddle2.position[1] + paddle2.size[1] / 2.0
			&& bal.position[2] >= paddle2.position[2] - paddle2.size[0] / 2.0
	                && bal.position[2] <= paddle2.position[2] + paddle2.size[0] / 2.0) {
			// Accelerate forward
			bal.delta[0] *= 1.5;
		} else {
			// Accelerate sideways
			if (bal.delta[1] > 0.0) {
				if (bal.position[1] > paddle2.position[1])
					bal.delta[1] += 0.005;
				else
					bal.delta[1] = -bal.delta[1] + 0.005;
			} else {
				if (bal.position[1] < paddle2.position[1])
					bal.delta[1] -= 0.005;
				else
					bal.delta[1] = -bal.delta[1] - 0.005;
			}
			
			if (bal.delta[2] > 0.0) {
				if (bal.position[2] > paddle2.position[2])
					bal.delta[2] += 0.005;
				else
					bal.delta[2] = -bal.delta[2] + 0.005;
			} else {
				if (bal.position[2] < paddle2.position[2])
					bal.delta[2] -= 0.005;
				else
					bal.delta[2] = -bal.delta[2] - 0.005;
			}
		}
	}
	

	// So the ball was not bounced
	if (bal.position[0] > 2.0 - bal.radius) {
		winner = &paddle1;
		paddle1.score++;
		relaunchBal();
	}
	else if (bal.position[0] < -2.0 + bal.radius) {
		winner = &paddle2;
		paddle2.score++;
		relaunchBal();
	}
	
	// Normalize
	if (bal.position[1] > 1.0 - bal.radius)
		bal.position[1] = 1.0 - bal.radius;
	else if (bal.position[1] < -1.0 + bal.radius)
		bal.position[1] = -1.0 + bal.radius;
	if (bal.position[2] > 1.0 - bal.radius)
		bal.position[2] = 1.0 - bal.radius;
	else if (bal.position[2] < -1.0 + bal.radius)
		bal.position[2] = -1.0 + bal.radius;

	// it's a spinning ball
	bal.orientation[0] += bal.delta[3];
	bal.orientation[1] += bal.delta[4];
	bal.orientation[2] += bal.delta[5];
	
	// rocket moving thingy
	rocketRide -= 0.2;
	
	// set frequency to 100hz
	nextTime = currentTime + frameInterval;
	glutPostRedisplay();
}

void text(GLint x, GLint y, char *text)
{
	char *s;
	glRasterPos2i(x, y);
	for (s = text; *s; s++)
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s);
}

void polygon(int a, int b, int c , int d)
// bepaalt het polygon met hoekpunten a, b, c en d.
{
	glBegin(GL_POLYGON);
		glVertex3fv(vertices[a]);
		glVertex3fv(vertices[b]);
		glVertex3fv(vertices[c]);
		glVertex3fv(vertices[d]);
	glEnd();
}

void cube()
{
	polygon(0,3,2,1);
	polygon(2,3,7,6);
	polygon(0,4,7,3);
	polygon(1,2,6,5);
	polygon(4,5,6,7);
	polygon(0,1,5,4);
}

void colorcube()
{
	// elk zijvlak heeft een andere kleur. 
	// De 8 hoekpunten zijn genummerd: 0..7. 

    glColor3ub(255,0,0); 
	polygon(0,3,2,1);
	
    glColor3ub(0,255,0); 
	polygon(2,3,7,6);
	
	glColor3ub(0,0,255); 
	polygon(0,4,7,3);

	glColor3ub(0,255,255); 
	polygon(1,2,6,5);

	glColor3ub(255,0,255); 
	polygon(4,5,6,7);

	glColor3ub(255,255,0); 
	polygon(0,1,5,4);
}

void kegel()
{
	GLfloat x,y,z,phi,r, thet;
	// c is omreken factor van graden naar radialen
    double c = 3.14159/180.0;

	x = y = 0.0;
	z = 1.0;
	r = 1.0;

	// teken onderkant
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(x, y, z);

		for (phi = 0.0; phi <= 360; phi += 20.0) {
			x = r * cos(c*phi);
			y = r * sin(c*phi);
			glVertex3f(x, y, z);
		}
	glEnd();

	x = y = 0.0;

	// teken punt
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(x, y, -z);

		for (phi = 0.0; phi <= 360; phi += 20.0) {
			x = r * cos(c*phi);
			y = r * sin(c*phi);
			glVertex3f(x, y, z);
		}
	glEnd();

	thet = 2.0 / 0.70;
	for (z = -1.0; z <= 1.0; z += 0.1) {
		glBegin(GL_QUAD_STRIP);
			r = (z+1.0) / thet;

			for (phi = 0.0; phi <= 360; phi += 20.0) {
				x = r * cos(c*phi);
				y = r * sin(c*phi);
				glVertex3f(x, y, z);

				x = r * cos(c*(phi+20.0));
				y = r * sin(c*(phi+20.0));
				glVertex3f(x, y, z);
			}
		glEnd();
	}
}

void cilinder()
{
	GLfloat x,y,z,phi;
	// c is omreken factor van graden naar radialen
    double c = 3.14159/180.0;

	x = y = 0.0;
	z = 1.0;

	// teken deksels
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(x, y, z);

		for (phi = 0.0; phi <= 360; phi += 20.0) {
			x = cos(c*phi);
			y = sin(c*phi);
			glVertex3f(x, y, z);
		}
	glEnd();

	x = y = 0.0;
	z = -1.0;

	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(x, y, z);

		for (phi = 0.0; phi <= 360; phi += 20.0) {
			x = cos(c*phi);
			y = sin(c*phi);
			glVertex3f(x, y, z);
		}
	glEnd();

	// teken omtrek
	for (z = -1.0; z <= 0.8; z += 0.20) {
		glBegin(GL_QUAD_STRIP);
			for (phi = 0.0; phi < 360; phi += 20.0) {
				x = cos(c*phi);
				y = sin(c*phi);
				glVertex3f(x, y, z);

				x = cos(c*(phi+20.0));
				y = sin(c*(phi+20.0));
				glVertex3f(x, y, z);

				x = cos(c*phi);
				y = sin(c*phi);
				glVertex3f(x, y, z+0.20);

				x = cos(c*(phi+20.0));
				y = sin(c*(phi+20.0));
				glVertex3f(x, y, z+0.20);
			}
		glEnd();
	}
}

void assenstelsel()
{
	glBegin(GL_LINES);
		glColor3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(100.0, 0.0, 0.0);

		glColor3f(0.0, 1.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 100.0, 0.0);

		glColor3f(0.0, 0.0, 1.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, 100.0);
	glEnd();

	glLineStipple(1, 0xF00F);	// geef stippeling dmv bitpartoon
	glEnable(GL_LINE_STIPPLE);
	glBegin(GL_LINES);
		glColor3f(1.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(-100.0, 0.0, 0.0);

		glColor3f(0.0, 1.0, 0.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, -100.0, 0.0);

		glColor3f(0.0, 0.0, 1.0);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, -100.0);
	glEnd();
	glDisable(GL_LINE_STIPPLE);
}

void drawWall()
{
	glBegin(GL_QUADS);
		// bottom
		glNormal3f(0.0, 1.0, 0.0);
		glVertex3f(-1.0, -1.0, 1.0);
		glVertex3f(1.0, -1.0, 1.0);
		glVertex3f(1.0, -1.0, -1.0);
		glVertex3f(-1.0, -1.0, -1.0);

		// top
		glNormal3f(0.0, -1.0, 0.0);
		glVertex3f(-1.0, 1.0, 1.0);
		glVertex3f(1.0, 1.0, 1.0);
		glVertex3f(1.0, 1.0, -1.0);
		glVertex3f(-1.0, 1.0, -1.0);

		// near-side
		glNormal3f(0.0, 0.0, 1.0);
		glVertex3f(-1.0, 1.0, 1.0);
		glVertex3f(1.0, 1.0, 1.0);
		glVertex3f(1.0, -1.0, 1.0);
		glVertex3f(-1.0, -1.0, 1.0);

		// far-side
		glNormal3f(0.0, 0.0, -1.0);
		glVertex3f(-1.0, 1.0, -1.0);
		glVertex3f(1.0, 1.0, -1.0);
		glVertex3f(1.0, -1.0, -1.0);
		glVertex3f(-1.0, -1.0, -1.0);
	glEnd();
}

void drawPaddle1()
{
	// middle
	glPushMatrix();
		glScalef(1.0, 0.5, 0.5);
		cube();
	glPopMatrix();

	// horizontal cross
	glPushMatrix();
		glScalef(0.5, 0.3, 1.0);
		cube();
	glPopMatrix();

	// vertical cross
	glPushMatrix();
		glScalef(0.5, 1.0, 0.3);
		cube();
	glPopMatrix();
}

void drawPaddle2()
{
	// middle
	glPushMatrix();
		glScalef(1.0, 0.75, 0.75);
		cube();
	glPopMatrix();

	// side cilinders
	glPushMatrix();
		glRotatef(90.0, 1.0, 0.0, 0.0);
		glScalef(1.0, 0.15, 1.0);
		glTranslatef(0.0, 5.0, 0.0);
		cilinder();

		glTranslatef(0.0, -10.0, 0.0);
		cilinder();
	glPopMatrix();
}

void drawBal()
{
	// left bal
	glPushMatrix();
		glTranslatef(-2.0, 0.0, 0.0);
		glutSolidSphere(1.0, 6, 6);
	glPopMatrix();

	// connecting cilinder
	glPushMatrix();
		glRotatef(90.0, 0.0, 1.0, 0.0);
		glScalef(0.3, 0.3, 2.0);
		cilinder();
	glPopMatrix();
	
	// right bal
	glPushMatrix();
		glTranslatef(2.0, 0.0, 0.0);
		glutSolidSphere(1.0, 6, 6);
	glPopMatrix();
}

void drawLight()
{
	glPushMatrix();
		glScalef(2.0, 2.0, 1.0);
		kegel();
	glPopMatrix();
}

void drawRocket()
{
	// base
	glPushMatrix();
		glRotatef(90.0, 1.0, 0.0, 0.0);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glScalef(1.0, 1.0, 2.0);
		cilinder();
	glPopMatrix();

	// cone
	glPushMatrix();
		glRotatef(90.0, 1.0, 0.0, 0.0);
		glTranslatef(0.0, 0.0, -3.0);
		glColor4f(1.0, 0.0, 0.0, 1.0);
		kegel();
	glPopMatrix();

	// wings
	glPushMatrix();
		glColor3f(0.5, 0.5, 0.5);
		glScalef(2.0, 0.7, 0.2);
		glTranslatef(0.0, -1.5, 0.0);
		cube();
	glPopMatrix();

	glPushMatrix();
		glColor3f(0.5, 0.5, 0.5);
		glScalef(0.2, 0.7, 2.0);
		glTranslatef(0.0, -1.5, 0.0);
		cube();
	glPopMatrix();
}

void display(void)
{
	static char buffer[20];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1.0, 1.0, 1.0);

	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (light)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);

	if (alpha)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	glPushMatrix();
		glLoadIdentity();
		glTranslatef(-eye[0], -eye[1], -eye[2]);

		// Lighting
		glPushMatrix();
		{
			GLfloat LightPosition[] = { 0.0, 0.0, 0.0, 1.0f };
			glTranslatef(bal.position[0], bal.position[1], bal.position[2]);
			glLightfv(GL_LIGHT0, GL_POSITION,LightPosition);
		}
		glPopMatrix();

		// licht uit voor score
			glDisable(GL_LIGHTING);
			// player 1 score
			glColor3f(0.0, 1.0, 0.0);
			sprintf(buffer, "%d", paddle1.score);
			text(-2, 2, buffer);

			// player 2 score
			glColor3f(0.0, 0.0, 1.0);
			sprintf(buffer, "%d", paddle2.score);
			text(2, 2, buffer);
		// licht aan
		if (light)
			glEnable(GL_LIGHTING);

		glRotatef(schil[0], 1.0, 0.0, 0.0);
		glRotatef(schil[1], 0.0, 1.0, 0.0);
		glRotatef(schil[2], 0.0, 0.0, 1.0);

		// the ball
		glColor3f(bal.color[0], bal.color[1], bal.color[2]);
		glPushMatrix();
			glTranslatef(bal.position[0], bal.position[1], bal.position[2]);
			glScalef(bal.radius, bal.radius, bal.radius);
			glRotatef(bal.orientation[0], 1.0, 0.0, 0.0);
			glRotatef(bal.orientation[0], 0.0, 1.0, 0.0);
			glRotatef(bal.orientation[0], 0.0, 0.0, 1.0);
			drawBal();
		glPopMatrix();

		// opponent paddle
		glColor4f(paddle2.color[0], paddle2.color[1], paddle2.color[2], 0.5);
		glPushMatrix();
			glTranslatef(paddle2.position[0] - paddle2.size[2], paddle2.position[1], paddle2.position[2]);
			glScalef(paddle2.size[2], paddle2.size[1], paddle2.size[0]);
			drawPaddle2();
		glPopMatrix();

		// player paddle
		glColor4f(paddle1.color[0], paddle1.color[1], paddle1.color[2], 0.5);
		glPushMatrix();
			glTranslatef(paddle1.position[0] + paddle2.size[2], paddle1.position[1], paddle1.position[2]);
			glScalef(paddle1.size[2], paddle1.size[1], paddle1.size[0]);
			drawPaddle1();
		glPopMatrix();

		// playing area
		glColor4f(1.0, 0.0, 0.0, 0.3);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glPushMatrix();
			// object bestaat uit 2 balken
			glTranslatef(-1.0, 0.0, 0.0);
			drawWall();
			glTranslatef(2.0, 0.0, 0.0);
			drawWall();
		glPopMatrix();

		glColor3f(1.0, 1.0, 1.0);
		glPushMatrix();
			glRotatef(rocketRide, 1.0, 0.0, 0.5);
			glTranslatef(0.0, 0.0, 5.0);
			glScalef(0.2, 0.2, 0.2);
			drawRocket();
		glPopMatrix();
	glPopMatrix();

	glFlush();
	glutSwapBuffers();
}

void mouseMove(int x, int y)
{
	static GLint oldX = 0;
	static GLint oldY = 0;

	if (paused)
		return;

	// naar boven
	if (oldY > y && paddle1.position[1] < 1.0 - paddle1.size[1] - 0.01)
		paddle1.position[1] += 0.05;
	// naar beneden
	if (oldY < y && paddle1.position[1] > -1.0 + paddle1.size[1] + 0.01)
		paddle1.position[1] -= 0.05;

	// naar rechts
	if (oldX < x && paddle1.position[2] < 1.0 - paddle1.size[0] - 0.01)
		paddle1.position[2] += 0.05;
	// naar links
	if (oldX > x && paddle1.position[2] > -1.0 + paddle1.size[0] + 0.01)
		paddle1.position[2] -= 0.05;

	oldX = x;
	oldY = y;

	glutPostRedisplay();
}

void mouse(int btn, int state, int x, int y)
{
	if (btn == GLUT_LEFT_BUTTON) {
	} else if (btn == GLUT_MIDDLE_BUTTON) {
	} else if (btn == GLUT_RIGHT_BUTTON) {
	}

	glutPostRedisplay();
}

void special(int value, int x, int y)
{
    switch (value)
    {
        case GLUT_KEY_F1:
			glutFullScreen();
			break;
		case GLUT_KEY_F2:
			glutPositionWindow(100, 100);
			glutReshapeWindow(500, 500);
			break;
		case GLUT_KEY_UP:
			schil[0] -= 3.0;
			break;
		case GLUT_KEY_DOWN:
			schil[0] += 3.0;
			break;
		case GLUT_KEY_RIGHT:
			schil[1] += 3.0;
			break;
		case GLUT_KEY_LEFT:
			schil[1] -= 3.0;
			break;

    }

	glutPostRedisplay();
}

void keys(unsigned char key, int x, int y)
{
	switch (key) {
		case ' ': // pause toggle
			paused = !paused;
			break;
		case '+': // increase speed
			frameInterval /= 2;
			printf("Interval %d\n", frameInterval);
			break;
		case '-': // decrease speed
			frameInterval *= 2;
			if (frameInterval == 0)
				frameInterval = 1;
			printf("Interval %d\n", frameInterval);
			break;
		case 'w': // wireframe toggle
			wireframe = !wireframe;
			break;
		case 'a': // alpha-blending toggle
			alpha = !alpha;
			break;
		case 'l': // light toggle
			light = !light;
			break;
		case 27: // ESCAPE key
			exit(0);
			break;
		case 'r':
			eye[2] -= 1.0;
			break;
		case 'R':
			eye[2] += 1.0;
			break;
		case 'i':
			relaunchBal();
			break;
		case '1': // restore original position
			eye[0] = 0.0;
			eye[1] = 0.0;
			eye[2] = 7.0;
			schil[0] = 15.0;
			schil[1] = 85.0;
			schil[2] = 0.0;
			break;
		case '2': // backview
			eye[0] = 0.0;
			eye[1] = 0.0;
			eye[2] = 7.0;
			schil[0] = 0.0;
			schil[1] = 90.0;
			schil[2] = 0.0;
			break;
		case '3': // topview
			eye[0] = 0.0;
			eye[1] = 0.0;
			eye[2] = 7.0;
			schil[0] = 90.0;
			schil[1] = 90.0;
			schil[2] = 0.0;
			break;
		case '4': // reverse backview (crazy)
			eye[0] = 0.0;
			eye[1] = 0.0;
			eye[2] = 7.0;
			schil[0] = 0.0;
			schil[1] = -90.0;
			schil[2] = 0.0;
			break;
		case '5': // bottomview
			eye[0] = 0.0;
			eye[1] = 0.0;
			eye[2] = 7.0;
			schil[0] = -90.0;
			schil[1] = 90.0;
			schil[2] = 0.0;
			break;
		default:
			break;
	}

   glutPostRedisplay();
}

void myReshape(int w, int h)
{
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION); 
	glLoadIdentity();

	gluPerspective(45.0, (float)w/(float)h, 1.0, 300.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void disablePointer()
{
#ifdef WIN32
	ShowCursor(FALSE);
#endif

#ifdef LINUX
  if ( None == pointer ) {
	  char bm[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	  Pixmap pix = XCreateBitmapFromData( dpy, win, bm, 8, 8 );
	  XColor black;
	  memset( &black, 0, sizeof( XColor ) );
	  black.flags = DoRed | DoGreen | DoBlue;
	  pointer = XCreatePixmapCursor( dpy, pix, pix, &black, &black, 0, 0 );
	  XFreePixmap( dpy, pix );
  }
  XDefineCursor( dpy, win, pointer );
  XSync( dpy, False ); /* again, optional */
#endif
}

void initGL()
{
	GLfloat LightAmbient[]  = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat LightDiffuse[]  = { 0.5f, 0.5f, 0.5f, 0.5f };
	GLfloat LightPosition[] = { -eye[0], -eye[1] + 2.0, -eye[2], 1.0f };

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);

	// For the light
	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

	// create the window
	glutInitWindowSize(500, 500);
	glutCreateWindow("CGR - Meesterstuk van Stefan Verhoeff (VTI7-A3)");

	// misc initialisation
	initGL();
	srand( time(NULL) );
	relaunchBal(); // initiate the ball position
	disablePointer();

	// callbacks
	glutReshapeFunc(myReshape);
	glutDisplayFunc(display);
	glutIdleFunc(moveBal);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMove);
	glutPassiveMotionFunc(mouseMove);
	glutKeyboardFunc(keys);
	glutSpecialFunc(special);

	glutMainLoop();
	return 0;
}