#ifndef FILE_ORGANIZER_H
#define FILE_ORGANIZER_H

#include <filesystem>
#include <map>
#include <vector>
#include <string>

namespace fs = std::filesystem;

class FileOrganizer {
public:
    /**
     * Entry point for the organizer pipeline.
     * dryRun: If true, logs actions but performs no filesystem mutations.
     */
    void organize(const fs::path& sourceRoot,
                  const fs::path& destinationRoot,
                  bool dryRun);

    // Prints storage capacity and count of mounted volumes (Mac specific)
    void printDriveInfo(const fs::path& targetPath);

private:
    std::vector<fs::path> scanFiles(const fs::path& sourceRoot);
    std::map<std::string, std::string> buildCategoryMap();
    std::string classifyFile(const fs::path& filePath,
                             const std::map<std::string, std::string>& categoryMap);
    fs::path resolveDestinationPath(const fs::path& sourcePath,
                                    const fs::path& destinationDir,
                                    const fs::path& sourceRoot);
    void executeCopy(const fs::path& sourcePath,
                     const fs::path& destinationPath,
                     bool dryRun);
    std::string toLowerAscii(std::string data);
    bool areFilesIdentical(const fs::path& p1, const fs::path& p2);
};

#endif // FILE_ORGANIZER_H