#include <stdio.h>
#include <stdlib.h>
#include <stddef.h> /* offsetof */
#include "../GL/glew.h"
#include "../GL/glut.h"

#include "../shader_lib/shader.h"

void init(void);
void display(void);
void reshape(int width, int height);
void keyboard(unsigned char key, int x, int y);
void idle(void);

GLuint program;
GLuint vboName;
unsigned int  PG = 0;

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("OpenGL HW3 Example");
	glutReshapeWindow(512, 512);

	glewInit();

	init();

	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);

	glutMainLoop();

	return 0;
}

void init(void) {

	GLuint vert = createShader("Shaders/sin.vert", "vertex");
	GLuint frag = createShader("Shaders/sin.frag", "fragment");
	program = createProgram(vert, frag);

	glGenBuffers(1, &vboName);
	glBindBuffer(GL_ARRAY_BUFFER, vboName);
	float vertices[] = { 1.0, 0.0,  0.5, 0.0,
		0.5, 0.0,  0.0, 0.0,
		0.0, 0.0, -0.5, 0.0,
		-0.5, 0.0, -1.0, 0.0 };
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0); // stride 0 for tightly packed

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);
	glDrawArrays(GL_LINES, 0, 8);
	glUseProgram(0);
	glBegin(GL_LINES);
		glVertex3f(-1, -1, 0);
		glVertex3f(1, 1, 0);
	glEnd();

	glutSwapBuffers();
}

void reshape(int width, int height)
{
	glViewport(0, 0, width, height);
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
	{	//ESC
		exit(0);
		break;
	}
	default:
	{
		break;
	}
	}
}

void idle(void)
{
	glutPostRedisplay();
}