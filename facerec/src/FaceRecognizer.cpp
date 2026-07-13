#include "FaceRecognizer.h"

#include <algorithm>

#include "ofMain.h"

bool FaceRecognizer::setup(const std::string &modelPath)
{
    try
    {
        recognizer = cv::FaceRecognizerSF::create(modelPath, "");
    }
    catch (const cv::Exception &e)
    {
        ofLogError("FaceRecognizer") << "failed to load " << modelPath << ": " << e.what();
        recognizer = nullptr;
    }
    return isLoaded();
}

cv::Mat FaceRecognizer::embed(const cv::Mat &bgr, const FaceDetection &face)
{
    // rebuild the YuNet output row alignCrop() expects:
    // x, y, w, h, five landmark (x, y) pairs, score
    cv::Mat row(1, 15, CV_32F);
    float *f = row.ptr<float>(0);
    f[0] = face.box.x;
    f[1] = face.box.y;
    f[2] = face.box.width;
    f[3] = face.box.height;
    for (int k = 0; k < 5; k++)
    {
        f[4 + 2 * k] = face.landmarks[k].x;
        f[5 + 2 * k] = face.landmarks[k].y;
    }
    f[14] = face.confidence;

    try
    {
        cv::Mat aligned, feature;
        recognizer->alignCrop(bgr, row, aligned);
        recognizer->feature(aligned, feature);
        // feature() returns a view of an internal buffer; clone to keep it
        return feature.clone();
    }
    catch (const cv::Exception &e)
    {
        ofLogError("FaceRecognizer") << "embedding failed: " << e.what();
        return {};
    }
}

int FaceRecognizer::loadGallery(const std::string &dirPath, FaceDetector &detector)
{
    entries.clear();
    numPersons = 0;
    if (!isLoaded())
    {
        return 0;
    }

    ofDirectory root(dirPath);
    if (!root.exists())
    {
        ofLogWarning("FaceRecognizer") << "no gallery folder at " << dirPath;
        return 0;
    }
    root.listDir();

    for (const auto &personFile : root.getFiles())
    {
        if (!personFile.isDirectory())
        {
            continue;
        }
        std::string name = personFile.getBaseName();

        ofDirectory photos(personFile.getAbsolutePath());
        for (const std::string &ext : {"jpg", "jpeg", "png", "bmp", "pgm"})
        {
            photos.allowExt(ext);
        }
        photos.listDir();

        int before = int(entries.size());
        for (const auto &photo : photos.getFiles())
        {
            std::string path = photo.getAbsolutePath();
            ofPixels pixels;
            if (!ofLoadImage(pixels, path))
            {
                ofLogWarning("FaceRecognizer") << "could not load " << path;
                continue;
            }
            cv::Mat bgr = toBgr(std::move(pixels));
            auto faces = detector.detect(bgr);
            if (faces.empty())
            {
                ofLogWarning("FaceRecognizer") << "no face found in " << path;
                continue;
            }
            auto best =
                std::max_element(faces.begin(), faces.end(), [](const FaceDetection &a, const FaceDetection &b) {
                    return a.confidence < b.confidence;
                });
            cv::Mat feature = embed(bgr, *best);
            if (feature.empty())
            {
                continue;
            }
            entries.push_back({name, feature});
            ofLogNotice("FaceRecognizer") << name << ": embedded " << photo.getFileName();
        }
        if (int(entries.size()) > before)
        {
            numPersons++;
        }
        else
        {
            ofLogWarning("FaceRecognizer") << name << ": no usable photos, skipping";
        }
    }

    ofLogNotice("FaceRecognizer") << "gallery: " << entries.size() << " embedding(s) across " << numPersons
                                  << " person(s)";
    return int(entries.size());
}

std::vector<FaceMatch> FaceRecognizer::identify(const cv::Mat &bgr, const std::vector<FaceDetection> &faces)
{
    std::vector<FaceMatch> matches(faces.size());
    if (!isLoaded() || entries.empty())
    {
        return matches;
    }

    for (size_t i = 0; i < faces.size(); i++)
    {
        cv::Mat feature = embed(bgr, faces[i]);
        if (feature.empty())
        {
            continue;
        }
        for (const auto &entry : entries)
        {
            float score = float(recognizer->match(feature, entry.feature, cv::FaceRecognizerSF::DisType::FR_COSINE));
            if (score > matches[i].score)
            {
                matches[i] = {entry.name, score};
            }
        }
    }
    return matches;
}
