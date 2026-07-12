#include "ofApp.h"

#include <algorithm>

namespace
{

const std::vector<std::string> kVideoExtensions = {"mp4", "mov", "m4v", "avi", "mpg", "mpeg", "mkv", "webm"};

bool isVideoPath(const std::string &path)
{
    std::string ext = ofToLower(ofFilePath::getFileExt(path));
    return std::find(kVideoExtensions.begin(), kVideoExtensions.end(), ext) != kVideoExtensions.end();
}

} // namespace

void ofApp::setup()
{
    ofSetWindowTitle("facerec — M2 video");

    std::string modelPath = ofToDataPath("models/face_detection_yunet_2023mar.onnx");
    bool loaded = detector.setup(modelPath, scoreThreshold);

    status = loaded ? "Open an image or video (O key, drag & drop, or the panel), or turn on the webcam."
                    : "FAILED to load the YuNet model — run scripts/bootstrap.py first.";

    gui.setup("facerec");
    gui.add(openImageButton.setup("open image..."));
    gui.add(openVideoButton.setup("open video..."));
    gui.add(webcamOn);
    gui.add(scoreThreshold);
    openImageButton.addListener(this, &ofApp::onOpenImage);
    openVideoButton.addListener(this, &ofApp::onOpenVideo);
    webcamOn.addListener(this, &ofApp::onWebcamToggle);
    scoreThreshold.addListener(this, &ofApp::onScoreThreshold);

    // a plain (non-flag) argument is an image or video to open at startup
    for (const auto &arg : args)
    {
        if (!arg.empty() && arg[0] != '-')
        {
            openPath(ofToDataPath(arg));
            break;
        }
    }
}

bool ofApp::openPath(const std::string &path)
{
    return isVideoPath(path) ? loadVideo(path) : loadImage(path);
}

void ofApp::stopCurrentSource()
{
    if (mode == InputMode::Video)
    {
        video.close();
    }
    else if (mode == InputMode::Webcam)
    {
        grabber.close();
        webcamOn.setWithoutEventNotifications(false);
    }
    mode = InputMode::None;
    faces.clear();
    detectMillis = 0.0f;
}

bool ofApp::loadImage(const std::string &path)
{
    ofImage next;
    if (!next.load(path))
    {
        status = "Could not load image: " + path;
        return false;
    }
    stopCurrentSource();
    image = next;
    mode = InputMode::Image;
    sourceName = ofFilePath::getFileName(path);

    detectFrame(image.getPixels());
    ofLogNotice("facerec") << sourceName << ": " << faces.size() << " face(s) in " << detectMillis << " ms";
    return true;
}

bool ofApp::loadVideo(const std::string &path)
{
    stopCurrentSource();
    if (!video.load(path))
    {
        status = "Could not load video: " + path;
        return false;
    }
    video.setLoopState(OF_LOOP_NORMAL);
    video.play();
    mode = InputMode::Video;
    sourceName = ofFilePath::getFileName(path);
    return true;
}

void ofApp::update()
{
    if (mode == InputMode::Video)
    {
        video.update();
        if (video.isFrameNew())
        {
            detectFrame(video.getPixels());
        }
    }
    else if (mode == InputMode::Webcam)
    {
        grabber.update();
        if (grabber.isFrameNew())
        {
            detectFrame(grabber.getPixels());
        }
    }

    // once-a-second heartbeat so video/webcam runs are observable in the log
    if (mode == InputMode::Video || mode == InputMode::Webcam)
    {
        uint64_t now = ofGetElapsedTimeMillis();
        if (now - lastLogMillis >= 1000)
        {
            lastLogMillis = now;
            ofLogNotice("facerec") << sourceName << ": " << faces.size() << " face(s), " << ofToString(detectMillis, 1)
                                   << " ms/detect, " << ofToString(ofGetFrameRate(), 0) << " fps";
        }
    }
}

void ofApp::detectFrame(const ofPixels &pixels)
{
    uint64_t start = ofGetElapsedTimeMicros();
    faces = detectInPixels(detector, pixels);
    float millis = (ofGetElapsedTimeMicros() - start) / 1000.0f;
    // smooth the readout across frames; first measurement is taken as-is
    detectMillis = detectMillis == 0.0f ? millis : ofLerp(detectMillis, millis, 0.15f);
}

void ofApp::draw()
{
    ofBackground(24);

    float srcW = 0, srcH = 0;
    if (mode == InputMode::Image && image.isAllocated())
    {
        srcW = image.getWidth();
        srcH = image.getHeight();
    }
    else if (mode == InputMode::Video && video.isLoaded())
    {
        srcW = video.getWidth();
        srcH = video.getHeight();
    }
    else if (mode == InputMode::Webcam && grabber.isInitialized())
    {
        srcW = grabber.getWidth();
        srcH = grabber.getHeight();
    }

    if (srcW <= 0 || srcH <= 0)
    {
        ofSetColor(255);
        ofDrawBitmapString(status, 30, 40);
        gui.draw();
        return;
    }

    // fit the source into the window, letterboxed
    float scale = std::min(ofGetWidth() / srcW, ofGetHeight() / srcH);
    float offsetX = (ofGetWidth() - srcW * scale) / 2;
    float offsetY = (ofGetHeight() - srcH * scale) / 2;

    ofSetColor(255);
    if (mode == InputMode::Image)
    {
        image.draw(offsetX, offsetY, srcW * scale, srcH * scale);
    }
    else if (mode == InputMode::Video)
    {
        video.draw(offsetX, offsetY, srcW * scale, srcH * scale);
    }
    else
    {
        grabber.draw(offsetX, offsetY, srcW * scale, srcH * scale);
    }

    for (const auto &face : faces)
    {
        float x = offsetX + face.box.x * scale;
        float y = offsetY + face.box.y * scale;

        ofPushStyle();
        ofNoFill();
        ofSetLineWidth(2);
        ofSetColor(80, 255, 120);
        ofDrawRectangle(x, y, face.box.width * scale, face.box.height * scale);
        ofFill();
        for (const auto &lm : face.landmarks)
        {
            ofDrawCircle(offsetX + lm.x * scale, offsetY + lm.y * scale, 3);
        }
        ofPopStyle();

        ofDrawBitmapStringHighlight(ofToString(face.confidence, 2), x, y - 6);
    }

    ofDrawBitmapStringHighlight("Faces: " + ofToString(faces.size()), 20, ofGetHeight() - 44);

    std::string info = sourceName + "  (" + ofToString(detectMillis, 1) + " ms/detect";
    if (mode != InputMode::Image)
    {
        info += ", " + ofToString(ofGetFrameRate(), 0) + " fps";
    }
    if (mode == InputMode::Video)
    {
        info += video.isPaused() ? ", paused — space resumes" : ", space pauses";
    }
    info += ")";
    ofDrawBitmapStringHighlight(info, 20, ofGetHeight() - 20);

    gui.draw();
}

void ofApp::keyPressed(int key)
{
    if (key == 'o' || key == 'O')
    {
        auto result = ofSystemLoadDialog("Choose an image or video");
        if (result.bSuccess)
        {
            openPath(result.getPath());
        }
    }
    else if (key == ' ' && mode == InputMode::Video)
    {
        video.setPaused(!video.isPaused());
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo)
{
    if (!dragInfo.files.empty())
    {
        openPath(dragInfo.files.front());
    }
}

void ofApp::onOpenImage()
{
    auto result = ofSystemLoadDialog("Choose an image");
    if (result.bSuccess)
    {
        loadImage(result.getPath());
    }
}

void ofApp::onOpenVideo()
{
    auto result = ofSystemLoadDialog("Choose a video file");
    if (result.bSuccess)
    {
        loadVideo(result.getPath());
    }
}

void ofApp::onWebcamToggle(bool &on)
{
    if (!on)
    {
        if (mode == InputMode::Webcam)
        {
            stopCurrentSource();
            status = "Webcam off. Open an image or video, or turn the webcam back on.";
        }
        return;
    }
    stopCurrentSource();
    grabber.setDesiredFrameRate(30);
    grabber.setup(1280, 720);
    if (!grabber.isInitialized())
    {
        status = "Could not open the webcam.";
        webcamOn.setWithoutEventNotifications(false);
        return;
    }
    mode = InputMode::Webcam;
    sourceName = "webcam";
}

void ofApp::onScoreThreshold(float &value)
{
    detector.setScoreThreshold(value);
    // video/webcam pick the new threshold up on their next frame; a still
    // image has to be re-detected explicitly
    if (mode == InputMode::Image && image.isAllocated())
    {
        detectMillis = 0.0f;
        detectFrame(image.getPixels());
    }
}
