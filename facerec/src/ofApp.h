#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "FaceDetector.h"

// M2: the M1 detection pipeline on three switchable inputs — still image,
// video file, live webcam. Modes are switched from the GUI panel; opening a
// file (O key, drag & drop, or a plain <path> argument, resolved against
// bin/data/) picks image vs. video mode by file extension. Every new frame
// runs through YuNet; overlays show boxes, landmarks, face count and
// detection time.
//
// Headless modes (--selftest, --detect <image>) are handled in main() before
// any GL window is created, so they run on a display-less machine.
class ofApp : public ofBaseApp
{
  public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyPressed(int key) override;
    void dragEvent(ofDragInfo dragInfo) override;

    std::vector<std::string> args; // command-line args, set by main()

  private:
    enum class InputMode
    {
        None,
        Image,
        Video,
        Webcam
    };

    FaceDetector detector;
    InputMode mode = InputMode::None;

    ofImage image;
    ofVideoPlayer video;
    ofVideoGrabber grabber;
    std::string sourceName; // file name, or "webcam"

    std::vector<FaceDetection> faces;
    float detectMillis = 0.0f; // smoothed across frames for video sources
    uint64_t lastLogMillis = 0;
    std::string status; // message shown while no source is active

    ofxPanel gui;
    ofxButton openImageButton;
    ofxButton openVideoButton;
    ofParameter<bool> webcamOn{"webcam", false};
    ofParameter<float> scoreThreshold{"conf threshold", 0.6f, 0.05f, 0.95f};

    bool openPath(const std::string &path);
    bool loadImage(const std::string &path);
    bool loadVideo(const std::string &path);
    void stopCurrentSource();
    void detectFrame(const ofPixels &pixels);

    void onOpenImage();
    void onOpenVideo();
    void onWebcamToggle(bool &on);
    void onScoreThreshold(float &value);
};
