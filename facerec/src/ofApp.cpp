#include "ofApp.h"

#include <algorithm>

void ofApp::setup()
{
    ofSetWindowTitle("facerec — M1 images");

    std::string modelPath = ofToDataPath("models/face_detection_yunet_2023mar.onnx");
    bool loaded = detector.setup(modelPath);

    status = loaded ? "Press O to open an image, or drag & drop one onto the window."
                    : "FAILED to load the YuNet model — run scripts/bootstrap.py first.";

    // a plain (non-flag) argument is an image to open at startup
    for (const auto &arg : args)
    {
        if (!arg.empty() && arg[0] != '-')
        {
            loadImage(ofToDataPath(arg));
            break;
        }
    }
}

bool ofApp::loadImage(const std::string &path)
{
    ofImage next;
    if (!next.load(path))
    {
        status = "Could not load image: " + path;
        return false;
    }
    image = next;
    imageName = ofFilePath::getFileName(path);

    // detectInPixels() normalizes gray/alpha to RGB and converts to BGR itself.
    uint64_t start = ofGetElapsedTimeMicros();
    faces = detectInPixels(detector, image.getPixels());
    detectMillis = (ofGetElapsedTimeMicros() - start) / 1000.0f;

    ofLogNotice("facerec") << imageName << ": " << faces.size() << " face(s) in " << detectMillis << " ms";
    return true;
}

void ofApp::draw()
{
    ofBackground(24);

    if (!image.isAllocated())
    {
        ofSetColor(255);
        ofDrawBitmapString(status, 30, 40);
        return;
    }

    // fit the image into the window, letterboxed
    float scale = std::min(ofGetWidth() / image.getWidth(), ofGetHeight() / image.getHeight());
    float offsetX = (ofGetWidth() - image.getWidth() * scale) / 2;
    float offsetY = (ofGetHeight() - image.getHeight() * scale) / 2;

    ofSetColor(255);
    image.draw(offsetX, offsetY, image.getWidth() * scale, image.getHeight() * scale);

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
    ofDrawBitmapStringHighlight(imageName + "  (" + ofToString(detectMillis, 1) + " ms, O = open another image)", 20,
                                ofGetHeight() - 20);
}

void ofApp::keyPressed(int key)
{
    if (key == 'o' || key == 'O')
    {
        auto result = ofSystemLoadDialog("Choose an image");
        if (result.bSuccess)
        {
            loadImage(result.getPath());
        }
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo)
{
    if (!dragInfo.files.empty())
    {
        loadImage(dragInfo.files.front());
    }
}
