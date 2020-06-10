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
vector<vector<int> > handles;
vector<GLuint> non_constrain_idx, p_to_idx;

std::set<GLuint> *neighbors;

bool show_handles = true;
bool *is_constrain;
volatile bool busy = false;
Eigen::Matrix<GLfloat, 3, Eigen::Dynamic> *eij, *epij; // eij, e'ij
Eigen::Matrix3f *Ri;
GLfloat **w, *w_sum;
Eigen::Matrix<GLfloat, Eigen::Dynamic, Eigen::Dynamic> *W;
GLuint numvertices, total_cond;
float **b;
LeastSquaresSparseSolver solver;
int selected_handle_id = -1;
bool deform_mesh_flag = false;

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
	glEnable(GL_LIGHTING);
	glColor3f(1.0 , 1.0 , 1.0f);
	glPolygonMode(GL_FRONT_AND_BACK , GL_FILL);
	glmDraw(mesh , GLM_SMOOTH);

	// render wire model
	glPolygonOffset(1.0 , 1.0);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glLineWidth(1.0f);
	glColor3f(0.6 , 0.0 , 0.8);
	glPolygonMode(GL_FRONT_AND_BACK , GL_LINE);
	glmDraw(mesh , GLM_SMOOTH);

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
			vector<int> this_handle;

			// project all mesh vertices to current viewport
			for(int vertIter=1; vertIter<=mesh->numvertices; vertIter++)
			{
				vector3 pt(mesh->vertices[3 * vertIter + 0] , mesh->vertices[3 * vertIter + 1] , mesh->vertices[3 * vertIter + 2]);
				vector2 pos = projection_helper(pt);

				// if the projection is inside the box specified by mouse click&drag, add it to current handle
				if(pos.x>=select_x && pos.y>=select_y && pos.x<=x && pos.y<=y)
				{
					this_handle.push_back(vertIter);
					is_constrain[vertIter] = true;
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
void compute_Ri();
void compute_p_prime();
void keyboard(unsigned char key, int x, int y )
{
	GLuint numcontrol = 0, num_constrain;
	GLuint idx = numvertices + 1;
	switch(key)
	{
	case 'h':
		show_handles = !show_handles; // toggle show handles
		std::cout << "show handles: " << show_handles << std::endl;
		break;
	case 'd':
		if (current_mode == DEFORM_MODE) {
			break;
		}
		// create solver
		
		for (auto handle : handles) {
			numcontrol += handle.size();
		}
		num_constrain = 0;
		non_constrain_idx.clear();
		for (GLuint i = 1; i <= numvertices; ++i) {
			if (is_constrain[i]) {
				num_constrain += 1;
			} else {
				p_to_idx[i] = non_constrain_idx.size();
				non_constrain_idx.push_back(i);
			}
		}
		
		total_cond = numvertices - num_constrain;
		std::cout << "conds:" << total_cond << std::endl;
		solver.Create(total_cond, total_cond, 3);
		// set system
		//solver.AddSysElement(0, 0, 1.0f);
		for (GLuint i = 0; i < non_constrain_idx.size(); ++i) {
			GLuint curidx = non_constrain_idx[i];
			solver.AddSysElement(i, i, w_sum[curidx]);
			
			for (auto p : neighbors[curidx]) {
				if (!is_constrain[p]) {
					solver.AddSysElement(i, p_to_idx[p], -w[curidx][p]);
				}
			}
		}

		
		solver.SetRightHandSideMatrix(b); // must set once before factorization or it will crash
		solver.CholoskyFactorization();

		current_mode = DEFORM_MODE;
		print_mode();
		break;
	
	case 's':
		current_mode = SELECT_MODE;
		
		while (busy) {
			// waiting for another thread complete one iteration
		}
		solver.ResetSolver(0, 0, 0);
		print_mode();
		break;
	case 'c':
		
		compute_p_prime();
		compute_Ri();
		break;
	default:
		break;
	}
}

// ----------------------------------------------------------------------------------------------------
// main function

void timf(int value)
{
	glutPostRedisplay();
	glutTimerFunc(1, timf, 0);

}

void compute_edge(Eigen::Matrix<GLfloat, 3, Eigen::Dynamic> *ei_j, GLuint numvertices) {
	for (int i = 1; i <= numvertices; ++i) {
		int idx = 0;
		for (auto j : neighbors[i]) {
			Eigen::Vector3f pi = Eigen::Vector3f(mesh->vertices[i * 3 + 0], mesh->vertices[i * 3 + 1], mesh->vertices[i * 3 + 2]);
			Eigen::Vector3f pj = Eigen::Vector3f(mesh->vertices[j * 3 + 0], mesh->vertices[j * 3 + 1], mesh->vertices[j * 3 + 2]);
			ei_j[i].col(idx++) = (pi - pj);
		}
	}
}

void compute_Ri() {
	compute_edge(epij, numvertices);
	for (int i = 1; i <= numvertices; ++i) {
		Eigen::Matrix3f Si = eij[i] * W[i] * epij[i].transpose(); // assume wij = 1
		Eigen::JacobiSVD<Eigen::Matrix3f> svd(Si, Eigen::ComputeFullU | Eigen::ComputeFullV);
		// note that svd.matrixV() is actually V^T!!
		Ri[i] = (svd.matrixV() * svd.matrixU().transpose()); // Ri

		Eigen::Matrix3f flip = Eigen::Matrix3f::Identity();
		flip(2, 2) = Ri[i].determinant();
		Ri[i] = svd.matrixV() * flip * svd.matrixU().transpose();
	}
}

void compute_p_prime() {
	for (int i = 0; i < non_constrain_idx.size(); ++i) {
		GLuint curidx = non_constrain_idx[i];
		GLuint nidx = 0;
		for (GLuint j = 0; j < 3; ++j) {
			b[j][i] = 0;
		}
		for (auto p : neighbors[curidx]) {
			Eigen::Vector3f bv = 0.5 * w[curidx][p] * (Ri[curidx] + Ri[p]) * eij[curidx].col(nidx++);
			for (GLuint j = 0; j < 3; ++j) {
				b[j][i] += bv[j];
			}

			if (is_constrain[p]) {
				for (GLuint j = 0; j < 3; ++j) {
					b[j][i] += mesh->vertices[3*p+j] * w[curidx][p];
				}
			}
		}
	}
	solver.SetRightHandSideMatrix(b);
	solver.CholoskySolve(0);
	solver.CholoskySolve(1);
	solver.CholoskySolve(2);

	for (GLuint i = 0; i < non_constrain_idx.size(); ++i) {
		GLuint curidx = non_constrain_idx[i];
		for (GLuint j = 0; j < 3; ++j) {
			mesh->vertices[3 * curidx + j] = solver.GetSolution(j, i);
		}
	}
}

void iterate_deform() {
	while (1) {
		if (current_mode == DEFORM_MODE) {
			busy = true;
			compute_p_prime();
			compute_Ri();
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

void compute_cot_weighting(GLfloat *vertices, GLuint a, GLuint b, GLuint c) {
	Eigen::Vector3f va(vertices[3 * b + 0] - vertices[3 * a + 0], vertices[3 * b + 1] - vertices[3 * a + 1], vertices[3 * b + 2] - vertices[3 * a + 2]);
	Eigen::Vector3f vb(vertices[3 * c + 0] - vertices[3 * a + 0], vertices[3 * c + 1] - vertices[3 * a + 1], vertices[3 * c + 2] - vertices[3 * a + 2]);

	va.normalize();
	vb.normalize();

	GLfloat cos = va.dot(vb);
	GLfloat cot = 0.5*(cos / sqrt(1.0 - cos*cos));

	bool uniform_weighting = false;
	if (uniform_weighting) {
		cot = 0.5;
	}
	w[b][c] += cot;
	w[c][b] += cot;
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
	switch (5) {
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
	for (GLuint i = 1; i <= numvertices; ++i) {
		for (int j = 0; j < 3; ++j) {
			mesh->vertices[3 * i + j] *= 20;
		}

	}
	glmUnitize(mesh);
	glmFacetNormals(mesh);
	glmVertexNormals(mesh, 90.0);
	std::cout << "numvertices: " << mesh->numvertices << std::endl;

	// pre compute
	std::cout << "initializing" << std::endl;
	numvertices = mesh->numvertices;
	w = new GLfloat*[numvertices + 1];
	w_sum = new GLfloat[numvertices + 1]();
	W = new Eigen::Matrix<GLfloat, Eigen::Dynamic, Eigen::Dynamic>[numvertices + 1];
	for (GLuint i = 1; i <= numvertices; ++i) {
		w[i] = new GLfloat[numvertices + 1]();
		for (GLuint j = 1; j <= numvertices; ++j) {
			w[i][j] = 0.0;
		}
	}
	eij = new Eigen::Matrix<GLfloat, 3, Eigen::Dynamic>[numvertices + 1];
	epij = new Eigen::Matrix<GLfloat, 3, Eigen::Dynamic>[numvertices + 1];

	Ri = new Eigen::Matrix3f[numvertices + 1];
	for (GLuint i = 1; i <= numvertices; ++i) {
		Ri[i] = Eigen::Matrix3f::Identity();
	}

	b = new float*[3];
	for (int i = 0; i < 3; ++i) {
		b[i] = new float[numvertices + 1];
	}

	is_constrain = new bool[numvertices + 1]();
	for (GLuint i = 1; i <= numvertices; ++i) {
		is_constrain[i] = false;
	}
	p_to_idx.resize(numvertices + 1);
	// neighbor
	{
		neighbors = new std::set<GLuint>[numvertices + 1];
		for (int t = 0; t < mesh->numtriangles; ++t) {
			GLMtriangle &tri = mesh->triangles[t];
			compute_cot_weighting(mesh->vertices, tri.vindices[0], tri.vindices[1], tri.vindices[2]);
			compute_cot_weighting(mesh->vertices, tri.vindices[1], tri.vindices[2], tri.vindices[0]);
			compute_cot_weighting(mesh->vertices, tri.vindices[2], tri.vindices[0], tri.vindices[1]);

			for (int i = 0; i < 3; ++i) {
				// p_i: index of pi
				GLuint p_i = tri.vindices[i];
				for (int j = 0; j < 3; ++j) {
					GLuint p_j = tri.vindices[j];
					if (i != j) {
						neighbors[p_i].insert(p_j);
					}
				}
			}
		}
		
		float minw = 1e9;
		float maxw = 1e-9;
		for (GLuint i = 1; i <= numvertices; ++i) {
			W[i].resize(neighbors[i].size(), neighbors[i].size());
			W[i].setZero();
			
			GLuint idx = 0;
			for (auto p : neighbors[i]) {
				W[i](idx, idx) = w[i][p];
				if (w[i][p] < minw){
					minw = w[i][p];
				}
				if (w[i][p] > maxw) {
					maxw = w[i][p];
				}
				w_sum[i] += w[i][p];
				idx++;
			}
		}

		std::cout << "Max wij:" << maxw << std::endl;
		std::cout << "Min wij:" << minw << std::endl;

		for (GLuint i = 1; i <= numvertices; ++i) {
			eij[i].resize(3, neighbors[i].size());
			epij[i].resize(3, neighbors[i].size());
		}
	}

	// eij
	compute_edge(eij, numvertices);

	// start
	print_mode();
	std::thread update_thread(iterate_deform);
	glutMainLoop();
	
	return 0;

}