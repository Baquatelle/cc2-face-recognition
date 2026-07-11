#include "ofMain.h"
#include "ofApp.h"
#include "FaceDetector.h"

#include <algorithm>
#include <cstdio>
#include <iterator>
#include <string>
#include <vector>

namespace
{

const char *kYunetModel = "models/face_detection_yunet_2023mar.onnx";
const char *kSfaceModel = "models/face_recognition_sface_2021dec.onnx";

// M0 environment checks. Run by scripts/build.py --check to verify the build
// without opening a window.
int runSelftest()
{
    bool allPassed = true;
    auto check = [&allPassed](bool ok, const std::string &what) {
        ofLogNotice("facerec") << (ok ? "[ok]   " : "[FAIL] ") << what;
        if (!ok)
            allPassed = false;
    };

    ofLogNotice("facerec") << "openFrameworks " << ofGetVersionInfo();
    ofLogNotice("facerec") << "OpenCV " << cv::getVersionString();

    int cvVersion = CV_VERSION_MAJOR * 10000 + CV_VERSION_MINOR * 100 + CV_VERSION_REVISION;
    check(cvVersion >= 40504, "OpenCV >= 4.5.4 (required by YuNet)");

    std::string yunetPath = ofToDataPath(kYunetModel);
    std::string sfacePath = ofToDataPath(kSfaceModel);
    check(ofFile::doesFileExist(yunetPath), "YuNet model file in data/models");
    check(ofFile::doesFileExist(sfacePath), "SFace model file in data/models");

    FaceDetector yunet;
    check(yunet.setup(yunetPath), "cv::FaceDetectorYN loads the YuNet model");
    try
    {
        auto recognizer = cv::FaceRecognizerSF::create(sfacePath, "");
        check(recognizer != nullptr, "cv::FaceRecognizerSF loads the SFace model");
    }
    catch (const cv::Exception &e)
    {
        check(false, std::string("cv::FaceRecognizerSF loads the SFace model — ") + e.what());
    }

    ofLogNotice("facerec") << (allPassed ? "selftest passed" : "selftest FAILED");
    return allPassed ? 0 : 1;
}

// Detect faces in one image, print results, and exit. Relative paths resolve
// against bin/data/, matching the plain-argument startup path.
int runHeadlessDetect(const std::string &path)
{
    FaceDetector detector;
    if (!detector.setup(ofToDataPath(kYunetModel)))
    {
        std::fprintf(stderr, "could not load the YuNet model — run scripts/bootstrap.py first\n");
        return 1;
    }

    ofPixels pixels;
    if (!ofLoadImage(pixels, ofToDataPath(path)))
    {
        std::fprintf(stderr, "could not load image: %s\n", path.c_str());
        return 1;
    }

    auto detections = detectInPixels(detector, pixels);
    std::printf("faces: %zu\n", detections.size());
    for (size_t i = 0; i < detections.size(); i++)
    {
        const auto &d = detections[i];
        std::printf("face %zu: x=%.0f y=%.0f w=%.0f h=%.0f confidence=%.2f\n", i, d.box.x, d.box.y, d.box.width,
                    d.box.height, d.confidence);
    }
    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++)
    {
        args.push_back(argv[i]);
    }

    // Headless modes run before any window is created, so they work on a
    // display-less machine (e.g. CI running scripts/build.py --check).
    if (std::find(args.begin(), args.end(), "--selftest") != args.end())
    {
        return runSelftest();
    }
    auto detectIt = std::find(args.begin(), args.end(), "--detect");
    if (detectIt != args.end())
    {
        auto imageIt = std::next(detectIt);
        if (imageIt == args.end() || (!imageIt->empty() && (*imageIt)[0] == '-'))
        {
            std::fprintf(stderr, "usage: facerec --detect <image>\n");
            return 1;
        }
        return runHeadlessDetect(*imageIt);
    }

    ofGLWindowSettings settings;
    settings.setSize(1024, 768);
    settings.title = "facerec";

    auto window = ofCreateWindow(settings);
    auto app = std::make_shared<ofApp>();
    app->args = args;
    ofRunApp(window, app);
    return ofRunMainLoop();
}
