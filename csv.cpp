/**
 * @file csv.cpp
 * @brief CSV/TSV export functionality implementation for eTree
 *
 * This file implements the TSV (Tab-Separated Values) export functionality,
 * which writes directory tree data to a file that can be opened in Excel
 * or other spreadsheet applications. The output uses UTF-8 encoding with
 * BOM for maximum compatibility.
 */

#include "csv.h"
#include <fstream>
#include <iostream>

/**
 * @brief Export tree statistics to a TSV (Tab-Separated Values) file
 *
 * This function creates a TSV file with the following structure:
 * - First line: UTF-8 BOM (\xEF\xBB\xBF) for proper encoding detection
 * - Second line: Header row with column names
 * - Remaining lines: One row per file/folder with tab-separated values
 *
 * Columns in the output:
 * 1. Relative Path - Path from root directory
 * 2. Name - Filename or directory name
 * 3. Type - "file" or "folder"
 * 4. Size (bytes) - File size in bytes (0 for folders)
 * 5. Created - Creation timestamp (YYYY-MM-DD HH:MM:SS)
 * 6. Modified - Last modification timestamp (YYYY-MM-DD HH:MM:SS)
 * 7. Permissions - Permission string (platform-specific format)
 *
 * The TSV format uses tabs (\t) as delimiters, making it compatible with
 * Excel and other spreadsheet applications. UTF-8 with BOM ensures that
 * Unicode characters (including RTL text) display correctly.
 *
 * @param filename Path to the output TSV file to create
 * @param stats TreeStats structure containing the collected directory data
 */
void writeTsv(const std::string &filename, const TreeStats &stats) {
  // Open file in binary mode to have full control over line endings and
  // encoding
  std::ofstream out(filename, std::ios::out | std::ios::binary);

  // Check if file was opened successfully
  if (!out.is_open()) {
    std::cerr << "Error: Could not create file " << filename << std::endl;
    return;
  }

  // Write UTF-8 BOM (Byte Order Mark) to signal UTF-8 encoding
  // This helps Excel and other applications detect the encoding correctly
  out << "\xEF\xBB\xBF";

  // Write header row with column names (tab-separated)
  out << "Relative Path\tName\tType\tSize "
         "(bytes)\tCreated\tModified\tPermissions\n";

  // Write data rows - one row per file/folder
  for (const auto &row : stats.csvRows) {
    out << row.relpath << '\t'  // Relative path from root
        << row.name << '\t'     // Filename or directory name
        << row.type << '\t'     // "file" or "folder"
        << row.bytes << '\t'    // Size in bytes (0 for folders)
        << row.created << '\t'  // Creation timestamp
        << row.modified << '\t' // Modification timestamp
        << row.perms << '\n';   // Permissions string
  }

  // Close the file
  out.close();

  // Check if any write errors occurred during the process
  if (out.fail()) {
    std::cerr << "Error: Could not write to file " << filename << std::endl;
  }
}