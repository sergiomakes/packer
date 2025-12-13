# Packer

A lightweight, cross-platform asset packer tool written in C that bundles multiple files into a single binary pack file with a generated C header for easy integration into your projects.

## Features

- 📦 **Simple Asset Bundling** - Pack multiple files into a single `.nxap` binary file
- 🔧 **Auto-Generated Headers** - Creates C header files with asset metadata and lookup tables
- 🚀 **Cross-Platform** - Supports Windows, Linux, and macOS
- ⚡ **Fast & Lightweight** - Written in pure C with minimal dependencies
- 🎯 **Easy Integration** - Generated headers make asset loading straightforward

## Installation

### Pre-built Binaries

Download the latest release for your platform from the [Releases](https://github.com/wherd/packer/releases) page:

- **Windows**: `packer-windows-x64.exe`
- **Linux**: `packer-linux-x64`
- **macOS**: `packer-macos-universal`

### Building from Source

#### Windows

```cmd
scripts\build-windows.bat release
```

#### Linux

```bash
chmod +x scripts/build-linux.sh
scripts/build-linux.sh release
```

#### macOS

```bash
chmod +x scripts/build-macos.sh
scripts/build-macos.sh release
```

The compiled binary will be in the `build/` directory.

## Usage

```bash
packer <config_file> <output_name>
```

### Example

```bash
packer config.txt assets
```

This will generate:
- `assets.nxap` - Binary pack file containing all assets
- `assets.h` - C header file with asset definitions and lookup table

## Configuration File Format

Create a text file (e.g., `config.txt`) with the following format:

```
PACKER <file_path> <ASSET_NAME>
```

### Example Configuration

```
PACKER img/background.png BACKGROUND
PACKER img/player.png PLAYER_SPRITE
PACKER audio/music.ogg BACKGROUND_MUSIC
PACKER data/level1.json LEVEL_1_DATA
```

- Lines starting with `#` are treated as comments
- Empty lines are ignored
- Each asset gets a unique name (used as an enum identifier)

## Generated Files

### Binary Pack File (`.nxap`)

The pack file contains:
- **Magic Number**: `0x4E584150` ("NXAP" - NX Asset Pack)
- **Version**: Format version number
- **Asset Data**: Concatenated binary data of all assets

### Header File (`.h`)

The generated header includes:

```c
// Asset IDs as enum
typedef enum {
    ASSET_BACKGROUND = 0,
    ASSET_PLAYER_SPRITE = 1,
    ASSET_BACKGROUND_MUSIC = 2,
    ASSET_LEVEL_1_DATA = 3,
    ASSET_COUNT = 4
} asset_id;

// Asset metadata structure
typedef struct {
    size_t offset;
    size_t size;
} asset_t;

// Asset lookup table
static asset_t ASSET_TABLE[] = {
    [ASSET_BACKGROUND] = {.offset = 8, .size = 12345},
    [ASSET_PLAYER_SPRITE] = {.offset = 12353, .size = 6789},
    // ...
};
```

## Integration Example

```c
#include "assets.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Open the pack file
    FILE* pack = fopen("assets.nxap", "rb");
    if (!pack) {
        printf("Failed to open pack file\n");
        return 1;
    }
    
    // Load a specific asset
    asset_t* asset = &ASSET_TABLE[ASSET_BACKGROUND];
    
    // Allocate memory for the asset
    void* data = malloc(asset->size);
    
    // Seek to asset position and read
    fseek(pack, asset->offset, SEEK_SET);
    fread(data, 1, asset->size, pack);
    
    // Use the asset data...
    
    free(data);
    fclose(pack);
    return 0;
}
```

## Build Types

The build scripts support three build types:

- **debug** (default): Unoptimized with debug symbols (`-O0 -g`)
- **internal**: Optimized with internal debugging enabled (`-O2`)
- **release**: Fully optimized for production (`-O2`)

Example:
```bash
scripts/build-linux.sh release
```

## Limitations

- Maximum assets: 512
- Maximum asset name length: 128 characters
- Maximum file path length: 256 characters

These limits can be adjusted by modifying the constants in `src/main.c`.

## File Format Specification

### Pack File Structure

```
[Magic Number: 4 bytes] - 0x4E584150 ("NXAP")
[Version: 4 bytes]      - Format version (currently 1)
[Asset 1 Data]          - Raw binary data
[Asset 2 Data]          - Raw binary data
...
```

The header file contains the offset and size information needed to extract individual assets from the pack file.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

Copyright 2025 Sergio Leal

## Release Process

See [RELEASE.md](.github/RELEASE.md) for information about creating releases.

