#include "ofMain.h"
#include "ofApp.h"
#include "FaceDetector.h"
#include "FaceRecognizer.h"

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

// Detect + recognize faces in one image against the data/gallery folder,
// print name/score per face, and exit. `name` applies the default match
// threshold; `best`/`score` always show the closest gallery person, so
// threshold tuning can be done from the raw output.
int runHeadlessIdentify(const std::string &path)
{
    FaceDetector detector;
    if (!detector.setup(ofToDataPath(kYunetModel)))
    {
        std::fprintf(stderr, "could not load the YuNet model — run scripts/bootstrap.py first\n");
        return 1;
    }
    FaceRecognizer recognizer;
    if (!recognizer.setup(ofToDataPath(kSfaceModel)))
    {
        std::fprintf(stderr, "could not load the SFace model — run scripts/bootstrap.py first\n");
        return 1;
    }
    if (recognizer.loadGallery(ofToDataPath("gallery"), detector) == 0)
    {
        std::fprintf(stderr, "no usable gallery at data/gallery — run scripts/bootstrap.py first\n");
        return 1;
    }

    ofPixels pixels;
    if (!ofLoadImage(pixels, ofToDataPath(path)))
    {
        std::fprintf(stderr, "could not load image: %s\n", path.c_str());
        return 1;
    }

    cv::Mat bgr = toBgr(std::move(pixels));
    auto detections = detector.detect(bgr);
    auto matches = recognizer.identify(bgr, detections);
    std::printf("gallery: %d person(s)\n", recognizer.personCount());
    std::printf("faces: %zu\n", detections.size());
    for (size_t i = 0; i < detections.size(); i++)
    {
        const auto &d = detections[i];
        const auto &m = matches[i];
        bool recognized = m.score >= FaceRecognizer::kDefaultMatchThreshold;
        std::string score = m.score < 0 ? "n/a" : ofToString(m.score, 2);
        std::printf("face %zu: name=%s best=%s score=%s x=%.0f y=%.0f w=%.0f h=%.0f\n", i,
                    recognized ? m.name.c_str() : "unknown", m.name.empty() ? "-" : m.name.c_str(), score.c_str(),
                    d.box.x, d.box.y, d.box.width, d.box.height);
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
    for (auto [flag, run] : {std::pair{"--detect", runHeadlessDetect}, std::pair{"--identify", runHeadlessIdentify}})
    {
        auto flagIt = std::find(args.begin(), args.end(), flag);
        if (flagIt == args.end())
        {
            continue;
        }
        auto imageIt = std::next(flagIt);
        if (imageIt == args.end() || (!imageIt->empty() && (*imageIt)[0] == '-'))
        {
            std::fprintf(stderr, "usage: facerec %s <image>\n", flag);
            return 1;
        }
        return run(*imageIt);
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
