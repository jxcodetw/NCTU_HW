#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <set>
#include <numeric>
#include <iostream>
#include <limits>
#include <Windows.h>
#include <gl/GL.h>
#include <glut.h>
#include <thread> 

#include "glm.h"
#include "mtxlib.h"
#include "trackball.h"
#include "LeastSquaresSparseSolver.h"
#include "TimeRecorder.h"

#include "arap_solver.h"
#include "isoline.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>

using namespace std;

// ----------------------------------------------------------------------------------------------------
// global variables
_GLMmodel *mesh;

int WindWidth, WindHeight;
int last_x , last_y;
int select_x, select_y;

typedef enum { SELECT_MODE, DEFORM_MODE } ControlMode;
ControlMode current_mode = SELECT_MODE;

vector<float*> colors;
vector<vector<GLuint> > handles;

bool show_handles = true;
volatile bool busy = false;
ArapSolver *arapsolver;
Isoline *isolineSolver;

int selected_handle_id = -1;
bool deform_mesh_flag = false;

GLuint displayHandle = 0;
GLuint iteration = 0;
GLuint update_times = 0;
// ----------------------------------------------------------------------------------------------------
// render related functions

void Reshape(int width, int height)
{
	int base = min(width , height);

	tbReshape(width, height);
	glViewport(0 , 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0,(GLdouble)width / (GLdouble)height , 1.0, 128.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -3.5);

	WindWidth = width;
	WindHeight = height;
}

void Display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	tbMatrix();

	// render solid model
	glDisable(GL_LIGHTING);
	glColor3f(1.0 , 1.0 , 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK , GL_FILL);
	//glmDraw(mesh , GLM_SMOOTH);
	glBegin(GL_TRIANGLES);
	for (GLuint i = 0; i < mesh->numtriangles; ++i) {
		GLMtriangle &T = mesh->triangles[i];
		for (int j = 0; j < 3; ++j) {
			GLuint vIdx = T.vindices[j];
			if (isolineSolver->phis.size() > 0) {
				GLfloat phi = isolineSolver->phis[displayHandle][vIdx];
				float h = (1 - phi) * 240.0;
				float s = 1;
				float v = 1;

				float r, g, b; // 0.0-1.0

				int   hi = (int)(h / 60.0f) % 6;
				float f = (h / 60.0f) - hi;
				float p = v * (1.0f - s);
				float q = v * (1.0f - s * f);
				float t = v * (1.0f - s * (1.0f - f));

				switch (hi) {
					case 0: r = v, g = t, b = p; break;
					case 1: r = q, g = v, b = p; break;
					case 2: r = p, g = v, b = t; break;
					case 3: r = p, g = q, b = v; break;
					case 4: r = t, g = p, b = v; break;
					case 5: r = v, g = p, b = q; break;
				}
				glColor3f(r, g, b);
			}
			glVertex3f(mesh->vertices[3 * vIdx + 0], mesh->vertices[3 * vIdx + 1], mesh->vertices[3 * vIdx + 2]);
		}
	}
	glEnd();

	// render wire model
	if (current_mode == SELECT_MODE)  {
		glEnable(GL_LIGHTING);
		glPolygonOffset(1.0, 1.0);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glLineWidth(1.0f);
		glColor3f(0.6, 0.0, 0.8);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glmDraw(mesh, GLM_SMOOTH);
	}
	

	// render handle points
	if (show_handles) {
		glPointSize(10.0);
		glEnable(GL_POINT_SMOOTH);
		glDisable(GL_LIGHTING);
		glBegin(GL_POINTS);
		for (int handleIter = 0; handleIter<handles.size(); handleIter++)
		{
			glColor3fv(colors[handleIter%colors.size()]);
			for (int vertIter = 0; vertIter<handles[handleIter].size(); vertIter++)
			{
				int idx = handles[handleIter][vertIter];
				glVertex3fv((float *)&mesh->vertices[3 * idx]);
				if (current_mode == DEFORM_MODE) {
					break;
				}
			}
		}
		glEnd();
	}
	

	glPopMatrix();

	glFlush();  
	glutSwapBuffers();
}

// ----------------------------------------------------------------------------------------------------
// mouse related functions

vector3 Unprojection(vector2 _2Dpos)
{
	float Depth;
	int viewport[4];
	double ModelViewMatrix[16];    // Model_view matrix
	double ProjectionMatrix[16];   // Projection matrix

	glPushMatrix();
	tbMatrix();

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, ModelViewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);

	glPopMatrix();

	glReadPixels((int)_2Dpos.x , viewport[3] - (int)_2Dpos.y , 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &Depth);

	double X = _2Dpos.x;
	double Y = _2Dpos.y;
	double wpos[3] = {0.0 , 0.0 , 0.0};

	gluUnProject(X , ((double)viewport[3] - Y) , (double)Depth , ModelViewMatrix , ProjectionMatrix , viewport, &wpos[0] , &wpos[1] , &wpos[2]);

	return vector3(wpos[0] , wpos[1] , wpos[2]);
}

vector2 projection_helper(vector3 _3Dpos)
{
	int viewport[4];
	double ModelViewMatrix[16];    // Model_view matrix
	double ProjectionMatrix[16];   // Projection matrix

	glPushMatrix();
	tbMatrix();

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, ModelViewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, ProjectionMatrix);

	glPopMatrix();

	double wpos[3] = {0.0 , 0.0 , 0.0};
	gluProject(_3Dpos.x, _3Dpos.y, _3Dpos.z, ModelViewMatrix, ProjectionMatrix, viewport, &wpos[0] , &wpos[1] , &wpos[2]);

	return vector2(wpos[0], (double)viewport[3]-wpos[1]);
}

void mouse(int button, int state, int x, int y)
{
	tbMouse(button, state, x, y);

	if( current_mode==SELECT_MODE && button==GLUT_RIGHT_BUTTON )
	{
		if(state==GLUT_DOWN)
		{
			select_x = x;
			select_y = y;
		}
		else
		{
			vector<GLuint> this_handle;

			// project all mesh vertices to current viewport
			for(int vertIter=1; vertIter<=mesh->numvertices; vertIter++)
			{
				vector3 pt(mesh->vertices[3 * vertIter + 0] , mesh->vertices[3 * vertIter + 1] , mesh->vertices[3 * vertIter + 2]);
				vector2 pos = projection_helper(pt);

				// if the projection is inside the box specified by mouse click&drag, add it to current handle
				if(pos.x>=select_x && pos.y>=select_y && pos.x<=x && pos.y<=y)
				{
					this_handle.push_back(vertIter);
					arapsolver->setConstrain(vertIter);
				}
			}
			handles.push_back(this_handle);
		}
	}
	// select handle
	else if( current_mode==DEFORM_MODE && button==GLUT_RIGHT_BUTTON && state==GLUT_DOWN )
	{
		// project all handle vertices to current viewport
		// see which is closest to selection point
		double min_dist = 999999;
		int handle_id = -1;
		for(int handleIter=0; handleIter<handles.size(); handleIter++)
		{
			for(int vertIter=0; vertIter<handles[handleIter].size(); vertIter++)
			{
				int idx = handles[handleIter][vertIter];
				vector3 pt(mesh->vertices[3 * idx + 0] , mesh->vertices[3 * idx + 1] , mesh->vertices[3 * idx + 2]);
				vector2 pos = projection_helper(pt);

				double this_dist = sqrt((double)(pos.x-x)*(pos.x-x) + (double)(pos.y-y)*(pos.y-y));
				if(this_dist<min_dist)
				{
					min_dist = this_dist;
					handle_id = handleIter;
				}
			}
		}

		selected_handle_id = handle_id;
		displayHandle = selected_handle_id;
		deform_mesh_flag = true;
	}

	if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
		deform_mesh_flag = false;

	last_x = x;
	last_y = y;
}

void motion(int x, int y)
{
	tbMotion(x, y);

	// if in deform mode and a handle is selected, deform the mesh
	if( current_mode==DEFORM_MODE && deform_mesh_flag==true )
	{
		matrix44 m;
		vector4 vec = vector4((float)(x - last_x) / 1000.0f , (float)(y - last_y) / 1000.0f , 0.0 , 1.0);

		gettbMatrix((float *)&m);
		vec = m * vec;

		// deform handle points
		for(int vertIter=0; vertIter<handles[selected_handle_id].size(); vertIter++)
		{
			int idx = handles[selected_handle_id][vertIter];
			vector3 pt(mesh->vertices[3*idx+0]+vec.x, mesh->vertices[3*idx+1]+vec.y, mesh->vertices[3*idx+2]+vec.z);
			mesh->vertices[3 * idx + 0] = pt[0];
			mesh->vertices[3 * idx + 1] = pt[1];
			mesh->vertices[3 * idx + 2] = pt[2];
		}
		iteration = 0;
		update_times = 0;
	}

	last_x = x;
	last_y = y;
}

// helper functions

void print_mode() {
	switch (current_mode)
	{
	case DEFORM_MODE:
		puts("current mode=DEFORM_MODE");
		break;
	case SELECT_MODE:
		puts("current mode=SELECT_MODE");
		break;
	}
}

// ----------------------------------------------------------------------------------------------------
// keyboard related functions
void keyboard(unsigned char key, int x, int y )
{
	TimeRecorder timer;
	switch(key)
	{
	case 'h':
		show_handles = !show_handles; // toggle show handles
		std::cout << "show handles: " << (show_handles?"true":"false") << std::endl;
		break;
	case 'd':
		if (current_mode == DEFORM_MODE) {
			break;
		}
		std::cout << "Factorizing ... ";
		timer.ResetTimer();
		arapsolver->init();
		std::cout << "done (takes " << timer.PassedTime() << "sec)" << std::endl;
		current_mode = DEFORM_MODE;
		print_mode();
		break;
	
	case 's':
		current_mode = SELECT_MODE;
		while (busy) {
			// waiting for another thread complete one iteration
		}
		arapsolver->reset();
		print_mode();
		break;
	case 'x':
		for (int i = 0; i < handles.size();++i) {
			isolineSolver->addHandle(handles[i]);
		}
		for (int i = 0; i < handles.size(); ++i) {
			isolineSolver->getPhi(i);
		}
		break;
	case 'c':
		displayHandle = (displayHandle + 1) % handles.size();
		break;
	default:
		break;
	}
}

// ----------------------------------------------------------------------------------------------------
// main function

void timf(int value)
{
	std::string title = "ARAP iteration: " + std::to_string(iteration) + ", update:" + std::to_string(update_times);
	glutSetWindowTitle(title.c_str());
	if (arapsolver->modified) {
		arapsolver->modified = false;
		arapsolver->updateMesh();
		update_times++;
	}
	glutPostRedisplay();
	glutTimerFunc(1, timf, 0);
	

}

void iterate_deform() {
	while (1) {
		if (current_mode == DEFORM_MODE) {
			busy = true;
			arapsolver->updatePPrime();
			arapsolver->updateRotation();
			iteration++;
		}
		busy = false;
	}
}

void help() {
	std::cout << "press s to select new handles" << std::endl;
	std::cout << "press d to deform" << std::endl;
	std::cout << "press h to toggle the display of handles" << std::endl;
	std::cout << "=========================================" << std::endl;
}

int main(int argc, char *argv[])
{
	help();

	WindWidth = 800;
	WindHeight = 800;

	GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat light_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat light_position[] = { 0.0, 0.0, 1.0, 0.0 };

	// color list for rendering handles
	float red[] = { 1.0, 0.0, 0.0 };
	colors.push_back(red);
	float yellow[] = { 1.0, 1.0, 0.0 };
	colors.push_back(yellow);
	float blue[] = { 0.0, 1.0, 1.0 };
	colors.push_back(blue);
	float green[] = { 0.0, 1.0, 0.0 };
	colors.push_back(green);

	glutInit(&argc, argv);
	glutInitWindowSize(WindWidth, WindHeight);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("ARAP");

	glutReshapeFunc(Reshape);
	glutDisplayFunc(Display);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardFunc(keyboard);
	glClearColor(0, 0, 0, 0);

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	tbInit(GLUT_LEFT_BUTTON);
	tbAnimate(GL_TRUE);

	glutTimerFunc(40, timf, 0); // Set up timer for 40ms, about 25 fps

	// load 3D model
	std::cout << "loading model" << std::endl;
	switch (1) {
	case -2:
		mesh = glmReadOBJ("../data/Dino_2k.obj");
		break;
	case -1:
		mesh = glmReadOBJ("../data/man_5k.obj");
		break;
	case 0:
		mesh = glmReadOBJ("../data/Armadillo_o.obj");
		break;
	case 1:
		mesh = glmReadOBJ("../data/Armadillo.obj");
		break;
	case 2:
		mesh = glmReadOBJ("../data/Dino.obj");
		break;
	case 3:
		mesh = glmReadOBJ("../data/man.obj");
		break;
	case 4:
		mesh = glmReadOBJ("../data/murphy.obj");
		break;
	case 5:
		mesh = glmReadOBJ("../data/square_21_spikes.obj");
		break;
	case 6:
		mesh = glmReadOBJ("../data/cactus_small.obj");
		break;
	case 7:
		mesh = glmReadOBJ("../data/cactus_highres.obj");
		break;
	case 8:
		mesh = glmReadOBJ("../data/bar1.obj");
		break;
	case 9:
		mesh = glmReadOBJ("../data/bar3.obj");
		break;
	}
	glmUnitize(mesh);
	glmFacetNormals(mesh);
	glmVertexNormals(mesh, 90.0);
	std::cout << "numvertices: " << mesh->numvertices << std::endl;
	std::cout << "numtriangles: " << mesh->numtriangles << std::endl;

	// pre compute
	TimeRecorder timer;
	std::cout << "initializing arapsolver ... ";
	timer.ResetTimer();
	arapsolver = new ArapSolver(mesh, false);
	
	std::cout << "done (takes " << timer.PassedTime() << "sec)" << std::endl;

	isolineSolver = new Isoline(mesh);

	// start
	print_mode();
	std::thread update_thread(iterate_deform);
	glutMainLoop();
	
	return 0;

}