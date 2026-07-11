#pragma once

#include "ofMain.h"

#include "FaceDetector.h"

// M1: load a still image (press O for a file dialog, or drag & drop one onto
// the window), run YuNet face detection, draw a box + landmarks per face and
// show the face count.
//
// A plain <image> argument opens that image at startup (relative paths are
// resolved against bin/data/).
//
// Headless modes (--selftest, --detect <image>) are handled in main() before
// any GL window is created, so they run on a display-less machine.
class ofApp : public ofBaseApp
{
  public:
    void setup() override;
    void draw() override;
    void keyPressed(int key) override;
    void dragEvent(ofDragInfo dragInfo) override;

    std::vector<std::string> args; // command-line args, set by main()

  private:
    FaceDetector detector;
    ofImage image;
    std::string imageName;
    std::vector<FaceDetection> faces;
    float detectMillis = 0.0f;
    std::string status; // message shown while no image is loaded

    bool loadImage(const std::string &path);
};
