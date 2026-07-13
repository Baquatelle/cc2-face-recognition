#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

#include "FaceDetector.h"

// Best gallery match for one detected face. `name` is always the closest
// gallery person; whether the face is labelled with it or shown as "unknown"
// is decided by comparing `score` against the caller's match threshold, so a
// threshold change needs no re-embedding.
struct FaceMatch
{
    std::string name;
    float score = -1.0f; // cosine similarity, higher = more similar
};

// Wraps OpenCV's SFace recognizer (cv::FaceRecognizerSF) plus the gallery of
// known people. Each face is aligned via its landmarks, embedded into a 128-d
// vector, and matched against gallery embeddings by cosine similarity.
class FaceRecognizer
{
  public:
    // SFace's canonical cosine-similarity decision threshold.
    static constexpr float kDefaultMatchThreshold = 0.363f;

    bool setup(const std::string &modelPath);
    bool isLoaded() const
    {
        return recognizer != nullptr;
    }

    // Load a gallery folder with one subfolder per person, e.g.
    // gallery/alice/*.jpg. Detects the single most confident face in each
    // photo (using `detector`) and stores its embedding under the folder
    // name; a person may have several photos and therefore several
    // embeddings. Replaces any previously loaded gallery. Returns the number
    // of embeddings loaded.
    int loadGallery(const std::string &dirPath, FaceDetector &detector);
    bool hasGallery() const
    {
        return !entries.empty();
    }
    int personCount() const
    {
        return numPersons;
    }

    // Embed each detected face (bgr must be the same image the detections
    // came from, original resolution) and return its best gallery match,
    // index-aligned with `faces`.
    std::vector<FaceMatch> identify(const cv::Mat &bgr, const std::vector<FaceDetection> &faces);

  private:
    struct Entry
    {
        std::string name;
        cv::Mat feature; // 1x128 float embedding
    };

    // alignCrop + feature; empty Mat on failure
    cv::Mat embed(const cv::Mat &bgr, const FaceDetection &face);

    cv::Ptr<cv::FaceRecognizerSF> recognizer;
    std::vector<Entry> entries;
    int numPersons = 0;
};
