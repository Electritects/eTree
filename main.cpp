/**
 * @file main.cpp
 * @brief Main entry point for eTree directory tree viewer
 *
 * This file contains the main() function which:
 * - Parses command-line arguments
 * - Configures console output mode (UTF-16 for console, UTF-8 for files)
 * - Initializes ANSI color support on Windows
 * - Orchestrates the directory tree traversal and output
 * - Handles CSV export if requested
 *
 * Platform-specific behavior:
 * - Windows: Supports UTF-16 console output, UTF-8 file output, ANSI colors
 * - Unix/Linux: Uses UTF-8 for all output, ANSI colors via terminal
 */

#include "args.h"
#include "csv.h"
#include "etree.h"
#include "help.h"
#include <iostream>


#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>

#endif

/**
 * @brief Main entry point for eTree application
 *
 * Execution flow:
 * 1. Parse command-line arguments
 * 2. Configure stdout mode based on output destination (console vs file)
 * 3. Handle --help and --version flags
 * 4. Traverse directory tree and display/collect data
 * 5. Export to CSV if requested, or display summary statistics
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return 0 on success, 1 on error (invalid arguments)
 */
int main(int argc, char *argv[]) {
  // Parse command-line arguments into Args structure
  Args args;
  bool argsOK = parseArgs(argc, argv, args);

#ifdef _WIN32
  // Windows-specific: Configure stdout mode based on output destination
  if (is_console()) {
    // Console output: Use UTF-16 mode for wide character support
    // This allows proper display of Unicode characters including RTL text
    _setmode(_fileno(stdout), _O_U16TEXT);

    // Enable ANSI escape sequence processing for colored output
    // This is required for Windows Terminal and modern console applications
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  } else {
    // File output: Use binary mode to have full control over encoding
    // We'll manually output UTF-8 with BOM to signal proper encoding
    _setmode(_fileno(stdout), _O_BINARY);

    // Output UTF-8 BOM (Byte Order Mark) to signal UTF-8 encoding
    // This helps text editors and applications detect the encoding correctly
    // Note: PowerShell's > operator will still convert to UTF-16LE, so users
    // should use cmd.exe or PowerShell's Out-File cmdlet for proper UTF-8
    std::cout << "\xEF\xBB\xBF";
  }
#endif

  // Handle invalid arguments
  if (!argsOK) {
#ifdef _WIN32
    if (is_console()) {
      std::wcerr << L"Error: Unknown or invalid argument(s)\n\n";
    } else {
      std::cerr << "Error: Unknown or invalid argument(s)\n\n";
    }
    printUsage();
#else
    std::cerr << "Error: Unknown or invalid argument(s)\n\n";
    printUsage();
#endif
    return 1;
  }

  // Handle --version flag
  if (args.showVersion) {
#ifdef _WIN32
    if (is_console()) {
      std::wcout << L"eTree version 1.0.0\n";
    } else {
      std::cout << "eTree version 1.0.0\n";
    }
#else
    std::cout << "eTree version 1.0.0\n";
#endif
    return 0;
  }

  // Handle --help flag
  if (args.showHelp) {
    printUsage();
    return 0;
  }

  // Disable colors if output is redirected to a file
  // Colors are only useful for console output
  if (!is_console())
    args.nocolors = true;

  // Initialize statistics structure to collect tree data
  TreeStats stats;

#ifdef _WIN32
  // Windows version: Handle wide character paths and conditional output

  // Display root directory name (unless doing CSV export)
  if (args.csvOut.empty()) {
    // Convert folder path to wide string for Unicode support
    std::wstring folderW(args.folder.begin(), args.folder.end());

    if (is_console()) {
      // Console: Use wcout with wide characters and optional colors
      std::wcout << (enable_colors(args.nocolors) ? dircolor : L"") << folderW
                 << (enable_colors(args.nocolors) ? resetcolor : L"")
                 << std::endl;
    } else {
      // File: Convert to UTF-8 and use cout
      std::cout << wstring_to_utf8(enable_colors(args.nocolors) ? dircolor
                                                                : L"")
                << wstring_to_utf8(folderW)
                << wstring_to_utf8(enable_colors(args.nocolors) ? resetcolor
                                                                : L"")
                << std::endl;
    }
  }

  // Traverse directory tree starting from specified folder
  // Level 1 = root level, empty prefix, isLast=true, empty relpath
  printTree(args.folder, args, 1, L"", true, stats, L"");

  // Handle output based on whether CSV export was requested
  if (!args.csvOut.empty()) {
    // CSV export mode: Write collected data to TSV file
    writeTsv(args.csvOut, stats);
  } else {
    // Console/file output mode: Display summary statistics
    if (is_console()) {
      // Console: Use wcout with wide characters
      std::wcout << L"\nThe tree counts " << stats.maxDepth << L" layers, "
                 << stats.folders << L" folders, " << stats.files
                 << L" files.\n";
    } else {
      // File: Use cout with narrow characters
      std::cout << "\nThe tree counts " << stats.maxDepth << " layers, "
                << stats.folders << " folders, " << stats.files << " files.\n";
    }
  }

#else
  // Unix/Linux version: Simpler handling with UTF-8 throughout

  // Display root directory name (unless doing CSV export)
  if (args.csvOut.empty())
    std::cout << (enable_colors(args.nocolors) ? dircolor : "") << args.folder
              << (enable_colors(args.nocolors) ? resetcolor : "") << std::endl;

  // Traverse directory tree
  printTree(args.folder, args, 1, "", true, stats, "");

  // Handle output
  if (!args.csvOut.empty())
    writeTsv(args.csvOut, stats);
  else
    std::cout << "\nThe tree counts " << stats.maxDepth << " layers, "
              << stats.folders << " folders, " << stats.files << " files."
              << std::endl;
#endif

  return 0;
}