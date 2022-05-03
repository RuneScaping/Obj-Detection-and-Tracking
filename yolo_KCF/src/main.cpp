
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/core/ocl.hpp>

#include "utils.h"

using namespace std;

int main(int argc, char **argv)
{
    // List of tracker types in OpenCV 3.2
    // NOTE : GOTURN implementation is buggy and does not work.
    string trackerTypes[7] = {"BOOSTING", "MIL", "KCF", "TLD","MEDIANFLOW", "GOTURN", "CSRT"};
    // vector <string> trackerTypes(types, std::end(types));

    // Create a tracker
    string trackerType = trackerTypes[3];

    cv::Ptr<cv::Tracker> tracker;

    if (trackerType == "BOOSTING")
        tracker = cv::TrackerBoosting::create();
    if (trackerType == "MIL")
        tracker = cv::TrackerMIL::create();
    if (trackerType == "KCF")
        tracker = cv::TrackerKCF::create();
    if (trackerType == "TLD")
        tracker = cv::TrackerTLD::create();
    if (trackerType == "MEDIANFLOW")
        tracker = cv::TrackerMedianFlow::create();
    if (trackerType == "GOTURN")