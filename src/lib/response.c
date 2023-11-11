/*----------------------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved. https://github.com/piot/clash-c
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------------------*/
#include <clash/response.h>
#include <flood/out_stream.h>

void clashResponseSetColor(ClashResponse* self, uint8_t colorIndex)
{
    tingeStateFgColorIndex(&self->tintState, colorIndex);
}

void clashResponseResetColor(ClashResponse* self)
{
    tingeStateReset(&self->tintState);
}

int clashResponseWritef(ClashResponse* self, const char* fmt, ...)
{
    va_list pl;

    va_start(pl, fmt);
    int ret = fldOutStreamWritevf(self->outStream, fmt, pl);
    va_end(pl);
    return ret;
}

int clashResponseWritecf(ClashResponse* self, uint8_t colorIndex, const char* fmt, ...)
{
    va_list pl;

    clashResponseSetColor(self, colorIndex);
    va_start(pl, fmt);
    int ret = fldOutStreamWritevf(self->outStream, fmt, pl);
    va_end(pl);
    return ret;
}
