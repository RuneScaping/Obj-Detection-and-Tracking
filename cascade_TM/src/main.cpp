
#include "template.h"

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

void limitRect(cv::Rect &location, cv::Size sz)
{
    cv::Rect window(cv::Point(0, 0), sz);
    location = location & window;
}

int main(int argc, char * argv[])
{
    //VideoWriter output_dst("newtracker.avi", CV_FOURCC('M', 'J', 'P', 'G'), 60, Size(640, 480), 1);
    Template tracker;
    cv::VideoCapture capture;
    capture.open("new.avi");
    if(!capture.isOpened())
    {
        std::cout << "fail to open" << std::endl;
        exit(0);
    }
    cv::String cascade_name = "cascade.xml";;
    cv::CascadeClassifier detector;
    if( !detector.load( cascade_name ) )
    { 
        printf("--(!)Error loading face cascade\n"); 
        return -1; 
    };


    int64 tic, toc;
    double time = 0;
    bool show_visualization = true;
    int status = 0;     //0：没有目标，1：找到目标进行跟踪
    cv::Mat frame;
    int frame_num = 0;
    Rect location;
    std::vector<cv::Rect_<float>> result_rects;
    while ( capture.read(frame) )
    {