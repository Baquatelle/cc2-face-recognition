#pragma once

#include "ofMain.h"
#include "ofxGui.h"

#include "FaceDetector.h"
#include "FaceRecognizer.h"

// M3: detection (M1/M2) plus recognition. Three switchable inputs — still
// image, video file, live webcam — switched from the GUI panel; opening a
// file (O key, drag & drop, or a plain <path> argument, resolved against
// bin/data/) picks image vs. video mode by file extension. Every new frame
// runs through YuNet, then each face is embedded with SFace and matched
// against the gallery (data/gallery/<person>/*.jpg, auto-loaded at startup,
// re-loadable from the panel). Overlays show boxes, landmarks, name or
// "unknown" with the match score, face count and pipeline time. The match
// threshold slider only changes labelling, so it is applied at draw time —
// no re-embedding.
//
// Headless modes (--selftest, --detect <image>, --identify <image>) are
// handled in main() before any GL window is created, so they run on a
// display-less machine.
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
    FaceRecognizer recognizer;
    InputMode mode = InputMode::None;

    ofImage image;
    ofVideoPlayer video;
    ofVideoGrabber grabber;
    std::string sourceName; // file name, or "webcam"

    std::vector<FaceDetection> faces;
    std::vector<FaceMatch> matches; // index-aligned with faces; empty if no gallery
    float detectMillis = 0.0f;      // smoothed across frames for video sources
    uint64_t lastLogMillis = 0;
    std::string status; // message shown while no source is active

    ofxPanel gui;
    ofxButton openImageButton;
    ofxButton openVideoButton;
    ofxButton loadGalleryButton;
    ofParameter<bool> webcamOn{"webcam", false};
    ofParameter<float> scoreThreshold{"conf threshold", 0.6f, 0.05f, 0.95f};
    // kDefaultMatchThreshold is odr-used here; no out-of-line definition is
    // needed because C++17 makes static constexpr members implicitly inline.
    ofParameter<float> matchThreshold{"match threshold", FaceRecognizer::kDefaultMatchThreshold, 0.0f, 1.0f};

    bool openPath(const std::string &path);
    bool loadImage(const std::string &path);
    bool loadVideo(const std::string &path);
    void stopCurrentSource();
    void detectFrame(const ofPixels &pixels);
    void loadGallery(const std::string &path);

    void onOpenImage();
    void onOpenVideo();
    void onLoadGallery();
    void onWebcamToggle(bool &on);
    void onScoreThreshold(float &value);
};
