//@author: Aditya Sundararajan and Team (Inside Intel)
// www.github.com/1143stuff


#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <sys/time.h>
#include "stdio.h"

using namespace cv;

static struct timeval tm1;

static inline void mystart()
{
    gettimeofday(&tm1, NULL);
}

static inline void stop()
{
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    unsigned long long t = 1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
    printf("%llu ms\n", t);
}

void buildGaussianPyramid(Mat &n, vector<Mat> &bgr, vector<Mat> &GauPyr)
{
    int i = 0;
    split(n, bgr);
	n = (bgr[0] + bgr[1] + bgr[2]) / 3;

	vector<Mat> GauPyr2(9);

	GauPyr[0] = n;

	printf("Optimization check\n");

	mystart();
	for(i = 1; i < 9; i++)
    {
        pyrDown(GauPyr[i - 1], GauPyr[i]);
        //resize(GauPyr[i - 1], GauPyr[i], GauPyr[i - 1].size(), 0, 0, INTER_LINEAR);
    }
    stop();

	mystart();
	buildPyramid(n, GauPyr2, 8);
	stop();
}

void buildRGBY(Mat &n, vector<Mat> &bgr, vector<Mat> &R_GauPyr, vector<Mat> &G_GauPyr, vector<Mat> &B_GauPyr, vector<Mat> &Y_GauPyr)
{
	double maxVal;
	int i, j;
	minMaxLoc(n, NULL, &maxVal, NULL, NULL);

	for(i = 0; i < n.rows; i++)
	{
		for(j = 0; j < n.cols; j++)
		{
			if(n.at<uchar>(i,j) > (int)(maxVal/10))
			{
				bgr[0].at<uchar>(i,j) = (uchar)((int)(bgr[0].at<uchar>(i,j) * 255) / maxVal);
				bgr[1].at<uchar>(i,j) = (uchar)((int)(bgr[1].at<uchar>(i,j) * 255) / maxVal);
				bgr[2].at<uchar>(i,j) = (uchar)((int)(bgr[2].at<uchar>(i,j) * 255) / maxVal);
			}

			else
			{
				bgr[0].at<uchar>(i,j) = (uchar)0;
				bgr[1].at<uchar>(i,j) = (uchar)0;
				bgr[2].at<uchar>(i,j) = (uchar)0;
			}
		}
	}

	Mat R = (bgr[2] - (bgr[1] /2  + bgr[0] / 2 )) ;
	R = R.setTo(0, R < 0);

	Mat G = (bgr[1]) - ((bgr[0] / 2 + (bgr[2]) / 2));
	G = G.setTo(0, G < 0);

	Mat B = bgr[0] - (((bgr[1]) / 2 + (bgr[2]) / 2));
	B = B.setTo(0, B < 0);

	Mat Y = (((bgr[1] / 2) + (bgr[2] / 2))) - (abs((bgr[1] / 2) - bgr[2] / 2)) - bgr[0];
	Y = Y.setTo(0, Y < 0);

	buildPyramid(R, R_GauPyr, 8);
	buildPyramid(G, G_GauPyr, 8);
	buildPyramid(B, B_GauPyr, 8);
	buildPyramid(Y, Y_GauPyr, 8);
}

void buildIntensityMap(vector<Mat> &GauPyr, Mat &dest, vector<Mat> &IntCon)
{
    int i = 0, c, del;

    for(c = 4; c <= 6; c++)
	{
		for(del = -4; del <= -3; del++)
		{
			resize(GauPyr[c], dest, GauPyr[c + del].size(), 0, 0, INTER_LINEAR);
			GauPyr[c] = dest;
			IntCon[i] = abs(GauPyr[c + del] - GauPyr[c]);
			i++;
		}
	}
}

void buildColorMap(vector<Mat> &R_GauPyr, vector<Mat> &G_GauPyr, vector<Mat> &B_GauPyr, vector<Mat> &Y_GauPyr, Mat &dest, vector<Mat> &RG, vector<Mat> &BY)
{
    Mat temp1 = R_GauPyr[0]; //Think about optimizing this by changing 0 to 8
	Mat temp2 = R_GauPyr[0]; //Think about optimizing this by changing 0 to 8
	int i = 0, c, del;

	for(c = 4; c <= 6; c++)
	{
		for(del = -4; del <= -3; del++)
		{
			temp1 = R_GauPyr[c] - G_GauPyr[c];
			temp2 = G_GauPyr[c + del] - R_GauPyr[c + del];
			resize(temp1, dest, temp2.size(), 0, 0, INTER_LINEAR);
			temp1 = dest;
			RG[i] = abs(temp1 - temp2);

			temp1 = Y_GauPyr[c + del] - B_GauPyr[c + del];
			temp2 = B_GauPyr[c] - Y_GauPyr[c];
			resize(temp1, dest, temp2.size(), 0, 0, INTER_LINEAR);
			temp1 = dest;
			BY[i] = abs(temp1 - temp2);
			i++;
		}
	}
}

void buildOrientationMap(vector<Mat> &GauPyr, vector<Mat> &orientation, Mat &dest)
{
    vector<Mat> kernel(4);
    int theta, i, c, del;
    vector<Mat> o_map(36);

	for(theta = 0; theta < 4; theta++)
	{
		kernel[theta] = getGaborKernel(Size (9, 9), 1, theta * (CV_PI/4), 1.0, 0.02, 0);
	}

	for(i = 0; i < 9; i++)
	{
		for(theta = 0; theta < 4; theta++)
		{
			filter2D(GauPyr[i], o_map[(i * 4) + theta], -1, kernel[theta], Point(-1, -1), 0, BORDER_DEFAULT);
		}
	}

	i = 0;
	for(c = 4; c <= 6; c++)
	{
		for(del = -4; del <= -3; del++)
        {
            for(theta = 0; theta < 4; theta++)
            {
                resize(o_map[(c * 4) + theta], dest, o_map[((c + del) * 4) + theta].size(), 0, 0, INTER_LINEAR);
                o_map[(c * 4) + theta] = dest;
                orientation[i] = abs(o_map[(c * 4) + theta] - o_map[((c + del) * 4) + theta]);
                i++;
            }
        }
	}
}

void resizeMaps(vector<Mat> &IntCon, vector<Mat> &RG, vector<Mat> &BY, vector<Mat> &orientation, Mat &dest)
{
    int i;
    pyrDown(IntCon[5], dest);// Size(0, 0), IntCon[0].rows/16, IntCon[0].cols/16, INTER_LINEAR);
    IntCon[5] = dest;

    for(i = 0; i < 5; i++)
    {
        resize(IntCon[i], dest, IntCon[5].size(), 0, 0, INTER_LINEAR);
        IntCon[i] = dest;
    }

    for(i = 0; i < 6; i++)
    {
        resize(RG[i], dest, IntCon[5].size(), 0, 0, INTER_LINEAR);
        RG[i] = dest;
        resize(BY[i], dest, IntCon[5].size(), 0, 0, INTER_LINEAR);
        BY[i] = dest;
    }

    for(i = 0; i < 24; i++)
    {
        resize(orientation[i], dest, IntCon[5].size(), 0, 0, INTER_LINEAR);
        orientation[i] = dest;
    }
}

void buildConspicuityMap(vector<Mat> &IntCon, vector<Mat> &RG, vector<Mat> &BY, vector<Mat> &orientation, Mat &I_bar, Mat &C_bar, Mat &O_bar)
{
    int i;
    double maxVal;

    I_bar = Mat::zeros(IntCon[0].size(), 0);
    C_bar = I_bar;
	O_bar = I_bar;

	for(i = 0; i < 6; i++)
	{
		minMaxLoc(IntCon[i], NULL, &maxVal, NULL, NULL);
		normalize(IntCon[i], IntCon[i], 0, maxVal, NORM_MINMAX, -1);
		I_bar += (IntCon[i] / 6);
	}
	//I_bar /= 6;

	for(i = 0; i < 6; i++)
	{
		minMaxLoc(RG[i], NULL, &maxVal, NULL, NULL);
		normalize(RG[i], RG[i], 0, maxVal, NORM_MINMAX, -1);
		minMaxLoc(BY[i], NULL, &maxVal, NULL, NULL);
		normalize(BY[i], BY[i], 0, maxVal, NORM_MINMAX, -1);
		C_bar += ((RG[i] / 6) + (BY[i] / 6));
	}
	//C_bar /= 6;

	for(i = 0; i < 24; i++)
	{
		minMaxLoc(orientation[i], NULL, &maxVal, NULL, NULL);
		normalize(orientation[i], orientation[i], 0, maxVal, NORM_MINMAX, -1);
		O_bar += (orientation[i] / 24);
	}
	//O_bar /= 24;

	minMaxLoc(I_bar, NULL, &maxVal, NULL, NULL);
	normalize(I_bar, I_bar, 0, maxVal, NORM_MINMAX, -1);
	minMaxLoc(C_bar, NULL, &maxVal, NULL, NULL);
	normalize(C_bar, C_bar, 0, maxVal, NORM_MINMAX, -1);
	minMaxLoc(O_bar, NULL, &maxVal, NULL, NULL);
	normalize(O_bar, O_bar, 0, maxVal, NORM_MINMAX, -1);


}

void buildSaliencyMap(Mat &I_bar, Mat &C_bar, Mat &O_bar, Mat &dest, Mat &image)
{
    resize(I_bar, dest, image.size(), 0, 0, INTER_LINEAR);
	I_bar = dest;
	resize(C_bar, dest, image.size(), 0, 0, INTER_LINEAR);
	C_bar = dest;
	resize(O_bar, dest, image.size(), 0, 0, INTER_LINEAR);
	O_bar = dest;
    Mat s = ((I_bar / 3) + (C_bar / 3) + (O_bar / 3));

	//resize(s, dest, image.size(), 0, 0, INTER_LINEAR);
	//s = dest;

	imwrite("sal_map.bmp", s);
	//imwrite("con1.bmp", I_bar);
	//imwrite("con2.bmp", C_bar);
	//imwrite("con3.bmp", O_bar);

	int i = 0, count = 0;
	double maxVal;
	Point prevmaxLoc, maxLoc;
	minMaxLoc(s, NULL, &maxVal, NULL, &maxLoc);

	while(count < 6)
	{
		if(i != 0)
		circle(image, maxLoc, 50, Scalar(255, 255, 0), 1, 8, 0);

		else
		{
		    circle(image, maxLoc, 50, Scalar(0, 0, 255), 1, 8, 0);
		    //line(finalImage, prevmaxLoc, maxLoc, CV_RGB(255, 255, 255), 1, 8, 0);
		}

		circle(s, maxLoc, 30, Scalar(0, 0, 0), -1);

        prevmaxLoc = maxLoc;
		minMaxLoc(s, NULL, &maxVal, NULL, &maxLoc);
		i = 1;
		count++;
	}
	imwrite("saliency_output5.bmp", image);

}

int main(int argc, char *argv[]) {

	time_t start, end;

	vector<Mat> bgr(3);
	vector<Mat> GauPyr(9);
	vector<Mat> IntCon(6);
	vector<Mat> RG(6);
	vector<Mat> BY(6);
	vector<Mat> orientation(24);
	vector<Mat> R_GauPyr(9);
	vector<Mat> G_GauPyr(9);
	vector<Mat> B_GauPyr(9);
	vector<Mat> Y_GauPyr(9);

	Mat I_bar, C_bar, O_bar;

	Mat image = imread("gr.png");
	Mat n = image.clone();
	Mat dest;

//	Mat I_bar, C_bar, O_bar;

    //mystart();
    buildGaussianPyramid(n, bgr, GauPyr);


   // mystart();
	buildRGBY(n, bgr, R_GauPyr, G_GauPyr, B_GauPyr, Y_GauPyr);


   // mystart();
	buildIntensityMap(GauPyr, dest, IntCon);  //Intensity Map


    //mystart();
	buildColorMap(R_GauPyr, G_GauPyr, B_GauPyr, Y_GauPyr, dest, RG, BY);


   // mystart();
    buildOrientationMap(GauPyr, orientation, dest);


   // mystart();
    resizeMaps(IntCon, RG, BY, orientation, dest);


    //mystart();
    buildConspicuityMap(IntCon, RG, BY, orientation, I_bar, C_bar, O_bar);//, I_bar, C_bar, O_bar);


    //mystart();
    buildSaliencyMap(I_bar, C_bar, O_bar, dest, image);

}
