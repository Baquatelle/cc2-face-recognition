#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

// FrameFilter - Modar Issa
// Adds a few display modes for the frame before it gets drawn on screen.
// The face detection still runs on the normal frame underneath, this only
// changes what you actually see in the window.
//
// 7.3a - greyscale
// 7.3b / 7.3c - edge detection
// 7.3d - face box crop

// ---- how to plug this into Abel's ofApp ----
//
// 1. add a member in ofApp.h:   FrameFilter frameFilter;
//
// 2. in keyPressed():           if (key == 'f' || key == 'F') frameFilter.nextMode();
//
// 3. in drawSource(), wrap the block that draws the image/video/webcam:
//
//    if (frameFilter.getMode() != FilterMode::NORMAL && !lastFrameBgr.empty()) {
//        // Abel's faces use ofRectangle .box, FrameFilter wants cv::Rect
//        std::vector<cv::Rect> faceRects;
//        for (int i = 0; i < faces.size(); i++) {
//            cv::Rect r;
//            r.x = (int)faces[i].box.x;
//            r.y = (int)faces[i].box.y;
//            r.width = (int)faces[i].box.width;
//            r.height = (int)faces[i].box.height;
//            faceRects.push_back(r);
//        }
//
//        cv::Mat filtered = frameFilter.apply(lastFrameBgr, faceRects);
//
//        // openFrameworks wants RGB but OpenCV gives BGR, so swap it first
//        cv::Mat rgb;
//        cv::cvtColor(filtered, rgb, cv::COLOR_BGR2RGB);
//        filteredImg.setFromPixels(rgb.data, rgb.cols, rgb.rows, OF_IMAGE_COLOR);
//        filteredImg.draw(offsetX, offsetY, srcW * scale, srcH * scale);
//    }
//    else {
//        // Abel's existing draw block, unchanged
//        if      (mode == InputMode::Image) image.draw(offsetX, offsetY, srcW * scale, srcH * scale);
//        else if (mode == InputMode::Video) video.draw(offsetX, offsetY, srcW * scale, srcH * scale);
//        else                               grabber.draw(offsetX, offsetY, srcW * scale, srcH * scale);
//    }
//
// 4. show which mode is on, at the end of draw():
//    ofDrawBitmapString("Filter [F]: " + frameFilter.getModeName(), 10, ofGetHeight() - 10);
//
// note: filteredImg is an ofImage member in ofApp.h, not a local variable.
// I made it a member so it doesn't rebuild the texture every single frame.

enum class FilterMode {
    NORMAL,      // no filter, frame goes through as it is
    GREYSCALE,   // 7.3a
    EDGES,       // 7.3b / 7.3c
    FACE_CROP    // 7.3d
};

class FrameFilter {
public:
    FrameFilter();

    // the main one - gives back the filtered frame
    // frame is BGR from the webcam or video, faceRects comes from the detector
    // the frame that comes back is always BGR and the same size as the one going in
    cv::Mat apply(const cv::Mat& frame, std::vector<cv::Rect> faceRects);

    // NORMAL -> GREYSCALE -> EDGES -> FACE_CROP -> back to NORMAL
    void nextMode();

    void setMode(FilterMode m);
    FilterMode getMode();
    std::string getModeName();

    // canny thresholds, change these if the edges come out too messy or too empty
    int cannyLow = 50;
    int cannyHigh = 150;

private:
    FilterMode mode;

    cv::Mat applyGreyscale(const cv::Mat& frame);
    cv::Mat applyEdges(const cv::Mat& frame);
    cv::Mat applyFaceCrop(const cv::Mat& frame, std::vector<cv::Rect> faceRects);
};
