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
       