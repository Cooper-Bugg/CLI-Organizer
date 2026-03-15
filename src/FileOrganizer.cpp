#include "FileOrganizer.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <system_error>
#include <iomanip> // For formatted output (GB/MB)

// =================
// Public Interface
// =================

void FileOrganizer::organize(const fs::path& sourceRoot, const fs::path& destinationRoot, bool dryRun) {
    auto categoryMap = buildCategoryMap();
    std::cout << "Scanning files...\n";
    auto files = scanFiles(sourceRoot);
    size_t totalFiles = files.size();

    std::cout << "Found " << totalFiles << " files. Starting transfer...\n";
    if (dryRun) std::cout << "--- DRY RUN MODE ---\n";

    size_t processedCount = 0;
    int lastPrintedPercent = -1;

        for (const auto& file : files) {
            std::string category = classifyFile(file, categoryMap);
            fs::path categoryDir = destinationRoot / category;
            fs::path destPath = resolveDestinationPath(file, categoryDir, sourceRoot);

            if (dryRun) {
                executeCopy(file, destPath, true);
            } else {
                if (!destPath.empty()) {
                    std::error_code ec;
                    fs::create_directories(destPath.parent_path(), ec);
                    fs::copy_file(file, destPath, fs::copy_options::none, ec);
                }
            }

            // Progress Bar Logic (Updates every 1% to keep console responsive)
            processedCount++;
            int currentPercent = (processedCount * 100) / totalFiles;
            if (currentPercent > lastPrintedPercent) {
                std::cout << "\rProgress: [" << currentPercent << "%] ("
                          << processedCount << "/" << totalFiles << ")" << std::flush;
                lastPrintedPercent = currentPercent;
            }
        }
        std::cout << "\nOperation Complete.\n";
}

void FileOrganizer::printDriveInfo(const fs::path& targetPath) {
    std::error_code ec;
    // 1. Get Space Info for the specific target path
    fs::space_info si = fs::space(targetPath, ec);
    
    if (ec) {
        std::cerr << "[ERROR] Could not query drive space: " << ec.message() << "\n";
        return;
    }

    const double gb = 1024 * 1024 * 1024;
    std::cout << "--- Drive Info for: " << targetPath << " ---\n";
    std::cout << "Capacity:  " << std::fixed << std::setprecision(2) << (si.capacity / gb) << " GB\n";
    std::cout << "Free:      " << (si.free / gb) << " GB\n";
    std::cout << "Available: " << (si.available / gb) << " GB\n";

    // 2. Count Mounted Drives (macOS Specific Logic)
    int driveCount = 0;
    fs::path volumesPath = "/Volumes";
    
    if (fs::exists(volumesPath) && fs::is_directory(volumesPath)) {
        for (const auto& entry : fs::directory_iterator(volumesPath)) {
            driveCount++;
        }
        std::cout << "Total Mounted Volumes (System + External): " << driveCount << "\n";
    } else {
        std::cout << "Could not scan /Volumes (Non-macOS system?)\n";
    }
}

// ======================
// Private Helper Methods
// ======================

std::vector<fs::path> FileOrganizer::scanFiles(const fs::path& sourceRoot) {
    std::vector<fs::path> files;
    std::error_code ec;
    auto options = fs::directory_options::skip_permission_denied;

    auto it = fs::recursive_directory_iterator(sourceRoot, options, ec);
    if (ec) {
        std::cerr << "[WARNING] Could not access source root: " << ec.message() << "\n";
        return files;
    }

    auto end = fs::recursive_directory_iterator();

    while (it != end) {
        try {
            const auto& entry = *it;
            std::string name = entry.path().filename().string();

            // If it's a hidden folder (starts with dot), DO NOT enter it.
            // This prevents the "Operation not permitted" crash on .TemporaryItems / .Trashes
            if (!name.empty() && name[0] == '.') {
                if (fs::is_directory(entry.status())) {
                    it.disable_recursion_pending();
                }
            }

            // Only add visible files to our list (skips ._.Trashes junk too)
            if (!name.empty() && name[0] != '.' && fs::is_regular_file(entry.symlink_status())) {
                files.push_back(entry.path());
            }

            // Attempt to move to the next file
            it.increment(ec);
            if (ec) {
                // If we specifically hit a lock, log it but TRY to continue if possible.
                // However, usually an increment error kills the iterator. 
                // The fix above (disable_recursion) should prevent us from ever reaching this line.
                std::cerr << "[SKIP] Locked item encountered, stopping scan of this branch.\n";
                break; 
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[SKIP] Error reading entry: " << e.what() << "\n";
            it.disable_recursion_pending();
            it.increment(ec);
            if (ec) break;
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

std::map<std::string, std::string> FileOrganizer::buildCategoryMap() {
    return {
        {".jpg", "Images"}, {".jpeg", "Images"}, {".png", "Images"}, {".gif", "Images"},
        {".mp4", "Video"}, {".mov", "Video"}, {".avi", "Video"},
        {".pdf", "Documents"}, {".docx", "Documents"}, {".txt", "Documents"},
        {".zip", "Archives"}, {".tar", "Archives"}
    };
        return {
            {".jpg", "Images"}, {".jpeg", "Images"}, {".png", "Images"}, {".gif", "Images"}, {".bmp", "Images"}, {".tiff", "Images"},
            {".mp4", "Video"}, {".mov", "Video"}, {".avi", "Video"}, {".mkv", "Video"}, {".wmv", "Video"},
            {".mp3", "Audio"}, {".wav", "Audio"}, {".flac", "Audio"},
            {".pdf", "Documents"}, {".docx", "Documents"}, {".doc", "Documents"}, {".txt", "Documents"}, {".rtf", "Documents"},
            {".xlsx", "Documents"}, {".xls", "Documents"}, {".csv", "Documents"}, {".pptx", "Documents"}, {".ppt", "Documents"},
            {".zip", "Archives"}, {".tar", "Archives"}, {".gz", "Archives"}, {".rar", "Archives"},
            {".html", "Code"}, {".css", "Code"}, {".js", "Code"}, {".ts", "Code"}, {".py", "Code"}, {".cpp", "Code"}, {".c", "Code"}, {".h", "Code"}, {".java", "Code"}, {".cs", "Code"}, {".json", "Code"}, {".xml", "Code"}, {".sh", "Code"}, {".bat", "Code"}
        };

std::string FileOrganizer::classifyFile(const fs::path& filePath,
                                        const std::map<std::string, std::string>& categoryMap) {
    std::string ext = toLowerAscii(filePath.extension().string());
    
    auto it = categoryMap.find(ext);
    if (it != categoryMap.end()) {
        return it->second;
    }
    return "Misc";
}

fs::path FileOrganizer::resolveDestinationPath(const fs::path& sourcePath, 
                                               const fs::path& categoryDir, 
                                               const fs::path& sourceRoot) {
    // Calculate the structure relative to the source drive
    fs::path relativePath = fs::relative(sourcePath, sourceRoot);
    // Construct the full destination path
    fs::path candidate = categoryDir / relativePath;

    fs::path parentFolder = candidate.parent_path();
    fs::path fileName = candidate.filename();
    fs::path stem = fileName.stem();
    fs::path ext = fileName.extension();

    int counter = 1;
    while (fs::exists(candidate)) {
        if (areFilesIdentical(sourcePath, candidate)) {
            return ""; 
        }
        // Add suffix to filename, keep in same subfolder
        std::string newName = stem.string() + " (" + std::to_string(counter) + ")" + ext.string();
        candidate = parentFolder / newName;
        counter++;
    }

    return candidate;
}

void FileOrganizer::executeCopy(const fs::path& sourcePath, const fs::path& destinationPath, bool dryRun) {
    if (destinationPath.empty()) {
        return; 
    }

    std::cout << (dryRun ? "[DRY-RUN] " : "[COPY] ") 
              << sourcePath.filename() << " -> " << destinationPath << "\n";

    if (!dryRun) {
        std::error_code ec;
        fs::create_directories(destinationPath.parent_path(), ec);
        if (ec) {
            std::cerr << "Error creating directory: " << ec.message() << "\n";
            return;
        }

        fs::copy_file(sourcePath, destinationPath, fs::copy_options::none, ec);
        if (ec) {
            std::cerr << "Error copying file: " << ec.message() << "\n";
        }
    }
}

std::string FileOrganizer::toLowerAscii(std::string data) {
    std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return data;
}

bool FileOrganizer::areFilesIdentical(const fs::path& p1, const fs::path& p2) {
    std::error_code ec;
    uintmax_t s1 = fs::file_size(p1, ec);
    uintmax_t s2 = fs::file_size(p2, ec);

    if (ec || s1 != s2) return false;

    // Small file check
    if (s1 < 8192) {
        std::ifstream f1(p1, std::ios::binary);
        std::ifstream f2(p2, std::ios::binary);
        return std::equal(std::istreambuf_iterator<char>(f1),
                          std::istreambuf_iterator<char>(),
                          std::istreambuf_iterator<char>(f2));
    }

    // Tiered check (Header/Footer)
    auto checkChunk = [&](uintmax_t offset) -> bool {
        char buf1[4096];
        char buf2[4096];
        std::ifstream f1(p1, std::ios::binary);
        std::ifstream f2(p2, std::ios::binary);
        
        f1.seekg(offset);
        f2.seekg(offset);
        f1.read(buf1, sizeof(buf1));
        f2.read(buf2, sizeof(buf2));
        
        return std::equal(buf1, buf1 + f1.gcount(), buf2);
    };

    if (!checkChunk(0)) return false;
    if (!checkChunk(s1 - 4096)) return false;

    // Full fallback
    std::ifstream f1(p1, std::ios::binary);
    std::ifstream f2(p2, std::ios::binary);
    return std::equal(std::istreambuf_iterator<char>(f1),
                      std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(f2));
}