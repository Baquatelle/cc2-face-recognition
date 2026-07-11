#pragma once

#include <array>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

#include "ofMain.h"

// One detected face, in pixel coordinates of the image passed to detect().
struct FaceDetection
{
    ofRectangle box;
    // YuNet order: right eye, left eye, nose tip, right/left mouth corner
    std::array<glm::vec2, 5> landmarks;
    float confidence = 0.0f;
};

// Wraps OpenCV's YuNet face detector (cv::FaceDetectorYN). Owns the model and
// the coordinate bookkeeping: large images are downscaled before inference and
// the results mapped back, so callers always work in original-image pixels.
class FaceDetector
{
  public:
    // scoreThreshold: minimum detector confidence (0..1) to keep a face
    bool setup(const std::string &modelPath, float scoreThreshold = 0.6f);
    bool isLoaded() const
    {
        return detector != nullptr;
    }

    void setScoreThreshold(float threshold);

    // bgr: 8-bit 3-channel BGR image (OpenCV's native layout)
    std::vector<FaceDetection> detect(const cv::Mat &bgr);

  private:
    cv::Ptr<cv::FaceDetectorYN> detector;
    // YuNet's cost scales with input area; images bigger than this on their
    // longest side are detected at reduced resolution and mapped back.
    static constexpr int maxInferenceSide = 1280;
};

// Normalize openFrameworks pixels (any type) to BGR and run detection. Keeps
// the gray/alpha -> RGB -> BGR conversion in one place so the interactive and
// headless (--detect) paths can't drift apart. Takes pixels by value because
// setImageType() mutates.
std::vector<FaceDetection> detectInPixels(FaceDetector &detector, ofPixels pixels);
