//	The OpenGL/FreeGLUT modelling of a gear transmission with arbitrary wheel parameters.
//	Teethed gear parameters are set up by constants.
//
//	The provided sample program draws axes, change the color of the axes and creates menu.
//
//	The left mouse button does rotation
//	The middle mouse button does scaling
//	The user interface allows:
//		1. The axes to be turned on and off
//		2. The color of the axes to be changed
//		3. Debugging to be turned on and off
//		4. Depth cueing to be turned on and off
//		5. The projection to be changed
//		6. The transformations to be reset
//		7. The program to quit
//
//	The "glslprogram.cpp" program, provided by Professor Mike Bailey, handles the shaders infrastructure.
#include "glslprogram.cpp"

// window title:
const char* WINDOWTITLE = "CS 550 Final Project -- Evgeny Ovechnikov";

// what the glui package defines as true and false:
const int GLUITRUE = true;
const int GLUIFALSE = false;

// the escape key:
const int ESCAPE = 0x1b;

// initial window size:
const int INIT_WINDOW_SIZE = 1024;

// size of the 3d box to be drawn:
const float BOXSIZE = 2.f;

// multiplication factors for input interaction:
//  (these are known from previous experience)
const float ANGFACT = 1.f;
const float SCLFACT = 0.005f;

// minimum allowable scale factor:
const float MINSCALE = 0.05f;

// scroll wheel button values:
const int SCROLL_WHEEL_UP = 3;
const int SCROLL_WHEEL_DOWN = 4;

// equivalent mouse movement when we click the scroll wheel:
const float SCROLL_WHEEL_CLICK_FACTOR = 5.f;

// active mouse buttons (or them together):
const int LEFT = 4;
const int MIDDLE = 2;
const int RIGHT = 1;

// which projection:
enum Projections
{
	ORTHO,
	PERSP
};

// which button:
enum ButtonVals
{
	RESET,
	QUIT
};

// window background color (rgba):
const GLfloat BACKCOLOR[] = { 0., 0., 0., 1. };

// line width for the axes:
const GLfloat AXES_WIDTH = 3.;

// the color numbers:
// this order must match the radio button order, which must match the order of the color names,
// 	which must match the order of the color RGB values
enum Colors
{
	RED,
	YELLOW,
	GREEN,
	CYAN,
	BLUE,
	MAGENTA,
	WHITE,
	BLACK
};

char* ColorNames[] =
{
	(char*)"Red",
	(char*)"Yellow",
	(char*)"Green",
	(char*)"Cyan",
	(char*)"Blue",
	(char*)"Magenta",
	(char*)"White",
	(char*)"Black"
};

// the color definitions:
// this order must match the menu order
const GLfloat Colors[][3] =
{
	{ 1., 0., 0. },		// red
	{ 1., 1., 0. },		// yellow
	{ 0., 1., 0. },		// green
	{ 0., 1., 1. },		// cyan
	{ 0., 0., 1. },		// blue
	{ 1., 0., 1. },		// magenta
	{ 1., 1., 1. },		// white
	{ 0., 0., 0. },		// black
};

// fog parameters:
const GLfloat FOGCOLOR[4] = { .0f, .0f, .0f, 1.f };
const GLenum  FOGMODE = GL_LINEAR;
const GLfloat FOGDENSITY = 0.30f;
const GLfloat FOGSTART = 1.5f;
const GLfloat FOGEND = 4.f;

// non-constant global variables:
int		ActiveButton;			// current button that is down
GLuint	AxesList;				// list to hold the axes
bool	AxesOn;					// != 0 means to draw the axes
int		DebugOn;				// != 0 means to print debugging info
int		DepthCueOn;				// != 0 means to use intensity depth cueing
int		DepthBufferOn;			// != 0 means to use the z-buffer
int		DepthFightingOn;		// != 0 means to force the creation of z-fighting
int		MainWindow;				// window id for main graphics window
float	Scale;					// scaling factor
int		WhichColor;				// index into Colors[ ]
int		WhichProjection;		// ORTHO or PERSP
int		Xmouse, Ymouse;			// mouse values
float	Xrot, Yrot;				// rotation angles in degrees

// Added
float	White[] = { 1., 1., 1., 1. };
bool	Freeze = 0;
bool	Light0On = false;
float	angle = 0;
float	Time;
bool	ControlLinesAreShown = false;

GLuint	Gear1List;				// Gear 1 display list
GLuint	Gear2List;				// Gear 2 display list
GLuint	DebugLinesList;			// Debugging Lines display list

#define MS_PER_CYCLE	500000
#define MS_SPEED		400

// Gear transmission input parameters
#define GEAR_NUMTEETH1	23
#define GEAR_NUMTEETH2	47
#define GEAR_RADIUS1	10.
#define GEAR_TEETH_HGT	2.
#define GEAR_THICKNESS	2.
#define GEAR_ARMS1		3
#define GEAR_ARMS2		5

#define GEAR_DUMMY_COEFFICIENT	0.35
#define GEAR_GLOBAL_TOLERANCE	0.0000001

#define GEAR_POLYGONS	20

// Parameters from Project 4
#define ORBIT_HEIGHT	24.
#define ORBIT_SLOWDOWN	4.

// Added for Shaders
GLSLProgram* Pattern;
bool IsCorroded = false;

// function prototypes:
void	Animate();
void	Display();
void	DoAxesMenu(int);
void	DoColorMenu(int);
void	DoDepthBufferMenu(int);
void	DoDepthFightingMenu(int);
void	DoDebugMenu(int);
void	DoDepthMenu(int);
void	DoMainMenu(int);
void	DoProjectMenu(int);
void	DoRasterString(float, float, float, char*);
void	DoStrokeString(float, float, float, float, char*);
float	ElapsedSeconds();
void	InitGraphics();
void	InitLists();
void	InitMenus();
void	Keyboard(unsigned char, int, int);
void	MouseButton(int, int, int, int);
void	MouseMotion(int, int);
void	Reset();
void	Resize(int, int);
void	Visibility(int);
void	Axes(float);

struct point
{
	float x, y, z;		// coordinates
	float nx, ny, nz;	// surface normal
	float s, t;			// texture coords
};

inline
void
DrawPoint(struct point* p)
{
	glNormal3fv(&p->nx);
	glTexCoord2fv(&p->s);
	glVertex3fv(&p->x);
}

// a multiplier and an array utility to create an array from:
float*
MulArray3(float factor, float array0[3])
{
	static float array[4];
	array[0] = factor * array0[0];
	array[1] = factor * array0[1];
	array[2] = factor * array0[2];
	array[3] = 1.;
	return array;
}

// Function to create gear display list
GLuint
CreateGearDisplayList(int gNumTeeth, float gRadius, float gTeethHeight, float gThickness, int gArms, int gPolygons, bool gCorrosion) {
	point* contactPoints = new point[gPolygons + 1];
	point* smallCirclePoints = new point[gPolygons + 1];
	point* bigCirclePoints = new point[gPolygons + 1];

	// Calculating point on involute using binary search
	float pAlpha = 0;
	float qAlpha = M_PI / 2;
	float tAlpha;
	float xt;
	float yt;
	float rt;
	do {
		tAlpha = (pAlpha + qAlpha) / 2.;
		xt = gRadius * (cos(tAlpha) + tAlpha * sin(tAlpha));
		yt = gRadius * (sin(tAlpha) - tAlpha * cos(tAlpha));
		rt = sqrt(xt * xt + yt * yt);
		if (rt > gRadius + gTeethHeight) {
			qAlpha = tAlpha;
		}
		else {
			pAlpha = tAlpha;
		}
	} while (abs(rt - gRadius - gTeethHeight) > GEAR_GLOBAL_TOLERANCE);

	float contactAlpha = atan(yt / xt);
	float toothAngle = 2 * M_PI / gNumTeeth;
	if (2 * contactAlpha > toothAngle) {
		fprintf(stderr, "Incorrect Gear Parameters!\n");
		return NULL;
	}
	float thetaBig = (toothAngle - 2 * contactAlpha) * GEAR_DUMMY_COEFFICIENT;
	float thetaSmall = (toothAngle - 2 * contactAlpha) * (1 - GEAR_DUMMY_COEFFICIENT);

	for (int i = 0; i <= gPolygons; i++) {
		float c = i * tAlpha / gPolygons;
		contactPoints[i].x = gRadius * (cos(c) + c * sin(c));
		contactPoints[i].y = gRadius * (sin(c) - c * cos(c));
		contactPoints[i].z = 0.;
		contactPoints[i].nx = sin(c);
		contactPoints[i].ny = -cos(c);
		contactPoints[i].nz = 0.;
		contactPoints[i].s = gCorrosion ? rand() % 10 : 0;
	}

	for (int i = 0; i <= gPolygons; i++) {
		float cSmall = -thetaSmall / 2 + i * thetaSmall / gPolygons;
		smallCirclePoints[i].x = gRadius * cos(cSmall);
		smallCirclePoints[i].y = gRadius * sin(cSmall);
		smallCirclePoints[i].z = 0.;
		smallCirclePoints[i].nx = cos(cSmall);
		smallCirclePoints[i].ny = sin(cSmall);
		smallCirclePoints[i].nz = 0.;
		smallCirclePoints[i].s = gCorrosion ? rand() % 10 : 0;

		float cBig = thetaSmall / 2 + contactAlpha + i * thetaBig / gPolygons;
		bigCirclePoints[i].x = (gRadius + gTeethHeight) * cos(cBig);
		bigCirclePoints[i].y = (gRadius + gTeethHeight) * sin(cBig);
		bigCirclePoints[i].z = 0.;
		bigCirclePoints[i].nx = cos(cBig);
		bigCirclePoints[i].ny = sin(cBig);
		bigCirclePoints[i].nz = 0.;
		bigCirclePoints[i].s = gCorrosion ? rand() % 10 : 0;
	}

	// Gear Display List
	GLuint dList = glGenLists(1);
	glNewList(dList, GL_COMPILE);

	// Let's draw the gear in 3D
	point p0, p1;
	for (float phi = 0; phi < 360; phi += 360. / gNumTeeth) {
		glPushMatrix();
		glRotatef(phi, 0., 0., 1.);

		// Contact Surface left
		glPushMatrix();
		glRotatef(thetaSmall * 90 / M_PI, 0., 0., 1.);
		glBegin(GL_QUAD_STRIP);
		for (int i = 1; i <= gPolygons; i++) {
			DrawPoint(&contactPoints[i]);
			DrawPoint(&contactPoints[i - 1]);
			for (int j = 0; j <= gPolygons; j++) {
				p0 = contactPoints[i];
				p0.z -= j * gThickness / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1 = contactPoints[i - 1];
				p1.z -= j * gThickness / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
		}
		glEnd();
		glPopMatrix();

		// Contact Surface right
		glPushMatrix();
		glRotatef(180., 1., 0., 0.);
		glRotatef(thetaSmall * 90 / M_PI, 0., 0., 1.);
		glBegin(GL_QUAD_STRIP);
		for (int i = 1; i <= gPolygons; i++) {
			DrawPoint(&contactPoints[i]);
			DrawPoint(&contactPoints[i - 1]);
			for (int j = 0; j <= gPolygons; j++) {
				p0 = contactPoints[i];
				p0.z += j * gThickness / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1 = contactPoints[i - 1];
				p1.z += j * gThickness / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
		}
		glEnd();
		glPopMatrix();

		// Outer and Inner tooth radiuses
		glPushMatrix();
		glBegin(GL_QUAD_STRIP);
		for (int i = 1; i <= gPolygons; i++) {
			DrawPoint(&smallCirclePoints[i]);
			DrawPoint(&smallCirclePoints[i - 1]);
			for (int j = 0; j <= gPolygons; j++) {
				p0 = smallCirclePoints[i];
				p0.z -= j * gThickness / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1 = smallCirclePoints[i - 1];
				p1.z -= j * gThickness / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
		}
		glEnd();
		glBegin(GL_QUAD_STRIP);
		for (int i = 1; i <= gPolygons; i++) {
			DrawPoint(&bigCirclePoints[i]);
			DrawPoint(&bigCirclePoints[i - 1]);
			for (int j = 0; j <= gPolygons; j++) {
				p0 = bigCirclePoints[i];
				p0.z -= j * gThickness / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1 = bigCirclePoints[i - 1];
				p1.z -= j * gThickness / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
		}
		glEnd();
		glPopMatrix();

		// Filling Teeth Surfaces
		for (float zNorm = 1.; zNorm >= -1.; zNorm -= 2.) {
			glPushMatrix();
			glRotatef(thetaSmall * 90 / M_PI, 0., 0., 1.);
			glBegin(GL_QUAD_STRIP);

			for (int i = 0; i < gPolygons; i++) {	// contactPoints loop
				float tempR = sqrt(contactPoints[i].x * contactPoints[i].x + contactPoints[i].y * contactPoints[i].y);
				float tempR1 = sqrt(contactPoints[i + 1].x * contactPoints[i + 1].x + contactPoints[i + 1].y * contactPoints[i + 1].y);
				float phi0R = atan(contactPoints[i].y / contactPoints[i].x);
				float phi1R = 2 * contactAlpha + thetaBig - phi0R;
				float phi0R1 = atan(contactPoints[i + 1].y / contactPoints[i + 1].x);
				float phi1R1 = 2 * contactAlpha + thetaBig - phi0R1;

				for (int j = 0; j <= gPolygons; j++) {
					p0.x = tempR * cos(phi0R + j * (phi1R - phi0R) / gPolygons);
					p0.y = tempR * sin(phi0R + j * (phi1R - phi0R) / gPolygons);
					p0.z = (zNorm == 1.) ? 0. : -gThickness;
					p0.nx = 0.;
					p0.ny = 0.;
					p0.nz = zNorm;
					p0.s = gCorrosion ? rand() % 10 : 0;

					p1.x = tempR1 * cos(phi0R1 + j * (phi1R1 - phi0R1) / gPolygons);
					p1.y = tempR1 * sin(phi0R1 + j * (phi1R1 - phi0R1) / gPolygons);
					p1.z = (zNorm == 1.) ? 0. : -gThickness;
					p1.nx = 0.;
					p1.ny = 0.;
					p1.nz = zNorm;
					p1.s = gCorrosion ? rand() % 10 : 0;

					DrawPoint(&p0);
					DrawPoint(&p1);
				}
			}
			glEnd();
			glPopMatrix();
		}

		// Drawing rim
		for (float zNorm = 1.; zNorm >= -1.; zNorm -= 2.) {
			glPushMatrix();
			glRotatef(thetaSmall * 90 / M_PI, 0., 0., 1.);

			for (int i = 0; i < gPolygons; i++) {
				glBegin(GL_QUAD_STRIP);
				float tempR = gRadius - i * 0.2 / gPolygons * gRadius;
				float tempRNext = gRadius - (i + 1) * 0.2 / gPolygons * gRadius;
				float phi0R = 0;
				float phi1R = 2 * contactAlpha + thetaBig;

				for (int j = 0; j <= gPolygons; j++) {
					p0.x = tempR * cos(phi0R + j * (phi1R - phi0R) / gPolygons);
					p0.y = tempR * sin(phi0R + j * (phi1R - phi0R) / gPolygons);
					p0.z = (zNorm == 1.) ? 0. : -gThickness;
					p0.nx = 0.;
					p0.ny = 0.;
					p0.nz = zNorm;
					p0.s = gCorrosion ? rand() % 10 : 0;

					p1.x = tempRNext * cos(phi0R + j * (phi1R - phi0R) / gPolygons);
					p1.y = tempRNext * sin(phi0R + j * (phi1R - phi0R) / gPolygons);
					p1.z = (zNorm == 1.) ? 0. : -gThickness;
					p1.nx = 0.;
					p1.ny = 0.;
					p1.nz = zNorm;
					p1.s = gCorrosion ? rand() % 10 : 0;

					DrawPoint(&p0);
					DrawPoint(&p1);
				}
				glEnd();
			}

			for (int i = 0; i < gPolygons; i++) {
				glBegin(GL_QUAD_STRIP);
				float tempR = gRadius - i * 0.2 / gPolygons * gRadius;
				float tempRNext = gRadius - (i + 1) * 0.2 / gPolygons * gRadius;
				float phi0R = -thetaSmall;
				float phi1R = 0.;

				for (int j = 0; j <= gPolygons; j++) {
					p0.x = tempR * cos(phi0R + j * (phi1R - phi0R) / gPolygons);
					p0.y = tempR * sin(phi0R + j * (phi1R - phi0R) / gPolygons);
					p0.z = (zNorm == 1.) ? 0. : -gThickness;
					p0.nx = 0.;
					p0.ny = 0.;
					p0.nz = zNorm;
					p0.s = gCorrosion ? rand() % 10 : 0;

					p1.x = tempRNext * cos(phi0R + j * (phi1R - phi0R) / gPolygons);
					p1.y = tempRNext * sin(phi0R + j * (phi1R - phi0R) / gPolygons);
					p1.z = (zNorm == 1.) ? 0. : -gThickness;
					p1.nx = 0.;
					p1.ny = 0.;
					p1.nz = zNorm;
					p1.s = gCorrosion ? rand() % 10 : 0;

					DrawPoint(&p0);
					DrawPoint(&p1);
				}
				glEnd();
			}
			glPopMatrix();
		}

		// Filling inner radius, 1st part
		glPushMatrix();
		glRotatef(thetaSmall * 90 / M_PI, 0., 0., 1.);
		float tempR = 0.8 * gRadius;
		float phi0R = 0;
		float phi1R = 2 * contactAlpha + thetaBig;

		for (int i = 1; i <= gPolygons; i++) {
			p0.x = tempR * cos(phi0R + (i - 1) * (phi1R - phi0R) / gPolygons);
			p0.y = tempR * sin(phi0R + (i - 1) * (phi1R - phi0R) / gPolygons);
			p0.z = 0.;
			p0.nx = -cos(phi0R + (i - 1) * (phi1R - phi0R) / gPolygons);
			p0.ny = -sin(phi0R + (i - 1) * (phi1R - phi0R) / gPolygons);
			p0.nz = 0.;
			p0.s = gCorrosion ? rand() % 10 : 0;

			p1.x = tempR * cos(phi0R + (i) * (phi1R - phi0R) / gPolygons);
			p1.y = tempR * sin(phi0R + (i) * (phi1R - phi0R) / gPolygons);
			p1.z = 0.;
			p1.nx = -cos(phi0R + (i) * (phi1R - phi0R) / gPolygons);
			p1.ny = -sin(phi0R + (i) * (phi1R - phi0R) / gPolygons);
			p1.nz = 0.;
			p1.s = gCorrosion ? rand() % 10 : 0;

			glBegin(GL_QUAD_STRIP);
			DrawPoint(&p0);
			DrawPoint(&p1);
			for (int j = 1; j <= gPolygons; j++) {
				p0.z -= gThickness / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1.z -= gThickness / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
			glEnd();
		}
		glPopMatrix();

		// Filling inner radius, 2nd part
		glPushMatrix();
		glRotatef(thetaSmall * 90 / M_PI, 0., 0., 1.);
		phi0R = -thetaSmall;
		phi1R = 0.;

		for (int i = 1; i <= gPolygons; i++) {
			p0.x = tempR * cos(phi0R + (i - 1) * (phi1R - phi0R) / gPolygons);
			p0.y = tempR * sin(phi0R + (i - 1) * (phi1R - phi0R) / gPolygons);
			p0.z = 0.;
			p0.nx = -cos(phi0R + (i - 1) * (phi1R - phi0R) / gPolygons);
			p0.ny = -sin(phi0R + (i - 1) * (phi1R - phi0R) / gPolygons);
			p0.nz = 0.;
			p0.s = gCorrosion ? rand() % 10 : 0;

			p1.x = tempR * cos(phi0R + (i) * (phi1R - phi0R) / gPolygons);
			p1.y = tempR * sin(phi0R + (i) * (phi1R - phi0R) / gPolygons);
			p1.z = 0.;
			p1.nx = -cos(phi0R + (i) * (phi1R - phi0R) / gPolygons);
			p1.ny = -sin(phi0R + (i) * (phi1R - phi0R) / gPolygons);
			p1.nz = 0.;
			p1.s = gCorrosion ? rand() % 10 : 0;

			glBegin(GL_QUAD_STRIP);
			DrawPoint(&p0);
			DrawPoint(&p1);
			for (int j = 1; j <= gPolygons; j++) {
				p0.z -= gThickness / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1.z -= gThickness / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
			glEnd();
		}
		glPopMatrix();

		glPopMatrix();
	}

	// Drawing hob
	glPushMatrix();
	for (double phi = 1; phi <= 360; phi++) {
		// outer circle
		p0.x = gRadius * 0.2 * cos((phi - 1) * M_PI / 180);
		p0.y = gRadius * 0.2 * sin((phi - 1) * M_PI / 180);
		p0.z = 0.;
		p0.nx = cos((phi - 1) * M_PI / 180);
		p0.ny = sin((phi - 1) * M_PI / 180);
		p0.nz = 0.;
		p0.s = gCorrosion ? rand() % 10 : 0;

		p1.x = gRadius * 0.2 * cos((phi)*M_PI / 180);
		p1.y = gRadius * 0.2 * sin((phi)*M_PI / 180);
		p1.z = 0.;
		p1.nx = cos((phi)*M_PI / 180);
		p1.ny = sin((phi)*M_PI / 180);
		p1.nz = 0.;
		p1.s = gCorrosion ? rand() % 10 : 0;

		glBegin(GL_QUAD_STRIP);

		DrawPoint(&p0);
		DrawPoint(&p1);

		for (int j = 1; j <= gPolygons; j++) {
			p0.z -= gThickness / gPolygons;
			p0.s = gCorrosion ? rand() % 10 : 0;
			p1.z -= gThickness / gPolygons;
			p1.s = gCorrosion ? rand() % 10 : 0;
			DrawPoint(&p0);
			DrawPoint(&p1);
		}
		glEnd();

		// inner circle
		p0.x = gRadius * 0.1 * cos((phi - 1) * M_PI / 180);
		p0.y = gRadius * 0.1 * sin((phi - 1) * M_PI / 180);
		p0.z = 0.;
		p0.nx = -cos((phi - 1) * M_PI / 180);
		p0.ny = -sin((phi - 1) * M_PI / 180);
		p0.nz = 0.;
		p0.s = gCorrosion ? rand() % 10 : 0;

		p1.x = gRadius * 0.1 * cos((phi)*M_PI / 180);
		p1.y = gRadius * 0.1 * sin((phi)*M_PI / 180);
		p1.z = 0.;
		p1.nx = -cos((phi)*M_PI / 180);
		p1.ny = -sin((phi)*M_PI / 180);
		p1.nz = 0.;
		p1.s = gCorrosion ? rand() % 10 : 0;

		glBegin(GL_QUAD_STRIP);

		DrawPoint(&p0);
		DrawPoint(&p1);

		for (int j = 1; j <= gPolygons; j++) {
			p0.z -= gThickness / gPolygons;
			p0.s = gCorrosion ? rand() % 10 : 0;
			p1.z -= gThickness / gPolygons;
			p1.s = gCorrosion ? rand() % 10 : 0;
			DrawPoint(&p0);
			DrawPoint(&p1);
		}
		glEnd();

		// filling top surface
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= gPolygons; j++) {
			p0.x = gRadius * (0.2 - 0.1 * j / gPolygons) * cos((phi - 1) * M_PI / 180);
			p0.y = gRadius * (0.2 - 0.1 * j / gPolygons) * sin((phi - 1) * M_PI / 180);
			p0.z = 0.;
			p0.nx = 0.;
			p0.ny = 0.;
			p0.nz = 1.;
			p0.s = gCorrosion ? rand() % 10 : 0;

			p1.x = gRadius * (0.2 - 0.1 * j / gPolygons) * cos((phi) * M_PI / 180);
			p1.y = gRadius * (0.2 - 0.1 * j / gPolygons) * sin((phi) * M_PI / 180);
			p1.z = 0.;
			p1.nx = 0.;
			p1.ny = 0.;
			p1.nz = 1.;
			p1.s = gCorrosion ? rand() % 10 : 0;

			DrawPoint(&p0);
			DrawPoint(&p1);
		}
		glEnd();

		// filling bottom surface
		glBegin(GL_QUAD_STRIP);
		for (int j = 0; j <= gPolygons; j++) {
			p0.x = gRadius * (0.2 - 0.1 * j / gPolygons) * cos((phi - 1) * M_PI / 180);
			p0.y = gRadius * (0.2 - 0.1 * j / gPolygons) * sin((phi - 1) * M_PI / 180);
			p0.z = -gThickness;
			p0.nx = 0.;
			p0.ny = 0.;
			p0.nz = -1.;
			p0.s = gCorrosion ? rand() % 10 : 0;

			p1.x = gRadius * (0.2 - 0.1 * j / gPolygons) * cos((phi) * M_PI / 180);
			p1.y = gRadius * (0.2 - 0.1 * j / gPolygons) * sin((phi) * M_PI / 180);
			p1.z = -gThickness;
			p1.nx = 0.;
			p1.ny = 0.;
			p1.nz = -1.;
			p1.s = gCorrosion ? rand() % 10 : 0;

			DrawPoint(&p0);
			DrawPoint(&p1);
		}
		glEnd();
	}
	glPopMatrix();

	// Drawing arms of Wheel
	for (float phi = 0; phi < 360; phi += 360. / gArms) {
		glPushMatrix();
		if (Freeze) {
			glRotatef(phi, 0., 0., 1.);
		}
		else {
			glRotatef(phi + 2 * M_PI * Time, 0., 0., 1.);
		}

		float phi0p0 = asin(0.05 / 0.2);
		float phi1p0 = -phi0p0;
		float phi0p1 = asin(0.05 / 0.8);
		float phi1p1 = -phi0p1;

		for (int i = 1; i <= gPolygons; i++) {
			float phip0 = phi1p0 + i * (phi0p0 - phi1p0) / gPolygons;
			float phip1 = phi1p1 + i * (phi0p1 - phi1p1) / gPolygons;
			float phip0prev = phi1p0 + (i - 1) * (phi0p0 - phi1p0) / gPolygons;
			float phip1prev = phi1p1 + (i - 1) * (phi0p1 - phi1p1) / gPolygons;
			float xp0 = 0.2 * gRadius * cos(phip0);
			float yp0 = 0.2 * gRadius * sin(phip0);
			float xp0prev = 0.2 * gRadius * cos(phip0prev);
			float yp0prev = 0.2 * gRadius * sin(phip0prev);
			float xp1 = 0.8 * gRadius * cos(phip1);
			float xp1prev = 0.8 * gRadius * cos(phip1prev);

			// Drawing top surface
			p0.x = xp0prev;
			p0.y = yp0prev;
			p0.z = 0.;
			p0.nx = 0.;
			p0.ny = 0.;
			p0.nz = 1.;
			p0.s = gCorrosion ? rand() % 10 : 0;

			p1.x = xp0;
			p1.y = yp0;
			p1.z = 0.;
			p1.nx = 0.;
			p1.ny = 0.;
			p1.nz = 1.;
			p1.s = gCorrosion ? rand() % 10 : 0;

			glBegin(GL_QUAD_STRIP);

			DrawPoint(&p0);
			DrawPoint(&p1);

			for (int j = 1; j <= gPolygons; j++) {
				p0.x += (xp1prev - xp0prev) / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1.x += (xp1 - xp0) / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
			glEnd();

			// Drawing bot surface
			p0.x = xp0prev;
			p0.y = yp0prev;
			p0.z = -gThickness;
			p0.nx = 0.;
			p0.ny = 0.;
			p0.nz = -1.;
			p0.s = gCorrosion ? rand() % 10 : 0;

			p1.x = xp0;
			p1.y = yp0;
			p1.z = -gThickness;
			p1.nx = 0.;
			p1.ny = 0.;
			p1.nz = -1.;
			p1.s = gCorrosion ? rand() % 10 : 0;

			glBegin(GL_QUAD_STRIP);

			DrawPoint(&p0);
			DrawPoint(&p1);

			for (int j = 1; j <= gPolygons; j++) {
				p0.x += (xp1prev - xp0prev) / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1.x += (xp1 - xp0) / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
			glEnd();
		}

		for (int i = 1; i <= gPolygons; i++) {
			float xp0 = 0.2 * gRadius * cos(phi0p0);
			float yp0 = 0.2 * gRadius * sin(phi0p0);
			float xp1 = 0.8 * gRadius * cos(phi0p1);

			// Drawing top surface
			p0.x = xp0;
			p0.y = yp0;
			p0.z = -(i - 1) * gThickness / gPolygons;
			p0.nx = 0.;
			p0.ny = 1.;
			p0.nz = 0.;
			p0.s = gCorrosion ? rand() % 10 : 0;

			p1.x = xp0;
			p1.y = yp0;
			p1.z = -i * gThickness / gPolygons;
			p1.nx = 0.;
			p1.ny = 1.;
			p1.nz = 0.;
			p1.s = gCorrosion ? rand() % 10 : 0;

			glBegin(GL_QUAD_STRIP);

			DrawPoint(&p0);
			DrawPoint(&p1);

			for (int j = 1; j <= gPolygons; j++) {
				p0.x += (xp1 - xp0) / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1.x += (xp1 - xp0) / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
			glEnd();

			// Drawing bot surface
			p0.x = xp0;
			p0.y = -yp0;
			p0.z = -(i - 1) * gThickness / gPolygons;
			p0.nx = 0.;
			p0.ny = -1.;
			p0.nz = 0.;
			p0.s = gCorrosion ? rand() % 10 : 0;

			p1.x = xp0;
			p1.y = -yp0;
			p1.z = -i * gThickness / gPolygons;
			p1.nx = 0.;
			p1.ny = -1.;
			p1.nz = 0.;
			p1.s = gCorrosion ? rand() % 10 : 0;

			glBegin(GL_QUAD_STRIP);

			DrawPoint(&p0);
			DrawPoint(&p1);

			for (int j = 1; j <= gPolygons; j++) {
				p0.x += (xp1 - xp0) / gPolygons;
				p0.s = gCorrosion ? rand() % 10 : 0;
				p1.x += (xp1 - xp0) / gPolygons;
				p1.s = gCorrosion ? rand() % 10 : 0;
				DrawPoint(&p0);
				DrawPoint(&p1);
			}
			glEnd();
		}

		glPopMatrix();
	}
	// End of Gear List
	glEndList();

	delete[] contactPoints;
	delete[] smallCirclePoints;
	delete[] bigCirclePoints;

	return dList;
}

// main program:
int
main(int argc, char* argv[])
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)
	glutInit(&argc, argv);

	// setup all the graphics stuff:
	InitGraphics();

	// create the display structures that will not change:
	InitLists();

	// init all the global variables used by Display( ):
	// this will also post a redisplay
	Reset();

	// setup all the user interface stuff:
	InitMenus();

	// draw the scene once and wait for some interaction:
	// (this will never return)
	glutSetWindow(MainWindow);
	glutMainLoop();

	// glutMainLoop() never actually returns
	// the following line is here to make the compiler happy:
	return 0;
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display( ) from here -- let glutPostRedisplay( ) do it
void
Animate()
{
	// put animation stuff in here -- change some global variables
	int ms = glutGet(GLUT_ELAPSED_TIME);
	ms %= MS_PER_CYCLE;
	Time = MS_SPEED * (float)ms / (float)MS_PER_CYCLE;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


// draw the complete scene:
void
Display()
{
	// set which window we want to do the graphics into:
	if (DebugOn != 0)
	{
		fprintf(stderr, "Display\n");
	}

	//Window in which we wnat graphics 
	glutSetWindow(MainWindow);


	// erase the background:
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
#ifdef DEMO_DEPTH_BUFFER
	if (DepthBufferOn == 0)
		glDisable(GL_DEPTH_TEST);
#endif

	// specify shading to be Flat:
	glShadeModel(GL_SMOOTH);

	// set the viewport to a square centered in the window:
	GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
	GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = (vx - v) / 2;
	GLint yb = (vy - v) / 2;
	glViewport(xl, yb, v, v);


	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	// USE gluOrtho2D( ) IF YOU ARE DOING 2D !
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (WhichProjection == ORTHO)
		glOrtho(-2.f, 2.f, -2.f, 2.f, 0.1f, 1000.f);
	else
		gluPerspective(70.f, 1.f, 0.1f, 1000.f);

	// place the objects into the scene:
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// set the eye position, look-at position, and up-vector:
	gluLookAt(0., 0., 30., 0., 0., 0., 0., 1., 0.);

	// rotate the scene:
	glRotatef((GLfloat)Yrot, 0.f, 1.f, 0.f);
	glRotatef((GLfloat)Xrot, 1.f, 0.f, 0.f);

	// uniformly scale the scene:
	if (Scale < MINSCALE)
		Scale = MINSCALE;
	glScalef((GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale);

		// set the fog parameters:
	if (DepthCueOn != 0)
	{
		glFogi(GL_FOG_MODE, FOGMODE);
		glFogfv(GL_FOG_COLOR, FOGCOLOR);
		glFogf(GL_FOG_DENSITY, FOGDENSITY);
		glFogf(GL_FOG_START, FOGSTART);
		glFogf(GL_FOG_END, FOGEND);
		glEnable(GL_FOG);
	}
	else
	{
		glDisable(GL_FOG);
	}

	// possibly draw the axes:
	if (AxesOn)
	{
		glColor3fv(&Colors[WhichColor][0]);
		glCallList(AxesList);
	}

	// since we are using glScalef( ), be sure the normals get unitized:
	glEnable(GL_NORMALIZE);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, MulArray3(.2, White));
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

	// Light 0
	float xLight = -ORBIT_HEIGHT * sin(2 * M_PI * Time / ORBIT_SLOWDOWN);
	float yLight = 0.;
	float zLight = ORBIT_HEIGHT * cos(2 * M_PI * Time / ORBIT_SLOWDOWN);
	//SetPointLight(GL_LIGHT0, xLight, yLight, zLight, 1., 1., 1.);
	if (Light0On) {
		glPushMatrix();
		//glDisable(GL_LIGHTING);
		glColor3f(1., 1., 1.);
		glTranslatef(xLight, yLight, zLight);
		glutSolidSphere(2.0, 180, 180);
		glEnd();
		//glEnable(GL_LIGHTING);
		glPopMatrix();
	}

	if (Light0On) {
		glEnable(GL_LIGHT0);
	}
	else {
		glDisable(GL_LIGHT0);
		xLight = -60.;
		yLight = 10.;
		zLight = 10.;
	}

	if (ControlLinesAreShown) {
		glPushMatrix();
		glTranslatef(-GEAR_RADIUS1, 0., 0.);
		glCallList(DebugLinesList);
		glPopMatrix();
	}

	// Shaders part
	Pattern->Use();

	// Components for per-fragment lighting
	Pattern->SetUniformVariable("xLight", xLight);
	Pattern->SetUniformVariable("yLight", yLight);
	Pattern->SetUniformVariable("zLight", zLight);
	Pattern->SetUniformVariable("uKa", (float)0.33);
	Pattern->SetUniformVariable("uKd", (float)0.33);
	Pattern->SetUniformVariable("uKs", (float)0.33);
	Pattern->SetUniformVariable("uColor", 0.039, 0.492, 0.547);
	Pattern->SetUniformVariable("uSpecularColor", 0.7, 0.7, 0.6);
	Pattern->SetUniformVariable("uShininess", (float)20.);
	Pattern->SetUniformVariable("isSecond", false);
	Pattern->SetUniformVariable("isCorroded", IsCorroded);

	glPushMatrix();
	glTranslatef(-GEAR_RADIUS1, 0., 0.);
	if (!Freeze) {
		glRotatef(2 * M_PI * Time, 0., 0., 1.);
	}
	glCallList(Gear1List);
	glPopMatrix();

	// Components for per-fragment lighting
	Pattern->SetUniformVariable("uColor", 0.715, 0.254, 0.055);
	Pattern->SetUniformVariable("isSecond", true);

	glPushMatrix();
	glTranslatef(GEAR_TEETH_HGT + GEAR_RADIUS1 * GEAR_NUMTEETH2 / GEAR_NUMTEETH1, 0., 0.);
	if (!Freeze) {
		glRotatef(- 2 * M_PI * Time * GEAR_NUMTEETH1 / GEAR_NUMTEETH2, 0., 0., 1.);
	}
	glCallList(Gear2List);
	glPopMatrix();

	// Shaders off
	Pattern->Use(0);

	glDisable(GL_DEPTH_TEST);
	glColor3f(0.f, 1.f, 1.f);
	//DoRasterString( 0.f, 1.f, 0.f, (char *)"Gear Transmission" );

	// the projection matrix is reset to define a scene whose
	// world coordinate system goes from 0-100 in each axis
	//
	// this is called "percent units", and is just a convenience
	//
	// the modelview matrix is reset to identity as we don't
	// want to transform these coordinates
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.f, 100.f, 0.f, 100.f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor3f(1.f, 1.f, 1.f);
	//DoRasterString(5.f, 5.f, 0.f, (char*)"Lighting");

	// swap the double-buffered framebuffers:
	glutSwapBuffers();

	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !
	glFlush();
}

void
DoAxesMenu(int id)
{
	AxesOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoColorMenu(int id)
{
	WhichColor = id - RED;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoDebugMenu(int id)
{
	DebugOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoDepthMenu(int id)
{
	DepthCueOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoDepthBufferMenu(int id)
{
	DepthBufferOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoDepthFightingMenu(int id)
{
	DepthFightingOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

// main menu callback:
void
DoMainMenu(int id)
{
	switch (id)
	{
	case RESET:
		Reset();
		break;

	case QUIT:
		// gracefully close out the graphics:
		// gracefully close the graphics window:
		// gracefully exit the program:
		glutSetWindow(MainWindow);
		glFinish();
		glutDestroyWindow(MainWindow);
		exit(0);
		break;

	default:
		fprintf(stderr, "Don't know what to do with Main Menu ID %d\n", id);
	}

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoProjectMenu(int id)
{
	WhichProjection = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

// use glut to display a string of characters using a raster font:
void
DoRasterString(float x, float y, float z, char* s)
{
	glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

	char c;			// one character to print
	for (; (c = *s) != '\0'; s++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
	}
}

// use glut to display a string of characters using a stroke font:
void
DoStrokeString(float x, float y, float z, float ht, char* s)
{
	glPushMatrix();
	glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
	float sf = ht / (119.05f + 33.33f);
	glScalef((GLfloat)sf, (GLfloat)sf, (GLfloat)sf);
	char c;			// one character to print
	for (; (c = *s) != '\0'; s++)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
	}
	glPopMatrix();
}

// return the number of seconds since the start of the program:
float
ElapsedSeconds()
{
	// get # of milliseconds since the start of the program:
	int ms = glutGet(GLUT_ELAPSED_TIME);

	// convert it to seconds:
	return (float)ms / 1000.f;
}

// initialize the glui window:
void
InitMenus()
{
	glutSetWindow(MainWindow);

	int numColors = sizeof(Colors) / (3 * sizeof(int));
	int colormenu = glutCreateMenu(DoColorMenu);
	for (int i = 0; i < numColors; i++)
	{
		glutAddMenuEntry(ColorNames[i], i);
	}

	int axesmenu = glutCreateMenu(DoAxesMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int depthcuemenu = glutCreateMenu(DoDepthMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int depthbuffermenu = glutCreateMenu(DoDepthBufferMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int depthfightingmenu = glutCreateMenu(DoDepthFightingMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int debugmenu = glutCreateMenu(DoDebugMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int projmenu = glutCreateMenu(DoProjectMenu);
	glutAddMenuEntry("Orthographic", ORTHO);
	glutAddMenuEntry("Perspective", PERSP);

	int mainmenu = glutCreateMenu(DoMainMenu);
	glutAddSubMenu("Axes", axesmenu);
	glutAddSubMenu("Axis Colors", colormenu);

	glutAddSubMenu("Depth Cue", depthcuemenu);
	glutAddSubMenu("Projection", projmenu);
	glutAddMenuEntry("Reset", RESET);
	glutAddSubMenu("Debug", debugmenu);
	glutAddMenuEntry("Quit", QUIT);

	// attach the pop-up menu to the right mouse button:
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// initialize the glut and OpenGL libraries:
// also setup callback functions
void
InitGraphics()
{
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	// set the initial window configuration:
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(INIT_WINDOW_SIZE, INIT_WINDOW_SIZE);

	// open the window and set its title:
	MainWindow = glutCreateWindow(WINDOWTITLE);
	glutSetWindowTitle(WINDOWTITLE);

	// set the framebuffer clear values:
	glClearColor(BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3]);

	// setup the callback functions:
	// DisplayFunc -- redraw the window
	// ReshapeFunc -- handle the user resizing the window
	// KeyboardFunc -- handle a keyboard input
	// MouseFunc -- handle the mouse button going down or up
	// MotionFunc -- handle the mouse moving with a button down
	// PassiveMotionFunc -- handle the mouse moving with a button up
	// VisibilityFunc -- handle a change in window visibility
	// EntryFunc	-- handle the cursor entering or leaving the window
	// SpecialFunc -- handle special keys on the keyboard
	// SpaceballMotionFunc -- handle spaceball translation
	// SpaceballRotateFunc -- handle spaceball rotation
	// SpaceballButtonFunc -- handle spaceball button hits
	// ButtonBoxFunc -- handle button box hits
	// DialsFunc -- handle dial rotations
	// TabletMotionFunc -- handle digitizing tablet motion
	// TabletButtonFunc -- handle digitizing tablet button hits
	// MenuStateFunc -- declare when a pop-up menu is in use
	// TimerFunc -- trigger something to happen a certain time from now
	// IdleFunc -- what to do when nothing else is going on
	glutSetWindow(MainWindow);
	glutDisplayFunc(Display);
	glutReshapeFunc(Resize);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseMotion);
	glutPassiveMotionFunc(MouseMotion);
	glutVisibilityFunc(Visibility);
	glutEntryFunc(NULL);
	glutSpecialFunc(NULL);
	glutSpaceballMotionFunc(NULL);
	glutSpaceballRotateFunc(NULL);
	glutSpaceballButtonFunc(NULL);
	glutButtonBoxFunc(NULL);
	glutDialsFunc(NULL);
	glutTabletMotionFunc(NULL);
	glutTabletButtonFunc(NULL);
	glutMenuStateFunc(NULL);
	glutTimerFunc(-1, NULL, 0);

	// setup glut to call Animate( ) every time it has
	// nothing it needs to respond to (which is most of the time)
	// we don't need to do this for this program, and really should set the argument to NULL
	// but, this sets us up nicely for doing animation
	glutIdleFunc(Animate);

	// init the glew package (a window must be open to do this):
#ifdef WIN32
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		fprintf(stderr, "glewInit Error\n");
	}
	else
	fprintf(stderr, "GLEW initialized OK\n");
	fprintf(stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

	// do this *after* opening the window and init'ing glew:
	Pattern = new GLSLProgram();
	bool valid = Pattern->Create("pattern.vert", "pattern.frag");
	if (!valid)
	{
		fprintf(stderr, "Shader cannot be created!\n");
	}
	else
	{
		fprintf(stderr, "Shader created.\n");
	}
	Pattern->SetVerbose(false);
}

// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )
void
InitLists()
{
	glutSetWindow(MainWindow);

	// create the axes:
	AxesList = glGenLists(1);
	glNewList(AxesList, GL_COMPILE);
	glLineWidth(AXES_WIDTH);
	Axes(1.5);
	glLineWidth(1.);
	glEndList();

	float gearRadius2 = GEAR_RADIUS1 * GEAR_NUMTEETH2 / GEAR_NUMTEETH1;

	// Calculating contact point 1.
	// Using "double" here for binary search to converge. Also, GEAR_GLOBAL_TOLERANCE can be increased.
	double globalTangentAlpha = acos((GEAR_RADIUS1 + gearRadius2) / (GEAR_RADIUS1 + gearRadius2 + GEAR_TEETH_HGT));
	double pxd1 = GEAR_RADIUS1;
	double qxd1 = GEAR_RADIUS1 + GEAR_TEETH_HGT;
	double xd1;
	double yd1;
	double radSq;
	do {
		xd1 = (pxd1 + qxd1) / 2;

		yd1 = -1 / tan(globalTangentAlpha) * (xd1 - GEAR_RADIUS1 * cos(globalTangentAlpha)) + GEAR_RADIUS1 * sin(globalTangentAlpha);
		radSq = (xd1 - GEAR_RADIUS1 - gearRadius2 - GEAR_TEETH_HGT) * (xd1 - GEAR_RADIUS1 - gearRadius2 - GEAR_TEETH_HGT) + yd1 * yd1;
		if (radSq > (gearRadius2 + GEAR_TEETH_HGT) * (gearRadius2 + GEAR_TEETH_HGT)) {
			pxd1 = xd1;
		}
		else {
			qxd1 = xd1;
		}
	} while (abs(radSq - (gearRadius2 + GEAR_TEETH_HGT) * (gearRadius2 + GEAR_TEETH_HGT)) > GEAR_GLOBAL_TOLERANCE);

	// Calculating contact point 2
	double pxd2 = GEAR_RADIUS1;
	double qxd2 = GEAR_RADIUS1 + GEAR_TEETH_HGT;
	double xd2;
	double yd2;
	do {
		xd2 = (pxd2 + qxd2) / 2;

		yd2 = 1 / tan(globalTangentAlpha) * (xd2 - (GEAR_RADIUS1 + GEAR_TEETH_HGT + gearRadius2 - gearRadius2 * cos(globalTangentAlpha))) + gearRadius2 * sin(globalTangentAlpha);
		radSq = xd2 * xd2 + yd2 * yd2;
		if (radSq > (GEAR_RADIUS1 + GEAR_TEETH_HGT) * (GEAR_RADIUS1 + GEAR_TEETH_HGT)) {
			qxd2 = xd2;
		}
		else {
			pxd2 = xd2;
		}
	} while (abs(radSq - (GEAR_RADIUS1 + GEAR_TEETH_HGT) * (GEAR_RADIUS1 + GEAR_TEETH_HGT)) > GEAR_GLOBAL_TOLERANCE);

	// Debugging lines Display List
	DebugLinesList = glGenLists(1);
	glNewList(DebugLinesList, GL_COMPILE);
		glBegin(GL_POINTS);
		glPointSize(5.);
		glColor3f(1., 1., 1.);
		glVertex3f(xd1, yd1, 0.);
		glVertex3f(xd2, yd2, 0.);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(GEAR_RADIUS1 * cos(globalTangentAlpha), GEAR_RADIUS1 * sin(globalTangentAlpha), 0.);
		glVertex3f((GEAR_RADIUS1 + GEAR_TEETH_HGT + gearRadius2) - gearRadius2 * cos(globalTangentAlpha), -1 / tan(globalTangentAlpha) * ((GEAR_RADIUS1 + GEAR_TEETH_HGT + gearRadius2) - gearRadius2 * cos(globalTangentAlpha) - GEAR_RADIUS1 * cos(globalTangentAlpha)) + GEAR_RADIUS1 * sin(globalTangentAlpha), 0.);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f((GEAR_RADIUS1 + GEAR_TEETH_HGT + gearRadius2) - gearRadius2 * cos(globalTangentAlpha), gearRadius2 * sin(globalTangentAlpha), 0.);
		glVertex3f(GEAR_RADIUS1 * cos(globalTangentAlpha), 1 / tan(globalTangentAlpha) * (GEAR_RADIUS1 * cos(globalTangentAlpha) - (GEAR_RADIUS1 + GEAR_TEETH_HGT + gearRadius2) + gearRadius2 * cos(globalTangentAlpha)) + gearRadius2 * sin(globalTangentAlpha), 0.);
		glEnd();
	glEndList();

	// Gear 1 Display List
	Gear1List = CreateGearDisplayList(GEAR_NUMTEETH1, GEAR_RADIUS1, GEAR_TEETH_HGT, GEAR_THICKNESS, GEAR_ARMS1, GEAR_POLYGONS, false);

	// Gear 2 Display List
	Gear2List = CreateGearDisplayList(GEAR_NUMTEETH2, gearRadius2, GEAR_TEETH_HGT, GEAR_THICKNESS, GEAR_ARMS2, GEAR_POLYGONS, true);
}

// the keyboard callback:
void
Keyboard(unsigned char c, int x, int y)
{
	if (DebugOn != 0)
		fprintf(stderr, "Keyboard: '%c' (0x%0x)\n", c, c);

	switch (c)
	{
	case 'o':
	case 'O':
		WhichProjection = ORTHO;
		break;

	case 'p':
	case 'P':
		WhichProjection = PERSP;
		break;

	case 'q':
	case 'Q':
	case ESCAPE:
		DoMainMenu(QUIT);	// will not return here
		break;				// happy compiler

	case 'f':
	case 'F':
		Freeze = !Freeze;
		if (Freeze)
			glutIdleFunc(NULL);
		else
			glutIdleFunc(Animate);
		break;

	case 'l':
	case 'L':
		ControlLinesAreShown = !ControlLinesAreShown;
		break;

	case 'c':
	case 'C':
		IsCorroded = !IsCorroded;
		break;

	case 'x':
	case 'X':
		AxesOn = !AxesOn;
		break;

	case '0':
		Light0On = !Light0On;
		break;

	default:
		fprintf(stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c);
	}

	// force a call to Display( ):
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

// called when the mouse button transitions down or up:
void
MouseButton(int button, int state, int x, int y)
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if (DebugOn != 0)
		fprintf(stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y);

	// get the proper button bit mask:
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		b = LEFT;
		break;

	case GLUT_MIDDLE_BUTTON:
		b = MIDDLE;
		break;

	case GLUT_RIGHT_BUTTON:
		b = RIGHT;
		break;

	case SCROLL_WHEEL_UP:
		Scale += SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
		// keep object from turning inside-out or disappearing:
		if (Scale < MINSCALE) {
			Scale = MINSCALE;
		}
		break;

	case SCROLL_WHEEL_DOWN:
		Scale -= SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
		// keep object from turning inside-out or disappearing:
		if (Scale < MINSCALE) {
			Scale = MINSCALE;
		}
		break;

	default:
		b = 0;
		fprintf(stderr, "Unknown mouse button: %d\n", button);
	}

	// button down sets the bit, up clears the bit:
	if (state == GLUT_DOWN)
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

// called when the mouse moves while a button is down:
void
MouseMotion(int x, int y)
{
	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if ((ActiveButton & LEFT) != 0)
	{
		Xrot += (ANGFACT * dy);
		Yrot += (ANGFACT * dx);
	}

	if ((ActiveButton & MIDDLE) != 0)
	{
		Scale += SCLFACT * (float)(dx - dy);

		// keep object from turning inside-out or disappearing:
		if (Scale < MINSCALE) {
			Scale = MINSCALE;
		}
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene
void
Reset()
{
	ActiveButton = 0;
	AxesOn = 0;
	DebugOn = 0;
	DepthBufferOn = 1;
	DepthFightingOn = 0;
	DepthCueOn = 0;
	Scale = 1.0;
	WhichColor = WHITE;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
}

// called when user resizes the window:
void
Resize(int width, int height)
{
	// don't really need to do anything since window size is
	// checked each time in Display( ):
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

// handle a change to the window's visibility:
void
Visibility(int state)
{
	if (DebugOn != 0)
		fprintf(stderr, "Visibility: %d\n", state);

	if (state == GLUT_VISIBLE)
	{
		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}


///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////

// the stroke characters 'X' 'Y' 'Z' :
static float xx[] = { 0.f, 1.f, 0.f, 1.f };
static float xy[] = { -.5f, .5f, .5f, -.5f };
static int xorder[] = { 1, 2, -3, 4 };
static float yx[] = { 0.f, 0.f, -.5f, .5f };
static float yy[] = { 0.f, .6f, 1.f, 1.f };
static int yorder[] = { 1, 2, 3, -2, 4 };
static float zx[] = { 1.f, 0.f, 1.f, 0.f, .25f, .75f };
static float zy[] = { .5f, .5f, -.5f, -.5f, 0.f, 0.f };
static int zorder[] = { 1, 2, 3, 4, -5, 6 };

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

//	Draw a set of 3D axes:
//	(length is the axis length in world coordinates)
void
Axes(float length)
{
	glBegin(GL_LINE_STRIP);
	glVertex3f(length, 0., 0.);
	glVertex3f(0., 0., 0.);
	glVertex3f(0., length, 0.);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(0., 0., 0.);
	glVertex3f(0., 0., length);
	glEnd();

	float fact = LENFRAC * length;
	float base = BASEFRAC * length;

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < 4; i++)
	{
		int j = xorder[i];
		if (j < 0)
		{
			glEnd();
			glBegin(GL_LINE_STRIP);
			j = -j;
		}
		j--;
		glVertex3f(base + fact * xx[j], fact * xy[j], 0.0);
	}
	glEnd();

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < 5; i++)
	{
		int j = yorder[i];
		if (j < 0)
		{

			glEnd();
			glBegin(GL_LINE_STRIP);
			j = -j;
		}
		j--;
		glVertex3f(fact * yx[j], base + fact * yy[j], 0.0);
	}
	glEnd();

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < 6; i++)
	{
		int j = zorder[i];
		if (j < 0)
		{

			glEnd();
			glBegin(GL_LINE_STRIP);
			j = -j;
		}
		j--;
		glVertex3f(0.0, fact * zy[j], base + fact * zx[j]);
	}
	glEnd();
}
