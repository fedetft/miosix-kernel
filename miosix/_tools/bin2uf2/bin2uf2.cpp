/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <unistd.h>
#include <getopt.h>
#include <string>
#include <optional>

[[noreturn]] static void fail(const char *fmt, ...)
{
    fprintf(stderr, "error: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

static void warn(const char *fmt, ...)
{
    fprintf(stderr, "warning: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}


struct Bin2UF2Options
{
    bool verbose = false;
    std::string inputFile;
    std::optional<std::string> outputFile;
    uint32_t loadAddr;
    uint32_t maxPayloadSize = 256;
    std::optional<uint32_t> familyID;
    bool pad = false;
};

struct UF2Block
{
    UF2Block(FILE *fp, size_t blockSize, bool pad)
    {
        assert(blockSize > 0 && blockSize <= maxSize);
        size = fread(buffer, 1, blockSize, fp);
        if (size == 0 && ferror(fp))
            fail("unable to read (%s)\n", strerror(errno));
        if (pad) size = blockSize;
    }

    void write(FILE *fp, uint32_t address, uint32_t blockNumber, 
               uint32_t numBlocks, std::optional<uint32_t> familyID)
    {
        writeNextField(fp, 0x0A324655); // magicStart0
        writeNextField(fp, 0x9E5D5157); // magicStart1
        uint32_t flags = familyID.has_value() ? 0x2000 : 0;
        writeNextField(fp, flags); // flags
        writeNextField(fp, address); // targetAddr
        writeNextField(fp, static_cast<uint32_t>(size)); // payloadSize
        writeNextField(fp, blockNumber); // blockNo
        writeNextField(fp, numBlocks); // numBlocks
        writeNextField(fp, familyID.value_or(0));
        if (fwrite(buffer, maxSize, 1, fp) != 1)
            fail("unable to write (%s)\n", strerror(errno));
        writeNextField(fp, 0x0AB16F30); // magicEnd
    }

    size_t blockSize() { return size; }

    static const uint32_t maxSize = 476;

private:
    static void writeNextField(FILE *fp, uint32_t val)
    {
        // For those who think that I'm doing a lot of work for nothing,
        // writing the integer directly from its pointer is undefined behavior.
        uint8_t tmp[4] = {static_cast<uint8_t>(val),
                          static_cast<uint8_t>(val >> 8),
                          static_cast<uint8_t>(val >> 16), 
                          static_cast<uint8_t>(val >> 24)};
        if (fwrite(tmp, 4, 1, fp) != 1)
            fail("unable to write (%s)\n", strerror(errno));
    }

    size_t size = 0;
    uint8_t buffer[maxSize] = {0};
};

void bin2uf2(Bin2UF2Options& opts)
{
    FILE *inFp = fopen(opts.inputFile.c_str(), "r");
    if (!inFp) fail("could not open input file (%s)\n", strerror(errno));

    fseek(inFp, 0, SEEK_END);
    long fileSize = ftell(inFp);
    if (fileSize < 0) fail("%s\n", strerror(errno));
    if (fileSize == 0) fail("input file is empty\n");
    fseeko(inFp, 0, SEEK_SET);
    uint32_t numBlocks = (fileSize+(opts.maxPayloadSize-1))/opts.maxPayloadSize;

    FILE *outFp;
    if (!opts.outputFile.has_value()) outFp = stdout;
    else {
        outFp = fopen(opts.outputFile.value().c_str(), "w");
        if (!outFp) fail("could not open output file (%s)\n", strerror(errno));
    }
    
    uint32_t curAddr = opts.loadAddr;
    uint32_t curIndex = 0;
    while (!feof(inFp))
    {
        UF2Block block(inFp, opts.maxPayloadSize, opts.pad);
        block.write(outFp, curAddr, curIndex, numBlocks, opts.familyID);
        if (opts.verbose)
            fprintf(stderr, "block %u, size %zu, loaded at 0x%08x\n",
                    curIndex, block.blockSize(), curAddr);
        curAddr += block.blockSize();
        curIndex++;
    }
    if (opts.verbose)
        fprintf(stderr, "done; %u blocks\n", numBlocks);

    fclose(inFp);
    if (outFp != stdout) fclose(outFp);
}


static void usage(const char *name)
{
    printf("usage: %s [options] -s <addr> input.bin\n\n", name);
    puts("Options:\n"
         "  -h, --help           Display available options\n"
         "  -o <file>            Write output to file\n"
         "  -s, --start <addr>   Initial load address\n"
         "  -b, --max-block <n>  Maximum data size in each block (default=256)\n"
         "  -f, --family <id>    Family ID\n"
         "  -p, --pad            Pad data to a multiple of the block size\n"
         "  -v, --verbose        Verbose mode");
}

static uint32_t parseIntegerOptArgument(const char *what)
{
    char *endptr;
    uint32_t res = strtoul(optarg, &endptr, 0);
    if (*endptr != '\0') fail("invalid %s\n", what);
    return res;
}

static uint32_t parseIntegerOptArgument(uint32_t min, uint32_t max, const char *what)
{
    uint32_t res = parseIntegerOptArgument(what);
    if (res < min || res > max) fail("invalid %s\n", what);
    return res;
}

int main(int argc, char *argv[])
{
    const char *name = argv[0];
    const option options[] = {
        {"help",      no_argument,       NULL, 'h'},
        {"verbose",   no_argument,       NULL, 'v'},
        {"start",     required_argument, NULL, 's'},
        {"max-block", required_argument, NULL, 'b'},
        {"family",    required_argument, NULL, 'f'},
        {"pad",       no_argument,       NULL, 'p'},
        {NULL,                        0, NULL, 0  }};
    std::optional<uint32_t> loadAddr;
    Bin2UF2Options opts;

    int ch;
    while ((ch = getopt_long(argc, argv, "ho:vs:b:f:p", options, NULL)) != -1)
    {
        switch (ch)
        {
        case 'h':
            usage(name);
            exit(0);
        case 'o':
            opts.outputFile = std::string(optarg);
            break;
        case 'v':
            opts.verbose = true;
            break;
        case 's':
            loadAddr = parseIntegerOptArgument("start address");
            break;
        case 'b':
            opts.maxPayloadSize =
                parseIntegerOptArgument(1, UF2Block::maxSize, "maximum block size");
            break;
        case 'f':
            opts.familyID = parseIntegerOptArgument("family ID");
            break;
        case 'p':
            opts.pad = true;
            break;
        default:
            fprintf(stderr, "error: invalid option\n");
            usage(name);
            exit(1);
        }
    }
    argc -= optind;
    argv += optind;
    if (argc != 1)
    {
        if (argc == 0) fprintf(stderr, "error: input file not specified\n");
        else fprintf(stderr, "error: only one input file is allowed\n");
        usage(name);
        exit(1);
    }
    opts.inputFile = argv[0];

    if (!loadAddr.has_value())
    {
        fprintf(stderr, "error: start address option is mandatory\n");
        usage(name);
        exit(1);
    }
    opts.loadAddr = loadAddr.value();

    if (!opts.familyID.has_value())
        warn("family ID not specified (file may be ignored by device)\n");

    bin2uf2(opts);
    return 0;
}

