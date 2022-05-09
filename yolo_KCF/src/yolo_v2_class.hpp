
#pragma once
#ifdef YOLODLL_EXPORTS
#if defined(_MSC_VER)
#define YOLODLL_API __declspec(dllexport)
#else
#define YOLODLL_API __attribute__((visibility("default")))
#endif
#else
#if defined(_MSC_VER)
#define YOLODLL_API __declspec(dllimport)
#else
#define YOLODLL_API
#endif
#endif

struct bbox_t {
    unsigned int x, y, w, h;    // (x,y) - top-left corner, (w, h) - width & height of bounded box
    float prob;                    // confidence - probability that the object was found correctly
    unsigned int obj_id;        // class of object - from range [0, classes-1]
    unsigned int track_id;        // tracking id for video (0 - untracked, 1 - inf - tracked object)
    unsigned int frames_counter;// counter of frames on which the object was detected
};

struct image_t {
    int h;                        // height
    int w;                        // width
    int c;                        // number of chanels (3 - for RGB)
    float *data;                // pointer to the image data
};

#define C_SHARP_MAX_OBJECTS 1000
struct bbox_t_container {
    bbox_t candidates[C_SHARP_MAX_OBJECTS];
};

#ifdef __cplusplus
#include <memory>
#include <vector>
#include <deque>
#include <algorithm>

#define OPENCV
#ifdef OPENCV
#include <opencv2/opencv.hpp>            // C++
#include "opencv2/highgui/highgui_c.h"    // C
#include "opencv2/imgproc/imgproc_c.h"    // C
#endif    // OPENCV

extern "C" YOLODLL_API int init(const char *configurationFilename, const char *weightsFilename, int gpu);
extern "C" YOLODLL_API int detect_image(const char *filename, bbox_t_container &container);
extern "C" YOLODLL_API int detect_mat(const uint8_t* data, const size_t data_length, bbox_t_container &container);