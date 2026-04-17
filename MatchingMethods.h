#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <array>

// Result structure returned by image processing methods
struct ProcessResult
{
    int status;                  // 0 = success, <0 = error, >0 = execution warning
    cv::Mat processed_image;     // Processed/annotated image for visualization
    cv::Point2f button1;         // Button 1 (mode-dependent) absolute coordinates
    cv::Point2f button2;         // Button 2 absolute coordinates
    cv::Point2f button3;         // Button 3 absolute coordinates
    std::array<cv::Point2f, 4> edge_midpoints; // Detected midpoints of the diamond edges (order: lb, rt, lt, rb)
    std::string debug_info;      // Debug message for logging
};

// Line detection based button detection algorithm
// Processes captured image: binary threshold -> ROI extraction -> Hough line detection -> button calculation
// Input: cv::Mat image in CV_8UC4 format, image width and height, is_test_mode for visualization
// Output: ProcessResult with processed image (if test mode) and button coordinates
ProcessResult ProcessViaLineDetection(const cv::Mat& image, int width, int height, bool is_test_mode = false);
ProcessResult ProcessViaFastLineDetector(const cv::Mat& image, int width, int height, bool is_test_mode = false);
ProcessResult ProcessViaRotatedHistogram(const cv::Mat& image, int width, int height, bool is_test_mode = false);
