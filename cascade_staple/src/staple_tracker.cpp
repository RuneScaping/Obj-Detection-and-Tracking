
/*
 * cv::Size(width, height)
 * cv::Point(y, x)
 * cv::Mat(height, width, channels, ... )
 * cv::Mat save by row after row
 *   2d: address = j * width + i
 *   3d: address = j * width * channels + i * channels + k
 * ------------------------------------------------------------
 * row == heigh == Point.y
 * col == width == Point.x
 * Mat::at(Point(x, y)) == Mat::at(y,x)
 */

#include "fhog.h"
#include "staple_tracker.hpp"
#include <iomanip>

// mexResize got different results using different OpenCV, it's not trustable
// I found this bug by running vot2015/tunnel, it happened when frameno+1==22 after frameno+1==21
void STAPLE_TRACKER::mexResize(const cv::Mat &im, cv::Mat &output, cv::Size newsz, const char *method) {
    int interpolation = cv::INTER_LINEAR;

    cv::Size sz = im.size();

    if(!strcmp(method, "antialias")){
      interpolation = cv::INTER_AREA;
    } else if (!strcmp(method, "linear")){
      interpolation = cv::INTER_LINEAR;
    } else if (!strcmp(method, "auto")){
      if(newsz.width > sz.width){ // xxx
        interpolation = cv::INTER_LINEAR;
      }else{
        interpolation = cv::INTER_AREA;
      }
    } else {
        assert(0);
        return;
    }

  resize(im, output, newsz, 0, 0, interpolation);
}

staple_cfg STAPLE_TRACKER::default_parameters_staple(staple_cfg cfg)
{
    return cfg;
}

void STAPLE_TRACKER::initializeAllAreas(const cv::Mat &im)
{
    // we want a regular frame surrounding the object
    double avg_dim = (cfg.target_sz.width + cfg.target_sz.height) / 2.0;

    bg_area.width = round(cfg.target_sz.width + avg_dim);
    bg_area.height = round(cfg.target_sz.height + avg_dim);

    // pick a "safe" region smaller than bbox to avoid mislabeling
    fg_area.width = round(cfg.target_sz.width - avg_dim * cfg.inner_padding);
    fg_area.height = round(cfg.target_sz.height - avg_dim * cfg.inner_padding);

    // saturate to image size
    cv::Size imsize = im.size();

    bg_area.width = std::min(bg_area.width, imsize.width - 1);
    bg_area.height = std::min(bg_area.height, imsize.height - 1);

    // make sure the differences are a multiple of 2 (makes things easier later in color histograms)
    bg_area.width = bg_area.width - (bg_area.width - cfg.target_sz.width) % 2;
    bg_area.height = bg_area.height - (bg_area.height - cfg.target_sz.height) % 2;

    fg_area.width = fg_area.width + (bg_area.width - fg_area.width) % 2;
    fg_area.height = fg_area.height + (bg_area.height - fg_area.width) % 2;

    //std::cout << "bg_area.width " << bg_area.width << " bg_area.height " << bg_area.height << std::endl;
    //std::cout << "fg_area.width " << fg_area.width << " fg_area.height " << fg_area.height << std::endl;

    // Compute the rectangle with (or close to) params.fixedArea
    // and same aspect ratio as the target bbox

    area_resize_factor = sqrt(cfg.fixed_area / double(bg_area.width * bg_area.height));
    norm_bg_area.width = round(bg_area.width * area_resize_factor);
    norm_bg_area.height = round(bg_area.height * area_resize_factor);

    //std::cout << "area_resize_factor " << area_resize_factor << " norm_bg_area.width " << norm_bg_area.width << " norm_bg_area.height " << norm_bg_area.height << std::endl;

    // Correlation Filter (HOG) feature space
    // It smaller that the norm bg area if HOG cell size is > 1
    cf_response_size.width = floor(norm_bg_area.width / cfg.hog_cell_size);
    cf_response_size.height = floor(norm_bg_area.height / cfg.hog_cell_size);