/*HW1-A Simple Solar System
in this homework, you need to modify:
init() function, for texture building.
we provide code that read 24-bit bmp into a char* buffer(inside struct Image)

Display() function, for drawing required scene

and you also need to find the proper place(outside of init func.) to put the setting function of light source position
so that the light source truly lies at world coordinate (0,0,0), where the sun is located
**modifying LightPos[] is forbidden**(you can modify it to see the diffrence, but it should be (0,0,0,1) in your final code)

You can define your own function/datatype for drawing certain object or perform certain transformation...
*/
#include "../GL/glut.h"
#include <GL/glu.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

//IMPORTANT data type for image texture, you need to acesse its member for texture building
struct Image {
	unsigned long sizeX;
	unsigned long sizeY;
	char *data;
};
typedef struct Image Image;



//number of textures desired, you may want to change it to get bonus point
#define TEXTURE_NUM 7
//directories of image files
char* texture_name[TEXTURE_NUM] = {
	"../Resource/sun.bmp",
	"../Resource/mercury.bmp",
	"../Resource/earth.bmp",
	"../Resource/moon.bmp",
	"../Resource/jupiter.bmp",
	"../Resource/europa.bmp",
	//you may add additional image files here
	"../Resource/poke.bmp",
};
//texture id array
GLuint textureid[TEXTURE_NUM];

Image* textures[TEXTURE_NUM];


//time parameter for helping coordinate your animation, you may utilize it to help perform animation
#define deltaTime 10
double time = 0.0;

//light sorce parameter, no need to modify
//actually, modification of these parameter is forbidden in this homework
float LightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };//Light position
float LightAmb[] = { 0.0f, 0.0f, 0.0f, 1.0f };//Ambient Light Values
float LightDif[] = { 1.0f, 1.0f, 1.0f, 1.0f };//Diffuse Light Values
float LightSpc[] = { 1.0f, 1.0f, 1.0f, 1.0f };//Specular Light Values

//24-bit bmp loading function, no need to modify it
int ImageLoad(char *filename, Image *image) {
	FILE *file;
	unsigned long size; // size of the image in bytes.
	unsigned long i; // standard counter.
	unsigned short int planes; // number of planes in image (must be 1)
	unsigned short int bpp; // number of bits per pixel (must be 24)
	char temp; // temporary color storage for bgr-rgb conversion.
			   // make sure the file is there.
	if ((file = fopen(filename, "rb")) == NULL) {
		printf("File Not Found : %s\n", filename);
		return 0;
	}

	// seek through the bmp header, up to the width/height:
	fseek(file, 18, SEEK_CUR);
	// read the width
	if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
		printf("Error reading width from %s.\n", filename);
		return 0;
	}

	//printf("Width of %s: %lu\n", filename, image->sizeX);
	// read the height
	if ((i = fread(&image->sizeY, 4, 1, file)) != 1) {
		printf("Error reading height from %s.\n", filename);
		return 0;
	}
	//printf("Height of %s: %lu\n", filename, image->sizeY);
	// calculate the size (assuming 24 bits or 3 bytes per pixel).
	size = image->sizeX * image->sizeY * 3;
	// read the planes
	if ((fread(&planes, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);
		return 0;
	}
	if (planes != 1) {
		printf("Planes from %s is not 1: %u\n", filename, planes);
		return 0;
	}
	// read the bitsperpixel
	if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);
		return 0;
	}
	if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);
		return 0;
	}
	// seek past the rest of the bitmap header.
	fseek(file, 24, SEEK_CUR);
	// read the data.
	image->data = (char *)malloc(size);
	if (image->data == NULL) {
		printf("Error allocating memory for color-corrected image data");
		return 0;
	}
	if ((i = fread(image->data, size, 1, file)) != 1) {
		printf("Error reading image data from %s.\n", filename);
		return 0;
	}
	for (i = 0; i<size; i += 3) { // reverse all of the colors. (bgr -> rgb)
		temp = image->data[i];
		image->data[i] = image->data[i + 2];
		image->data[i + 2] = temp;
	}
	// we're done.
	return 1;
}
//memory allocation and file reading for an Image datatype, no need to modify it
Image * loadTexture(char *filename) {
	Image *image;
	// allocate space for texture
	image = (Image *)malloc(sizeof(Image));
	if (image == NULL) {
		printf("Error allocating space for image");
		getchar();
		exit(0);
	}
	if (!ImageLoad(filename, image)) {
		getchar();
		exit(1);
	}
	return image;
}

GLdouble angle = 0;
GLfloat no_mat[] = { 0.0, 0.0, 0.0, 1.0 };
GLfloat full_mat[] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat camera_z_target = 30;
GLfloat camera_z = 30;
//callback function for drawing a frame
void Display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	camera_z += (camera_z_target - camera_z) / 10; // smooth camera move
	gluLookAt(0.0, 10, camera_z, 0, 0, 0, 0, 1, 0);//set the view part of modelview matrix
	GLUquadricObj* quad = gluNewQuadric();//glu quadratic object for drawing shape like sphere and cylinder
	GLUquadricObj* quaddisk = gluNewQuadric(); // for disk
	//please draw the scene in the following region
	//here are some example objects without texture
	//please notice how they look like without rotation
	
	gluQuadricTexture(quad, GL_TRUE);
	glEnable(GL_TEXTURE_2D);

	glLightfv(GL_LIGHT1, GL_POSITION, LightPos);
	glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
	// sun
	glPushMatrix();
		glRotated(angle/10, 0, 1, 0);
		glRotated(-90, 1, 0, 0);

		
		glBindTexture(GL_TEXTURE_2D, textureid[0]);
		gluSphere(quad, 2, 20, 20);
	glPopMatrix();
	
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	// custom planet
	glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	for (float angle = 0.0f; angle <= (2.0f*3.14159f); angle += 0.1f)
	{
		glVertex3f(20 * sin(angle), 0, 20 * cos(angle));
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	glPushMatrix();
		glRotated(angle*1.1, 0, 1, 0);
		glTranslatef(20.0, 0.0, 0.0);
		glRotated(angle*5, 0, 1, 0);
		glRotated(-90, 1, 0, 0);

		glBindTexture(GL_TEXTURE_2D, textureid[6]);
		gluSphere(quad, 2, 20, 20);
	glPopMatrix();

	// mercury
	glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	for (float angle = 0.0f; angle <= (2.0f*3.14159f); angle += 0.1f)
	{
		glVertex3f(4 * sin(angle), 0, 4 * cos(angle));
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);


	glPushMatrix();
		glRotated(angle, 0, 1, 0);
		glTranslatef(4.0, 0.0, 0.0);
		glRotated(angle*2, 0, 1, 0);
		glRotated(-90, 1, 0, 0);

		glBindTexture(GL_TEXTURE_2D, textureid[1]);
		gluSphere(quad, 0.6, 20, 20);
	glPopMatrix();

	// earth
	glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	for (float angle = 0.0f; angle <= (2.0f*3.14159f); angle += 0.1f)
	{
		glVertex3f(9 * sin(angle), 0, 9 * cos(angle));
	}
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);

	glPushMatrix();
		glRotated(angle/3, 0, 1, 0);
		glTranslatef(9.0, 0.0, 0.0);
		// artificial satellite
		glDisable(GL_TEXTURE_2D);
		glPushMatrix();
			glRotated(-30, 20, 30, 1);
			glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINE_LOOP);
			for (float angle = 0.0f; angle <= (2.0f*3.14159f); angle += 0.1f)
			{
				glVertex3f(3.5 * sin(angle), 0, 3.5 * cos(angle));
			}
			glEnd();
			glEnable(GL_TEXTURE_2D);
			glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
			glRotated(angle*0.7, 0, 1, 0);
			
			glTranslatef(3.5, 0.0, 0.0);
			glRotated(-angle, 0, 1, 0); // fix

			glRotated(angle, 0, 1, 0); // self rotation
			glRotated(90, 0, 1, 0); // face to earth
			glRotated(-90, 1, 0, 0); // face up

			//glScalef(3, 3, 3);
			glScalef(0.5, 0.5, 0.5);

			glColor3f(1.0, 0.0, 1.0);
			// top disk
			glPushMatrix();
				glTranslatef(0.0, 0.0, 0.5);
				gluDisk(quad, 0.0, 0.3, 64, 1);
			glPopMatrix();
			// bottom disk
			glPushMatrix();
				glTranslatef(0.0, 0.0, -0.5);
				gluDisk(quad, 0.0, 0.3, 64, 1);
			glPopMatrix();
			// wing
			glPushMatrix();
				glScalef(5, 0.2, 1.5);
				glutSolidCube(0.6);
			glPopMatrix();
			// radar
			glPushMatrix();
				glTranslatef(0.0, 0.6, 0.0);
				glRotated(90, 1, 0, 0); // face 
				glutSolidCone(0.6, 0.3, 20, 20);
			glPopMatrix();
			// body
			glPushMatrix();
				glTranslatef(0.0, 0, -0.5);
				gluCylinder(quad, 0.3, 0.3, 1, 20, 20);
			glPopMatrix();
			glColor3f(1.0, 1.0, 1.0);
		glPopMatrix();
		glEnable(GL_TEXTURE_2D);
		
		// moon
		glPushMatrix();
			glRotated(45, 1, 0, 0);
			glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINE_LOOP);
			for (float angle = 0.0f; angle <= (2.0f*3.14159f); angle += 0.1f)
			{
				glVertex3f(3 * sin(angle), 0, 3 * cos(angle));
			}
			glEnd();
			glEnable(GL_TEXTURE_2D);
			glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
			glRotated(angle, 0, 1, 0);

			glTranslatef(3.0, 0.0, 0.0);
			glRotated(-angle, 0, 1, 0); // fix

			glRotated(angle, 0, 1, 0); // self rotation
			glRotated(90, 0, 1, 0); // face to earth
			glRotated(-90, 1, 0, 0); // face up

			
			glBindTexture(GL_TEXTURE_2D, textureid[3]);
			gluSphere(quad, 0.5, 20, 20);
		glPopMatrix();

		glRotated(-angle / 3, 0, 1, 0); // fix
		glRotated(30, 0, 0, 1); // tilt axis
		glRotated(angle, 0, 1, 0);
		glRotated(-90, 1, 0, 0);

		glBindTexture(GL_TEXTURE_2D, textureid[2]);
		glScalef(1.0, 1.0, 0.85); // flat earth
		gluSphere(quad, 2, 20, 20);
	glPopMatrix();

	// jupyter
	glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	for (float angle = 0.0f; angle <= (2.0f*3.14159f); angle += 0.1f)
	{
		glVertex3f(15 * sin(angle), 0, 15 * cos(angle));
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
	glPushMatrix();
		// revolve
		glRotated(angle / 2, 0, 1, 0);
		glTranslatef(15.0, 0.0, 0.0);

		// artificial satellite
		glDisable(GL_TEXTURE_2D);
		glPushMatrix();
			glRotated(-60, 1, 0, 0);
			glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINE_LOOP);
			for (float angle = 0.0f; angle <= (2.0f*3.14159f); angle += 0.1f)
			{
				glVertex3f(4 * sin(angle), 0, 4 * cos(angle));
			}
			glEnd();
			glEnable(GL_TEXTURE_2D);
			glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
			glRotated(angle * 1.5, 0, 1, 0);

			glTranslatef(4, 0.0, 0.0);
			glRotated(-angle, 0, 1, 0); // fix

			glRotated(angle, 0, 1, 0); // self rotation
			glRotated(90, 0, 1, 0); // face to earth
			glRotated(-90, 1, 0, 0); // face up

									 //glScalef(3, 3, 3);
			glScalef(0.8, 0.8, 0.8);

			glColor3f(0.0, 1.0, 1.0);
			// top disk
			glPushMatrix();
				glTranslatef(0.0, 0.0, 0.5);
				gluDisk(quad, 0.0, 0.3, 64, 1);
			glPopMatrix();
			// bottom disk
			glPushMatrix();
				glTranslatef(0.0, 0.0, -0.5);
				gluDisk(quad, 0.0, 0.3, 64, 1);
			glPopMatrix();
			// wing
			glPushMatrix();
				glScalef(5, 0.2, 1.5);
				glutSolidCube(0.6);
			glPopMatrix();
			// radar
			glPushMatrix();
				glTranslatef(0.0, 0.6, 0.0);
				glRotated(90, 1, 0, 0); // face 
				glutSolidCone(0.6, 0.3, 20, 20);
			glPopMatrix();
			// body
			glPushMatrix();
				glTranslatef(0.0, 0, -0.5);
				gluCylinder(quad, 0.3, 0.3, 1, 20, 20);
			glPopMatrix();
			glColor3f(1.0, 1.0, 1.0);
		glPopMatrix();
		glEnable(GL_TEXTURE_2D);

		// europa
		glPushMatrix();
			glRotated(60, 0, 0, 1);
			glMaterialfv(GL_FRONT, GL_EMISSION, full_mat);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINE_LOOP);
			for (float angle = 0.0f; angle <= (2.0f*3.14159f); angle += 0.1f)
			{
				glVertex3f(4 * sin(angle), 0, 4 * cos(angle));
			}
			glEnd();
			glEnable(GL_TEXTURE_2D);
			glMaterialfv(GL_FRONT, GL_EMISSION, no_mat);
			glRotated(angle/2, 0, 1, 0);
			glTranslatef(4.0, 0.0, 0.0);
			glRotated(-angle, 0, 1, 0); // fix

			glRotated(angle, 0, 1, 0); // self rotation
			glRotated(90, 0, 1, 0); // face to earth
			glRotated(-90, 1, 0, 0); // face up

			glBindTexture(GL_TEXTURE_2D, textureid[5]);
			gluSphere(quad, 0.7, 20, 20);
		glPopMatrix();

		glRotated(-angle / 2, 0, 1, 0); // fix
		
		glRotated(angle/1.5, 0, 1, 0); // self rotation
		glRotated(-90, 1, 0, 0); // face up
		
		glBindTexture(GL_TEXTURE_2D, textureid[4]);
		gluSphere(quad,2.5,20,20);
	glPopMatrix();
	angle += 1;
	glDisable(GL_TEXTURE_2D);//when you draw something without texture, be sure to disable GL_TEXTURE_2D

	
	glutSwapBuffers();//swap the drawn buffer to the window
}

//callback funtion as a timer, no need to modify it
void Tick(int id){
	double d = deltaTime / 1000.0;
	time += d;
	glutPostRedisplay();
	glutTimerFunc(deltaTime, Tick, 0); // 100ms for time step size
}

//callback function when the window size is changed, no need to modify it
void WindowSize(int w, int h)
{
	glViewport(0, 0, w, h);							//changing the buffer size the same to the window size
	glMatrixMode(GL_PROJECTION);					//choose the projection matrix
	glLoadIdentity();
	gluPerspective(60.0, (double)w /(double)h, 1.0, 1000.0);//set the projection matrix as perspective mode
	glMatrixMode(GL_MODELVIEW);						//it is suggested that modelview matrix is chosen if nothing specific being performed
	glLoadIdentity();
}

//initialization for parameters of this program, you must perform something here
void init()
{
	glClearColor(0.0, 0.0, 0.0, 1.0);//set what color should be used when we clean the color buffer
	glEnable(GL_LIGHT1);//Enable Light1
	glEnable(GL_LIGHTING);//Enable Lighting
	//***********
	//glLightfv(GL_LIGHT1, GL_POSITION, LightPos);//Set Light1 Position, this setting function should be at another place
	//***********
	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmb);//Set Light1 Ambience
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDif);//Set Light1 Diffuse
	glLightfv(GL_LIGHT1, GL_SPECULAR, LightSpc);//Set Light1 Specular
	//since a light source is also an object in the 3D scene, we set its position in the display function after gluLookAt()
	//you should know that straight texture mapping(without modifying shader) may not have shading effect
	//you need to tweak certain parameter(when building texture) to obtain a lit and textured object
	glShadeModel(GL_SMOOTH);//shading model
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);//you can choose which part of lighting model should be modified by texture mapping
	glEnable(GL_COLOR_MATERIAL);//enable this parameter to use glColor() as material of lighting model

	glEnable(GL_TEXTURE_2D);
	//please load all the textures here
	//use Image* loadTexture(file_directory) function to obtain char* data and size info.
	glGenTextures(TEXTURE_NUM, textureid);
	for (int i = 0; i < sizeof(texture_name) / sizeof(char*); ++i) {
		printf("load texture: %s\n", texture_name[i]);
		glBindTexture(GL_TEXTURE_2D, textureid[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		textures[i] = loadTexture(texture_name[i]);
		glTexImage2D(GL_TEXTURE_2D,
			0,
			3,
			128,
			128,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			textures[i]->data);
	}

}

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
	{
		camera_z_target = 30;
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		camera_z_target -= 3;
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		camera_z_target += 3;
	}
}

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);//glut function for simplifying OpenGL initialization
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);//demanding: double-framed buffer | RGB colorbuffer | depth buffer
	glutInitWindowPosition(100, 50);//set the initial window position
	glutInitWindowSize(800, 600);//set the initial window size
	//**************************
	glutCreateWindow("CG_HW1_0656078");//IMPORTANT!!­«­n!! Create the window and set its title, please replace 0123456 with your own student ID
	//**************************
	glutDisplayFunc(Display);//callback funtion for drawing a frame
	glutReshapeFunc(WindowSize);//callback function when the window size is changed
	glutMouseFunc(mouse); // register for mouse
	glutTimerFunc(deltaTime, Tick, 0);//timer function
	//you may want to write your own callback funtion for other events(not demanded nor forbidden)
	init();//self-defined initialization function for the elegance of your code
	puts("Press mouse left to move camera near sun");
	puts("Press mouse right to move camera away from sun");
	puts("Press mouse middle to reset camera");
	glutMainLoop();
	return 0;
}