#pragma once

#include "ofMain.h"

// M0 skeleton: verifies that openFrameworks runs and OpenCV is linked with a
// version new enough for the face pipeline (YuNet needs >= 4.5.4). Reports
// on screen and on the console; later milestones replace this with the real
// detection pipeline.
class ofApp : public ofBaseApp
{
  public:
    void setup() override;
    void draw() override;

    std::vector<std::string> args; // command-line args, set by main()

  private:
    std::vector<std::string> report;
    bool allChecksPassed = false;

    void runChecks();
};
