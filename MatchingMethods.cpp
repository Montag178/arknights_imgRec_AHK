#include <opencv2/opencv.hpp>
// for fast line detection, you also need to run "vcpkg install opencv4[contrib]:x64-windows-static"
// #include <opencv2/ximgproc.hpp>

#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>

#include "MatchingMethods.h"

ProcessResult ProcessViaLineDetection(const cv::Mat& image, int width, int height, bool is_test_mode)
{
    ProcessResult result = {};
    result.status = 0;
    result.button1 = {0, 0}; // retreat button
    result.button2 = {0, 0}; // skill button
    result.button3 = {0, 0}; // cancel button (little above the diamond)
    
    try {
        // Binary thresholding: keep white pixels (for button detection)
        cv::Mat binary_img;
        cv::Scalar lower(240, 240, 240, 0);
        cv::Scalar upper(255, 255, 255, 255);
        cv::inRange(image, lower, upper, binary_img);

        // Define ROI (Region of Interest) for button area
        cv::Rect roi_rect(0.3 * width, 0.13 * height, 0.55 * width, 0.685 * height);
        cv::Mat roi = binary_img(roi_rect);

        // Detect lines using Hough probabilistic line transform
        std::vector<cv::Vec4i> lines;
        cv::HoughLinesP(roi, lines, 1, CV_PI / 180, 100, 0.104167 * width, 0.01 * width);

        // Track line endpoints to find button corners
        int min_x = roi.cols, min_y = roi.rows, max_x = 0, max_y = 0;
        cv::Point ptLeft = {0, 0}, ptTop = {0, 0}, ptRight = {0, 0}, ptBottom = {0, 0};
        if (is_test_mode) {
            result.debug_info += "Window size: width=" + std::to_string(width) + ", height=" + std::to_string(height) + "\n";
            result.debug_info += "ROI: x=" + std::to_string(roi_rect.x) + ", y=" + std::to_string(roi_rect.y) + 
                                ", width=" + std::to_string(roi_rect.width) + ", height=" + std::to_string(roi_rect.height) + "\n";
            result.debug_info += "Detected lines: " + std::to_string(lines.size()) + "\n";
        }

        // Filter lines by angle (diagonal lines ~30-45 degrees for button corners)
        for (size_t i = 0; i < lines.size(); i++) {
            cv::Vec4i l = lines[i];
            double angle = (std::abs)(std::atan2(l[3] - l[1], l[2] - l[0]) * 180.0 / CV_PI);
            
            if ((angle > 25 && angle < 45)) {
                cv::Point p1(l[0], l[1]), p2(l[2], l[3]);
                std::vector<cv::Point> points = {p1, p2};
                
                for (const auto& pt : points) {
                    if (pt.x < min_x) { min_x = pt.x; ptLeft = pt; }
                    if (pt.x > max_x) { max_x = pt.x; ptRight = pt; }
                    if (pt.y < min_y) { min_y = pt.y; ptTop = pt; }
                    if (pt.y > max_y) { max_y = pt.y; ptBottom = pt; }
                }

                result.debug_info += "Line " + std::to_string(i) + ": (" + std::to_string(l[0]) + ", " + 
                                    std::to_string(l[1]) + ") to (" + std::to_string(l[2]) + ", " + 
                                    std::to_string(l[3]) + "), angle: " + std::to_string(angle) + "\n";
            }
        }

        cv::Mat black_img; // Declare outside if block for scope
        if (is_test_mode) {
            // Create annotated image showing detected lines
            black_img = cv::Mat::zeros(roi.size(), CV_8UC1);
            for (size_t i = 0; i < lines.size(); i++) {
                cv::Vec4i l = lines[i];
                double angle = (std::abs)(std::atan2(l[3] - l[1], l[2] - l[0]) * 180.0 / CV_PI);
                if ((angle > 25 && angle < 45)) {
                    cv::line(black_img, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255), 2);
                }
            }
            result.processed_image = black_img.clone();
        }

        // Validate that we found two distinct corner lines
        if (ptTop.x - ptLeft.x < 0.1 * width || ptRight.x - ptBottom.x < 0.1 * width) {
            result.debug_info += "could not find two distinct lines\n";
            result.status = 1;  // Warning: line detection failed
            return result;
        }

        if (ptLeft.x == 0 || ptTop.y == 0 || ptRight.x == roi.cols || ptBottom.y == roi.rows) {
            result.debug_info += "maybe ROI is too small, but continue processing\n";
        }

        // Calculate button centers from corner points
        cv::Point2f button1_roi = (cv::Point2f(ptLeft) + cv::Point2f(ptTop)) * 0.5f;
        cv::Point2f button2_roi = (cv::Point2f(ptBottom) + cv::Point2f(ptRight)) * 0.5f;
        cv::Point2f button3_roi = cv::Point2f(ptTop);

        // Convert from ROI coordinates to absolute screen coordinates
        result.button1 = cv::Point2f(button1_roi.x + roi_rect.x, button1_roi.y + roi_rect.y);
        result.button2 = cv::Point2f(button2_roi.x + roi_rect.x, button2_roi.y + roi_rect.y);
        result.button3 = cv::Point2f(button3_roi.x + roi_rect.x, std::max(0.01f, button3_roi.y + roi_rect.y - 0.05f * height));

        if (is_test_mode) {
            // Draw circles on annotated image to show detected button positions
            cv::circle(black_img, button1_roi, 10, cv::Scalar(255), 3);
            cv::circle(black_img, button2_roi, 10, cv::Scalar(140), 3);
            result.processed_image = black_img.clone();
            result.debug_info += "ptLeft: (" + std::to_string(ptLeft.x) + ", " + std::to_string(ptLeft.y) + ")\n";
            result.debug_info += "ptTop: (" + std::to_string(ptTop.x) + ", " + std::to_string(ptTop.y) + ")\n";
            result.debug_info += "ptRight: (" + std::to_string(ptRight.x) + ", " + std::to_string(ptRight.y) + ")\n";
            result.debug_info += "ptBottom: (" + std::to_string(ptBottom.x) + ", " + std::to_string(ptBottom.y) + ")\n";
            result.debug_info += "Retreat button coords: (" + std::to_string(button1_roi.x) + ", " + 
                                    std::to_string(result.button1.y) + ")\n";
            result.debug_info += "Skill button coords: (" + std::to_string(button2_roi.x) + ", " + 
                                    std::to_string(result.button2.y) + ")\n";
            result.debug_info += "Cancel button coords: (" + std::to_string(button3_roi.x) + ", " + 
                                    std::to_string(result.button3.y) + ")\n";
        }

    } catch (const std::exception& ex) {
        result.status = -1;
        result.debug_info = std::string("ProcessViaLineDetection error: ") + ex.what();
    } catch (...) {
        result.status = -2;
        result.debug_info = "ProcessViaLineDetection: unknown error";
    }

    return result;
}

std::vector<int> find_two_peaks(const std::vector<int>& col_hist, 
                                 int min_distance = 4, 
                                 float min_height_ratio = 0.3) {
    int hist_size = col_hist.size();
    float max_val = *std::max_element(col_hist.begin(), col_hist.end());
    int min_height = static_cast<int>(max_val * min_height_ratio);
    
    // スムージング（OpenCV の gaussianBlur を利用）
    cv::Mat hist_cv(1, hist_size, CV_32F);
    for (int i = 0; i < hist_size; i++) {
        hist_cv.at<float>(0, i) = col_hist[i];
    }
    
    cv::Mat smoothed;
    cv::GaussianBlur(hist_cv, smoothed, cv::Size(5, 1), 1.0);
    
    // ピーク検出（手動実装）
    std::vector<int> peaks;
    for (int i = 1; i < hist_size - 1; i++) {
        float val = smoothed.at<float>(0, i);
        float left = smoothed.at<float>(0, i - 1);
        float right = smoothed.at<float>(0, i + 1);
        
        // ローカルマキシマムをチェック
        if (val > left && val > right && val > min_height) {
            // 既に検出されたピークとの距離をチェック
            bool far_enough = true;
            for (int p : peaks) {
                if (std::abs(i - p) < min_distance) {
                    far_enough = false;
                    break;
                }
            }
            if (far_enough) {
                peaks.push_back(i);
            }
        }
    }
    
    // ピークが2つ未満の場合、高さで補う
    if (peaks.size() < 2) {
        std::vector<std::pair<float, int>> peak_candidates;
        for (int i = 0; i < hist_size; i++) {
            peak_candidates.push_back({smoothed.at<float>(0, i), i});
        }
        std::sort(peak_candidates.rbegin(), peak_candidates.rend());
        
        peaks.clear();
        peaks.push_back(peak_candidates[0].second);
        if (peak_candidates.size() > 1) {
            peaks.push_back(peak_candidates[1].second);
        }
    } else if (peaks.size() > 2) {
        // ピークが2つを超える場合、最大の2つを選ぶ
        std::vector<std::pair<float, int>> peak_values;
        for (int p : peaks) {
            peak_values.push_back({smoothed.at<float>(0, p), p});
        }
        std::sort(peak_values.rbegin(), peak_values.rend());
        
        peaks.clear();
        peaks.push_back(peak_values[0].second);
        
        // 最初に選んだピークから距��が遠いピークを探す
        for (size_t i = 1; i < peak_values.size(); i++) {
            if (std::abs(peak_values[i].second - peaks[0]) > min_distance) {
                peaks.push_back(peak_values[i].second);
                break;
            }
        }
    }
    
    // ソート
    std::sort(peaks.begin(), peaks.end());
    return peaks;
}

ProcessResult ProcessViaRotatedHistogram(const cv::Mat& image, int width, int height, bool is_test_mode)
{
    ProcessResult result = {};
    result.status = 0;
    result.button1 = {0, 0}; // retreat button
    result.button2 = {0, 0}; // skill button
    result.button3 = {0, 0}; // cancel button (little above the diamond)
    
    try {
        // Binary thresholding: keep white pixels (for button detection)
        cv::Mat binary_img;
        cv::Scalar lower(240, 240, 240, 0);
        cv::Scalar upper(255, 255, 255, 255);
        cv::inRange(image, lower, upper, binary_img);

        // Define ROI (Region of Interest) for button area
        cv::Rect roi_rect(0.3 * width, 0.13 * height, 0.55 * width, 0.685 * height);
        cv::Mat roi = binary_img(roi_rect);

        // Detect lines using Hough probabilistic line transform
        std::vector<cv::Vec4i> lines;
        cv::HoughLinesP(roi, lines, 1, CV_PI / 180, 100, 0.104167 * width, 0.01 * width);

        // Track line endpoints to find button corners
        int min_x = roi.cols, min_y = roi.rows, max_x = 0, max_y = 0;
        cv::Point ptLeft = {0, 0}, ptTop = {0, 0}, ptRight = {0, 0}, ptBottom = {0, 0};
        if (is_test_mode) {
            result.debug_info += "Window size: width=" + std::to_string(width) + ", height=" + std::to_string(height) + "\n";
            result.debug_info += "ROI: x=" + std::to_string(roi_rect.x) + ", y=" + std::to_string(roi_rect.y) + 
                                ", width=" + std::to_string(roi_rect.width) + ", height=" + std::to_string(roi_rect.height) + "\n";
            result.debug_info += "Detected lines: " + std::to_string(lines.size()) + "\n";
        }

        // Filter lines by angle (diagonal lines ~30-45 degrees for button corners)
        for (size_t i = 0; i < lines.size(); i++) {
            cv::Vec4i l = lines[i];
            double angle = (std::abs)(std::atan2(l[3] - l[1], l[2] - l[0]) * 180.0 / CV_PI);
            
            if ((angle > 25 && angle < 45)) {
                cv::Point p1(l[0], l[1]), p2(l[2], l[3]);
                std::vector<cv::Point> points = {p1, p2};
                
                for (const auto& pt : points) {
                    if (pt.x < min_x) { min_x = pt.x; ptLeft = pt; }
                    if (pt.x > max_x) { max_x = pt.x; ptRight = pt; }
                    if (pt.y < min_y) { min_y = pt.y; ptTop = pt; }
                    if (pt.y > max_y) { max_y = pt.y; ptBottom = pt; }
                }

                result.debug_info += "Line " + std::to_string(i) + ": (" + std::to_string(l[0]) + ", " + 
                                    std::to_string(l[1]) + ") to (" + std::to_string(l[2]) + ", " + 
                                    std::to_string(l[3]) + "), angle: " + std::to_string(angle) + "\n";
            }
        }

        cv::Mat black_img; // Declare outside if block for scope
        if (is_test_mode) {
            // Create annotated image showing detected lines
            black_img = cv::Mat::zeros(roi.size(), CV_8UC1);
            for (size_t i = 0; i < lines.size(); i++) {
                cv::Vec4i l = lines[i];
                double angle = (std::abs)(std::atan2(l[3] - l[1], l[2] - l[0]) * 180.0 / CV_PI);
                if ((angle > 25 && angle < 45)) {
                    cv::line(black_img, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255), 2);
                }
            }
            result.processed_image = black_img.clone();
        }

        // Validate that we found two distinct corner lines
        if (ptTop.x - ptLeft.x < 0.1 * width || ptRight.x - ptBottom.x < 0.1 * width) {
            result.debug_info += "could not find two distinct lines\n";
            result.status = 1;  // Warning: line detection failed
            return result;
        }

        if (ptLeft.x == 0 || ptTop.y == 0 || ptRight.x == roi.cols || ptBottom.y == roi.rows) {
            result.debug_info += "maybe ROI is too small, but continue processing\n";
        }

        // Calculate button centers from corner points
        cv::Point2f button1_roi = (cv::Point2f(ptLeft) + cv::Point2f(ptTop)) * 0.5f;
        cv::Point2f button2_roi = (cv::Point2f(ptBottom) + cv::Point2f(ptRight)) * 0.5f;
        cv::Point2f button3_roi = cv::Point2f(ptTop);

        // Convert from ROI coordinates to absolute screen coordinates
        result.button1 = cv::Point2f(button1_roi.x + roi_rect.x, button1_roi.y + roi_rect.y);
        result.button2 = cv::Point2f(button2_roi.x + roi_rect.x, button2_roi.y + roi_rect.y);
        result.button3 = cv::Point2f(button3_roi.x + roi_rect.x, std::max(0.01f, button3_roi.y + roi_rect.y - 0.05f * height));

        if (is_test_mode) {
            // Draw circles on annotated image to show detected button positions
            cv::circle(black_img, button1_roi, 10, cv::Scalar(255), 3);
            cv::circle(black_img, button2_roi, 10, cv::Scalar(140), 3);
            result.processed_image = black_img.clone();
            result.debug_info += "ptLeft: (" + std::to_string(ptLeft.x) + ", " + std::to_string(ptLeft.y) + ")\n";
            result.debug_info += "ptTop: (" + std::to_string(ptTop.x) + ", " + std::to_string(ptTop.y) + ")\n";
            result.debug_info += "ptRight: (" + std::to_string(ptRight.x) + ", " + std::to_string(ptRight.y) + ")\n";
            result.debug_info += "ptBottom: (" + std::to_string(ptBottom.x) + ", " + std::to_string(ptBottom.y) + ")\n";
            result.debug_info += "Retreat button coords: (" + std::to_string(button1_roi.x) + ", " + 
                                    std::to_string(result.button1.y) + ")\n";
            result.debug_info += "Skill button coords: (" + std::to_string(button2_roi.x) + ", " + 
                                    std::to_string(result.button2.y) + ")\n";
            result.debug_info += "Cancel button coords: (" + std::to_string(button3_roi.x) + ", " + 
                                    std::to_string(result.button3.y) + ")\n";
        }

    } catch (const std::exception& ex) {
        result.status = -1;
        result.debug_info = std::string("ProcessViaLineDetection error: ") + ex.what();
    } catch (...) {
        result.status = -2;
        result.debug_info = "ProcessViaLineDetection: unknown error";
    }

    return result;
}

/* 90ms -> 60ms, but this requires OpenCV contrib
// if you use vcpkg run "vcpkg install opencv4[contrib]:x64-windows-static" and include the header <opencv2/ximgproc.hpp>
ProcessResult ProcessViaFastLineDetector(const cv::Mat& image, int width, int height, bool is_test_mode)
{
    ProcessResult result = {};
    result.status = 0;
    result.button1 = {0, 0}; // retreat button
    result.button2 = {0, 0}; // skill button
    result.button3 = {0, 0}; // cancel button (little above the diamond)
    
    try {
        // Binary thresholding: keep white pixels (for button detection)
        cv::Mat binary_img;
        cv::Scalar lower(240, 240, 240, 0);
        cv::Scalar upper(255, 255, 255, 255);
        cv::inRange(image, lower, upper, binary_img);

        // Define ROI (Region of Interest) for button area
        cv::Rect roi_rect(0.3 * width, 0.13 * height, 0.55 * width, 0.685 * height);
        cv::Mat roi = binary_img(roi_rect);

        // Detect lines using Hough probabilistic line transform
        std::vector<cv::Vec4i> lines;
        int length_threshold = 0.104167 * width;
        float distance_threshold = 0.01 * width;
        double canny_th1 = 50.0;
        double canny_th2 = 50.0;
        int canny_aperture_size = 0;
        bool do_merge = false;
        cv::Ptr<cv::ximgproc::FastLineDetector> fld = cv::ximgproc::createFastLineDetector(length_threshold,
                distance_threshold, canny_th1, canny_th2, canny_aperture_size,
                do_merge);

        fld->detect(roi, lines);

        // Track line endpoints to find button corners
        int min_x = roi.cols, min_y = roi.rows, max_x = 0, max_y = 0;
        cv::Point ptLeft = {0, 0}, ptTop = {0, 0}, ptRight = {0, 0}, ptBottom = {0, 0};
        if (is_test_mode) {
            result.debug_info += "this is FLD mode\n";
            result.debug_info += "Window size: width=" + std::to_string(width) + ", height=" + std::to_string(height) + "\n";
            result.debug_info += "ROI: x=" + std::to_string(roi_rect.x) + ", y=" + std::to_string(roi_rect.y) + 
                                ", width=" + std::to_string(roi_rect.width) + ", height=" + std::to_string(roi_rect.height) + "\n";
            result.debug_info += "Detected lines: " + std::to_string(lines.size()) + "\n";
        }

        // Filter lines by angle (diagonal lines ~30-45 degrees for button corners)
        for (size_t i = 0; i < lines.size(); i++) {
            cv::Vec4i l = lines[i];
            double angle = (std::abs)(std::atan2(l[3] - l[1], l[2] - l[0]) * 180.0 / CV_PI);
            
            if ((angle > 25 && angle < 45)) {
                cv::Point p1(l[0], l[1]), p2(l[2], l[3]);
                std::vector<cv::Point> points = {p1, p2};
                
                for (const auto& pt : points) {
                    if (pt.x < min_x) { min_x = pt.x; ptLeft = pt; }
                    if (pt.x > max_x) { max_x = pt.x; ptRight = pt; }
                    if (pt.y < min_y) { min_y = pt.y; ptTop = pt; }
                    if (pt.y > max_y) { max_y = pt.y; ptBottom = pt; }
                }

                result.debug_info += "Line " + std::to_string(i) + ": (" + std::to_string(l[0]) + ", " + 
                                    std::to_string(l[1]) + ") to (" + std::to_string(l[2]) + ", " + 
                                    std::to_string(l[3]) + "), angle: " + std::to_string(angle) + "\n";
            }
        }

        cv::Mat black_img; // Declare outside if block for scope
        if (is_test_mode) {
            // Create annotated image showing detected lines
            black_img = cv::Mat::zeros(roi.size(), CV_8UC1);
            for (size_t i = 0; i < lines.size(); i++) {
                cv::Vec4i l = lines[i];
                double angle = (std::abs)(std::atan2(l[3] - l[1], l[2] - l[0]) * 180.0 / CV_PI);
                cv::line(black_img, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255), 2);

                // if ((angle > 25 && angle < 45)) {
                //     cv::line(black_img, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(255), 2);
                // }
            }
            result.processed_image = black_img.clone();
        }

        // Validate that we found two distinct corner lines
        if (ptTop.x - ptLeft.x < 0.1 * width || ptRight.x - ptBottom.x < 0.1 * width) {
            result.debug_info += "could not find two distinct lines\n";
            result.status = 1;  // Warning: line detection failed
            return result;
        }

        if (ptLeft.x == 0 || ptTop.y == 0 || ptRight.x == roi.cols || ptBottom.y == roi.rows) {
            result.debug_info += "maybe ROI is too small, but continue processing\n";
        }

        // Calculate button centers from corner points
        cv::Point2f button1_roi = (cv::Point2f(ptLeft) + cv::Point2f(ptTop)) * 0.5f;
        cv::Point2f button2_roi = (cv::Point2f(ptBottom) + cv::Point2f(ptRight)) * 0.5f;
        cv::Point2f button3_roi = cv::Point2f(ptTop);

        // Convert from ROI coordinates to absolute screen coordinates
        result.button1 = cv::Point2f(button1_roi.x + roi_rect.x, button1_roi.y + roi_rect.y);
        result.button2 = cv::Point2f(button2_roi.x + roi_rect.x, button2_roi.y + roi_rect.y);
        result.button3 = cv::Point2f(button3_roi.x + roi_rect.x, std::max(0.01f, button3_roi.y + roi_rect.y - 0.05f * height));

        if (is_test_mode) {
            // Draw circles on annotated image to show detected button positions
            cv::circle(black_img, button1_roi, 10, cv::Scalar(255), 3);
            cv::circle(black_img, button2_roi, 10, cv::Scalar(140), 3);
            result.processed_image = black_img.clone();
            result.debug_info += "ptLeft: (" + std::to_string(ptLeft.x) + ", " + std::to_string(ptLeft.y) + ")\n";
            result.debug_info += "ptTop: (" + std::to_string(ptTop.x) + ", " + std::to_string(ptTop.y) + ")\n";
            result.debug_info += "ptRight: (" + std::to_string(ptRight.x) + ", " + std::to_string(ptRight.y) + ")\n";
            result.debug_info += "ptBottom: (" + std::to_string(ptBottom.x) + ", " + std::to_string(ptBottom.y) + ")\n";
            result.debug_info += "Retreat button coords: (" + std::to_string(button1_roi.x) + ", " + 
                                    std::to_string(result.button1.y) + ")\n";
            result.debug_info += "Skill button coords: (" + std::to_string(button2_roi.x) + ", " + 
                                    std::to_string(result.button2.y) + ")\n";
            result.debug_info += "Cancel button coords: (" + std::to_string(button3_roi.x) + ", " + 
                                    std::to_string(result.button3.y) + ")\n";
        }

    } catch (const std::exception& ex) {
        result.status = -1;
        result.debug_info = std::string("ProcessViaLineDetection error: ") + ex.what();
    } catch (...) {
        result.status = -2;
        result.debug_info = "ProcessViaLineDetection: unknown error";
    }

    return result;
}
*/