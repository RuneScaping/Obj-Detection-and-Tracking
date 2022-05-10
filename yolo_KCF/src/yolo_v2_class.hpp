
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
extern "C" YOLODLL_API int dispose();
extern "C" YOLODLL_API int get_device_count();
extern "C" YOLODLL_API int get_device_name(int gpu, char* deviceName);

class Detector {
    std::shared_ptr<void> detector_gpu_ptr;
    std::deque<std::vector<bbox_t>> prev_bbox_vec_deque;
    const int cur_gpu_id;
public:
    float nms = .4;
    bool wait_stream;

    YOLODLL_API Detector(std::string cfg_filename, std::string weight_filename, int gpu_id = 0);
    YOLODLL_API ~Detector();

    YOLODLL_API std::vector<bbox_t> detect(std::string image_filename, float thresh = 0.2, bool use_mean = false);
    YOLODLL_API std::vector<bbox_t> detect(image_t img, float thresh = 0.2, bool use_mean = false);
    static YOLODLL_API image_t load_image(std::string image_filename);
    static YOLODLL_API void free_image(image_t m);
    YOLODLL_API int get_net_width() const;
    YOLODLL_API int get_net_height() const;
    YOLODLL_API int get_net_color_depth() const;

    YOLODLL_API std::vector<bbox_t> tracking_id(std::vector<bbox_t> cur_bbox_vec, bool const change_history = true,
                                                int const frames_story = 10, int const max_dist = 150);

    std::vector<bbox_t> detect_resized(image_t img, int init_w, int init_h, float thresh = 0.2, bool use_mean = false)
    {
        if (img.data == NULL)
            throw std::runtime_error("Image is empty");
        auto detection_boxes = detect(img, thresh, use_mean);
        float wk = (float)init_w / img.w, hk = (float)init_h / img.h;
        for (auto &i : detection_boxes) i.x *= wk, i.w *= wk, i.y *= hk, i.h *= hk;
        return detection_boxes;
    }

#ifdef OPENCV
    std::vector<bbox_t> detect(cv::Mat mat, float thresh = 0.2, bool use_mean = false)
    {
        if(mat.data == NULL)
            throw std::runtime_error("Image is empty");
        auto image_ptr = mat_to_image_resize(mat);
        return detect_resized(*image_ptr, mat.cols, mat.rows, thresh, use_mean);
    }

    std::shared_ptr<image_t> mat_to_image_resize(cv::Mat mat) const
    {
        if (mat.data == NULL) return std::shared_ptr<image_t>(NULL);

        cv::Size network_size = cv::Size(get_net_width(), get_net_height());
        cv::Mat det_mat;
        if (mat.size() != network_size)
            cv::resize(mat, det_mat, network_size);
        else
            det_mat = mat;  // only reference is copied

        return mat_to_image(det_mat);
    }

    static std::shared_ptr<image_t> mat_to_image(cv::Mat img_src)
    {
        cv::Mat img;
        cv::cvtColor(img_src, img, cv::COLOR_RGB2BGR);
        std::shared_ptr<image_t> image_ptr(new image_t, [](image_t *img) { free_image(*img); delete img; });
        std::shared_ptr<IplImage> ipl_small = std::make_shared<IplImage>(img);
        *image_ptr = ipl_to_image(ipl_small.get());
        return image_ptr;
    }

private:

    static image_t ipl_to_image(IplImage* src)
    {
        unsigned char *data = (unsigned char *)src->imageData;
        int h = src->height;
        int w = src->width;
        int c = src->nChannels;
        int step = src->widthStep;
        image_t out = make_image_custom(w, h, c);
        int count = 0;

        for (int k = 0; k < c; ++k) {
            for (int i = 0; i < h; ++i) {
                int i_step = i*step;
                for (int j = 0; j < w; ++j) {
                    out.data[count++] = data[i_step + j*c + k] / 255.;
                }
            }
        }

        return out;
    }

    static image_t make_empty_image(int w, int h, int c)
    {
        image_t out;
        out.data = 0;
        out.h = h;
        out.w = w;
        out.c = c;
        return out;
    }

    static image_t make_image_custom(int w, int h, int c)
    {
        image_t out = make_empty_image(w, h, c);
        out.data = (float *)calloc(h*w*c, sizeof(float));
        return out;
    }

#endif    // OPENCV

};



#if defined(TRACK_OPTFLOW) && defined(OPENCV) && defined(GPU)

#include <opencv2/cudaoptflow.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/core/cuda.hpp>

class Tracker_optflow {
public:
    const int gpu_count;
    const int gpu_id;
    const int flow_error;


    Tracker_optflow(int _gpu_id = 0, int win_size = 9, int max_level = 3, int iterations = 8000, int _flow_error = -1) :
        gpu_count(cv::cuda::getCudaEnabledDeviceCount()), gpu_id(std::min(_gpu_id, gpu_count-1)),
        flow_error((_flow_error > 0)? _flow_error:(win_size*4))
    {
        int const old_gpu_id = cv::cuda::getDevice();
        cv::cuda::setDevice(gpu_id);

        stream = cv::cuda::Stream();

        sync_PyrLKOpticalFlow_gpu = cv::cuda::SparsePyrLKOpticalFlow::create();
        sync_PyrLKOpticalFlow_gpu->setWinSize(cv::Size(win_size, win_size));    // 9, 15, 21, 31
        sync_PyrLKOpticalFlow_gpu->setMaxLevel(max_level);        // +- 3 pt
        sync_PyrLKOpticalFlow_gpu->setNumIters(iterations);    // 2000, def: 30

        cv::cuda::setDevice(old_gpu_id);
    }

    // just to avoid extra allocations
    cv::cuda::GpuMat src_mat_gpu;
    cv::cuda::GpuMat dst_mat_gpu, dst_grey_gpu;
    cv::cuda::GpuMat prev_pts_flow_gpu, cur_pts_flow_gpu;
    cv::cuda::GpuMat status_gpu, err_gpu;

    cv::cuda::GpuMat src_grey_gpu;    // used in both functions
    cv::Ptr<cv::cuda::SparsePyrLKOpticalFlow> sync_PyrLKOpticalFlow_gpu;
    cv::cuda::Stream stream;

    std::vector<bbox_t> cur_bbox_vec;
    std::vector<bool> good_bbox_vec_flags;
    cv::Mat prev_pts_flow_cpu;

    void update_cur_bbox_vec(std::vector<bbox_t> _cur_bbox_vec)
    {
        cur_bbox_vec = _cur_bbox_vec;
        good_bbox_vec_flags = std::vector<bool>(cur_bbox_vec.size(), true);
        cv::Mat prev_pts, cur_pts_flow_cpu;

        for (auto &i : cur_bbox_vec) {
            float x_center = (i.x + i.w / 2.0F);
            float y_center = (i.y + i.h / 2.0F);
            prev_pts.push_back(cv::Point2f(x_center, y_center));
        }

        if (prev_pts.rows == 0)
            prev_pts_flow_cpu = cv::Mat();
        else
            cv::transpose(prev_pts, prev_pts_flow_cpu);

        if (prev_pts_flow_gpu.cols < prev_pts_flow_cpu.cols) {
            prev_pts_flow_gpu = cv::cuda::GpuMat(prev_pts_flow_cpu.size(), prev_pts_flow_cpu.type());
            cur_pts_flow_gpu = cv::cuda::GpuMat(prev_pts_flow_cpu.size(), prev_pts_flow_cpu.type());

            status_gpu = cv::cuda::GpuMat(prev_pts_flow_cpu.size(), CV_8UC1);
            err_gpu = cv::cuda::GpuMat(prev_pts_flow_cpu.size(), CV_32FC1);
        }

        prev_pts_flow_gpu.upload(cv::Mat(prev_pts_flow_cpu), stream);
    }


    void update_tracking_flow(cv::Mat src_mat, std::vector<bbox_t> _cur_bbox_vec)
    {
        int const old_gpu_id = cv::cuda::getDevice();
        if (old_gpu_id != gpu_id)
            cv::cuda::setDevice(gpu_id);

        if (src_mat.channels() == 3) {
            if (src_mat_gpu.cols == 0) {
                src_mat_gpu = cv::cuda::GpuMat(src_mat.size(), src_mat.type());
                src_grey_gpu = cv::cuda::GpuMat(src_mat.size(), CV_8UC1);
            }

            update_cur_bbox_vec(_cur_bbox_vec);

            //src_grey_gpu.upload(src_mat, stream);    // use BGR
            src_mat_gpu.upload(src_mat, stream);
            cv::cuda::cvtColor(src_mat_gpu, src_grey_gpu, CV_BGR2GRAY, 1, stream);
        }
        if (old_gpu_id != gpu_id)
            cv::cuda::setDevice(old_gpu_id);
    }


    std::vector<bbox_t> tracking_flow(cv::Mat dst_mat, bool check_error = true)
    {
        if (sync_PyrLKOpticalFlow_gpu.empty()) {
            std::cout << "sync_PyrLKOpticalFlow_gpu isn't initialized \n";
            return cur_bbox_vec;
        }

        int const old_gpu_id = cv::cuda::getDevice();
        if(old_gpu_id != gpu_id)
            cv::cuda::setDevice(gpu_id);

        if (dst_mat_gpu.cols == 0) {
            dst_mat_gpu = cv::cuda::GpuMat(dst_mat.size(), dst_mat.type());
            dst_grey_gpu = cv::cuda::GpuMat(dst_mat.size(), CV_8UC1);
        }

        //dst_grey_gpu.upload(dst_mat, stream);    // use BGR
        dst_mat_gpu.upload(dst_mat, stream);
        cv::cuda::cvtColor(dst_mat_gpu, dst_grey_gpu, CV_BGR2GRAY, 1, stream);

        if (src_grey_gpu.rows != dst_grey_gpu.rows || src_grey_gpu.cols != dst_grey_gpu.cols) {
            stream.waitForCompletion();
            src_grey_gpu = dst_grey_gpu.clone();
            cv::cuda::setDevice(old_gpu_id);
            return cur_bbox_vec;
        }

        ////sync_PyrLKOpticalFlow_gpu.sparse(src_grey_gpu, dst_grey_gpu, prev_pts_flow_gpu, cur_pts_flow_gpu, status_gpu, &err_gpu);    // OpenCV 2.4.x
        sync_PyrLKOpticalFlow_gpu->calc(src_grey_gpu, dst_grey_gpu, prev_pts_flow_gpu, cur_pts_flow_gpu, status_gpu, err_gpu, stream);    // OpenCV 3.x

        cv::Mat cur_pts_flow_cpu;
        cur_pts_flow_gpu.download(cur_pts_flow_cpu, stream);

        dst_grey_gpu.copyTo(src_grey_gpu, stream);

        cv::Mat err_cpu, status_cpu;
        err_gpu.download(err_cpu, stream);
        status_gpu.download(status_cpu, stream);

        stream.waitForCompletion();