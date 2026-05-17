#pragma once
#include <string>

struct ValidationResult {
    bool        valid;
    std::string reason;
};

// ─────────────────────────────────────────────
//  VideoFile
//  Stores metadata and validates video quality.
// ─────────────────────────────────────────────
class VideoFile {
private:
    std::string filePath;       // private – no outside code touches raw path

protected:
    bool isValidated;           // protected – subclasses may inspect

    // Protected helper – subclasses could override for different rules
    bool validateResolution() const;

public:
    int         fileId;
    std::string filename;
    double      sizeMB;
    std::string resolution;     // e.g. "720p", "1080p", "4K"

    VideoFile(int fileId, const std::string& filename,
              const std::string& filePath,
              double sizeMB, const std::string& resolution);

    // Public interface
    ValidationResult validate();
    std::string      getFilePath() const;   // controlled read-only access
    bool             getIsValidated() const;

    void print() const;
};
