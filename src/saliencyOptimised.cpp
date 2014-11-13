/*

Aditya Sundararajan and team (Inside Intel)
Nanyang Technological University, Singapore

www.github.com/1143stuff

*/

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <sys/time.h>
#include "stdio.h"

using namespace cv;

static struct timeval tm1;

//Start timer
static inline void mystart() 
{
    gettimeofday(&tm1, NULL);
}

//Stop timer and display time taken
static inline void stop() 
{
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    unsigned long long t = 1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
    printf ("%llu ms\n", t);
}

// Generate Gaussian Pyramid of intensity image
void buildGaussianPyramid(Mat &intensity, vector<Mat> &GauPyr)
{
	vector<Mat> bgr(3);
    
    	split(intensity, bgr);	//Split the input image into 3 separate channels and store it in bgr
	intensity = (bgr[0] + bgr[1] + bgr[2]) / 3;	//Take average of the 3 separated channels. This is the intensity.

	buildPyramid(intensity, GauPyr, 8);	//Construct Gaussian Pyramid of the intensity image
}

//Resize all the images of Feature Map and add them for conspicuity map
void resizeMap(vector<Mat> &featureMap, Mat &conspicuity) 
{
   	int i;
   	Mat dest;
    	conspicuity = Mat::zeros(featureMap[5].size(), 0); //Initialize all pixels to zero

    	for(i = 0; i < 6; i++)
    	{
        	while(featureMap[i].rows != featureMap[5].rows) //Perform pyrDown till the all the images of a Map are of same size
        	{
        	    	pyrDown(featureMap[i], dest);
        	    	featureMap[i] = dest;
        	}
		conspicuity += featureMap[i]; //Add all the images of a feature map and store it in conspicuity map.
    	}
}

//Generate Feature Maps
void buildMap(vector<Mat> &pyramid, Mat &conspicuity)
{
    	int i, c, del, j;
    	Mat src, dest;
    	vector<Mat> featureMap(6);

    	i = 0;

	// Perform centre-surround of Gaussian Pyramid
	// surround = 2 to 4; centre = c + del = 5 to 8
    	for(c = 2; c <= 4; c++)
	{
		for(del = 3; del <= 4; del++)
		{
			src = pyramid[c + del];
		    	for(j = 1; j <= del; j++)
            		{
                		pyrUp(src, dest);
			    	src = dest;
            		}
            	featureMap[i] = abs(pyramid[c] - src(Range(0, pyramid[c].rows), Range(0, pyramid[c].cols)));
            	i++;
		}
	}

	resizeMap(featureMap, conspicuity);
}

//Generate Red, Green, Blue, and Yellow maps required for Color Map
void buildRGBY(vector<Mat> bgr, Mat &intensity, Mat &imageCopy, vector<Mat> &R_GauPyr, vector<Mat> &G_GauPyr, vector<Mat> &B_GauPyr, vector<Mat> &Y_GauPyr)
{
	double maxVal;
	int i, j;
	Vec3b zero(0, 0, 0);
	
#if 0 
	
	vector<Mat> channel(3);

	minMaxLoc(intensity, NULL, &maxVal, NULL, NULL);
	for(i = 0; i < imageCopy.rows; i++)
	{
		for(j = 0; j < imageCopy.cols; j++)
		{
			if(intensity.at<uchar>(i,j) > (int)(maxVal/10))
			{
				imageCopy.at<Vec3b>(i, j)[0] = saturate_cast<uchar>( (255 *imageCopy.at<Vec3b>(i, j)[0])/maxVal );
                imageCopy.at<Vec3b>(i, j)[1] = saturate_cast<uchar>( (255 *imageCopy.at<Vec3b>(i, j)[1])/maxVal );
                imageCopy.at<Vec3b>(i, j)[2] = saturate_cast<uchar>( (255 *imageCopy.at<Vec3b>(i, j)[2])/maxVal );
			}

			else
			{
				imageCopy.at<Vec3b>(i, j) = zero;
			}
		}
	}
	
	split(imageCopy, channel);


    Mat R = channel[2] - ((channel[1]  + channel[0]) / 2) ;
	threshold(R, R, 0, 255, THRESH_TOZERO);

	Mat G = channel[1] - ((channel[0] + channel[2]) / 2);
	threshold(G, G, 0, 255, THRESH_TOZERO);

	Mat B = channel[0] - ((channel[1] + channel[2]) / 2);
	threshold(B, B, 0, 255, THRESH_TOZERO);

	Mat Y = (((bgr[2] + bgr[1]) / 2) - abs(bgr[2] - bgr[1]) / 2 - bgr[0]);
	threshold(Y, Y, 0, 255, THRESH_TOZERO);
	//stop();

	buildPyramid(R, R_GauPyr, 8);
	buildPyramid(G, G_GauPyr, 8);
	buildPyramid(B, B_GauPyr, 8);
	buildPyramid(Y, Y_GauPyr, 8);
	#else
	Mat Y = (((bgr[2] + bgr[1]) / 2) - abs(bgr[2] - bgr[1]) / 2 - bgr[0]);                                                                             
        threshold(Y, Y, 0, 255, THRESH_TOZERO);   

	//Here, find RED GREEN BLUE directly from input image
	buildPyramid(bgr[2], R_GauPyr, 8);
	buildPyramid(bgr[1], G_GauPyr, 8);
	buildPyramid(bgr[0], B_GauPyr, 8);
	buildPyramid(Y, Y_GauPyr, 8);
	#endif
	
}

//Generate Intensity Map
void buildIntensityMap(vector<Mat> &GauPyr, Mat &I_bar)
{
    buildMap(GauPyr, I_bar);
}

//Generate Color Map
void buildColorMap(Mat &intensity, Mat &imageCopy, Mat &C_bar)
{
    	vector<Mat> R_GauPyr(9);
	vector<Mat> G_GauPyr(9);
	vector<Mat> B_GauPyr(9);
	vector<Mat> Y_GauPyr(9);
    	vector<Mat> RG(6);
	vector<Mat> BY(6);
	vector<Mat> colorMap(6);

	Mat temp1, temp2, src, dest;

	int i, c, del, j;

    	buildRGBY(intensity, imageCopy, R_GauPyr, G_GauPyr, B_GauPyr, Y_GauPyr);


	// Perform centre-surround of Gaussian Pyramid;
	// surround = 2 to 4; centre = c + del = 5 to 8
    	i = 0;
	for(c = 2; c <= 4; c++)
	{
		for(del = 3; del <= 4; del++)
		{
			temp1 = R_GauPyr[c + del] - G_GauPyr[c + del];
			temp2 = Y_GauPyr[c + del] - B_GauPyr[c + del];

			for(j = 1; j <= del; j++)
            		{
                		pyrUp(temp1, dest);
                		temp1 = dest;

		                pyrUp(temp2, dest);
		                temp2 = dest;
            		}

		RG[i] = abs((R_GauPyr[c] - G_GauPyr[c]) - (temp1(Range(0, R_GauPyr[c].rows), Range(0, R_GauPyr[c].cols))));
		BY[i] = abs(temp1(Range(0, B_GauPyr[c].rows), Range(0, B_GauPyr[c].cols)) - (B_GauPyr[c] - Y_GauPyr[c]));

		colorMap[i] = RG[i] + BY[i];
		i++;
		}
	}

	resizeMap(colorMap, C_bar);
}


//Generate Orientation Map
void buildOrientationMap(Mat &intensity, Mat &O_bar)
{
    	vector<Mat> kernel(4);
	vector<Mat> gaborImage(4);

	vector<Mat> GauPyr_0(9);
	vector<Mat> GauPyr_45(9);
	vector<Mat> GauPyr_90(9);
	vector<Mat> GauPyr_135(9);

	Mat totalOriMap_0, totalOriMap_45, totalOriMap_90, totalOriMap_135;
    	int theta, i, j = 0;

	//Calculate Gabor Filter for theta = 0, 45, 90, 135	
	for(theta = 0; theta < 4; theta++)
	{
		kernel[theta] = getGaborKernel(Size (7, 7), 3, theta * (CV_PI/4), 1.0, 0, 0);
		filter2D(intensity, gaborImage[theta], -1, kernel[theta]);
	}

	buildPyramid(gaborImage[0], GauPyr_0, 8);
	buildPyramid(gaborImage[1], GauPyr_45, 8);
	buildPyramid(gaborImage[2], GauPyr_90, 8);
	buildPyramid(gaborImage[3], GauPyr_135, 8);

	buildMap(GauPyr_0, totalOriMap_0);
	buildMap(GauPyr_45, totalOriMap_45);
	buildMap(GauPyr_90, totalOriMap_90);
	buildMap(GauPyr_135, totalOriMap_135);

	//Add each conspicuity map for theta = 0, 45, 90, 135
	//This will give conspicuity map for orientation	
	O_bar = totalOriMap_0 + totalOriMap_45 + totalOriMap_90 + totalOriMap_135;
}

//Build Saliency Map
void buildSaliencyMap(Mat &I_bar, Mat &C_bar, Mat &O_bar, Mat &image)
{

    	double maxVal;
    	Mat dest;

	Mat saliencyMap = (I_bar + C_bar + O_bar) / 3; //Saliency Map is average of the 3 conspicuity maps
        
	
	resize(saliencyMap, dest, image.size(), 0, 0, INTER_LINEAR); //resize saliency map to size of input image	
	saliencyMap = dest;
	//imwrite("sal_map.bmp", saliencyMap);

	int i = 0, count = 0;

	int thresh = 160;

   	threshold(saliencyMap, saliencyMap, thresh, 255, THRESH_BINARY); //Perform binary threshold on Saliency Map image
    	//imwrite("sal_map2.bmp", saliencyMap);
    	
	Point prevmaxLoc, maxLoc;
	minMaxLoc(saliencyMap, NULL, &maxVal, NULL, &maxLoc);
	prevmaxLoc = maxLoc;

	while(maxVal > 0 && count < 8)
	{
		if(i != 0)
	    		circle(image, maxLoc, 50, Scalar(255, 0, 0), 2, 8, 0); //Draw circle on salient object
		else
			circle(image, maxLoc, 50, Scalar(0, 0, 255), 2, 8, 0); //Draw circle on first salient object

		line(image, prevmaxLoc, maxLoc, CV_RGB(0, 0, 0), 2, 8, 0);     //Draw line passing through each salient object

		circle(saliencyMap, maxLoc, 150, Scalar(0, 0, 0), -1);	       //After detection of salient object, remove that object

        	prevmaxLoc = maxLoc;
		minMaxLoc(saliencyMap, NULL, &maxVal, NULL, &maxLoc);
		
		i++;
		count++;
	}

	imwrite("saliencyOut_unoptimised.bmp", image); //output image
	stop();
}

int main(int argc, char *argv[]) {

	time_t start, end;

	vector<Mat> GauPyr(9);

	Mat I_bar, C_bar, O_bar;
        mystart();
	Mat image = imread(argv[1]);
	stop();
	Mat imageCopy = image;
	Mat intensity = image;

    	mystart();

    	buildGaussianPyramid(intensity, GauPyr);	//Generate Gaussian Pyramid
    	printf("\nGuassianPyramid::");
	stop();
   
    	buildIntensityMap(GauPyr, I_bar);		//Generate Intensity Map
    	printf("\nIntensity Map::");
	stop();


    	buildColorMap(intensity, imageCopy, C_bar);	//Generate Color Map
	printf("\nColor Map::");
	stop();	

	buildOrientationMap(intensity, O_bar);		//Generate Orientation Map
    	printf("\nOrientation Map::");
	stop();

 
    	buildSaliencyMap(I_bar, C_bar, O_bar, image);	//Generate Saliency Map
    	printf("\nSaliency Map::");
	stop();
	
    return 0;
}
