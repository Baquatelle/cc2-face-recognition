#pragma once

#include <array>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

#include "ofMain.h"

// YuNet emits one row per face as a flat float array laid out as
// [x, y, w, h, (landmark x, y) x kNumLandmarks, score] = kYunetRowSize columns.
// These names are the single source of truth for that layout.
inline constexpr int kNumLandmarks = 5;
inline constexpr int kBoxXIndex = 0;
inline constexpr int kBoxYIndex = 1;
inline constexpr int kBoxWidthIndex = 2;
inline constexpr int kBoxHeightIndex = 3;
inline constexpr int kLandmarkOffset = 4;                                     // index of the first landmark x
inline constexpr int kYunetRowSize = kLandmarkOffset + 2 * kNumLandmarks + 1; // 15
inline constexpr int kConfidenceIndex = kYunetRowSize - 1;                    // 14

// One detected face, in pixel coordinates of the image passed to detect().
struct FaceDetection
{
    ofRectangle box;
    // YuNet order: right eye, left eye, nose tip, right/left mouth corner
    std::array<glm::vec2, kNumLandmarks> landmarks;
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

// Normalize openFrameworks pixels (any type: gray/alpha/RGB) to an 8-bit BGR
// Mat, OpenCV's native layout. One place for the conversion so the interactive
// and headless paths can't drift apart. Takes pixels by value because
// setImageType() mutates.
cv::Mat toBgr(ofPixels pixels);

// Convenience: toBgr() + detect(), for callers that only need detection.
std::vector<FaceDetection> detectInPixels(FaceDetector &detector, ofPixels pixels);
