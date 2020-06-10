#include "isoline.h"

Isoline::Isoline(_GLMmodel *mesh) :
mesh(mesh)
{
	isHandle.resize(mesh->numvertices + 1);
	
	// compute neighbors
	this->neighbors = new std::vector<GLuint>[mesh->numvertices + 1];
	std::set<GLuint> *neighbor_set = new std::set<GLuint>[mesh->numvertices + 1];
	for (GLuint t = 0; t < mesh->numtriangles; ++t) {
		GLMtriangle &tri = mesh->triangles[t];
		for (GLuint i = 0; i < 3; ++i) {
			for (GLuint j = 0; j < 3; ++j) {
				if (i != j) {
					neighbor_set[tri.vindices[i]].insert(tri.vindices[j]);
				}
			}
		}
	}
	for (GLuint i = 1; i <= mesh->numvertices; ++i) {
		neighbors[i] = std::vector<GLuint>(neighbor_set[i].begin(), neighbor_set[i].end());
	}
	delete[] neighbor_set;
}

void Isoline::addHandle(std::vector<GLuint> handle) {
	this->handles.push_back(handle);
	this->isHandle[handle[0]] = true;
}

void Isoline::getPhi(GLuint handleIdx) {
	LeastSquaresSparseSolver solver;
	int row = this->mesh->numvertices + this->handles.size();
	int col = this->mesh->numvertices;
	GLfloat *phi = new GLfloat[this->mesh->numvertices+1];
	GLfloat **b = new GLfloat*[1];
	b[0] = new GLfloat[row]();

	solver.Create(row, col, 1);

	// setup L
	for (GLuint i = 0; i < this->mesh->numvertices; ++i) {
		solver.AddSysElement(i, i, this->neighbors[i + 1].size());

		for (int j = 0; j < this->neighbors[i + 1].size(); ++j) {
			GLuint nidx = this->neighbors[i + 1][j];
			solver.AddSysElement(i, nidx - 1, -1);
		}
	}

	for (GLuint i = 0; i < this->handles.size(); ++i) {
		solver.AddSysElement(i + this->mesh->numvertices, this->handles[i][0], 1);
	}
	
	// setup right hand side b

	b[0][this->mesh->numvertices + handleIdx] = 1;

	solver.SetRightHandSideMatrix(b);
	solver.CholoskyFactorization();
	solver.CholoskySolve(0);
	
	GLfloat mx = -1e9;
	GLfloat mn = 1e9;
	for (GLuint i = 0; i < this->mesh->numvertices; ++i) {
		phi[i + 1] = solver.GetSolution(0, i);
		if (phi[i + 1] > mx) {
			mx = phi[i + 1];
		}
		if (phi[i + 1] < mn) {
			mn = phi[i + 1];
		}
	}
	printf("%u: %f, %f\n", handleIdx, mn, mx);
	solver.ResetSolver(0, 0, 0);
	this->phis.push_back(phi);
}