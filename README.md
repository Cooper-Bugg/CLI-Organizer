# CLI File Organizer

A robust, safety-first C++ CLI tool that organizes files into category-based directories (Images, Video, Documents, etc.). Designed with strict non-destructive invariants to prevent data loss during migration.

## Features

- **Non-Destructive by Default**: Uses `std::filesystem::copy_file` with `copy_options::none`. It never overwrites existing files.
- **Collision Resolution**: Automatically handles duplicate filenames by verifying content identity before creating numbered suffixes (e.g., `file (1).jpg`).
- **Tiered Content Comparison**: 
  - Optimized for older hardware/HDDs.
  - Checks File Size -> Header/Footer Sampling (4KB) -> Full Byte Comparison.
  - Prevents redundant copies of identical files.
- **Dry Run Mode**: Default execution mode logs planned actions without mutating the filesystem.
- **Recursive Scan**: Safely navigates directory trees, skipping permission errors and symlinks to avoid cycles.

## Project Structure

```text
.
├── src/
│   ├── main.cpp            # Entry point and arg parsing
│   ├── FileOrganizer.cpp   # Core implementation
│   └── FileOrganizer.h     # Class definition
└── README.md
```
