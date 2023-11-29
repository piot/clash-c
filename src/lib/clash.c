/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/clash-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <clash/clash.h>
#include <clash/response.h>
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <stdio.h>
#include <string.h>
#include <tiny-libc/tiny_libc.h>

typedef struct ClashStructValue {
    const char* value;
    int count;
} ClashStructValue;

typedef struct ClashStructValues {
    ClashStructValue* values;
    size_t count;
} ClashStructValues;

typedef struct ClashState {
    const ClashCommand* command;
    const ClashOption* nameOption;
    int nameOptionIndex;
    ClashStructValues values;
    int argIndex;
} ClashState;

static ClashCommand* clashDefFindCommand(const ClashDefinition* definition, const char* name)
{
    for (size_t i = 0; i < definition->commandCount; ++i) {
        if (tc_str_equal(name, definition->commands[i].name)) {
            return &definition->commands[i];
        }
    }

    return 0;
}

static int clashCommandFindNameOption(const struct ClashCommand* command, const char* name)
{
    for (size_t i = 0; i < command->optionCount; ++i) {
        if (tc_str_equal(name, command->options[i].name)) {
            return (int)i;
        }
    }

    return -1;
}

static int clashCommandFindShortOption(const struct ClashCommand* command, const char shortName)
{
    for (size_t i = 0; i < command->optionCount; ++i) {
        if (command->options[i].shortName == shortName) {
            return (int)i;
        }
    }

    return -1;
}

static int clashCommandFindSubCommand(const struct ClashCommand* command, const char* shortName)
{
    for (size_t i = 0; i < command->subCommandsCount; ++i) {
        if (tc_str_equal(command->subCommands[i].name, shortName)) {
            return (int)i;
        }
    }

    return -1;
}

static int parseNameOption(ClashState* state, const char* name)
{
    if (state->command == 0) {
        return -5;
    }

    state->nameOptionIndex = clashCommandFindNameOption(state->command, name);
    if (state->nameOptionIndex == -1) {
        CLOG_SOFT_ERROR("could not find option '%s'", name)
        return -6;
    }

    state->nameOption = &state->command->options[state->nameOptionIndex];
    printf("found name option '%s'\n", state->nameOption->name);
    return 0;
}

#if defined CLASH_DEBUG_OUTPUT

static void valuesDebugOutput(const ClashStructValues* values, const struct ClashCommand* command)
{
    printf("\n Resulting values ----------- \n");
    if (command->optionCount == 0) {
        printf("problem\n");
        return;
    }
    for (size_t i = 0; i < values->count; ++i) {
        const ClashStructValue* item = &values->values[i];
        const struct ClashOption* option = &command->options[i];
        printf("%zu: %s = '%s' (%d) %s\n", i, option->name, item->value, item->count,
            option->description);
    }
}

#endif

static int setOptionValue(ClashStructValues* values, int optionIndex, const char* value)
{
    values->values[optionIndex].value = value;
    values->values[optionIndex].count++;
    // printf("* set option %d = '%s' (count:%d)\n", optionIndex, value, values->values[optionIndex].count);
    return 0;
}

static int parseNameOptionValue(ClashState* state, const char* value)
{
    if (state->nameOptionIndex == -1) {
        return -6;
    }
    if (state->nameOptionIndex >= (int)state->values.count) {
        return -4;
    }

    setOptionValue(&state->values, state->nameOptionIndex, value);

    state->nameOptionIndex = -1;
    state->nameOption = 0;

    return 0;
}

static int parseShortOption(struct ClashState* state, const char s)
{
    int index = clashCommandFindShortOption(state->command, s);
    if (index < 0) {
        printf("unknown short option %c\n", s);
        return index;
    }

    setOptionValue(&state->values, index, "");
    return 0;
}

static void selectCommand(struct ClashState* state, const struct ClashCommand* command)
{
    state->command = command;
    state->values.values = tc_malloc_type_count(ClashStructValue, command->optionCount);
    state->values.count = command->optionCount;
    for (size_t i = 0; i < state->values.count; ++i) {
        state->values.values[i].count = 0;
        state->values.values[i].value = command->options[i].value;
    }
}

static int parseArg(ClashState* state, const char* value)
{
    if (state->argIndex >= (int)state->command->optionCount) {
        return -1;
    }

    const ClashOption* nextOption = &state->command->options[state->argIndex];
    if (!(nextOption->type & ClashTypeArg)) {
        return -2;
    }

    state->nameOption = nextOption;
    state->nameOptionIndex = state->argIndex;
    state->argIndex++;
    parseNameOptionValue(state, value);

    return 0;
}

static int parseSubCommand(struct ClashState* state, const char* commandName)
{
    if (state->command == 0) {
        return -2;
    }

    if (state->command->subCommands == 0) {
        return -4;
    }

    int foundIndex = clashCommandFindSubCommand(state->command, commandName);
    if (foundIndex < 0) {
        return -5;
    }

    const ClashCommand* foundCommand = &state->command->subCommands[foundIndex];

    selectCommand(state, foundCommand);

    return 00;
}

static int parseOption(ClashState* state, const char* s, size_t len)
{
    int errorCode = 0;
    if (len >= 2 && s[0] == '-') {
        errorCode = parseNameOption(state, &s[1]);
    } else {
        for (size_t optionIndex = 0; optionIndex < len; ++optionIndex) {
            errorCode = parseShortOption(state, s[optionIndex]);
            if (errorCode < 0) {
                return errorCode;
            }
        }
    }

    return errorCode;
}

static int parseOptionSetCommandIfNeeded(
    ClashState* state, const ClashDefinition* definition, const char* s, size_t len)
{
    if (state->command == 0) {
        selectCommand(state, &definition->commands[0]);
    }

    return parseOption(state, s, len);
}

static void clashStateInit(struct ClashState* state)
{
    tc_mem_clear_type(state);
}

static uint64_t toUInt64(const char* s)
{
    int base = 10;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    }

    return tc_str_to_uint64(s, base);
}

static void* convertToStruct(const ClashCommand* command, const ClashStructValues* values)
{
    uint8_t* data = tc_malloc(command->structSize);
    for (size_t i = 0; i < command->optionCount; ++i) {
        const ClashOption* option = &command->options[i];
        void* p = (void*)(data + option->structOffset);
        const ClashStructValue* item = &values->values[i];
        const char* value = item->value;
        if (i == 0 && value == 0) {
            return data;
        }

        switch (option->type & 0x7) {
        case ClashTypeBool:
            *((bool*)p) = item->count != 0;
            break;
        case ClashTypeInt:
            *((int*)p) = atoi(value);
            break;
        case ClashTypeUInt64:
            *((uint64_t*)p) = toUInt64(value);
            break;
        case ClashTypeString:
            *((const char**)p) = value;
            break;
        case ClashTypeFlag:
            *((int*)p) = item->count;
            break;
        }
    }

    return data;
}

int clashParse(const ClashDefinition* definition, const char** argv, int argc, void* userData,
    FldOutStream* responseStream)
{
    ClashState state;
    clashStateInit(&state);
    int errorCode;
    for (size_t i = 0; i < (size_t)argc; ++i) {
        const char* s = argv[i];
        if (s == 0) {
            return -2;
        }
        size_t len = strlen(s);

        if (len != 0 && s[0] == '-') {
            if (state.nameOption != 0) {
                printf("expected named option value");
                return -6;
            }
            errorCode = parseOptionSetCommandIfNeeded(&state, definition, &s[1], len - 1);
            if (errorCode < 0) {
                return errorCode;
            }
        } else {
            if (state.nameOption != 0) {
                errorCode = parseNameOptionValue(&state, s);
                if (errorCode < 0) {
                    return errorCode;
                }
            } else if (state.command == 0) {
                const ClashCommand* foundCommand = clashDefFindCommand(definition, s);
                if (foundCommand == 0) {
                    return -4;
                }
                selectCommand(&state, foundCommand);
            } else {
                if (state.command->subCommands != 0) {
                    errorCode = parseSubCommand(&state, s);
                    if (errorCode < 0) {
                        return errorCode;
                    }
                } else {
                    errorCode = parseArg(&state, s);
                    if (errorCode < 0) {
                        return errorCode;
                    }
                }
            }
        }
    }

#if defined CLASH_DEBUG_OUTPUT

    valuesDebugOutput(&state.values, state.command);

#endif
    ClashResponse response;
    response.outStream = responseStream;
    tingeStateInit(&response.tintState, responseStream);

    if (state.command && state.command->fn) {
        void* structData = convertToStruct(state.command, &state.values);
        state.command->fn(userData, structData, &response);
        tc_free((void*)structData);
    }

    clashResponseResetColor(&response);
    fldOutStreamWriteUInt8(responseStream, 0);

    return 0;
}

int clashParseString(
    const ClashDefinition* definition, const char* s, void* userData, FldOutStream* responseStream)
{
    char temp[512];
    const char* tempArgs[512];
    int countFound = clashSplitString(s, temp, 512, tempArgs, 512);
    if (countFound < 0) {
        return countFound;
    }

    return clashParse(definition, tempArgs, countFound, userData, responseStream);
}

static int isWhitespace(const char ch)
{
    return (ch == ' ' || ch == 9);
}

static const char* skipWhitespace(const char* s, int* wasEnd)
{
    const char* p = s;
    while (*p != 0) {
        if (!isWhitespace(*p)) {
            *wasEnd = 0;
            return p;
        }
        p++;
    }

    *wasEnd = 1;
    return p;
}

static const char* skipToWhitespace(const char* s, int* wasEnd)
{
    const char* p = s;
    while (*p != 0) {
        if (isWhitespace(*p)) {
            *wasEnd = 0;
            return p;
        }
        p++;
    }

    *wasEnd = 1;
    return p;
}

static const char* skipToQuotation(const char* s, int* wasEnd)
{
    const char* p = s;
    while (*p != 0) {
        if (*p == '\"') {
            *wasEnd = 0;
            return p;
        }
        p++;
    }

    *wasEnd = 1;
    return p;
}

int clashSplitString(
    const char* s, char* temp, size_t maxCount, const char** out, size_t arrayCount)
{
    (void)maxCount;

    char* p = temp;

    int wasEnd = 0;

    const char* source = s;
    int index = 0;

    while (!wasEnd) {
        const char* start = skipWhitespace(source, &wasEnd);
        if (wasEnd) {
            break;
        }

        source = start;
        const char* end;
        if (*source == '\"') {
            source++;
            start++;
            end = skipToQuotation(source, &wasEnd);
            if (wasEnd) {
                return -1;
            }
            source = end + 1;
        } else {
            end = skipToWhitespace(source, &wasEnd);
            source = end;
        }
        size_t count = (size_t)(end - start);
        tc_memcpy_octets(p, start, count);
        if (index >= (int)arrayCount) {
            return -2;
        }
        out[index++] = p;
        p += count;
        *p = 0;
        p++;
    }

    *p = 0;

    return index;
}

static void insertIndentation(FldOutStream* stream, size_t indentation)
{
    for (size_t i = 0; i < indentation; ++i) {
        fldOutStreamWrites(stream, "    ");
    }
}

static void usageCommand(FldOutStream* stream, const struct ClashCommand* cmd, size_t indentation)
{
    fldOutStreamWritef(stream, "\n");
    insertIndentation(stream, indentation);
    fldOutStreamWritef(stream, "%s", cmd->name);
    if (cmd->subCommands != 0) {
        fldOutStreamWritef(stream, "\n");
        for (size_t i = 0; i < cmd->subCommandsCount; ++i) {
            const ClashCommand* subCommand = &cmd->subCommands[i];
            usageCommand(stream, subCommand, indentation + 1);
        }
        return;
    }

    for (size_t i = 0; i < cmd->optionCount; ++i) {
        const ClashOption* option = &cmd->options[i];
        if (option->type & ClashTypeArg) {
            fldOutStreamWritef(stream, " [%s]", option->name);
        } else {
            fldOutStreamWritef(stream, " [options]");
            break;
        }
    }

    fldOutStreamWritef(stream, "\n");

    for (size_t j = 0; j < cmd->optionCount; ++j) {
        insertIndentation(stream, indentation + 1);
        const ClashOption* option = &cmd->options[j];
        if (option->type & ClashTypeArg) {
            fldOutStreamWritef(stream, "   %-10s", option->name);
        } else {
            if (option->shortName != ' ' && option->shortName != 0) {
                fldOutStreamWritef(stream, "   -%c", option->shortName);
            }

            if (option->name != 0) {
                fldOutStreamWritef(stream, "   --%s", option->name);
            }
        }
        if (option->value != 0 && tc_strlen(option->value) > 0) {
            fldOutStreamWritef(stream, " default: '%s'", option->value);
        }

        fldOutStreamWritef(stream, "   %s\n", option->description);
    }
}

void clashUsageToStream(const ClashDefinition* definition, FldOutStream* outStream)
{
    for (size_t i = 0; i < definition->commandCount; ++i) {
        ClashCommand* cmd = &definition->commands[i];
        usageCommand(outStream, cmd, 0);
    }

    fldOutStreamWriteUInt8(outStream, 0);
}

const char* clashUsage(const ClashDefinition* definition, char* buf, size_t maxCount)
{
    FldOutStream outStream;
    fldOutStreamInit(&outStream, (uint8_t*)buf, maxCount);

    clashUsageToStream(definition, &outStream);

    return buf;
}
