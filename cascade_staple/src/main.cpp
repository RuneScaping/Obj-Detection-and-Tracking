
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

void limitRect(cv::Rect &location, cv::Size sz)
{
    cv::Rect window(cv::Point(0, 0), sz);
    location = location & window;
}

int main(int argc, char * argv[])
{
    //VideoWriter output_dst("detectracker.avi", CV_FOURCC('M', 'J', 'P', 'G'), 23, Size(640, 480), 1);
    STAPLE_TRACKER staple;
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
        if( frame.empty() )
        {
            printf(" --(!) No captured frame -- Break!");
            break;
        }
        frame_num++;
        tic = cv::getTickCount();
        if(status == 0)
        {
            //cout << "[debug] " << frame_num << ":" << " 没有目标" << endl;
            std::vector<Rect> boards;
            cv::Mat frame_gray;
            cv::cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
            detector.detectMultiScale( frame_gray, boards, 1.1, 2, 0|CASCADE_SCALE_IMAGE, Size(60, 60) ,Size(100, 100));
            if(boards.size() > 0)
            {
                cout << "[debug] " << frame_num << ":" << " cascade找到" << boards.size() << "个目标" << endl;
                for(int i = 0; i < boards.size(); i++)
                {
                    rectangle( frame, cvPoint(cvRound(boards[i].x), cvRound(boards[i].y)),
                             cvPoint(cvRound((boards[i].x + boards[i].width-1)), cvRound((boards[i].y + boards[i].height-1))),
                             Scalar( 20, 20, 20 ), 3, 8, 0);
                }
                cv::imshow("detectracker", frame);
                //waitKey(0);
                if(boards.size() == 1)
                    location = boards[0];
                else
                {
                    int max_area = boards[0].width * boards[0].height;
                    int max_index = 0;
                    for(int i = 1; i < boards.size(); i++)
                    {
                        int area = boards[i].width * boards[i].height;
                        if(area > max_index)
                        {
                            max_area = area;
                            max_index = i;
                        }
                    }
                    location = boards[max_index];
                }
                staple.tracker_staple_initialize(frame, location);
                staple.tracker_staple_train(frame, true);
                status = 1;
                cout << "[debug] " << frame_num << ":" << " 开始跟踪" << endl;
            }
        }
        else if(status == 1)
        {
            location = staple.tracker_staple_update(frame);
            limitRect(location, frame.size());
            if(location.empty())
            {
                status = 0;
                continue;
            }