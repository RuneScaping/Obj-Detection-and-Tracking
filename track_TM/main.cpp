#include <opencv2/opencv.hpp>
#include "template.h"
#include <iostream>
using namespace std;
using namespace cv;

Rect box;
bool drawingBox = false;
bool gotBB = false;

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
    Template tracker;
    //cout << "C++11 : " << to_string(2333) << endl;
    VideoCapture capture;
    capture.open(0);

    if(!capture.isOpened())
    {
        cout << "capture device failed to open!" << endl;
        return -1;
    }
    cvNamedWindow("Tracker", CV_WINDOW_AUTOSIZE);
    cvSetMouseCallback("Tracker", onMouse, NULL);

    Mat frame;
    capture >> fram