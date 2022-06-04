#include <opencv2/opencv.hpp>
#include <iostream>
using namespace std;
using namespace cv;

class Template{
    public:
        Template();
        void initTracking(Mat frame, Rect box, int scale = 5);
        Rect track(Mat frame);
        Rect getLocation();
    private:
        Mat model;
        Rect location;
        int scale;
};

Template::Template()
{
    this->scale = 5;
}

void Template::initTracking(Mat frame, Rect box, int scale)
{
    this->location = box;
    this->scale = scale;
    this->location &= Rect(0, 0, frame.cols, frame.rows);

    if(frame.empty())
    {
        cout << "ERROR: frame is empty." << endl;
        exit(0);
    }
    if(frame.channels() != 1)
    {
        cvtColor(frame, frame, CV_RGB2GRAY);
    }