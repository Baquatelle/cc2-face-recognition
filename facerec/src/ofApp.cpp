#include "ofApp.h"

#include <algorithm>
#include <cstdlib>

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

void ofApp::setup()
{
    ofSetWindowTitle("facerec — M0 skeleton");
    runChecks();

    for (const auto &line : report)
    {
        ofLogNotice("facerec") << line;
    }

    // scripts/build.py --check runs the binary with --selftest to verify the
    // build without leaving a window open
    bool selftest = std::find(args.begin(), args.end(), "--selftest") != args.end();
    if (selftest)
    {
        std::exit(allChecksPassed ? 0 : 1);
    }
}

void ofApp::runChecks()
{
    allChecksPassed = true;
    auto check = [this](bool ok, const std::string &what) {
        report.push_back(std::string(ok ? "[ok]   " : "[FAIL] ") + what);
        if (!ok)
            allChecksPassed = false;
    };

    report.push_back("openFrameworks " + ofGetVersionInfo());
    report.push_back("OpenCV " + cv::getVersionString());

    int cvVersion = CV_VERSION_MAJOR * 10000 + CV_VERSION_MINOR * 100 + CV_VERSION_REVISION;
    check(cvVersion >= 40504, "OpenCV >= 4.5.4 (required by YuNet)");

    std::string yunetPath = ofToDataPath("models/face_detection_yunet_2023mar.onnx");
    std::string sfacePath = ofToDataPath("models/face_recognition_sface_2021dec.onnx");
    bool yunetFound = ofFile::doesFileExist(yunetPath);
    bool sfaceFound = ofFile::doesFileExist(sfacePath);
    check(yunetFound, "YuNet model file in data/models (fetched by bootstrap.py)");
    check(sfaceFound, "SFace model file in data/models (fetched by bootstrap.py)");

    if (yunetFound)
    {
        try
        {
            auto detector = cv::FaceDetectorYN::create(yunetPath, "", cv::Size(320, 320));
            check(detector != nullptr, "cv::FaceDetectorYN loads the YuNet model");
        }
        catch (const cv::Exception &e)
        {
            check(false, std::string("cv::FaceDetectorYN loads the YuNet model — ") + e.what());
        }
    }
    if (sfaceFound)
    {
        try
        {
            auto recognizer = cv::FaceRecognizerSF::create(sfacePath, "");
            check(recognizer != nullptr, "cv::FaceRecognizerSF loads the SFace model");
        }
        catch (const cv::Exception &e)
        {
            check(false, std::string("cv::FaceRecognizerSF loads the SFace model — ") + e.what());
        }
    }

    report.push_back(allChecksPassed ? "M0 checks passed" : "M0 checks FAILED");
}

void ofApp::draw()
{
    ofBackground(allChecksPassed ? ofColor(20, 60, 30) : ofColor(70, 25, 25));
    ofSetColor(255);
    float y = 40;
    for (const auto &line : report)
    {
        ofDrawBitmapString(line, 30, y);
        y += 20;
    }
}
