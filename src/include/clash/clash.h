/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/clash-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#ifndef CLASH_TYPES_H
#define CLASH_TYPES_H

#include <stdlib.h>

struct ClashResponse;
struct FldOutStream;

typedef int ClashOptionType;

#define ClashTypeString (0x01)
#define ClashTypeInt (0x02)
#define ClashTypeFlag (0x03)
#define ClashTypeUInt64 (0x04)
#define ClashTypeArg (0x08)

typedef struct ClashOption {
    const char* name;
    const char shortName;
    const char* description;
    ClashOptionType type;
    const char* value;
    size_t structOffset;
} ClashOption;

typedef void (*ClashFn)(void* userData, const void* data, struct ClashResponse* response);

typedef struct ClashCommand {
    const char* name;
    const char* description;
    size_t structSize;
    const ClashOption* options;
    size_t optionCount;
    const struct ClashCommand* subCommands;
    size_t subCommandsCount;
    ClashFn fn;
} ClashCommand;

typedef struct ClashDefinition {
    struct ClashCommand* commands;
    size_t commandCount;
} ClashDefinition;

int clashParse(const ClashDefinition* definition, const char** argv, int argc, void* userData,
    struct FldOutStream* responseStream);
int clashParseString(const ClashDefinition* definition, const char* s, void* userData,
    struct FldOutStream* responseStream);

int clashSplitString(
    const char* s, char* buffer, size_t maxCount, const char** out, size_t arrayCount);

const char* clashUsage(const ClashDefinition* definition, char* buf, size_t maxCount);
void clashUsageToStream(const ClashDefinition* definition, struct FldOutStream* outStream);

#endif
