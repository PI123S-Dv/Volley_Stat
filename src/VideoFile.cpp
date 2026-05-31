#include "VideoFile.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>

static const double MIN_SIZE_MB = 10.0;
static const std::vector<std::string> VALID_RESOLUTIONS = {"720p","1080p","1440p","4K"};

VideoFile::VideoFile(int id, const std::string& fname, const std::string& fpath,
                     double size, const std::string& res)
    : filePath(fpath), isValidated(false),
      fileId(id), filename(fname), sizeMB(size), resolution(res) {}

bool VideoFile::validateResolution() const {
    for (const auto& r : VALID_RESOLUTIONS)
        if (r == resolution) return true;
    return false;
}

ValidationResult VideoFile::validate() {
    if (sizeMB < MIN_SIZE_MB) {
        return {false, "File too small (" + std::to_string(sizeMB) + " MB). Minimum is 10 MB."};
    }
    if (!validateResolution()) {
        return {false, "Resolution '" + resolution + "' not supported. Need 720p or higher."};
    }
    isValidated = true;
    return {true, "OK"};
}

std::string VideoFile::getFilePath()    const { return filePath; }
bool        VideoFile::getIsValidated() const { return isValidated; }

void VideoFile::print() const {
    std::cout << "VideoFile | ID: " << fileId
              << " | Name: " << filename
              << " | Size: " << sizeMB << " MB"
              << " | Resolution: " << resolution
              << " | Validated: " << (isValidated ? "Yes" : "No") << "\n";
}