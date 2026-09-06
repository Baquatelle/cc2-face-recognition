#include "FrameFilter.h"

using namespace std;

// ---- FrameFilter - Modar Issa ----
// Four display modes for the frame before it gets drawn.
// Detection and recognition still work on the original frame so nothing
// breaks, this is only about what shows up on screen.
//
// 7.3a greyscale, 7.3b/7.3c edges, 7.3d face crop using the detector boxes

FrameFilter::FrameFilter() {
    this->mode = FilterMode::NORMAL;
}

// ---- apply ----
// main function, picks the right filter for whatever mode we are in
cv::Mat FrameFilter::apply(const cv::Mat& frame, vector<cv::Rect> faceRects) {
    if (frame.empty()) {
        return frame;   // nothing to filter
    }

    if (mode == FilterMode::GREYSCALE) {
        return applyGreyscale(frame);
    }
    else if (mode == FilterMode::EDGES) {
        return applyEdges(frame);
    }
    else if (mode == FilterMode::FACE_CROP) {
        return applyFaceCrop(frame, faceRects);
    }

    return frame.clone();   // NORMAL, just hand the frame back
}

// ---- cycle through the modes ----
void FrameFilter::nextMode() {
    if (mode == FilterMode::NORMAL) {
        mode = FilterMode::GREYSCALE;
    }
    else if (mode == FilterMode::GREYSCALE) {
        mode = FilterMode::EDGES;
    }
    else if (mode == FilterMode::EDGES) {
        mode = FilterMode::FACE_CROP;
    }
    else {
        mode = FilterMode::NORMAL;
    }
}

// ---- getters and setters ----

void FrameFilter::setMode(FilterMode m) {
    this->mode = m;
}

FilterMode FrameFilter::getMode() {
    return mode;
}

string FrameFilter::getModeName() {
    if (mode == FilterMode::GREYSCALE) return "Greyscale";
    if (mode == FilterMode::EDGES) return "Edges";
    if (mode == FilterMode::FACE_CROP) return "Face Crop";
    return "Normal";
}

// ---- greyscale, 7.3a ----
// cvtColor takes the colour frame and squashes it down to one grey channel.
// every pixel becomes a mix of its blue, green and red values (OpenCV weights
// them, green counts the most because eyes notice green more).
// I turn it back into BGR straight away so it still has 3 channels and the
// rest of the app can draw it without changing anything.
cv::Mat FrameFilter::applyGreyscale(const cv::Mat& frame) {
    cv::Mat grey;
    cv::Mat out;

    cv::cvtColor(frame, grey, cv::COLOR_BGR2GRAY);
    cv::cvtColor(grey, out, cv::COLOR_GRAY2BGR);

    return out;
}

// ---- edges, 7.3b / 7.3c ----
// Canny finds the spots where the brightness changes fast, those are the edges.
// it needs a grey image to start with.
// the two thresholds work like this: anything stronger than cannyHigh is
// definitely an edge, anything weaker than cannyLow gets dropped, and the ones
// in the middle only stay if they are touching a strong edge.
// what comes out is black and white, so back to BGR again for drawing.
cv::Mat FrameFilter::applyEdges(const cv::Mat& frame) {
    cv::Mat grey;
    cv::Mat edges;
    cv::Mat out;

    cv::cvtColor(frame, grey, cv::COLOR_BGR2GRAY);
    cv::Canny(grey, edges, cannyLow, cannyHigh);
    cv::cvtColor(edges, out, cv::COLOR_GRAY2BGR);

    return out;
}

// ---- face crop, 7.3d ----
// takes the boxes the detector found and zooms into the biggest face.
// I add some padding around the box, without it the crop cuts off the chin
// and the top of the head which looks bad.
cv::Mat FrameFilter::applyFaceCrop(const cv::Mat& frame, vector<cv::Rect> faceRects) {
    if (faceRects.size() == 0) {
        return frame.clone();   // no face detected, show the normal frame
    }

    // find the biggest box
    cv::Rect box = faceRects[0];
    for (int i = 1; i < faceRects.size(); i++) {
        if (faceRects[i].area() > box.area()) {
            box = faceRects[i];
        }
    }

    // 20% padding on each side
    int padX = box.width * 0.20;
    int padY = box.height * 0.20;

    cv::Rect padded;
    padded.x = box.x - padX;
    padded.y = box.y - padY;
    padded.width = box.width + padX * 2;
    padded.height = box.height + padY * 2;

    // the padded box comes out close to square but the frame is 16:9, so
    // stretching it up to the full frame size squashes the face sideways.
    // giving the box the same shape as the frame first makes the resize a
    // clean zoom instead of a stretch.
    float frameAspect = (float)frame.cols / (float)frame.rows;
    float cropAspect = (float)padded.width / (float)padded.height;

    if (cropAspect < frameAspect) {
        // too narrow, widen it and keep the face in the middle
        int wanted = (int)(padded.height * frameAspect);
        padded.x = padded.x - (wanted - padded.width) / 2;
        padded.width = wanted;
    }
    else if (cropAspect > frameAspect) {
        // too wide, make it taller instead
        int wanted = (int)(padded.width / frameAspect);
        padded.y = padded.y - (wanted - padded.height) / 2;
        padded.height = wanted;
    }

    // if it ended up bigger than the frame, shrink it back but keep the shape
    if (padded.width > frame.cols) {
        padded.width = frame.cols;
        padded.height = (int)(frame.cols / frameAspect);
    }
    if (padded.height > frame.rows) {
        padded.height = frame.rows;
        padded.width = (int)(frame.rows * frameAspect);
    }

    // slide it back inside the frame instead of cutting it. cutting is what
    // the old code did and it changes the shape again, which brings the
    // stretch straight back
    if (padded.x < 0) padded.x = 0;
    if (padded.y < 0) padded.y = 0;
    if (padded.x + padded.width > frame.cols) padded.x = frame.cols - padded.width;
    if (padded.y + padded.height > frame.rows) padded.y = frame.rows - padded.height;

    if (padded.width <= 0 || padded.height <= 0) {
        return frame.clone();
    }

    // cut the face out and stretch it back up to the full frame size
    cv::Mat cropped = frame(padded).clone();
    cv::Mat out;
    cv::resize(cropped, out, cv::Size(frame.cols, frame.rows));

    return out;
}
