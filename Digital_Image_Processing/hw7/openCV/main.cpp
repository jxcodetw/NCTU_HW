#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <highgui.h>
#include <complex>
#define _USE_MATH_DEFINES
#include <cmath>
#include "cv.h"
using namespace std;
using namespace cv;

void oddeven(std::complex<double> *x, int N) {
	std::complex<double> *odd = new std::complex<double>[N / 2];
	std::complex<double> *even = new std::complex<double>[N / 2];

	for (int i = 0; i < N / 2; ++i) {
		int idx = 2 * i;
		even[i] = x[idx + 0];
		odd[i] = x[idx + 1];
	}

	for (int i = 0; i < N / 2; ++i) {
		int j = i + N / 2;
		x[i] = even[i];
		x[j] = odd[i];
	}

	delete[] odd;
	delete[] even;
}

void fft1D(std::complex<double> *x, int N) {
	if (N <= 1) {
		// do nothing
	} else {
		oddeven(x, N);
		fft1D(x, N / 2);
		fft1D(x + N / 2, N / 2);

		// combine
		for (int k = 0; k < N / 2; ++k) {
			std::complex<double> ek = x[k];
			std::complex<double> ok = x[k+N/2];

			double power = -2 * M_PI * k / (double)N;
			std::complex<double> twiddle = exp(complex<double>(0, power));
			x[k] = ek + twiddle * ok;
			x[k + N / 2] = ek - twiddle * ok;
		}
	}
}

double complex_len(std::complex<double> x) {
	double len;
	len = x.real() * x.real() + x.imag() * x.imag();
	return sqrt(len);
}

int main(){

	// Read input images
	// Fig3.tif is in openCV\bin\Release
	Mat SrcImg = imread("Q4_512.tif", CV_LOAD_IMAGE_GRAYSCALE);
	printf("%d, %d\n", SrcImg.rows, SrcImg.cols);

	// Create a grayscale output image matrix
	Mat DstImg = Mat(SrcImg.rows*2, SrcImg.cols*2, CV_8UC1);

	// copy image to complex value img
	std::complex<double> **img = new std::complex<double>*[SrcImg.rows];
	std::complex<double> **imgt = new std::complex<double>*[SrcImg.cols];
	for (int i = 0; i < SrcImg.rows; ++i) {
		img[i] = new std::complex<double>[SrcImg.cols];
		imgt[i] = new std::complex<double>[SrcImg.rows];
		for (int j = 0; j<SrcImg.cols; ++j)
		{
			img[i][j]._Val[0] = (double)SrcImg.at<uchar>(i, j);
			img[i][j]._Val[1] = 0;
		}
	}

	// todo: handle arbitrary size image

	// fft for each row
	for (int i = 0; i < SrcImg.rows; ++i) {
		fft1D(img[i], SrcImg.cols);
		// transpose
		for (int j = 0; j < SrcImg.cols; ++j) {
			imgt[j][i] = img[i][j];
		}
	}

	// fft for each column
	for (int j = 0; j < SrcImg.cols; ++j) {
		fft1D(imgt[j], SrcImg.rows);
		// transpose
		for (int i = 0; i < SrcImg.rows; ++i) {
			img[i][j] = imgt[j][i];
		}
	}

	double mx = -1e9;
	double mn = 1e9;
	double epsilon = 1e-9; // for log
	// log to get more detail
	for (int i = 0; i < SrcImg.rows; ++i) {
		for (int j = 0; j < SrcImg.cols; ++j) {
			img[i][j]._Val[0] = log(epsilon + complex_len(img[i][j]));
			img[i][j]._Val[1] = 0;

			if (img[i][j].real() > mx) {
				mx = img[i][j].real();
			}
			if (img[i][j].real() < mn) {
				mn = img[i][j].real();
			}
		}
	}
	printf("min value: %lf, max value: %lf\n", mn, mx);

	// resacle to 0 - 255
	for (int i = 0; i < SrcImg.rows; ++i) {
		for (int j = 0; j < SrcImg.cols; ++j) {
			double x = 255.0 * (img[i][j].real()-mn) / (mx-mn);
			DstImg.at<uchar>(i, j) = (uchar)x;
			DstImg.at<uchar>(i+SrcImg.rows-1, j) = (uchar)x;
			DstImg.at<uchar>(i, j+SrcImg.cols-1) = (uchar)x;
			DstImg.at<uchar>(i+SrcImg.rows-1, j + SrcImg.cols-1) = (uchar)x;
		}
	}


	// Show images
	imshow("Input Image", SrcImg);
	imshow("Output Image", DstImg);

	// Write output images
	imwrite("Fig3_output.tif", DstImg);
	
	waitKey(0);
	return 0;
}