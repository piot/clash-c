/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/clash-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#ifndef CLASH_EXAMPLE_RESPONSE_H
#define CLASH_EXAMPLE_RESPONSE_H

#include <tinge/tinge.h>

typedef struct ClashResponse {
    struct FldOutStream* outStream;
    TingeState tintState;
} ClashResponse;

void clashResponseSetColor(ClashResponse* self, uint8_t colorIndex);
void clashResponseResetColor(ClashResponse* self);
int clashResponseWritef(ClashResponse* self, const char* fmt, ...);
int clashResponseWritecf(ClashResponse* self, uint8_t colorIndex, const char* fmt, ...);

#endif
