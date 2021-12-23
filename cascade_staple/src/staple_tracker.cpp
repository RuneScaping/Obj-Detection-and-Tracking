
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

    // given the norm BG area, which is the corresponding target w and h?
    double norm_target_sz_w = 0.75*norm_bg_area.width - 0.25*norm_bg_area.height;
    double norm_target_sz_h = 0.75*norm_bg_area.height - 0.25*norm_bg_area.width;

    // norm_target_sz_w = params.target_sz(2) * params.norm_bg_area(2) / bg_area(2);
    // norm_target_sz_h = params.target_sz(1) * params.norm_bg_area(1) / bg_area(1);
    norm_target_sz.width = round(norm_target_sz_w);
    norm_target_sz.height = round(norm_target_sz_h);

    //std::cout << "norm_target_sz.width " << norm_target_sz.width << " norm_target_sz.height " << norm_target_sz.height << std::endl;

    // distance (on one side) between target and bg area
    cv::Size norm_pad;

    norm_pad.width = floor((norm_bg_area.width - norm_target_sz.width) / 2.0);
    norm_pad.height = floor((norm_bg_area.height - norm_target_sz.height) / 2.0);

    int radius = floor(fmin(norm_pad.width, norm_pad.height));

    // norm_delta_area is the number of rectangles that are considered.
    // it is the "sampling space" and the dimension of the final merged resposne
    // it is squared to not privilege any particular direction
    norm_delta_area = cv::Size((2*radius+1), (2*radius+1));

    // Rectangle in which the integral images are computed.
    // Grid of rectangles ( each of size norm_target_sz) has size norm_delta_area.
    norm_pwp_search_area.width = norm_target_sz.width + norm_delta_area.width - 1;
    norm_pwp_search_area.height = norm_target_sz.height + norm_delta_area.height - 1;

    //std::cout << "norm_pwp_search_area.width " << norm_pwp_search_area.width << " norm_pwp_search_area.height " << norm_pwp_search_area.height << std::endl;
}

// GET_SUBWINDOW Obtain image sub-window, padding is done by replicating border values.
//   Returns sub-window of image IM centered at POS ([y, x] coordinates),
//   with size MODEL_SZ ([height, width]). If any pixels are outside of the image,
//   they will replicate the values at the borders
void STAPLE_TRACKER::getSubwindow(const cv::Mat &im, cv::Point_<float> centerCoor, cv::Size model_sz, cv::Size scaled_sz, cv::Mat &output)
{
    cv::Size sz = scaled_sz; // scale adaptation

    // make sure the size is not to small
    sz.width = fmax(sz.width, 2);
    sz.height = fmax(sz.height, 2);

    cv::Mat subWindow;

    // xs = round(pos(2) + (1:sz(2)) - sz(2)/2);
    // ys = round(pos(1) + (1:sz(1)) - sz(1)/2);

    cv::Point lefttop(
        std::min(im.cols - 1, std::max(-sz.width + 1, int(centerCoor.x + 1 - sz.width/2.0+0.5))),
        std::min(im.rows - 1, std::max(-sz.height + 1, int(centerCoor.y + 1 - sz.height/2.0+0.5)))
        );

    cv::Point rightbottom(
        std::max(0, int(lefttop.x + sz.width - 1)),
        std::max(0, int(lefttop.y + sz.height - 1))
        );

    cv::Point lefttopLimit(
        std::max(lefttop.x, 0),
        std::max(lefttop.y, 0)
        );
    cv::Point rightbottomLimit(
        std::min(rightbottom.x, im.cols - 1),
        std::min(rightbottom.y, im.rows - 1)
        );

    rightbottomLimit.x += 1;
    rightbottomLimit.y += 1;
    cv::Rect roiRect(lefttopLimit, rightbottomLimit);

    im(roiRect).copyTo(subWindow);

    // imresize(subWindow, output, model_sz, 'bilinear', 'AntiAliasing', false)
    mexResize(subWindow, output, model_sz, "auto");
}

// UPDATEHISTMODEL create new models for foreground and background or update the current ones
void STAPLE_TRACKER::updateHistModel(bool new_model, cv::Mat &patch, double learning_rate_pwp)
{
    // Get BG (frame around target_sz) and FG masks (inner portion of target_sz)

    ////////////////////////////////////////////////////////////////////////
    cv::Size pad_offset1;

    // we constrained the difference to be mod2, so we do not have to round here
    pad_offset1.width = (bg_area.width - target_sz.width) / 2;
    pad_offset1.height = (bg_area.height - target_sz.height) / 2;

    // difference between bg_area and target_sz has to be even
    if (
        (
            (pad_offset1.width == round(pad_offset1.width)) &&
            (pad_offset1.height != round(pad_offset1.height))
        ) ||
        (
            (pad_offset1.width != round(pad_offset1.width)) &&
            (pad_offset1.height == round(pad_offset1.height))
        )) {
        assert(0);
    }

    pad_offset1.width = fmax(pad_offset1.width, 1);
    pad_offset1.height = fmax(pad_offset1.height, 1);

    //std::cout << "pad_offset1 " << pad_offset1 << std::endl;

    cv::Mat bg_mask(bg_area, CV_8UC1, cv::Scalar(1)); // init bg_mask

    // xxx: bg_mask(pad_offset1(1)+1:end-pad_offset1(1), pad_offset1(2)+1:end-pad_offset1(2)) = false;

    cv::Rect pad1_rect(
        pad_offset1.width,
        pad_offset1.height,
        bg_area.width - 2 * pad_offset1.width,
        bg_area.height - 2 * pad_offset1.height
        );

    bg_mask(pad1_rect) = false;

    ////////////////////////////////////////////////////////////////////////
    cv::Size pad_offset2;

    // we constrained the difference to be mod2, so we do not have to round here
    pad_offset2.width = (bg_area.width - fg_area.width) / 2;
    pad_offset2.height = (bg_area.height - fg_area.height) / 2;

    // difference between bg_area and fg_area has to be even
    if (
        (
            (pad_offset2.width == round(pad_offset2.width)) &&
            (pad_offset2.height != round(pad_offset2.height))
        ) ||
        (
            (pad_offset2.width != round(pad_offset2.width)) &&
            (pad_offset2.height == round(pad_offset2.height))
        )) {
        assert(0);
    }

    pad_offset2.width = fmax(pad_offset2.width, 1);
    pad_offset2.height = fmax(pad_offset2.height, 1);

    //std::cout << "pad_offset2 " << pad_offset2 << std::endl;

    cv::Mat fg_mask(bg_area, CV_8UC1, cv::Scalar(0)); // init fg_mask

    // xxx: fg_mask(pad_offset2(1)+1:end-pad_offset2(1), pad_offset2(2)+1:end-pad_offset2(2)) = true;

    cv::Rect pad2_rect(
        pad_offset2.width,
        pad_offset2.height,
        bg_area.width - 2 * pad_offset2.width,
        bg_area.height - 2 * pad_offset2.height
        );

    fg_mask(pad2_rect) = true;
    ////////////////////////////////////////////////////////////////////////

    cv::Mat fg_mask_new;
    cv::Mat bg_mask_new;

    mexResize(fg_mask, fg_mask_new, norm_bg_area, "auto");
    mexResize(bg_mask, bg_mask_new, norm_bg_area, "auto");

    int imgCount = 1;
    int dims = 3;
    const int sizes[] = { cfg.n_bins, cfg.n_bins, cfg.n_bins };
    const int channels[] = { 0, 1, 2 };
    float bRange[] = { 0, 256 };
    float gRange[] = { 0, 256 };
    float rRange[] = { 0, 256 };
    const float *ranges[] = { bRange, gRange, rRange };

    if (cfg.grayscale_sequence) {
        dims = 1;
    }

    // (TRAIN) BUILD THE MODEL
    if (new_model) {
        cv::calcHist(&patch, imgCount, channels, bg_mask_new, bg_hist, dims, sizes, ranges);
        cv::calcHist(&patch, imgCount, channels, fg_mask_new, fg_hist, dims, sizes, ranges);

        int bgtotal = cv::countNonZero(bg_mask_new);
        (bgtotal == 0) && (bgtotal = 1);
        bg_hist = bg_hist / bgtotal;

        int fgtotal = cv::countNonZero(fg_mask_new);
        (fgtotal == 0) && (fgtotal = 1);
        fg_hist = fg_hist / fgtotal;
    } else { // update the model
        cv::MatND bg_hist_tmp;
        cv::MatND fg_hist_tmp;

        cv::calcHist(&patch, imgCount, channels, bg_mask_new, bg_hist_tmp, dims, sizes, ranges);
        cv::calcHist(&patch, imgCount, channels, fg_mask_new, fg_hist_tmp, dims, sizes, ranges);

        int bgtotal = cv::countNonZero(bg_mask_new);
        (bgtotal == 0) && (bgtotal = 1);
        bg_hist_tmp = bg_hist_tmp / bgtotal;

        int fgtotal = cv::countNonZero(fg_mask_new);
        (fgtotal == 0) && (fgtotal = 1);
        fg_hist_tmp = fg_hist_tmp / fgtotal;

        // xxx
        bg_hist = (1 - learning_rate_pwp)*bg_hist + learning_rate_pwp*bg_hist_tmp;
        fg_hist = (1 - learning_rate_pwp)*fg_hist + learning_rate_pwp*fg_hist_tmp;