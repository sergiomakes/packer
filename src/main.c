#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define INTERNAL static
#define GLOBAL static

#ifndef OUT
#define OUT
#endif

#define MAGIC_NUMBER 0x4E584150  // "NXAP" (NX Asset Pack)
#define VERSION 1

#define MAX_ASSETS 512
#define MAX_NAME 128
#define MAX_CHARSET 128
#define MAX_FILENAME 256

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

typedef uint32_t bool32_t;

typedef struct
{
    char name[MAX_NAME];
    char path[MAX_PATH];
    size_t offset;
    size_t size;
} asset_entry_t;

GLOBAL asset_entry_t assets[MAX_ASSETS];

///////////////////////////////////////////////////////////////////////////////
// Config parser
///////////////////////////////////////////////////////////////////////////////

INTERNAL bool32_t
ParseConfig(const char* config, OUT asset_entry_t* assets, OUT size_t* asset_count)
{
    FILE* f = fopen(config, "r");
    if (!f)
    {
        printf("Error: Cannot open config file: %s\n", config);
        return 0;
    }

    *asset_count = 0;
    
    char line[MAX_CHARSET];
    while (fgets(line, sizeof(line), f))
    {
        if (line[0] == '\0' || line[0] == '#' || line[0] == '\r' || line[0] == '\n')
        {
            continue;
        }

        char cmd[MAX_NAME];
        sscanf(line, "%s", cmd);

        if (strncmp(cmd, "PACKER", 6) == 0)
        {
            char filename[MAX_FILENAME], name[MAX_NAME];
            if (sscanf(line, "PACKER %s %s", filename, name) == 2)
            {
                if (*asset_count < MAX_ASSETS)
                {
                    struct stat st;
                    asset_entry_t* asset = &assets[*asset_count];
                    strncpy(asset->name, name, MAX_NAME - 1);
                    strncpy(asset->path, filename, MAX_PATH - 1);
                    asset->name[MAX_NAME - 1] = '\0';
                    asset->path[MAX_PATH - 1] = '\0';
                    asset->size = (stat(asset->path, &st) != 0) ? 0 : st.st_size;    
                    (*asset_count)++;
                }
                else
                {
                    printf("Error: Too many assets (max %d)\n", MAX_ASSETS);
                    return 0;
                }
            }
            else
            {
                printf("Error: Invalid packer config: %s\n", line);
                return 0;
            }
        }
    }

    fclose(f);    
    return (*asset_count > 0);
}


///////////////////////////////////////////////////////////////////////////////
// Packer
///////////////////////////////////////////////////////////////////////////////

INTERNAL void
SanitizeName(const char* filename, char* out_name)
{
    const char* basename = strrchr(filename, '/');
    if (!basename) basename = strrchr(filename, '\\');
    basename = basename ? basename + 1 : filename;
    
    size_t len = strlen(basename);
    
    // Copy and uppercase name
    size_t j = 0;
    for (size_t i = 0; i < len && j < MAX_NAME - 1; i++)
    {
        char c = basename[i];
        if (isalnum(c)) {
            out_name[j++] = (char)toupper(c);
        } else if (c == '-' || c == ' ' || c == '.') {
            out_name[j++] = '_';
        }
    }
    out_name[j] = '\0';
}

INTERNAL bool32_t
CreatePacker(asset_entry_t* assets, size_t asset_count, const char* output_name)
{
    char pack_filename[MAX_PATH];
    snprintf(pack_filename, sizeof(pack_filename), "%s.gen.nxap", output_name);

    FILE *pack_file = fopen(pack_filename, "wb");
    if (!pack_file)
    {
        printf("Error: Cannot create pack file: %s\n", pack_filename);
        return 0;
    }

    // Write header
    uint32_t magic_number = MAGIC_NUMBER;
    uint32_t version = VERSION;
    size_t offset = sizeof(magic_number) + sizeof(version);

    fwrite(&magic_number, sizeof(magic_number), 1, pack_file);
    fwrite(&version, sizeof(version), 1, pack_file);

    size_t total_size =  offset;
    for (size_t i = 0; i < asset_count; i++)
    {
        asset_entry_t* asset = &assets[i];
        asset->offset = total_size;
        total_size += asset->size;

        FILE* asset_file = fopen(asset->path, "rb");
        if (!asset_file)
        {
            printf("Error: Cannot open asset: %s\n", asset->path);
            fclose(pack_file);
            return 0;
        }

        char buffer[4096];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), asset_file)) > 0)
        {
            fwrite(buffer, 1, bytes_read, pack_file);
        }

        fclose(asset_file);
    }

    fclose(pack_file);
    return 1;
}

INTERNAL bool32_t
CreateHeader(asset_entry_t* assets, size_t asset_count, const char* output_name)
{
    char header_filename[MAX_PATH];
    snprintf(header_filename, sizeof(header_filename), "%s.gen.h", output_name);

    FILE *header_file = fopen(header_filename, "w");
    if (!header_file)
    {
        printf("Error: Cannot create header file: %s\n", header_filename);
        return 0;
    }

    uint32_t magic_number = MAGIC_NUMBER;
    uint32_t version = VERSION;

    fprintf(header_file,
        "// Auto-generated asset pack - DO NOT EDIT!\n"
        "// Generated by packer tool\n"
        "// Total assets: %lld\n\n"
        "#pragma once\n\n"
        "#include <stdint.h>\n\n"
        "#define PACKER_MAGIC 0x%08X\n"
        "#define PACKER_VERSION %d\n\n"
        "typedef enum\n{\n",
        asset_count, magic_number, version);
    
    for (size_t i = 0; i < asset_count; i++)
    {
        fprintf(header_file, "    ASSET_%s = %d,\n", assets[i].name, i);
    }

    fprintf(header_file,
        "    ASSET_COUNT = %lld\n"
        "} asset_id;\n\n"
        "typedef struct\n{\n"
        "    size_t offset;\n"
        "    size_t size;\n"
        "} asset_t;\n\n"
        "static asset_t ASSET_TABLE[] = {\n",
        asset_count);

    for (size_t i = 0; i < asset_count; i++)
    {
        fprintf(header_file, "    [ASSET_%s] = {.offset = %zu, .size = %zu},\n",
                assets[i].name, assets[i].offset, assets[i].size);
    }
    fprintf(header_file, "};\n");
    fclose(header_file);
    return 1;
}

int
main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("Usage: %s <config_file> <output_name>\n\n"
               "Example: %s config.txt assets\n\n"
               "Example config.txt:\n"
               "PACKER img/background.png BACKGROUND\n",
               argv[0], argv[0]);

        return 1;
    }

    size_t asset_count = 0;
    if (!ParseConfig(argv[1], assets, &asset_count))
    {
        printf("Error: Invalid config file: %s\n", argv[1]);
        return 1;
    }

    if (!CreatePacker(assets, asset_count, argv[2]))
    {
        printf("Error: Failed to create pack file\n");
        return 1;
    }

    if (!CreateHeader(assets, asset_count, argv[2]))
    {
        printf("Error: Failed to create header file\n");
        return 1;
    }

    return 0;
}
