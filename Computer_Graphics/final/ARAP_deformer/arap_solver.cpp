#include "arap_solver.h"

ArapSolver::ArapSolver(_GLMmodel *_mesh, bool _cot_weighting) : 
	mesh(_mesh),
	cot_weighting(_cot_weighting) {
	// allocate memory
	this->modified = false;
	this->p_to_idx.resize(this->mesh->numvertices + 1);
	this->is_constrain = new bool[mesh->numvertices + 1]();
	this->neighbors = new std::vector<GLuint>[mesh->numvertices + 1];
	this->w_sum = new GLfloat[mesh->numvertices + 1]();
	this->W = new Eigen::Matrix<GLfloat, Eigen::Dynamic, Eigen::Dynamic>[mesh->numvertices + 1];
	this->e = new Eigen::Matrix<GLfloat, 3, Eigen::Dynamic>[mesh->numvertices + 1];
	this->e_prime = new Eigen::Matrix<GLfloat, 3, Eigen::Dynamic>[mesh->numvertices + 1];
	this->p_prime = new Eigen::Vector3f[mesh->numvertices + 1];
	this->R = new Eigen::Matrix3f[mesh->numvertices + 1];
	this->b = new GLfloat*[3];

	for (GLuint i = 0; i < 3; ++i) {
		this->b[i] = new GLfloat[mesh->numvertices + 1]();
	}
	
	// pre compute
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
		this->R[i] = Eigen::Matrix3f::Identity();
		this->W[i].resize(neighbors[i].size(), neighbors[i].size());
		this->W[i].setZero();
		this->e[i].resize(3, neighbors[i].size());
		this->e_prime[i].resize(3, neighbors[i].size());
		for (GLuint j = 0; j < 3; ++j) {
			this->p_prime[i](j) = this->mesh->vertices[3 * i + j];
		}

		for (GLuint j = 0; j < this->neighbors[i].size(); ++j) {
			GLuint nidx = neighbors[i][j];
			Eigen::Vector3f pi(this->mesh->vertices[3 * i + 0], this->mesh->vertices[3 * i + 1], this->mesh->vertices[3 * i + 2]);
			Eigen::Vector3f pj(this->mesh->vertices[3 * nidx + 0], this->mesh->vertices[3 * nidx + 1], this->mesh->vertices[3 * nidx + 2]);
			this->e[i].col(j) = pi - pj;
		}
	}
	delete[] neighbor_set;

	for (GLuint t = 0; t < this->mesh->numtriangles; ++t) {
		GLuint *vindices = this->mesh->triangles[t].vindices;
		this->compute_weighting(vindices[0], vindices[1], vindices[2]);
		this->compute_weighting(vindices[1], vindices[2], vindices[0]);
		this->compute_weighting(vindices[2], vindices[0], vindices[1]);
	}
}

ArapSolver::~ArapSolver() {
	delete[] this->is_constrain;
	delete[] this->W;
	delete[] this->e;
	delete[] this->e_prime;
}

void ArapSolver::compute_weighting(GLuint a, GLuint b, GLuint c) {
	GLfloat cot = 0.5; // default uniform weighting
	if (this->cot_weighting) {
		// cotangent weighting
		GLfloat *vertices = this->mesh->vertices;
		Eigen::Vector3f va(vertices[3 * b + 0] - this->mesh->vertices[3 * a + 0], vertices[3 * b + 1] - vertices[3 * a + 1], vertices[3 * b + 2] - vertices[3 * a + 2]);
		Eigen::Vector3f vb(vertices[3 * c + 0] - vertices[3 * a + 0], vertices[3 * c + 1] - vertices[3 * a + 1], vertices[3 * c + 2] - vertices[3 * a + 2]);

		va.normalize();
		vb.normalize();

		GLfloat cos = va.dot(vb);
		cot = 0.5*(cos / sqrt(1.0 - cos*cos));
	}
	this->w_sum[b] += cot;
	this->w_sum[c] += cot;
	// wij is store in this->W[i](j, j)
	for (GLuint j = 0; j < this->neighbors[b].size(); ++j) {
		if (this->neighbors[b][j] == c) {
			this->W[b](j, j) += cot;
		}
	}
	for (GLuint j = 0; j < this->neighbors[c].size(); ++j) {
		if (this->neighbors[c][j] == b) {
			this->W[c](j, j) += cot;
		}
	}
}

void ArapSolver::setConstrain(GLuint idx) {
	this->is_constrain[idx] = true;
	this->constrain_idx.push_back(idx);
}

void ArapSolver::init() {
	GLuint num_tosolve = this->mesh->numvertices - this->constrain_idx.size();
	// only solve none_constrain p'
	solver.Create(num_tosolve, num_tosolve, 3);

	this->non_constrain_idx.clear();
	for (GLuint i = 1; i <= this->mesh->numvertices; ++i) {
		if (!this->is_constrain[i]) {
			p_to_idx[i] = non_constrain_idx.size();
			non_constrain_idx.push_back(i);
		}
	}

	for (GLuint i = 0; i < this->non_constrain_idx.size(); ++i) {
		GLuint curidx = this->non_constrain_idx[i];
		solver.AddSysElement(i, i, this->w_sum[curidx]); // wij for all j

		for (GLuint j = 0; j < this->neighbors[curidx].size();++j) {
			GLuint nidx = this->neighbors[curidx][j];
			if (!this->is_constrain[nidx]) {
				solver.AddSysElement(i, this->p_to_idx[nidx], -this->W[curidx](j, j)); // -wij
			}
		}
	}

	this->solver.SetRightHandSideMatrix(b); // must set once before factorization or it will crash
	this->solver.CholoskyFactorization();
}

void ArapSolver::updatePPrime() {
	// setup right hand side b
	for (GLuint i = 0; i < this->non_constrain_idx.size(); ++i) {
		GLuint curidx = non_constrain_idx[i];

		for (GLuint j = 0; j < 3; ++j) {
			this->b[j][i] = 0;
		}
		for (GLuint j = 0; j < this->neighbors[curidx].size(); ++j) {
			GLuint nidx = neighbors[curidx][j];
			// wij/2 * (Ri+Rj) * eij
			Eigen::Vector3f bv = 0.5  * this->W[curidx](j, j) * (this->R[curidx] + this->R[nidx]) * this->e[curidx].col(j);
			for (GLuint j = 0; j < 3; ++j) {
				b[j][i] += bv[j];
			}

			// if it is constrain
			// update correspond right hand side
			// + wij * e'ij
			if (this->is_constrain[nidx]) {
				for (GLuint k = 0; k < 3; ++k) {
					b[k][i] += this->mesh->vertices[3 * nidx + k] * this->W[curidx](j, j);
				}
			}
		}
	}

	// solve
	this->solver.SetRightHandSideMatrix(this->b);
	this->solver.CholoskySolve(0);
	this->solver.CholoskySolve(1);
	this->solver.CholoskySolve(2);

	// get solution
	for (GLuint i = 0; i < this->non_constrain_idx.size(); ++i) {
		GLuint curidx = non_constrain_idx[i];
		for (GLuint j = 0; j < 3; ++j) {
			this->p_prime[curidx][j] = solver.GetSolution(j, i);
			//this->mesh->vertices[3 * curidx + j] = solver.GetSolution(j, i);
		}
	}
	for (auto curidx : this->constrain_idx) {
		for (GLuint j = 0; j < 3; ++j) {
			this->p_prime[curidx][j] = this->mesh->vertices[3 * curidx + j];
		}
	}
	this->modified = true;
}

void ArapSolver::updateRotation() {
	for (GLuint i = 1; i <= this->mesh->numvertices; ++i) {
		// compute e'ij
		for (GLuint j = 0; j < this->neighbors[i].size(); ++j) {
			GLuint nidx = neighbors[i][j];
			this->e_prime[i].col(j) = p_prime[i] - p_prime[nidx];
		}
		// SVD
		Eigen::Matrix3f S = this->e[i] * this->W[i] * this->e_prime[i].transpose();
		Eigen::JacobiSVD<Eigen::Matrix3f> svd(S, Eigen::ComputeFullU | Eigen::ComputeFullV);
		Eigen::Matrix3f V = svd.matrixV();
		Eigen::Matrix3f Ut = svd.matrixU().transpose();

		R[i] = V * Ut;
		// correction
		if (R[i].determinant() < 0) {
			Eigen::Matrix3f flip = Eigen::Matrix3f::Identity();
			flip(2, 2) = R[i].determinant();
			R[i] = V * flip * Ut;
		}
	}
}

void ArapSolver::updateMesh() {
	for (GLuint i = 0; i < this->non_constrain_idx.size(); ++i) {
		GLuint curidx = non_constrain_idx[i];
		for (GLuint j = 0; j < 3; ++j) {
			this->mesh->vertices[3 * curidx + j] = this->p_prime[curidx][j];
		}
	}
}

void ArapSolver::reset() {
	this->solver.ResetSolver(0, 0, 0);
}