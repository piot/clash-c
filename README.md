<div align="center">
<img src="docs/images/clash.svg" width="192" />
</div>

# Clash

Command parser written in C99.

## Usage

```c
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

```

Example:
```c
uint8_t tempResponse[512];
FldOutStream responseOut;
fldOutStreamInit(&responseOut, tempResponse, 512);

int errorCode = clashParseString(def, "record start somefile.mkv -vv", NULL, &responseOut);
```
