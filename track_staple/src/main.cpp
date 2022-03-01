
#include "staple_tracker.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <numeric>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

bool gotBB = false;
Rect box;
bool drawingBox = true;

void onMouse(int event, int x, int y, int flags, void *param)
{
    switch(event)
    {
        case CV_EVENT_MOUSEMOVE:
            if (drawingBox)
            {
                box.width = x - box.x;
                box.height = y - box.y;
            }
            break;
        case CV_EVENT_LBUTTONDOWN:
            drawingBox = true;
            box = Rect(x, y, 0, 0);
            break;
        case CV_EVENT_LBUTTONUP:
            drawingBox = false;
            if(box.width < 0)
            {
                box.x +=box.width;
                box.width *= -1;
            }
            if(box.height < 0)
            {
                box.y += box.height;
                box.height *= -1;
            }
            gotBB = true;
            break;
    }
}

int main(int argc, char * argv[])
{
    cv::VideoCapture capture;
    capture.open( 0 );
    if(!capture.isOpened())
    {
        std::cout << "fail to open" << std::endl;
        exit(0);
    }
    //int frame_number = capture.get(CV_CAP_PROP_FRAME_COUNT);
    
    cvNamedWindow("Tracker", CV_WINDOW_AUTOSIZE);
    cvSetMouseCallback("Tracker", onMouse, NULL);

    Mat image;
    capture >> image;
    while (!gotBB)
    {
        capture >> image;

        imshow("Tracker", image);
        if (cvWaitKey(10) == 'q')