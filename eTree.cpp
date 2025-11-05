/*
    eTree - Directory Tree Viewer (open-source)
    -------------------------------------------
    Author: Elie Accari
    License: MIT (or your preferred OSS license)

    Description:
      eTree displays folder/file structure as an ASCII tree, similar to 'tree' on UNIX.
      Features include color support, bytes-only size (with comma separators), permission display, exclusion filters,
      hidden file controls, summary stats, and help.

      Windows and UNIX (MSVC/gcc/clang) are supported.

    To contribute or report issues, see the GitHub repo.
*/

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <locale>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#undef max
#undef min
#endif

namespace fs = std::filesystem;

// -------- Version Number and Color Codes --------
const std::string APPVERSION = "1.1.0.0";

// ANSI color codes (used only for terminals)
const std::string dircolor = "\033[1;34m";
const std::string filecolor = "\033[0;32m";
const std::string permcolor = "\033[0;36m";
const std::string sizecolor = "\033[0;33m";
const std::string resetcolor = "\033[0m";

// -------- Terminal Detection, Color Enabling --------
bool is_stdout_tty() {
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdout));
#endif
}
bool enable_colors(bool nocolors) { return !nocolors && is_stdout_tty(); }

// -------- Bytes With Commas Formatter --------
std::string formatIntWithCommas(uintmax_t value) {
    std::ostringstream oss;
#if defined(_WIN32)
    // MSVC locale("") works for comma
    oss.imbue(std::locale(""));
#else
    // Safe fallback for POSIX
    oss.imbue(std::locale("en_US.UTF-8"));
#endif
    oss << value;
    return oss.str();
}

std::string formatSizeBytes(uintmax_t bytes) {
    return formatIntWithCommas(bytes) + " B";
}

// -------- File/Folder Permission/Attribute Formatting --------
std::string getPermissions(const fs::directory_entry& entry) {
    std::string perms;
#ifdef _WIN32
    DWORD attr = GetFileAttributesW(entry.path().c_str());
    if (attr & FILE_ATTRIBUTE_READONLY)   perms += 'R';
    if (attr & FILE_ATTRIBUTE_HIDDEN)     perms += 'H';
    if (attr & FILE_ATTRIBUTE_SYSTEM)     perms += 'S';
    if (attr & FILE_ATTRIBUTE_ARCHIVE)    perms += 'A';
#else
    fs::perms p = entry.status().permissions();
    perms += ((p & fs::perms::owner_read) != fs::perms::none) ? 'r' : '-';
    perms += ((p & fs::perms::owner_write) != fs::perms::none) ? 'w' : '-';
    perms += ((p & fs::perms::owner_exec) != fs::perms::none) ? 'x' : '-';
#endif
    return perms.empty() ? "-" : perms;
}

// -------- Pattern Filter (Exclusion) --------
bool matchesPattern(const std::string& name, const std::string& pattern) {
    if (pattern.empty()) return false;
    std::string regexPattern = std::regex_replace(pattern, std::regex("\\."), "\\.");
    regexPattern = std::regex_replace(regexPattern, std::regex("\\?"), ".");
    regexPattern = std::regex_replace(regexPattern, std::regex("\\*"), ".*");
    std::regex re("^" + regexPattern + "$", std::regex::icase);
    return std::regex_match(name, re);
}

// -------- Argument Struct --------
struct Args {
    bool showDirsOnly = false;
    bool showHidden = false;
    bool showSize = false;
    bool showHuman = false; // Ignored but retained for CLI compatibility
    bool showPerms = false;
    bool nocolors = false;
    bool showVersion = false;
    bool showHelp = false;
    int maxLevel = 0;
    std::string excludePattern = "";
    std::string folder = ".";
};

// -------- Usage Message --------
void printUsage() {
    std::cout <<
        "eTree - Directory Tree Viewer v" << APPVERSION << "\n"
        "Usage:\n"
        "  etree [options] [directory]\n"
        "\nOptions (use -, /, or -- for each option):\n"
        "  -d  /d       Only show directories (no files)\n"
        "  -a  /a       Show hidden files and folders\n"
        "  -I  /I pat   Exclude files or folders matching pattern (wildcards *, ? allowed)\n"
        "  -s  /s       Show file sizes in bytes (with thousands separators)\n"
        "  -p  /p       Show file permissions (RHSA on Windows, RWX on UNIX)\n"
        "  -l  /l N     Limit depth to N levels (default: unlimited)\n"
        "  -nc /nc      Disable color output (ASCII only, auto for file output)\n"
        "  -v  /v       Show program version\n"
        "  -?  /?       Show this help message\n"
        "  --help       Show this help message\n"
        "\nNotes:\n"
        "  - Options can be combined (-ds, /d/s) or separated (-d -s).\n"
        "  - By default, eTree displays the full folder tree recursively. Use -lN or /lN to limit levels.\n"
        "  - If output is redirected (e.g. > file.txt), colors are auto disabled.\n"
        "  - Summary counts only visible items (hidden/excluded skipped in total).\n"
        "  - All sizes displayed in bytes, with thousands separators, e.g., 3,456,789 B\n"
        "\nExamples:\n"
        "  etree                  Show full tree of current folder\n"
        "  etree -a               Include hidden files and folders\n"
        "  etree /ds /l2          Directories only, show sizes, 2 levels deep\n"
        "  etree -p               Show permissions\n"
        "  etree /I*.bak          Exclude files/folders matching '*.bak'\n"
        "  etree -nc >tree.txt    Output to tree.txt, no colors (auto disables anyway)\n"
        "  etree D:\\Projects     Show tree for a specific directory\n";
}

// -------- Argument Parsing --------
bool parseArgs(int argc, char* argv[], Args& args) {
    bool foundUnknown = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i], next = (i + 1 < argc ? argv[i + 1] : "");

        // Normalize Windows style /option to -option
        if (arg.size() > 1 && arg[0] == '/')
            arg = "-" + arg.substr(1);

        // Handle options
        if (arg == "-v" || arg == "--version") { args.showVersion = true; return true; }
        if (arg == "-?" || arg == "--help") { args.showHelp = true; return true; }
        if (arg == "-nc") { args.nocolors = true; continue; }
        if (arg.rfind("-l", 0) == 0) {
            if (arg.length() > 2)
                args.maxLevel = std::stoi(arg.substr(2));
            else if (!next.empty() && std::isdigit(next[0])) {
                args.maxLevel = std::stoi(next); ++i;
            }
            else
                args.maxLevel = 1;
            continue;
        }
        if (arg.rfind("-I", 0) == 0) {
            if (arg.length() > 2)
                args.excludePattern = arg.substr(2);
            else if (!next.empty()) { args.excludePattern = next; ++i; }
            continue;
        }
        if (arg.size() >= 2 && arg[0] == '-') {
            for (size_t j = 1; j < arg.size(); ++j) {
                switch (arg[j]) {
                case 'd': args.showDirsOnly = true; break;
                case 'a': args.showHidden = true; break;
                case 's': args.showSize = true; break;
                case 'h': args.showHuman = true; break; // for compatibility
                case 'p': args.showPerms = true; break;
                case 'l':
                case 'I':
                    break; // handled above
                default:
                    foundUnknown = true; // Invalid option character
                }
            }
            continue;
        }
        // Directory argument (only one allowed)
        if (args.folder == ".")
            args.folder = arg;
        else
            foundUnknown = true;
    }
    return !foundUnknown;
}

// -------- Stats Structure --------
struct TreeStats {
    int maxDepth = 0;
    int folders = 0;
    int files = 0;
};

// -------- Tree Printing --------
void printTree(const fs::path& dir, const Args& args, int level,
    std::string prefix, bool isLast, TreeStats& stats) {
    if (args.maxLevel > 0 && level > args.maxLevel)
        return;

    std::vector<fs::directory_entry> entries;
    for (const auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied)) {
        std::string name = entry.path().filename().string();
        if (!args.showHidden) {
#ifdef _WIN32
            DWORD attr = GetFileAttributesW(entry.path().c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_HIDDEN))
                continue;
#endif
            if (name.size() && name[0] == '.') continue;
        }
        if (!args.excludePattern.empty() && matchesPattern(name, args.excludePattern))
            continue;
        if (args.showDirsOnly && !entry.is_directory())
            continue;
        entries.push_back(entry);
    }
    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.path().filename().string() < b.path().filename().string();
        });

    size_t count = entries.size();
    for (size_t i = 0; i < count; ++i) {
        const auto& entry = entries[i];
        bool isDir = entry.is_directory();
        bool entryIsLast = (i + 1 == count);
        std::string branch = entryIsLast ? "`-- " : "|-- ";
        std::string color = (args.nocolors ? "" : (isDir ? dircolor : filecolor));
        std::string reset = (args.nocolors ? "" : resetcolor);

        std::cout << prefix << color << branch << entry.path().filename().string() << reset;

        if (args.showSize)
            std::cout << (args.nocolors ? "" : sizecolor)
            << " [" << formatSizeBytes(isDir ? 0 : entry.file_size()) << "]"
            << (args.nocolors ? "" : resetcolor);

        if (args.showPerms)
            std::cout << (args.nocolors ? "" : permcolor)
            << " (" << getPermissions(entry) << ")"
            << (args.nocolors ? "" : resetcolor);

        std::cout << "\n";

        if (isDir) {
            stats.folders++;
            printTree(entry.path(), args, level + 1, prefix + (entryIsLast ? "    " : "|   "), entryIsLast, stats);
        }
        else {
            stats.files++;
        }
    }
    stats.maxDepth = std::max(stats.maxDepth, level);
}

// -------- Main, Help, and Entry Point --------
int main(int argc, char* argv[]) {
    Args args;
    bool argsOK = parseArgs(argc, argv, args);

    if (!argsOK) {
        std::cerr << "Error: Unknown or invalid argument(s)\n\n";
        printUsage();
        return 1;
    }

    if (args.showVersion) {
        std::cout << "eTree version " << APPVERSION << std::endl;
        return 0;
    }
    if (args.showHelp) {
        printUsage();
        return 0;
    }

    if (!is_stdout_tty())
        args.nocolors = true;

    std::string rootcolor = (args.nocolors ? "" : dircolor);
    std::string rootreset = (args.nocolors ? "" : resetcolor);
    std::cout << rootcolor << args.folder << rootreset << "\n";

    TreeStats stats;
    printTree(args.folder, args, 1, "", true, stats);

    std::cout << "\nThe tree counts " << stats.maxDepth
        << " layer(s), " << stats.folders
        << " folder(s), and " << stats.files
        << " file(s)." << std::endl;

    return 0;
}

/*
    End of file.
    See project README and LICENSE on GitHub for more info.
*/
