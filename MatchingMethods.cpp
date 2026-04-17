#include <opencv2/opencv.hpp>
// for fast line detection, you also need to run "vcpkg install opencv4[contrib]:x64-windows-static"
#include <opencv2/ximgproc.hpp>

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

std::vector<int> find_two_peaks(const cv::Mat& hist_cv, 
                                 int min_distance = 80, 
                                 float min_height_ratio = 0.3) {
    int hist_size = hist_cv.cols;
    // Convert integer histogram to float for smoothing (GaussianBlur requires float/uchar types)
    cv::Mat hist_f;
    hist_cv.convertTo(hist_f, CV_32F);
    float max_val = 0;
    for (int i = 0; i < hist_size; i++) {
        max_val = std::max(max_val, hist_f.at<float>(0, i));
    }
    int min_height = static_cast<int>(max_val * min_height_ratio);

    cv::Mat smoothed;
    cv::GaussianBlur(hist_f, smoothed, cv::Size(5, 1), 1.0);
    
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
    
    // ピークが2つ未満の場合、高さで補う（min_distance を尊重する）
    if (peaks.size() < 2) {
        std::vector<std::pair<float, int>> peak_candidates;
        for (int i = 0; i < hist_size; i++) {
            peak_candidates.push_back({smoothed.at<float>(0, i), i});
        }
        std::sort(peak_candidates.rbegin(), peak_candidates.rend());

        peaks.clear();
        if (!peak_candidates.empty()) {
            int first_idx = peak_candidates[0].second;
            peaks.push_back(first_idx);

            // find a second candidate at least min_distance away
            int chosen = -1;
            for (size_t k = 1; k < peak_candidates.size(); ++k) {
                int cand = peak_candidates[k].second;
                if (std::abs(cand - first_idx) >= min_distance) {
                    chosen = cand;
                    break;
                }
            }
            // if none meet distance, pick the farthest candidate
            if (chosen == -1 && peak_candidates.size() > 1) {
                int best_dist = -1;
                for (size_t k = 1; k < peak_candidates.size(); ++k) {
                    int cand = peak_candidates[k].second;
                    int d = std::abs(cand - first_idx);
                    if (d > best_dist) { best_dist = d; chosen = cand; }
                }
            }
            if (chosen != -1) peaks.push_back(chosen);
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

        // Preserve original ROI and use separate rotated outputs to avoid in-place mutation
        cv::Mat roi_orig = roi.clone();

        // Rotate ROI by 32 degrees which is parallel to top-left and bottom-right diagonal of the diamond
        cv::Point2f center(static_cast<float>(roi_orig.cols) / 2.0f, static_cast<float>(roi_orig.rows) / 2.0f);
        double angle1 = -51; // degrees
        cv::Mat rotation_matrix1 = cv::getRotationMatrix2D(center, angle1, 1.0);
        cv::Mat roi_rot1;
        cv::warpAffine(roi_orig, roi_rot1, rotation_matrix1, roi_orig.size(), cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));
        cv::imshow("Rotated ROI (32 deg)", roi_rot1); // Debug visualization

        // Calculate column histogram of the rotated ROI
        cv::Mat col_hist;
        cv::reduce(roi_rot1, col_hist, 0, cv::REDUCE_SUM, CV_32S);

        // Find two peaks in the histogram which correspond to button columns
        std::vector<int> peaks = find_two_peaks(col_hist, 80, 0.5);

        // calculate rotated centroid (center of gravity) of each peak column
        if (peaks.size() < 2) {
            result.debug_info += "find_two_peaks returned fewer than 2 peaks\n";
            result.status = 1;
            return result;
        }

        cv::Point2f rot32_cog_a = {0, 0}, rot32_cog_b = {0, 0};
        int cnt32a = 0, cnt32b = 0;
        for (int y = 0; y < roi_rot1.rows; y++) {
            if (peaks[0] >= 0 && peaks[0] < roi_rot1.cols && roi_rot1.at<uchar>(y, peaks[0]) > 0) {
                rot32_cog_a += cv::Point2f(static_cast<float>(peaks[0]), static_cast<float>(y));
                ++cnt32a;
            }
            if (peaks[1] >= 0 && peaks[1] < roi_rot1.cols && roi_rot1.at<uchar>(y, peaks[1]) > 0) {
                rot32_cog_b += cv::Point2f(static_cast<float>(peaks[1]), static_cast<float>(y));
                ++cnt32b;
            }
        }

        if (cnt32a > 0) rot32_cog_a *= (1.0f / cnt32a);
        if (cnt32b > 0) rot32_cog_b *= (1.0f / cnt32b);

        // inverse-rotate these points back to ROI coordinate system
        cv::Mat inv_rot1;
        cv::invertAffineTransform(rotation_matrix1, inv_rot1);

        cv::Mat src_pts(1, 2, CV_32FC2);
        src_pts.at<cv::Vec2f>(0, 0) = cv::Vec2f(rot32_cog_a.x, rot32_cog_a.y);
        src_pts.at<cv::Vec2f>(0, 1) = cv::Vec2f(rot32_cog_b.x, rot32_cog_b.y);
        cv::Mat dst_pts;
        cv::transform(src_pts, dst_pts, inv_rot1);

        cv::Point2f cog1_roi = dst_pts.at<cv::Vec2f>(0, 0);
        cv::Point2f cog2_roi = dst_pts.at<cv::Vec2f>(0, 1);

        // Save the two midpoints from the 32 degree rotation (left-bottom, right-top)
        result.edge_midpoints[0] = cv::Point2f(cog1_roi.x + roi_rect.x, cog1_roi.y + roi_rect.y);
        result.edge_midpoints[1] = cv::Point2f(cog2_roi.x + roi_rect.x, cog2_roi.y + roi_rect.y);

        if (is_test_mode) {
            result.debug_info += "32deg peaks: col1=" + std::to_string(peaks[0]) + ", col2=" + std::to_string(peaks[1]) + "\n";
            result.debug_info += "rot32 cogs: (" + std::to_string(rot32_cog_a.x) + ", " + std::to_string(rot32_cog_a.y) + ") and (" + std::to_string(rot32_cog_b.x) + ", " + std::to_string(rot32_cog_b.y) + ")\n";
            result.debug_info += "mapped ROI points (32deg): (" + std::to_string(cog1_roi.x) + ", " + std::to_string(cog1_roi.y) + ") and (" + std::to_string(cog2_roi.x) + ", " + std::to_string(cog2_roi.y) + ")\n";
        }

        // --- Now repeat for the complementary diagonal (rotate by 122 degrees) ---
        double angle2 = 48.0;
        cv::Mat rotation_matrix2 = cv::getRotationMatrix2D(center, angle2, 1.0);
        cv::Mat roi_rot2;
        cv::warpAffine(roi_orig, roi_rot2, rotation_matrix2, roi_orig.size(), cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));
        cv::imshow("Rotated ROI (122 deg)", roi_rot2); // Debug visualization

        cv::Mat col_hist2;
        cv::reduce(roi_rot2, col_hist2, 0, cv::REDUCE_SUM, CV_32S);
        std::vector<int> peaks2 = find_two_peaks(col_hist2, 80, 0.3);

        if (peaks2.size() < 2) {
            result.debug_info += "find_two_peaks (122deg) returned fewer than 2 peaks\n";
            result.status = 1;
            return result;
        }

        cv::Point2f rot122_cog_c = {0, 0}, rot122_cog_d = {0, 0};
        int cnt122c = 0, cnt122d = 0;
        for (int y = 0; y < roi_rot2.rows; y++) {
            if (peaks2[0] >= 0 && peaks2[0] < roi_rot2.cols && roi_rot2.at<uchar>(y, peaks2[0]) > 0) {
                rot122_cog_c += cv::Point2f(static_cast<float>(peaks2[0]), static_cast<float>(y));
                ++cnt122c;
            }
            if (peaks2[1] >= 0 && peaks2[1] < roi_rot2.cols && roi_rot2.at<uchar>(y, peaks2[1]) > 0) {
                rot122_cog_d += cv::Point2f(static_cast<float>(peaks2[1]), static_cast<float>(y));
                ++cnt122d;
            }
        }
        if (cnt122c > 0) rot122_cog_c *= (1.0f / cnt122c);
        if (cnt122d > 0) rot122_cog_d *= (1.0f / cnt122d);

        cv::Mat inv_rot2;
        cv::invertAffineTransform(rotation_matrix2, inv_rot2);
        cv::Mat src2(1, 2, CV_32FC2);
        src2.at<cv::Vec2f>(0, 0) = cv::Vec2f(rot122_cog_c.x, rot122_cog_c.y);
        src2.at<cv::Vec2f>(0, 1) = cv::Vec2f(rot122_cog_d.x, rot122_cog_d.y);
        cv::Mat dst2;
        cv::transform(src2, dst2, inv_rot2);

        cv::Point2f cog3_roi = dst2.at<cv::Vec2f>(0, 0);
        cv::Point2f cog4_roi = dst2.at<cv::Vec2f>(0, 1);

        result.edge_midpoints[2] = cv::Point2f(cog3_roi.x + roi_rect.x, cog3_roi.y + roi_rect.y);
        result.edge_midpoints[3] = cv::Point2f(cog4_roi.x + roi_rect.x, cog4_roi.y + roi_rect.y);

        if (is_test_mode) {
            result.debug_info += "122deg peaks: col1=" + std::to_string(peaks2[0]) + ", col2=" + std::to_string(peaks2[1]) + "\n";
            result.debug_info += "rot122 cogs: (" + std::to_string(rot122_cog_c.x) + ", " + std::to_string(rot122_cog_c.y) + ") and (" + std::to_string(rot122_cog_d.x) + ", " + std::to_string(rot122_cog_d.y) + ")\n";
            result.debug_info += "mapped ROI points (122deg): (" + std::to_string(cog3_roi.x) + ", " + std::to_string(cog3_roi.y) + ") and (" + std::to_string(cog4_roi.x) + ", " + std::to_string(cog4_roi.y) + ")\n";
        }

        // For backwards compatibility, set button1/button2 to the first two midpoints
        result.button1 = result.edge_midpoints[0];
        result.button2 = result.edge_midpoints[1];
        result.button3 = result.edge_midpoints[2];

        if (is_test_mode) {
            // Annotate a visualization image showing the detected midpoints
            cv::Mat vis = cv::Mat::zeros(roi.size(), CV_8UC1);
            // draw vertical peak columns on rotated ROI for visualization
            for (int x = 0; x < roi.cols; x++) {
                int sum = 0;
                for (int y = 0; y < roi.rows; y++) {
                    if (roi.at<uchar>(y, x) > 0) vis.at<uchar>(y, x) = 80;
                }
            }
            // draw midpoints (in ROI coords) as bright circles
            cv::circle(vis, cog1_roi, 3, cv::Scalar(255), -1);
            cv::circle(vis, cog2_roi, 3, cv::Scalar(200), -1);
            cv::circle(vis, cog3_roi, 3, cv::Scalar(150), -1);
            cv::circle(vis, cog4_roi, 3, cv::Scalar(120), -1);
            // convert to full image coordinates for display
            result.processed_image = vis.clone();
        }


        /*

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
    */

    } catch (const std::exception& ex) {
        result.status = -1;
        result.debug_info = std::string("ProcessViaLineDetection error: ") + ex.what();
    } catch (...) {
        result.status = -2;
        result.debug_info = "ProcessViaLineDetection: unknown error";
    }
    return result;
}

//90ms -> 60ms, but this requires OpenCV contrib
// if you use, vcpkg run "vcpkg install opencv4[contrib]:x64-windows-static" and include the header <opencv2/ximgproc.hpp>
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
