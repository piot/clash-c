/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/clash-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <clash/clash.h>
#include <clash/response.h>
#include <clog/clog.h>
#include <clog/console.h>
#include <flood/out_stream.h>
#include <stddef.h>
#include <stdio.h>

clog_config g_clog;

typedef struct RecordStartCmd {
    int verbose;
    const char* filename;
} RecordStartCmd;

typedef struct RecordStopCmd {
    int verbose;
} RecordStopCmd;

typedef struct App {
    const char* secret;
} App;

static void onRecordStart(App* self, const RecordStartCmd* data, ClashResponse* response)
{
    clashResponseWritecf(response, 3, "\nrecord start: %s '", self->secret);
    clashResponseWritecf(response, 1, "%s", data->filename);
    clashResponseResetColor(response);
    clashResponseWritef(response, "'");
    clashResponseWritecf(response, 18, " verbose:%d\n", data->verbose);
}

static void onRecordStop(App* self, const RecordStopCmd* data, ClashResponse* response)
{
    (void)self;

    clashResponseWritecf(response, 22, "\nrecord stop:  %d\n\n", data->verbose);
}

static ClashOption recordStartOptions[]
    = { { "name", 'n', "the file name to store capture to", ClashTypeString | ClashTypeArg,
            "somefile.swamp-capture", offsetof(RecordStartCmd, filename) },
          { "verbose", 'v', "enable detailed output", ClashTypeFlag, "",
              offsetof(RecordStartCmd, verbose) } };

static ClashOption recordStopOptions[] = { { "verbose", 'v', "enable detailed output",
    ClashTypeFlag, "", offsetof(RecordStartCmd, verbose) } };

static ClashCommand recordCommands[] = {
    { "start", "start recording something", sizeof(struct RecordStartCmd), recordStartOptions,
        sizeof(recordStartOptions) / sizeof(recordStartOptions[0]), 0, 0, (ClashFn)onRecordStart },
    { "stop", "stops the current recording", sizeof(struct RecordStopCmd), recordStopOptions,
        sizeof(recordStopOptions) / sizeof(recordStopOptions[0]), 0, 0, (ClashFn)onRecordStop }
};

static ClashCommand mainCommands[] = { { "record", "recording commands", 0, 0, 0, recordCommands,
    sizeof(recordCommands) / sizeof(recordCommands[0]), 0 } };

static ClashDefinition definition
    = { mainCommands, sizeof(mainCommands) / sizeof(mainCommands[0]) };

int main(int argc, char* argv[])
{
    g_clog.log = clog_console;

    const char** adjustedArgs;
    int adjustedCount;

    if (argc <= 1) {
        static const char* arguments[] = { "record", "start", "-v", "myfile.swamp-capture" };
        adjustedArgs = (const char**)arguments;
        adjustedCount = sizeof(arguments) / sizeof(arguments[0]);
    } else {
        const char* ptr = argv[1];
        adjustedArgs = (const char**)&ptr;
        adjustedCount = argc - 1;
    }

    struct ClashDefinition* def = &definition;

    char usageBuf[512];
    printf("usage:\n%s\n", clashUsage(def, usageBuf, 512));

    uint8_t tempResponse[512];
    FldOutStream responseOut;
    fldOutStreamInit(&responseOut, tempResponse, 512);

    App app;
    app.secret = "VerySecret";

    int errorCode;
    if (adjustedCount == 1) {
        errorCode = clashParseString(def, adjustedArgs[0], &app, &responseOut);
    } else {
        errorCode = clashParse(def, adjustedArgs, adjustedCount, &app, &responseOut);
    }

    printf("response:\n%s", tempResponse);
    printf("errorCode:%d\n", errorCode);

    return errorCode;
}
