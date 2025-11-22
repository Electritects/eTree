/**
 * @file args.cpp
 * @brief Command-line argument parsing implementation for eTree
 *
 * This file implements the argument parsing logic that processes command-line
 * options and populates the Args structure. It supports both Unix-style (-a)
 * and Windows-style (/a) option formats, as well as long options (--all).
 */

#include "args.h"
#include <cctype>
#include <string>

/**
 * @brief Default constructor for Args structure
 *
 * Initializes all options to their default values:
 * - folder: current directory (".")
 * - All boolean flags: false
 * - maxLevel: 0 (unlimited depth)
 * - Strings: empty
 */
Args::Args()
    : folder("."), excludePattern(""), csvOut(""), maxLevel(0),
      showHidden(false), showDirsOnly(false), showSize(false), showPerms(false),
      nocolors(false), showHelp(false), showVersion(false) {}

/**
 * @brief Parse command-line arguments and populate Args structure
 *
 * This function processes command-line arguments in several formats:
 * - Short options: -a, -s, -p (can be combined: -asp)
 * - Long options: --help, --version
 * - Options with values: -l2, -l 2, -I*.tmp, -I *.tmp, -o file.csv
 * - Windows-style: /a, /s, /p (converted to Unix-style internally)
 * - Positional argument: directory path (if not starting with -)
 *
 * @param argc Number of command-line arguments (from main)
 * @param argv Array of command-line argument strings (from main)
 * @param args Reference to Args structure to populate with parsed values
 * @return true if all arguments were recognized and valid, false if unknown
 * options found
 */
bool parseArgs(int argc, char *argv[], Args &args) {
  bool foundUnknown = false; // Track if we encounter any unrecognized arguments

  // Process each argument (skip argv[0] which is the program name)
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i]; // Current argument
    std::string next =
        (i + 1 < argc ? argv[i + 1]
                      : ""); // Next argument (for options with values)

    // Convert Windows-style /option to Unix-style -option
    if (arg.size() > 1 && arg[0] == '/')
      arg = "-" + arg.substr(1);

    // Version flag: -v or --version
    // Show version and exit immediately
    if (arg == "-v" || arg == "--version") {
      args.showVersion = true;
      return true;
    }

    // Help flag: -? or --help
    // Show usage information and exit immediately
    if (arg == "-?" || arg == "--help") {
      args.showHelp = true;
      return true;
    }

    // No colors flag: -nc
    // Disable ANSI color codes in output
    if (arg == "-nc") {
      args.nocolors = true;
      continue;
    }

    // Output file option: -o filename
    // Specify CSV/TSV output file
    if (arg == "-o" && !next.empty()) {
      args.csvOut = next;
      ++i; // Skip next argument since we consumed it
      continue;
    }

    // Level/depth limit option: -l, -l2, -l 2
    // Limit directory traversal depth
    if (arg.rfind("-l", 0) == 0) {
      if (arg.length() > 2) {
        // Format: -l2 (number attached to option)
        args.maxLevel = std::stoi(arg.substr(2));
      } else if (!next.empty() && std::isdigit(next[0])) {
        // Format: -l 2 (number as separate argument)
        args.maxLevel = std::stoi(next);
        ++i; // Skip next argument
      } else {
        // Format: -l (no number specified, default to 1)
        args.maxLevel = 1;
      }
      continue;
    }

    // Exclude pattern option: -I, -I*.tmp, -I *.tmp
    // Specify wildcard pattern for files/folders to exclude
    if (arg.rfind("-I", 0) == 0) {
      if (arg.length() > 2) {
        // Format: -I*.tmp (pattern attached to option)
        args.excludePattern = arg.substr(2);
      } else if (!next.empty()) {
        // Format: -I *.tmp (pattern as separate argument)
        args.excludePattern = next;
        ++i; // Skip next argument
      }
      continue;
    }

    // Combined short options: -asp, -dsp, etc.
    // Process each character as a separate flag
    if (arg.size() >= 2 && arg[0] == '-') {
      for (size_t j = 1; j < arg.size(); ++j) {
        switch (arg[j]) {
        case 'd': // Directories only
          args.showDirsOnly = true;
          break;
        case 'a': // Show hidden files
          args.showHidden = true;
          break;
        case 's': // Show file sizes
          args.showSize = true;
          break;
        case 'p': // Show permissions
          args.showPerms = true;
          break;
        case 'l': // Level (handled above, skip here)
        case 'I': // Exclude pattern (handled above, skip here)
        case 'o': // Output file (handled above, skip here)
          break;
        default:
          // Unknown option character
          foundUnknown = true;
        }
      }
      continue;
    }

    // Positional argument: directory path
    // Only accept one directory path; additional paths are errors
    if (args.folder == ".")
      args.folder = arg;
    else
      foundUnknown = true; // Multiple directory paths specified
  }

  // Return true only if no unknown arguments were found
  return !foundUnknown;
}
