/**
 * @file etree.h
 * @brief Core directory tree traversal and display functionality for eTree
 *
 * This header defines the main data structures and functions for traversing
 * directory trees and displaying them in a tree format. It includes support
 * for:
 * - Cross-platform file system operations
 * - RTL (Right-to-Left) text handling for Arabic/Hebrew filenames
 * - Colored console output with ANSI escape codes
 * - CSV/TSV export functionality
 * - File metadata (size, permissions, timestamps)
 */

#ifndef ETREE_H
#define ETREE_H

#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

// Forward declaration of Args structure
struct Args;

/**
 * @struct CsvRow
 * @brief Represents a single row in the CSV/TSV export
 *
 * Each row corresponds to one file or directory in the tree,
 * containing all relevant metadata for export to spreadsheet applications.
 */
struct CsvRow {
  std::string relpath;  ///< Relative path from the root directory
  std::string name;     ///< Filename or directory name
  std::string type;     ///< "file" or "folder"
  std::string perms;    ///< Permission string (e.g., "RHSA" on Windows,
                        ///< "rwxr-xr-x" on Unix)
  uintmax_t bytes = 0;  ///< File size in bytes (0 for directories)
  std::string created;  ///< Creation timestamp (YYYY-MM-DD HH:MM:SS)
  std::string modified; ///< Last modification timestamp (YYYY-MM-DD HH:MM:SS)

  /**
   * @brief Default constructor - initializes bytes to 0
   */
  CsvRow() : bytes(0) {}
};

/**
 * @struct TreeStats
 * @brief Accumulates statistics during directory tree traversal
 *
 * This structure collects information about the directory tree as it's
 * being traversed, including depth, counts, and CSV export data.
 */
struct TreeStats {
  int maxDepth = 0;            ///< Maximum depth reached during traversal
  int folders = 0;             ///< Total number of directories encountered
  int files = 0;               ///< Total number of files encountered
  std::vector<CsvRow> csvRows; ///< Collection of rows for CSV export
};

// Platform-specific declarations
#ifdef _WIN32
// Windows-specific: Wide character (UTF-16) support for Unicode filenames

// ANSI color escape sequences for console output
extern const wchar_t *dircolor;   ///< Color for directory names (blue)
extern const wchar_t *filecolor;  ///< Color for file names (green)
extern const wchar_t *permcolor;  ///< Color for permissions (cyan)
extern const wchar_t *sizecolor;  ///< Color for file sizes (yellow)
extern const wchar_t *resetcolor; ///< Reset to default color

/**
 * @brief Check if running in Windows Terminal (supports ANSI colors)
 * @return true if Windows Terminal is detected, false otherwise
 */
bool is_windows_terminal();

/**
 * @brief Format an integer with locale-specific thousand separators
 * @param value The integer value to format
 * @return Formatted wide string with commas (e.g., "1,234,567")
 */
std::wstring formatIntWithCommas(uintmax_t value);

/**
 * @brief Format a byte count as a human-readable size string
 * @param bytes Number of bytes
 * @return Formatted string with " B" suffix (e.g., "1,234 B")
 */
std::wstring formatSizeBytes(uintmax_t bytes);

/**
 * @brief Check if a wide string contains RTL (Right-to-Left) characters
 *
 * Detects Arabic, Hebrew, and other RTL script characters that require
 * special handling for proper console display.
 *
 * @param s Wide string to check
 * @return true if RTL characters are found, false otherwise
 */
bool contains_rtl(const std::wstring &s);

/**
 * @brief Wrap RTL text for proper console display
 *
 * Windows console doesn't handle bidirectional text correctly, so this
 * function reverses RTL character sequences while preserving LTR text
 * and spacing. This is only needed for console output, not file output.
 *
 * @param s Wide string potentially containing RTL text
 * @return Wrapped string with RTL sequences reversed
 */
std::wstring wrap_rtl(std::wstring s);

/**
 * @brief Convert wide string (UTF-16) to UTF-8 string
 *
 * Used for file output where UTF-8 encoding is required instead of
 * the UTF-16 used for console output on Windows.
 *
 * @param w Wide string to convert
 * @return UTF-8 encoded string
 */
std::string wstring_to_utf8(const std::wstring &w);

/**
 * @brief Recursively print directory tree (Windows version)
 *
 * @param path Current directory path to traverse
 * @param args Command-line arguments and options
 * @param level Current depth level (starts at 1)
 * @param prefix Wide string prefix for tree drawing characters
 * @param isLast Whether this is the last entry in current directory
 * @param stats Reference to TreeStats for accumulating data
 * @param relpath Relative path from root (for CSV export)
 */
void printTree(const std::filesystem::path &, const Args &, int, std::wstring,
               bool, TreeStats &, std::wstring relpath = L"");

#else
// Unix/Linux-specific: Narrow character (UTF-8) support

// ANSI color escape sequences for console output
extern const char *dircolor;   ///< Color for directory names (blue)
extern const char *filecolor;  ///< Color for file names (green)
extern const char *permcolor;  ///< Color for permissions (cyan)
extern const char *sizecolor;  ///< Color for file sizes (yellow)
extern const char *resetcolor; ///< Reset to default color

/**
 * @brief Format an integer with locale-specific thousand separators
 * @param value The integer value to format
 * @return Formatted string with commas (e.g., "1,234,567")
 */
std::string formatIntWithCommas(uintmax_t value);

/**
 * @brief Format a byte count as a human-readable size string
 * @param bytes Number of bytes
 * @return Formatted string with " B" suffix (e.g., "1,234 B")
 */
std::string formatSizeBytes(uintmax_t bytes);

/**
 * @brief Recursively print directory tree (Unix/Linux version)
 *
 * @param path Current directory path to traverse
 * @param args Command-line arguments and options
 * @param level Current depth level (starts at 1)
 * @param prefix String prefix for tree drawing characters
 * @param isLast Whether this is the last entry in current directory
 * @param stats Reference to TreeStats for accumulating data
 * @param relpath Relative path from root (for CSV export)
 */
void printTree(const std::filesystem::path &, const Args &, int, std::string,
               bool, TreeStats &, std::string relpath = "");
#endif

/**
 * @brief Get permission string for a file or directory
 *
 * On Windows: Returns attribute flags (R=ReadOnly, H=Hidden, S=System,
 * A=Archive) On Unix/Linux: Returns Unix-style permissions (e.g., "rwxr-xr-x")
 *
 * @param entry Directory entry to get permissions for
 * @return Permission string
 */
std::string getPermissions(const std::filesystem::directory_entry &);

/**
 * @brief Determine if colored output should be enabled
 *
 * Colors are enabled if:
 * - nocolors flag is not set, AND
 * - Output is going to a console (not redirected to file)
 *
 * @param nocolors User's --no-colors flag
 * @return true if colors should be enabled, false otherwise
 */
bool enable_colors(bool nocolors);

/**
 * @brief Check if stdout is connected to a console
 *
 * Returns false if output is redirected to a file or piped to another program.
 * This is used to determine whether to use console-specific features like
 * colors and wide character output.
 *
 * @return true if stdout is a console, false if redirected/piped
 */
bool is_console();

#endif
