/**
 * @file etree.cpp
 * @brief Core directory tree traversal and display implementation for eTree
 *
 * This file implements the main tree traversal logic, including:
 * - Recursive directory traversal with depth limiting
 * - File filtering (hidden files, exclude patterns, directories-only)
 * - RTL (Right-to-Left) text handling for Arabic/Hebrew filenames
 * - Colored console output with ANSI escape codes
 * - File metadata retrieval (size, permissions, timestamps)
 * - CSV data collection for export
 * - Platform-specific implementations for Windows and Unix/Linux
 *
 * The implementation is split into Windows (_WIN32) and Unix/Linux sections
 * to handle platform-specific differences in:
 * - Character encoding (UTF-16 vs UTF-8)
 * - File permissions (Windows attributes vs Unix modes)
 * - Hidden file detection
 * - Console detection
 */

#include "etree.h"
#include "args.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <locale>
#include <regex>
#include <sstream>


#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>

// Undefine Windows macros that conflict with std::max/min
#undef max
#undef min

// ANSI color escape sequences for Windows console output
const wchar_t *dircolor = L"\033[1;34m";  // Bold blue for directories
const wchar_t *filecolor = L"\033[0;32m"; // Green for files
const wchar_t *permcolor = L"\033[0;36m"; // Cyan for permissions
const wchar_t *sizecolor = L"\033[0;33m"; // Yellow for file sizes
const wchar_t *resetcolor = L"\033[0m";   // Reset to default color

#else
#include <unistd.h>

// ANSI color escape sequences for Unix/Linux console output
const char *dircolor = "\033[1;34m";  // Bold blue for directories
const char *filecolor = "\033[0;32m"; // Green for files
const char *permcolor = "\033[0;36m"; // Cyan for permissions
const char *sizecolor = "\033[0;33m"; // Yellow for file sizes
const char *resetcolor = "\033[0m";   // Reset to default color
#endif

// Filesystem namespace alias for convenience
namespace fs = std::filesystem;

//=============================================================================
// Windows-specific helper functions
//=============================================================================
#ifdef _WIN32

/**
 * @brief Detect if running in Windows Terminal
 *
 * Windows Terminal sets environment variables that we can check to determine
 * if we're running in a modern terminal that supports ANSI colors.
 *
 * @return true if Windows Terminal is detected, false otherwise
 */
bool is_windows_terminal() {
  char *wt = nullptr;
  char *term = nullptr;

  // Check for Windows Terminal environment variables
  _dupenv_s(&wt, nullptr, "WT_SESSION");
  _dupenv_s(&term, nullptr, "TERM_PROGRAM");

  bool present = (wt && wt[0]) || (term && term[0]);

  // Free allocated memory
  if (wt)
    free(wt);
  if (term)
    free(term);

  return present;
}

/**
 * @brief Format an integer with locale-specific thousand separators
 *
 * Uses the system locale to insert commas (or other separators) for
 * readability. Example: 1234567 becomes "1,234,567"
 *
 * @param value The integer value to format
 * @return Formatted wide string with thousand separators
 */
std::wstring formatIntWithCommas(uintmax_t value) {
  std::wostringstream oss;
  oss.imbue(std::locale("")); // Use system locale for formatting
  oss << value;
  return oss.str();
}

/**
 * @brief Format a byte count as a human-readable size string
 *
 * Appends " B" suffix to the formatted number.
 * Example: 1234567 becomes "1,234,567 B"
 *
 * @param bytes Number of bytes
 * @return Formatted wide string with " B" suffix
 */
std::wstring formatSizeBytes(uintmax_t bytes) {
  return formatIntWithCommas(bytes) + L" B";
}

/**
 * @brief Check if a wide character is an RTL (Right-to-Left) character
 *
 * Detects Arabic, Hebrew, and other RTL script characters by checking
 * Unicode code point ranges:
 * - 0x0590-0x08FF: Hebrew, Arabic, Syriac, Thaana, NKo, Samaritan
 * - 0xFB50-0xFDFF: Arabic Presentation Forms-A
 * - 0xFE70-0xFEFF: Arabic Presentation Forms-B
 *
 * @param c Wide character to check
 * @return true if character is RTL, false otherwise
 */
bool is_arabic(wchar_t c) {
  return (c >= 0x0590 && c <= 0x08FF) || (c >= 0xFB50 && c <= 0xFDFF) ||
         (c >= 0xFE70 && c <= 0xFEFF);
}

/**
 * @brief Check if a wide string contains any RTL characters
 *
 * @param s Wide string to check
 * @return true if any RTL characters are found, false otherwise
 */
bool contains_rtl(const std::wstring &s) {
  for (wchar_t c : s) {
    if (is_arabic(c))
      return true;
  }
  return false;
}

/**
 * @brief Wrap RTL text for proper console display
 *
 * Windows console doesn't handle bidirectional text correctly, so we need
 * to manually reverse RTL character sequences. This function:
 * 1. Identifies RTL character sequences
 * 2. Reverses them while preserving LTR text
 * 3. Handles trailing spaces correctly
 *
 * Example: "Hello العالم World" becomes "Hello ملاعلا World"
 * (Arabic text is reversed, English text stays the same)
 *
 * Note: This is only needed for console output. File output should NOT
 * use this function, as text editors handle bidirectional text correctly.
 *
 * @param s Wide string potentially containing RTL text
 * @return Wrapped string with RTL sequences reversed
 */
std::wstring wrap_rtl(std::wstring s) {
  std::wstring res;         // Result string
  std::wstring buf;         // Buffer for current sequence
  bool buf_has_rtl = false; // Track if buffer contains RTL characters

  // Process each character
  for (wchar_t c : s) {
    if (is_arabic(c)) {
      // RTL character: add to buffer
      buf += c;
      buf_has_rtl = true;
    } else if (c == L' ') {
      // Space: add to buffer (could be part of RTL or LTR sequence)
      buf += c;
    } else {
      // LTR or other neutral character: flush buffer
      if (buf_has_rtl) {
        // Buffer contains RTL text: extract trailing spaces, reverse, append
        std::wstring trailing_spaces;
        while (!buf.empty() && buf.back() == L' ') {
          trailing_spaces += L' ';
          buf.pop_back();
        }
        std::reverse(buf.begin(), buf.end()); // Reverse RTL sequence
        res += buf;
        res += trailing_spaces;
      } else {
        // Buffer contains only LTR text: append as-is
        res += buf;
      }

      // Reset buffer and append current LTR character
      buf.clear();
      buf_has_rtl = false;
      res += c;
    }
  }

  // Flush remaining buffer at end of string
  if (buf_has_rtl) {
    std::wstring trailing_spaces;
    while (!buf.empty() && buf.back() == L' ') {
      trailing_spaces += L' ';
      buf.pop_back();
    }
    std::reverse(buf.begin(), buf.end());
    res += buf;
    res += trailing_spaces;
  } else {
    res += buf;
  }

  return res;
}

/**
 * @brief Convert wide string (UTF-16) to UTF-8 string
 *
 * Uses Windows API WideCharToMultiByte to convert UTF-16 (used internally
 * by Windows) to UTF-8 (used for file output and cross-platform compatibility).
 *
 * @param w Wide string to convert
 * @return UTF-8 encoded narrow string
 */
std::string wstring_to_utf8(const std::wstring &w) {
  if (w.empty())
    return std::string();

  // Calculate required buffer size
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                        NULL, 0, NULL, NULL);

  // Allocate buffer and perform conversion
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &strTo[0],
                      size_needed, NULL, NULL);

  return strTo;
}

#else
//=============================================================================
// Unix/Linux-specific helper functions
//=============================================================================

/**
 * @brief Format an integer with locale-specific thousand separators (Unix
 * version)
 *
 * @param value The integer value to format
 * @return Formatted string with thousand separators
 */
std::string formatIntWithCommas(uintmax_t value) {
  std::ostringstream oss;
  oss.imbue(std::locale(""));
  oss << value;
  return oss.str();
}

/**
 * @brief Format a byte count as a human-readable size string (Unix version)
 *
 * @param bytes Number of bytes
 * @return Formatted string with " B" suffix
 */
std::string formatSizeBytes(uintmax_t bytes) {
  return formatIntWithCommas(bytes) + " B";
}

#endif

//=============================================================================
// Cross-platform helper functions
//=============================================================================

/**
 * @brief Check if stdout is connected to a console
 *
 * Determines whether output is going to a console (terminal) or being
 * redirected to a file or piped to another program. This affects:
 * - Whether to use colored output
 * - Whether to use UTF-16 (console) or UTF-8 (file) on Windows
 * - Whether to apply RTL wrapping
 *
 * @return true if stdout is a console, false if redirected/piped
 */
bool is_console() {
#ifdef _WIN32
  // Windows: Check if stdout is a character device and has console mode
  DWORD mode;
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  return GetFileType(hOut) == FILE_TYPE_CHAR && GetConsoleMode(hOut, &mode);
#else
  // Unix/Linux: Use isatty() to check if stdout is a terminal
  return isatty(fileno(stdout));
#endif
}

/**
 * @brief Determine if colored output should be enabled
 *
 * Colors are enabled only if:
 * 1. User hasn't disabled colors with --no-colors flag, AND
 * 2. Output is going to a console (not redirected to file)
 *
 * @param nocolors User's --no-colors flag
 * @return true if colors should be enabled, false otherwise
 */
bool enable_colors(bool nocolors) { return !nocolors && is_console(); }

/**
 * @brief Get permission string for a file or directory
 *
 * Platform-specific implementation:
 * - Windows: Returns attribute flags (R=ReadOnly, H=Hidden, S=System,
 * A=Archive)
 * - Unix/Linux: Returns Unix-style permissions (e.g., "rwxr-xr-x")
 *
 * @param entry Directory entry to get permissions for
 * @return Permission string
 */
std::string getPermissions(const fs::directory_entry &entry) {
  std::string perms;

#ifdef _WIN32
  // Windows: Get file attributes
  DWORD attr = GetFileAttributesW(entry.path().c_str());
  if (attr == INVALID_FILE_ATTRIBUTES)
    return "-";

  // Build permission string from attribute flags
  if (attr & FILE_ATTRIBUTE_READONLY)
    perms += 'R'; // Read-only
  if (attr & FILE_ATTRIBUTE_HIDDEN)
    perms += 'H'; // Hidden
  if (attr & FILE_ATTRIBUTE_SYSTEM)
    perms += 'S'; // System
  if (attr & FILE_ATTRIBUTE_ARCHIVE)
    perms += 'A'; // Archive

#else
  // Unix/Linux: Get file permissions
  fs::perms p = entry.status().permissions();

  // Build Unix-style permission string (rwxrwxrwx)
  // Owner permissions
  perms += ((p & fs::perms::owner_read) != fs::perms::none) ? 'r' : '-';
  perms += ((p & fs::perms::owner_write) != fs::perms::none) ? 'w' : '-';
  perms += ((p & fs::perms::owner_exec) != fs::perms::none) ? 'x' : '-';
  // Group permissions
  perms += ((p & fs::perms::group_read) != fs::perms::none) ? 'r' : '-';
  perms += ((p & fs::perms::group_write) != fs::perms::none) ? 'w' : '-';
  perms += ((p & fs::perms::group_exec) != fs::perms::none) ? 'x' : '-';
  // Others permissions
  perms += ((p & fs::perms::others_read) != fs::perms::none) ? 'r' : '-';
  perms += ((p & fs::perms::others_write) != fs::perms::none) ? 'w' : '-';
  perms += ((p & fs::perms::others_exec) != fs::perms::none) ? 'x' : '-';
#endif

  return perms.empty() ? "-" : perms;
}

//=============================================================================
// Pattern matching for exclude functionality
//=============================================================================

#ifdef _WIN32
/**
 * @brief Check if a filename matches a wildcard pattern (Windows version)
 *
 * Converts wildcard pattern to regex and performs case-insensitive matching.
 * Supports * (any characters) and ? (single character) wildcards.
 *
 * Example: "*.tmp" matches "file.tmp", "data.tmp", etc.
 *
 * @param name Wide string filename to check
 * @param pattern Wildcard pattern (narrow string)
 * @return true if filename matches pattern, false otherwise
 */
bool matchesPatternW(const std::wstring &name, const std::string &pattern) {
  if (pattern.empty())
    return false;

  // Convert wide string filename to UTF-8 for regex matching
  std::string narrow = wstring_to_utf8(name);

  // Convert wildcard pattern to regex pattern
  std::string regexPattern =
      std::regex_replace(pattern, std::regex("\\."), "\\."); // Escape dots
  regexPattern =
      std::regex_replace(regexPattern, std::regex("\\?"), "."); // ? -> .
  regexPattern =
      std::regex_replace(regexPattern, std::regex("\\*"), ".*"); // * -> .*

  // Create regex with case-insensitive matching
  std::regex re("^" + regexPattern + "$", std::regex_constants::icase);

  return std::regex_match(narrow, re);
}

#else
/**
 * @brief Check if a filename matches a wildcard pattern (Unix version)
 *
 * @param name Filename to check
 * @param pattern Wildcard pattern
 * @return true if filename matches pattern, false otherwise
 */
bool matchesPattern(const std::string &name, const std::string &pattern) {
  if (pattern.empty())
    return false;

  // Convert wildcard pattern to regex pattern
  std::string regexPattern =
      std::regex_replace(pattern, std::regex("\\."), "\\.");
  regexPattern = std::regex_replace(regexPattern, std::regex("\\?"), ".");
  regexPattern = std::regex_replace(regexPattern, std::regex("\\*"), ".*");

  std::regex re("^" + regexPattern + "$", std::regex_constants::icase);

  return std::regex_match(name, re);
}
#endif

//=============================================================================
// File timestamp retrieval
//=============================================================================

#ifdef _WIN32
/**
 * @brief Get file creation and modification timestamps (Windows version)
 *
 * Uses Windows API to retrieve file times and convert them to local time
 * in YYYY-MM-DD HH:MM:SS format.
 *
 * @param path File path to get timestamps for
 * @return Pair of strings: (creation time, modification time)
 */
std::pair<std::string, std::string> get_file_times(const fs::path &path) {
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;

  // Get file attributes including timestamps
  if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
    // Lambda to convert FILETIME to formatted string
    auto fileTimeToString = [](const FILETIME &ft) -> std::string {
      SYSTEMTIME stUTC, stLocal;

      // Convert FILETIME to SYSTEMTIME (UTC)
      FileTimeToSystemTime(&ft, &stUTC);

      // Convert UTC to local time
      SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

      // Format as YYYY-MM-DD HH:MM:SS
      char buf[64];
      sprintf_s(buf, "%04d-%02d-%02d %02d:%02d:%02d", stLocal.wYear,
                stLocal.wMonth, stLocal.wDay, stLocal.wHour, stLocal.wMinute,
                stLocal.wSecond);

      return std::string(buf);
    };

    // Return creation and modification times
    return {fileTimeToString(fileInfo.ftCreationTime),
            fileTimeToString(fileInfo.ftLastWriteTime)};
  }

  // Return empty strings if file attributes couldn't be retrieved
  return {"", ""};
}

#else
/**
 * @brief Get file creation and modification timestamps (Unix version)
 *
 * Note: Unix doesn't reliably support creation time, so this returns empty
 * strings. Could be extended to use stat() for modification time if needed.
 *
 * @param path File path to get timestamps for
 * @return Pair of empty strings (creation time not available on Unix)
 */
std::pair<std::string, std::string> get_file_times(const fs::path &path) {
  return {"", ""}; // Unix doesn't have reliable creation time
}
#endif

//=============================================================================
// Main tree traversal and display function
//=============================================================================

#ifdef _WIN32
/**
 * @brief Recursively print directory tree (Windows version)
 *
 * This is the main workhorse function that:
 * 1. Reads directory contents
 * 2. Filters entries based on user options (hidden, exclude pattern, dirs-only)
 * 3. Sorts entries alphabetically
 * 4. Displays each entry with tree characters, colors, and metadata
 * 5. Collects data for CSV export if requested
 * 6. Recursively processes subdirectories
 *
 * @param dir Current directory path to traverse
 * @param args Command-line arguments and options
 * @param level Current depth level (1 = root)
 * @param prefix Wide string prefix for tree drawing characters
 * @param isLast Whether this directory is the last entry in its parent
 * @param stats Reference to TreeStats for accumulating data
 * @param relpath Relative path from root directory (for CSV export)
 */
void printTree(const fs::path &dir, const Args &args, int level,
               std::wstring prefix, bool isLast, TreeStats &stats,
               std::wstring relpath) {

  // Check depth limit
  if (args.maxLevel > 0 && level > args.maxLevel)
    return;

  // Collect directory entries
  std::vector<fs::directory_entry> entries;
  try {
    // Iterate through directory, skipping entries we don't have permission to
    // read
    for (const auto &entry : fs::directory_iterator(
             dir, fs::directory_options::skip_permission_denied)) {
      std::wstring name = entry.path().filename().wstring();

      // Filter hidden files if not showing hidden
      if (!args.showHidden) {
        DWORD attr = GetFileAttributesW(entry.path().c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_HIDDEN))
          continue; // Skip hidden files
        if (!name.empty() && name[0] == L'.')
          continue; // Skip dot files
      }

      // Filter by exclude pattern
      if (!args.excludePattern.empty() &&
          matchesPatternW(name, args.excludePattern))
        continue;

      // Filter to directories only if requested
      if (args.showDirsOnly && !entry.is_directory())
        continue;

      entries.push_back(entry);
    }
  } catch (const std::exception &ex) {
    // Handle permission denied or other errors
    std::wcerr << L"[etree] Failed to enumerate directory '" << dir.wstring()
               << L"': " << ex.what() << std::endl;
    return;
  }

  // Sort entries alphabetically by filename
  std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
    return a.path().filename().wstring() < b.path().filename().wstring();
  });

  // Process each entry
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto &entry = entries[i];
    bool isDir = entry.is_directory();
    bool entryIsLast = (i + 1 == entries.size());

    // Choose tree branch character: `-- for last entry, |-- for others
    std::wstring branch = entryIsLast ? L"`-- " : L"|-- ";

    // Choose color based on entry type (directory or file)
    const wchar_t *color =
        (enable_colors(args.nocolors) ? (isDir ? dircolor : filecolor) : L"");
    const wchar_t *reset = (enable_colors(args.nocolors) ? resetcolor : L"");

    std::wstring displayname = entry.path().filename().wstring();

    // Display entry name (unless doing CSV export)
    if (args.csvOut.empty()) {
      if (is_console()) {
        // Console output: Use wide characters, apply RTL wrapping if needed
        std::wstring consoleName = displayname;
        if (contains_rtl(consoleName))
          consoleName = wrap_rtl(consoleName);
        std::wcout << prefix << color << branch << consoleName << reset;
      } else {
        // File output: Convert to UTF-8
        std::cout << wstring_to_utf8(prefix)
                  << wstring_to_utf8(std::wstring(color))
                  << wstring_to_utf8(branch) << wstring_to_utf8(displayname)
                  << wstring_to_utf8(std::wstring(reset));
      }
    }

    // Collect CSV data if export requested
    if (!args.csvOut.empty()) {
      // Build relative path for this entry
      fs::path relp = relpath.empty()
                          ? entry.path().filename()
                          : (fs::path(relpath) / entry.path().filename());

      // Create CSV row
      CsvRow row;
      row.relpath = wstring_to_utf8(relp.wstring());
      row.name = wstring_to_utf8(entry.path().filename().wstring());
      row.type = isDir ? "folder" : "file";

      // Get file size (0 for directories)
      try {
        row.bytes = isDir ? 0 : entry.file_size();
      } catch (...) {
        row.bytes = 0; // If we can't get size, use 0
      }

      // Get permissions and timestamps
      row.perms = getPermissions(entry);
      auto times = get_file_times(entry.path());
      row.created = times.first;
      row.modified = times.second;

      stats.csvRows.push_back(row);
    }

    // Display additional metadata if requested (unless doing CSV export)
    if (args.csvOut.empty()) {
      // Show file size if requested
      if (args.showSize) {
        try {
          uintmax_t size = isDir ? 0 : entry.file_size();
          if (is_console()) {
            std::wcout << (enable_colors(args.nocolors) ? sizecolor : L"")
                       << L" [" << formatSizeBytes(size) << L"]"
                       << (enable_colors(args.nocolors) ? resetcolor : L"");
          } else {
            std::cout << wstring_to_utf8(
                             enable_colors(args.nocolors) ? sizecolor : L"")
                      << " [" << wstring_to_utf8(formatSizeBytes(size)) << "]"
                      << wstring_to_utf8(
                             enable_colors(args.nocolors) ? resetcolor : L"");
          }
        } catch (...) {
          // If we can't get size, show 0 B
          if (is_console()) {
            std::wcout << (enable_colors(args.nocolors) ? sizecolor : L"")
                       << L" [0 B]"
                       << (enable_colors(args.nocolors) ? resetcolor : L"");
          } else {
            std::cout << wstring_to_utf8(
                             enable_colors(args.nocolors) ? sizecolor : L"")
                      << " [0 B]"
                      << wstring_to_utf8(
                             enable_colors(args.nocolors) ? resetcolor : L"");
          }
        }
      }

      // Show permissions if requested
      if (args.showPerms) {
        std::string perms = getPermissions(entry);
        std::wstring perms_w(perms.begin(), perms.end());
        if (is_console()) {
          std::wcout << (enable_colors(args.nocolors) ? permcolor : L"")
                     << L" (" << perms_w << L")"
                     << (enable_colors(args.nocolors) ? resetcolor : L"");
        } else {
          std::cout << wstring_to_utf8(enable_colors(args.nocolors) ? permcolor
                                                                    : L"")
                    << " (" << perms << ")"
                    << wstring_to_utf8(enable_colors(args.nocolors) ? resetcolor
                                                                    : L"");
        }
      }

      // End line
      if (is_console())
        std::wcout << std::endl;
      else
        std::cout << std::endl;
    }

    // Recursively process subdirectories
    if (isDir) {
      stats.folders++;

      // Build new prefix for child entries
      // If this is the last entry, use spaces; otherwise use vertical bar
      std::wstring newPrefix = prefix + (entryIsLast ? L"    " : L"|   ");

      // Build new relative path for CSV export
      std::wstring newRelpath =
          relpath.empty()
              ? std::wstring(entry.path().filename())
              : (relpath + L"/" + entry.path().filename().wstring());

      // Recursive call
      printTree(entry.path(), args, level + 1, newPrefix, entryIsLast, stats,
                newRelpath);
    } else {
      stats.files++;
    }
  }

  // Update maximum depth reached
  stats.maxDepth = std::max(stats.maxDepth, level);
}

#else
//=============================================================================
// Unix/Linux version of printTree (simpler, no RTL handling needed)
//=============================================================================

/**
 * @brief Recursively print directory tree (Unix/Linux version)
 *
 * Similar to Windows version but simpler:
 * - Uses narrow strings (UTF-8) throughout
 * - No RTL wrapping needed (terminals handle it correctly)
 * - Hidden file detection is simpler (just check for leading dot)
 *
 * @param dir Current directory path to traverse
 * @param args Command-line arguments and options
 * @param level Current depth level (1 = root)
 * @param prefix String prefix for tree drawing characters
 * @param isLast Whether this directory is the last entry in its parent
 * @param stats Reference to TreeStats for accumulating data
 * @param relpath Relative path from root directory (for CSV export)
 */
void printTree(const fs::path &dir, const Args &args, int level,
               std::string prefix, bool isLast, TreeStats &stats,
               std::string relpath) {

  // Check depth limit
  if (args.maxLevel > 0 && level > args.maxLevel)
    return;

  // Collect directory entries
  std::vector<fs::directory_entry> entries;
  try {
    for (const auto &entry : fs::directory_iterator(
             dir, fs::directory_options::skip_permission_denied)) {
      std::string name = entry.path().filename().string();

      // Filter hidden files (files starting with dot)
      if (!args.showHidden) {
        if (!name.empty() && name[0] == '.')
          continue;
      }

      // Filter by exclude pattern
      if (!args.excludePattern.empty() &&
          matchesPattern(name, args.excludePattern))
        continue;

      // Filter to directories only if requested
      if (args.showDirsOnly && !entry.is_directory())
        continue;

      entries.push_back(entry);
    }
  } catch (const std::exception &ex) {
    std::cerr << "[etree] Failed to enumerate directory '" << dir.string()
              << "': " << ex.what() << std::endl;
    return;
  }

  // Sort entries alphabetically
  std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
    return a.path().filename().string() < b.path().filename().string();
  });

  // Process each entry
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto &entry = entries[i];
    bool isDir = entry.is_directory();
    bool entryIsLast = (i + 1 == entries.size());
    std::string branch = entryIsLast ? "`-- " : "|-- ";
    const char *color =
        (enable_colors(args.nocolors) ? (isDir ? dircolor : filecolor) : "");
    const char *reset = (enable_colors(args.nocolors) ? resetcolor : "");

    // Display entry name (unless doing CSV export)
    if (args.csvOut.empty())
      std::cout << prefix << color << branch << entry.path().filename().string()
                << reset;

    // Collect CSV data if export requested
    if (!args.csvOut.empty()) {
      fs::path relp = relpath.empty()
                          ? entry.path().filename()
                          : (fs::path(relpath) / entry.path().filename());
      CsvRow row;
      row.relpath = relp.string();
      row.name = entry.path().filename().string();
      row.type = isDir ? "folder" : "file";
      try {
        row.bytes = isDir ? 0 : entry.file_size();
      } catch (...) {
        row.bytes = 0;
      }
      row.perms = getPermissions(entry);
      auto times = get_file_times(entry.path());
      row.created = times.first;
      row.modified = times.second;
      stats.csvRows.push_back(row);
    }

    // Display additional metadata if requested
    if (args.csvOut.empty()) {
      if (args.showSize) {
        try {
          std::cout << (enable_colors(args.nocolors) ? sizecolor : "") << " ["
                    << formatSizeBytes(isDir ? 0 : entry.file_size()) << "]"
                    << (enable_colors(args.nocolors) ? resetcolor : "");
        } catch (...) {
          std::cout << (enable_colors(args.nocolors) ? sizecolor : "")
                    << " [0 B]"
                    << (enable_colors(args.nocolors) ? resetcolor : "");
        }
      }
      if (args.showPerms)
        std::cout << (enable_colors(args.nocolors) ? permcolor : "") << " ("
                  << getPermissions(entry) << ")"
                  << (enable_colors(args.nocolors) ? resetcolor : "");
      std::cout << std::endl;
    }

    // Recursively process subdirectories
    if (isDir) {
      stats.folders++;
      printTree(entry.path(), args, level + 1,
                prefix + (entryIsLast ? "    " : "|   "), entryIsLast, stats,
                relpath.empty()
                    ? entry.path().filename().string()
                    : (relpath + "/" + entry.path().filename().string()));
    } else {
      stats.files++;
    }
  }

  stats.maxDepth = std::max(stats.maxDepth, level);
}
#endif