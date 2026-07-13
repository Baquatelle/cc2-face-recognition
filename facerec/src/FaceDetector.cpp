#include "FaceDetector.h"

#include <cmath>

#include <opencv2/imgproc.hpp>

#include "ofxCv.h"

bool FaceDetector::setup(const std::string &modelPath, float scoreThreshold)
{
    try
    {
        detector = cv::FaceDetectorYN::create(modelPath, "", cv::Size(320, 320), scoreThreshold);
    }
    catch (const cv::Exception &e)
    {
        ofLogError("FaceDetector") << "failed to load " << modelPath << ": " << e.what();
        detector = nullptr;
    }
    return isLoaded();
}

void FaceDetector::setScoreThreshold(float threshold)
{
    if (detector)
    {
        detector->setScoreThreshold(threshold);
    }
}

std::vector<FaceDetection> FaceDetector::detect(const cv::Mat &bgr)
{
    std::vector<FaceDetection> result;
    if (!detector || bgr.empty())
    {
        return result;
    }

    float scale = 1.0f;
    cv::Mat input = bgr;
    int longSide = std::max(bgr.cols, bgr.rows);
    if (longSide > maxInferenceSide)
    {
        scale = float(longSide) / maxInferenceSide;
        cv::resize(bgr, input, cv::Size(std::lround(bgr.cols / scale), std::lround(bgr.rows / scale)), 0, 0,
                   cv::INTER_AREA);
    }

    detector->setInputSize(input.size());
    // one row per face: x, y, w, h, five landmark (x, y) pairs, score
    cv::Mat faces;
    detector->detect(input, faces);

    for (int i = 0; i < faces.rows; i++)
    {
        const float *f = faces.ptr<float>(i);
        FaceDetection d;
        d.box = ofRectangle(f[kBoxXIndex] * scale, f[kBoxYIndex] * scale, f[kBoxWidthIndex] * scale,
                            f[kBoxHeightIndex] * scale);
        for (int k = 0; k < kNumLandmarks; k++)
        {
            d.landmarks[k] = {f[kLandmarkOffset + 2 * k] * scale, f[kLandmarkOffset + 2 * k + 1] * scale};
        }
        d.confidence = f[kConfidenceIndex];
        result.push_back(d);
    }
    return result;
}

cv::Mat toBgr(ofPixels pixels)
{
    pixels.setImageType(OF_IMAGE_COLOR); // normalize gray/alpha to 8-bit RGB
    cv::Mat bgr;
    cv::cvtColor(ofxCv::toCv(pixels), bgr, cv::COLOR_RGB2BGR);
    return bgr;
}

std::vector<FaceDetection> detectInPixels(FaceDetector &detector, ofPixels pixels)
{
    return detector.detect(toBgr(std::move(pixels)));
}
