
#ifndef STAPLE_TRACKER_HPP
#define STAPLE_TRACKER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#define _PI 3.141592653589793
#define _2PI 6.283185307179586

struct staple_cfg
{
    bool grayscale_sequence = false;    // suppose that sequence is colour
    int hog_cell_size = 4;
    int fixed_area = 150*150;           // standard area to which we resize the target