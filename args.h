/**
 * @file args.h
 * @brief Command-line argument parsing declarations for eTree
 *
 * This header defines the Args structure which holds all command-line
 * arguments and options for the eTree directory tree viewer application.
 * It also declares the parseArgs function for processing command-line input.
 */

#ifndef ARGS_H
#define ARGS_H

#include <string>

/**
 * @struct Args
 * @brief Structure to hold all command-line arguments and options
 *
 * This structure contains all the configuration options that can be
 * specified via command-line arguments when running eTree.
 */
struct Args {
  std::string
      folder; ///< Target directory to display (default: current directory)
  std::string excludePattern; ///< Wildcard pattern for files/folders to exclude
                              ///< (e.g., "*.tmp")
  std::string
      csvOut;   ///< Output CSV/TSV filename (empty if no CSV output requested)
  int maxLevel; ///< Maximum depth to traverse (0 = unlimited)
  bool showHidden;   ///< Whether to show hidden files and folders
  bool showDirsOnly; ///< Whether to show only directories (no files)
  bool showSize;     ///< Whether to display file sizes
  bool showPerms;    ///< Whether to display file permissions
  bool nocolors;     ///< Whether to disable colored output
  bool showHelp;     ///< Whether to display help message
  bool showVersion;  ///< Whether to display version information

  /**
   * @brief Default constructor - initializes all options to default values
   */
  Args();
};

/**
 * @brief Parse command-line arguments and populate Args structure
 *
 * This function processes the command-line arguments passed to main()
 * and fills the Args structure with the appropriate values. It handles
 * both short (-a) and long (--all) option formats.
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @param args Reference to Args structure to populate
 * @return true if arguments were parsed successfully, false if there was an
 * error
 */
bool parseArgs(int argc, char *argv[], Args &args);

#endif
