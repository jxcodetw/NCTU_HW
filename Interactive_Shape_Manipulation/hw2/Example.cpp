#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctime>
#include <vector>
#include <set>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <Windows.h>
#include <gl/GL.h>
#include <glut.h>

#include "glm.h"
#include "mtxlib.h"
#include "trackball.h"
#include "LeastSquaresSparseSolver.h"
#include "TimeRecorder.h"

using namespace std;

_GLMmodel *mesh;
int WindWidth, WindHeight;

int last_x , last_y;
int selectedFeature = -1;
vector<int> featureList;

const int numsample = 1000;
std::set<int> neighbor_set[10000+100]; // record neighbor
vector<int> perm; // for random sample

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

  // render features
  glPointSize(10.0);
  glColor3f(1.0 , 0.0 , 0.0);
  glDisable(GL_LIGHTING);
  glBegin(GL_POINTS);
	for (int i = 0 ; i < featureList.size() ; i++)
	{
		int idx = featureList[i];

		glVertex3fv((float *)&mesh->vertices[3 * idx]);
	}
  glEnd();
  
  glPopMatrix();

  glFlush();  
  glutSwapBuffers();
}

vector3 Unprojection(vector2 _2Dpos)
{
	float Depth;
	int viewport[4];
	double ModelViewMatrix[16];				//Model_view matrix
	double ProjectionMatrix[16];			//Projection matrix

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

void mouse(int button, int state, int x, int y)
{
  tbMouse(button, state, x, y);

  // add feature
  if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN)
  {
	  int minIdx = 0;
	  float minDis = 9999999.0f;

	  vector3 pos = Unprojection(vector2((float)x , (float)y));

	  for (int i = 0 ; i < mesh->numvertices ; i++)
	  {
		  vector3 pt(mesh->vertices[3 * i + 0] , mesh->vertices[3 * i + 1] , mesh->vertices[3 * i + 2]);
		  float dis = (pos - pt).length();

		  if (minDis > dis)
		  {
			  minDis = dis;
			  minIdx = i;
		  }
	  }

	  featureList.push_back(minIdx);
  }

  // manipulate feature
  if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
  {
	  int minIdx = 0;
	  float minDis = 9999999.0f;

	  vector3 pos = Unprojection(vector2((float)x , (float)y));

	  for (int i = 0 ; i < featureList.size() ; i++)
	  {
		  int idx = featureList[i];
		  vector3 pt(mesh->vertices[3 * idx + 0] , mesh->vertices[3 * idx + 1] , mesh->vertices[3 * idx + 2]);
		  float dis = (pos - pt).length();

		  if (minDis > dis)
		  {
			  minDis = dis;
			  minIdx = featureList[i];
		  }
	  }

	  selectedFeature = minIdx;
  }

  if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
	  selectedFeature = -1;

  last_x = x;
  last_y = y;
}

void motion(int x, int y)
{
  tbMotion(x, y);

  if (selectedFeature != -1)
  {
	  matrix44 m;
	  vector4 vec = vector4((float)(x - last_x) / 100.0f , (float)(y - last_y) / 100.0f , 0.0 , 1.0);
	  
	  gettbMatrix((float *)&m);
	  vec = m * vec;

	  mesh->vertices[3 * selectedFeature + 0] += vec.x;
	  mesh->vertices[3 * selectedFeature + 1] -= vec.y;
	  mesh->vertices[3 * selectedFeature + 2] += vec.z;
  }

  last_x = x;
  last_y = y;
}

void timf(int value)
{
  glutPostRedisplay();
  glutTimerFunc(1, timf, 0);
}

double error = 0;
void solve() {
	LeastSquaresSparseSolver solver;

	int numrows = mesh->numvertices + numsample;
	solver.Create(numrows+1, mesh->numvertices+1, 3);

	// laplacian
	solver.AddSysElement(0, 0, 1.0f);
	for(int i=1;i<=mesh->numvertices;++i) {
		solver.AddSysElement(i, i, 1.0f);
		float factor = -1/((float)neighbor_set[i].size() - 1.0);
		for(std::set<int>::iterator it = neighbor_set[i].begin(); it != neighbor_set[i].end();++it) {
			if (*it == i) {
				continue;
			}
			solver.AddSysElement(i, *it, factor);
		}
	}
	// constrain
	for(int i=1;i<=numsample;++i) {
		solver.AddSysElement(mesh->numvertices+i, perm[i], 1.0f);
	}

	float **b = new float*[3];
	for(int i=0;i<3;++i) {
		b[i] = new float[numrows+1];
		for(int j=0;j<mesh->numvertices;++j) {
			b[i][j] = 0;
		}
		for(int j=1;j<=numsample;++j) {
			b[i][mesh->numvertices+j] = mesh->vertices[3*perm[j]+i];
		}
	}
	solver.SetRightHandSideMatrix(b);

	// direct solver
	solver.CholoskyFactorization();
	solver.CholoskySolve(0);
	solver.CholoskySolve(1);
	solver.CholoskySolve(2);

	// get result
	error = 0;
	for(int i=1;i<=mesh->numvertices;++i) {
		for(int j=0;j<3;++j) {
			float solved = solver.GetSolution(j , i);
			error += (mesh->vertices[3*i+j]-solved)*(mesh->vertices[3*i+j]-solved);
			mesh->vertices[3*i+j] = solved;
		}
	}
	error = sqrt(error/mesh->numvertices);
	printf("Root mean squared distance: %lf\n", error);

	// release
	solver.ResetSolver(0 , 0 , 0);
	for(int i=0;i<3;++i) {
		delete [] b[i];
	}
	delete [] b;
}

int main(int argc, char *argv[])
{
  WindWidth = 400;
  WindHeight = 400;
	
  GLfloat light_ambient[] = {0.0, 0.0, 0.0, 1.0};
  GLfloat light_diffuse[] = {0.8, 0.8, 0.8, 1.0};
  GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};
  GLfloat light_position[] = {0.0, 0.0, 1.0, 0.0};

  glutInit(&argc, argv);
  glutInitWindowSize(WindWidth, WindHeight);
  glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
  glutCreateWindow("Trackball Example");

  glutReshapeFunc(Reshape);
  glutDisplayFunc(Display);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
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
  mesh = glmReadOBJ("../data/Dino.obj");
  printf("numvertices: %d\n", mesh->numvertices);
  
  for(int i=0;i<mesh->numtriangles;++i) {
	  for(int j=0;j<3;++j) {
		  for(int k=0;k<3;++k) {
			  neighbor_set[mesh->triangles[i].vindices[j]].insert(mesh->triangles[i].vindices[k]);
		  }
	  }
	  
  }
  for(int i=1;i<=mesh->numvertices;++i) {
	  perm.push_back(i);
  }
  std::srand ( unsigned ( std::time(0) ) );
  std::random_shuffle(perm.begin(), perm.end());
  
  
  glmUnitize(mesh);
  glmFacetNormals(mesh);
  glmVertexNormals(mesh , 90.0);

  TimeRecorder timer;
  timer.ResetTimer();
  solve();
  printf("Time elapsed: %lf ms\n", timer.PassedTime());
  glutMainLoop();

  return 0;

}

