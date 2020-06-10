#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <highgui.h>
#include "cv.h"
#include <cmath>
using namespace std;
using namespace cv;

// for compute convolution with filter
namespace Kernel {
	class Base {
	protected:
		int max_row;
		int max_col;

		int kernel_row;
		int kernel_col;
		double **weights;

		// init weight
		Base(int row, int col) {
			this->kernel_row = row;
			this->kernel_col = col;

			this->weights = new double*[row];
			for (int i = 0; i < row; ++i) {
				this->weights[i] = new double[col];
			}
		}

		// check if the position is out of the image
		bool check_border(int row, int col) {
			return (row >= 0 && row < this->max_row &&
					col >= 0 && col < this->max_col);
		}
	public:
		// do the convolution
		double calc(Mat &SrcImg, int r, int c) {
			this->max_row = SrcImg.rows;
			this->max_col = SrcImg.cols;

			int row_range = this->kernel_row / 2;
			int col_range = this->kernel_col / 2;

			double ret = 0;
			for (int i = -row_range; i <= row_range; ++i) {
				for (int j = -col_range; j <= col_range; ++j) {
					if (check_border(r + i, c + j)) {
						ret += this->weights[i + row_range][j + col_range] * (double)SrcImg.at<uchar>(r + i, c + j);
					}
					
				}
			}
			return ret;
		}
	};

	// define filters
	class Laplacian : public Base {
	public:
		Laplacian() : Base(3, 3) {
			double w[3][3] = {
				{ 0, -1,  0 },
				{ -1, 4, -1 },
				{ 0, -1,  0 }
			};

			for (int i = 0; i < this->kernel_row; ++i) {
				for (int j = 0; j < this->kernel_col; ++j) {
					this->weights[i][j] = w[i][j];
				}
			}
		}
	};

	class SobelY : public Base {
	public:
		SobelY() : Base(3, 3) {
			double w[3][3] = {
				{ -1, 0, 1 },
				{ -2, 0, 2 },
				{ -1, 0, 1 }
			};

			for (int i = 0; i < this->kernel_row; ++i) {
				for (int j = 0; j < this->kernel_col; ++j) {
					this->weights[i][j] = w[i][j];
				}
			}
		}
	};

	class SobelX : public Base {
	public:
		SobelX() : Base(3, 3) {
			double w[3][3] = {
				{ -1,-2, -1},
				{  0, 0, 0 },
				{  1, 2, 1 }
			};

			for (int i = 0; i < this->kernel_row; ++i) {
				for (int j = 0; j < this->kernel_col; ++j) {
					this->weights[i][j] = w[i][j];
				}
			}
		}
	};

	class Avg55 : public Base {
	public:
		Avg55() : Base(5, 5) {
			double w[5][5] = {
				{ 1, 1, 1, 1, 1 },
				{ 1, 1, 1, 1, 1 },
				{ 1, 1, 1, 1, 1 },
				{ 1, 1, 1, 1, 1 },
				{ 1, 1, 1, 1, 1 }
			};

			for (int i = 0; i < this->kernel_row; ++i) {
				for (int j = 0; j < this->kernel_col; ++j) {
					this->weights[i][j] = w[i][j] / 25.0;
				}
			}
		}
	};
}

// singleton
Kernel::Laplacian laplacian;
Kernel::SobelY sobely;
Kernel::SobelX sobelx;
Kernel::Avg55 avg55;

Mat SrcImg;

// for generating a~h images
namespace Image {
	class Base {
	public:
		Mat img;

		// override this method to compute the outptu image
		virtual double calc(int i, int j) {
			puts("please implement this");
			return -1;
		}

		void display(char *name) {
			std::string title(name);
			img = Mat(SrcImg.rows, SrcImg.cols, CV_8UC1);

			double min = 1e9;
			double max = -1e9;
			// keep track of the min and max values
			Mat tmp = Mat(SrcImg.rows, SrcImg.cols, CV_64F);
			for (int i = 0; i < SrcImg.rows; ++i) {
				for (int j = 0; j<SrcImg.cols; ++j) {
					tmp.at<double>(i, j) = this->calc(i, j);
					if (tmp.at<double>(i, j) < min) { min = tmp.at<double>(i, j); }
					if (tmp.at<double>(i, j) > max) { max = tmp.at<double>(i, j); }
				}
			}
			
			// rescale the image value to 0~255
			for (int i = 0; i < SrcImg.rows; ++i) {
				for (int j = 0; j<SrcImg.cols; ++j) {
					img.at<uchar>(i, j) = (uchar)((tmp.at<double>(i, j) - min) / (max - min) * 255.0);
				}
			}
			

			imshow(title.c_str(), img);
			imwrite((title + ".tif").c_str(), img);
		}
	};

	// actual implementation of a~h filters
	class a: public Base {
		double calc(int i, int j) {
			return SrcImg.at<uchar>(i, j);
		}
	} A;

	class b: public Base {
		double calc(int i, int j) {
			return laplacian.calc(SrcImg, i, j);
		}
	} B;

	class c: public Base {
		double calc(int i, int j) {
			return laplacian.calc(SrcImg, i, j) + SrcImg.at<uchar>(i, j);
		}
	} C;

	class d: public Base {
		double calc(int i, int j) {
			double sx = sobelx.calc(SrcImg, i, j);
			double sy = sobely.calc(SrcImg, i, j);
			return sqrt(sx*sx+sy*sy);
		}
	} D;

	class e: public Base {
		double calc(int i, int j) {
			return avg55.calc(D.img, i, j);
		}
	} E;

	class f: public Base {
		double calc(int i, int j) {
			return (double)C.img.at<uchar>(i, j) * (double)E.img.at<uchar>(i, j);
		}
	} F;

	class g: public Base {
		double calc(int i, int j) {
			return (double)A.img.at<uchar>(i, j) + (double)F.img.at<uchar>(i, j);
		}
	} G;

	class h: public Base {
		double calc(int i, int j) {
			return pow((double)G.img.at<uchar>(i, j), 0.5);
		}
	} H;
};




int main() {

	// Read input images
	// Fig3.tif is in openCV\bin\Release

	// a
	SrcImg = imread("srcimg.tif", CV_LOAD_IMAGE_GRAYSCALE);
	
	// caculate and display images
	Image::A.display("(a) Input");
	Image::B.display("(b) Laplacian of (a)");
	Image::C.display("(c) (a)+(b)");
	Image::D.display("(d) Sobel of (a)");
	Image::E.display("(e) averaging of (d)");
	Image::F.display("(f) (c)x(e)");
	Image::G.display("(g) (a)+(f)");
	Image::H.display("(h) Power-law of (g) gamma=0.5");

	waitKey(0);
	return 0;
}