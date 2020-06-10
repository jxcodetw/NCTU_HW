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

class ArapSolver {
private:
	_GLMmodel *mesh;
	
	bool cot_weighting;
	bool *is_constrain;

	std::vector<GLuint> constrain_idx;
	std::vector<GLuint> non_constrain_idx; // contains p's index which are not constrain
	std::vector<GLuint> p_to_idx; // map from p's index to L's index
	std::vector<GLuint> *neighbors;
	Eigen::Matrix<GLfloat, 3, Eigen::Dynamic> *e; // eij
	Eigen::Matrix<GLfloat, 3, Eigen::Dynamic> *e_prime; // e'ij
	Eigen::Vector3f *p_prime;
	Eigen::Matrix3f *R;

	GLfloat *w_sum;
	Eigen::Matrix<GLfloat, Eigen::Dynamic, Eigen::Dynamic> *W;

	float **b;
	LeastSquaresSparseSolver solver;

	void compute_weighting(GLuint a, GLuint b, GLuint c);
public:
	bool modified;
	ArapSolver(_GLMmodel *_mesh, bool _cot_weighting);
	~ArapSolver();
	void init();
	void reset();
	void updatePPrime();
	void updateRotation();

	void updateMesh();

	void setConstrain(GLuint idx);
};