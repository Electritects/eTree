# eTree

**eTree** is a fast, flexible, and cross-platform directory tree viewer for Windows and Linux, written in modern C++. Instantly visualize folder structures, get file/folder stats with thousands separators, colorized output, and file attribute insights—all from a single icon-free executable. Open source, MIT licensed.

---

## Features

- **Colorized ASCII tree output** for easy readability.
- **Shows folders and files** (or just folders, with a switch).
- **Displays file sizes** in bytes, always with thousands (comma) separators.
- **Shows permissions/attributes** (RHSA on Windows, rwx on UNIX).
- **Supports hidden files** and powerful pattern exclusions.
- **Auto color disable for redirected output** or with `-nc`.
- **Layered depth limiting**, wildcard exclude, and more.
- **Cross-platform:** Works out of the box with MSVC, gcc, and clang.
- **MIT Licensed** – use, modify, enjoy!

---

## Getting Started

### **Build from Source**

You need a C++17 (or later) compiler.

**Windows (MSVC):**
```
cl /std:c++17 /EHsc eTree.cpp
```

**Linux/macOS (g++/clang++):**
```
g++ -std=c++17 -o etree eTree.cpp
```

### **Usage**

Run in your terminal, PowerShell, or CMD:

```
etree [options] [directory]
```

### **Options**

| Option         | Description                                                   |
|----------------|---------------------------------------------------------------|
| -d /d          | Only show directories (no files)                              |
| -a /a          | Show hidden files and folders                                 |
| -I /I pat      | Exclude files or folders matching pattern (wildcards *, ?)    |
| -s /s          | Show file sizes in bytes (with thousands separators)          |
| -p /p          | Show file permissions (RHSA on Windows, rwx on UNIX)          |
| -l /l N        | Limit depth to N levels (default: unlimited)                  |
| -nc /nc        | Disable color output (auto for file output)                   |
| -v /v          | Show program version                                          |
| -? /? --help   | Show this help & usage message                                |

*Options can be combined, e.g. `-dahp` or `/d/s/p`. All switches work with `-`, `/`, or `--`.*

---

## **Examples**

```
etree
# Show entire current folder treeetree -a
# Include hidden files/foldersetree -d -l2
# Only directories, two levels downetree -s -p
# File sizes (in bytes) and permissionsetree /I*.obj
# Exclude all '.obj' filesetree -nc >tree.txt
# Output to file, disables color codes```

---

## Output Example

```
.  
|-- eTree [11,624 B]  
|-- eTree.slnx [202 B]  
|-- x64  
|   |-- Debug  
|   |   |-- eTree.exe [1,128,448 B]  
|   |   `-- eTree.pdb [4,804,608 B]  
|   `-- Release  
|       |-- eTree.exe [140,288 B]  
|       `-- eTree.pdb [2,551,808 B]  
The tree counts 4 layer(s), 6 folder(s), and 5 file(s).
```

---

## License

MIT License – see [LICENSE](LICENSE) file.  
**Enjoy!** Contributions and pull requests welcome.

---

## Credits

Developed and maintained by Elie Accari 
If you use it or like it, give a ⭐ on GitHub!
```
