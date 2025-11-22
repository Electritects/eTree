/**
 * @file csv.h
 * @brief CSV/TSV export functionality declarations for eTree
 *
 * This header declares the function for exporting directory tree
 * statistics to a tab-separated values (TSV) file format, which
 * can be opened in Excel or other spreadsheet applications.
 */

#ifndef CSV_H
#define CSV_H

#include "etree.h"
#include <string>


/**
 * @brief Export tree statistics to a TSV (Tab-Separated Values) file
 *
 * This function writes the collected directory tree data to a TSV file
 * with UTF-8 encoding and BOM (Byte Order Mark). The file includes:
 * - Header row with column names
 * - One row per file/folder with: path, name, type, size, dates, permissions
 *
 * The TSV format is compatible with Excel and other spreadsheet applications.
 *
 * @param filename Path to the output TSV file to create
 * @param stats TreeStats structure containing the collected data to export
 */
void writeTsv(const std::string &filename, const TreeStats &stats);

#endif
