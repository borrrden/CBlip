#pragma once
#include "hashset.h"
#include <stdint.h>
#include <zlib.h>

struct blip_connection
{
    z_stream* decompress_stream;
    z_stream* recompress_stream;
    uint32_t crc;
    uint32_t crc_out;
    hashset_t started_msg_set;
    Byte decompress_buffer[16384];
    Byte recompress_buffer[16384];
};