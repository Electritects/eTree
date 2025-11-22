/**
 * @file help.h
 * @brief Help message display declarations for eTree
 *
 * This header declares the function for displaying usage information
 * and help text to the user when requested or when invalid arguments
 * are provided.
 */

#ifndef HELP_H
#define HELP_H

/**
 * @brief Display usage information and help text
 *
 * This function prints comprehensive usage information to the console,
 * including:
 * - Program description and purpose
 * - Command-line syntax
 * - List of all available options with descriptions
 * - Usage examples
 * - Notes about encoding and platform-specific behavior
 *
 * The output is formatted for readability and includes both short (-a)
 * and long (--all) option formats where applicable.
 */
void printUsage();

#endif
