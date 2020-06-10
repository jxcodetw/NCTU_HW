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
#include "LeastSquaresSparseSolver.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>
typedef unsigned int GLuint;

class Isoline {
private:
	_GLMmodel *mesh;
	std::vector<std::vector<GLuint>> handles;
	std::vector<bool> isHandle;
	std::vector<GLuint> *neighbors;
public:
	std::vector<GLfloat*> phis;
	Isoline(_GLMmodel *mesh);

	void addHandle(std::vector<GLuint> handle);
	void getPhi(GLuint idx);
};