/**
 * @file help.cpp
 * @brief Help message display implementation for eTree
 *
 * This file implements the printUsage() function which displays comprehensive
 * usage information and help text to the user. The output is platform-specific
 * to account for differences in console output between Windows and Unix/Linux.
 */

#include "help.h"
#include <iostream>

/**
 * @brief Display usage information and help text
 *
 * This function prints a comprehensive help message including:
 * - Program name and version
 * - Command-line syntax
 * - Detailed list of all available options with descriptions
 * - Usage notes about encoding, colors, and file formats
 * - Practical usage examples
 *
 * The output is platform-specific:
 * - Windows: Uses std::wcout for wide character support
 * - Unix/Linux: Uses std::cout for standard output
 *
 * The help text explains all command-line options, their short and long forms,
 * and provides context about Unicode support, RTL text handling, and TSV
 * export.
 */
void printUsage() {
#ifdef _WIN32
  // Windows version: Use wide character output for Unicode support
  std::wcout
      << L"eTree - Directory Tree Viewer v1.0.0\n"
         L"Usage:\n"
         L"  etree [options] [directory]\n"
         L"\nOptions (-, /, or -- for each option):\n"
         L"  -d /d         Only show directories (no files)\n"
         L"  -a /a         Show hidden files and folders\n"
         L"  -I /I pat     Exclude files/folders matching pattern (wildcards "
         L"*, ?)\n"
         L"  -s /s         Show file sizes in bytes (with thousands "
         L"separators)\n"
         L"  -p /p         Show file permissions (RHSA on Windows, rwx on "
         L"UNIX)\n"
         L"  -l /l N       Limit depth to N levels (default: unlimited)\n"
         L"  -nc /nc       Disable color output (ASCII only, auto for file "
         L"output)\n"
         L"  -o file       Output to file as TSV (tab-separated, UTF-8 for "
         L"Excel import)\n"
         L"  -v /v         Show program version\n"
         L"  -? /?         Show this help message\n"
         L"  --help        Show this help message\n"
         L"\nNotes:\n"
         L"  - Output file (-o): Exports all fields as tab-separated UTF-8 "
         L"(TSV, not CSV) to avoid field misparsing.\n"
         L"  - Hidden/system files included only with -a.\n"
         L"  - Control tree depth/output columns with -l, -p, -s etc.\n"
         L"  - Unicode: All filenames (including RTL/Arabic/Chinese) are "
         L"supported for output/import in Excel.\n"
         L"  - Coloring is auto-enabled in Windows Terminal, auto-disabled in "
         L"classic console (cmd.exe).\n"
         L"  - For file redirection with proper UTF-8 encoding, use: cmd /c "
         L"\"etree folder > output.txt\"\n"
         L"    or PowerShell: etree folder | Out-File -Encoding UTF8 "
         L"output.txt\n"
         L"\nExamples:\n"
         L"  etree -a -l2              # All visible/hiddens, 2 levels deep\n"
         L"  etree -o files.tsv        # TSV export for Excel import\n"
         L"  etree -a -o all.tsv       # All files, including hidden, to TSV\n"
         L"  etree -I*.tmp -s -p       # Exclude .tmp files, show sizes and "
         L"permissions\n";
#else
  // Unix/Linux version: Use standard character output
  std::cout
      << "eTree - Directory Tree Viewer v1.0.0\n"
         "Usage:\n"
         "  etree [options] [directory]\n"
         "\nOptions (-, /, or -- for each option):\n"
         "  -d /d         Only show directories (no files)\n"
         "  -a /a         Show hidden files and folders\n"
         "  -I /I pat     Exclude files/folders matching pattern (wildcards *, "
         "?)\n"
         "  -s /s         Show file sizes in bytes (with thousands "
         "separators)\n"
         "  -p /p         Show file permissions (RHSA on Windows, rwx on "
         "UNIX)\n"
         "  -l /l N       Limit depth to N levels (default: unlimited)\n"
         "  -nc /nc       Disable color output (ASCII only, auto for file "
         "output)\n"
         "  -o file       Output to file as TSV (tab-separated, UTF-8 for "
         "Excel import)\n"
         "  -v /v         Show program version\n"
         "  -? /?         Show this help message\n"
         "  --help        Show this help message\n"
         "\nNotes:\n"
         "  - Output file (-o): Exports all fields as tab-separated UTF-8 "
         "(TSV, not CSV) to avoid field misparsing.\n"
         "  - Hidden/system files included only with -a.\n"
         "  - Control tree depth/output columns with -l, -p, -s etc.\n"
         "  - Unicode: All filenames (including RTL/Arabic/Chinese) are "
         "supported for output/import in Excel.\n"
         "  - Coloring is auto-detected from tty and shell environment.\n"
         "\nExamples:\n"
         "  etree -a -l2              # All visible/hiddens, 2 levels deep\n"
         "  etree -o files.tsv        # TSV export for Excel import\n"
         "  etree -a -o all.tsv       # All files, including hidden, to TSV\n"
         "  etree -I*.tmp -s -p       # Exclude .tmp files, show sizes and "
         "permissions\n";
#endif
}
