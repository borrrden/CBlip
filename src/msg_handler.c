// 
//  msg_handler.c
// 
//  Copyright (c) 2018 Couchbase, Inc All rights reserved.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
// 
//  http://www.apache.org/licenses/LICENSE-2.0
// 
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// 

#include "msg_handler.h"
#include "hashset.h"
#include "types.h"
#include "endian.h"
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <stdlib.h>

#define BLIP_BODY_CHECKSUM_SIZE 4

static int blip_connection_saw_msg(blip_connection_t* connection, blip_message_t* msg)
{
    const hashset_t set = connection->started_msg_set;
    const bool found = hashset_is_member(set, &msg->msg_no);
    if (!found) {
        MessageNo* ptr = (MessageNo*)malloc(sizeof(MessageNo));
        if (ptr == NULL) {
            return -1;
        }

        *ptr = msg->msg_no;
        return hashset_add(set, ptr) == 0;
    }

    return 1;
}

static uint8_t* compress_body(blip_connection_t* connection, uint8_t* data, size_t size, size_t* out_size)
{
    z_stream* compress_stream = connection->recompress_stream;
    compress_stream->next_in = data;
    compress_stream->avail_in = (uInt)size;
    compress_stream->next_out = connection->recompress_buffer;
    compress_stream->avail_out = (uInt)16384;

    const uLong start = compress_stream->total_out;
    int err = deflate(compress_stream, Z_SYNC_FLUSH);
    if (err < 0) {
        printf("Error compressing: %d\n", err);
        return NULL;
    }

    *out_size = compress_stream->total_out - start - 4; // Cut off trailer
    uint8_t* compressed = malloc(*out_size);
    memcpy(compressed, connection->recompress_buffer, *out_size);
    return compressed;
}

static uint8_t* decompress_body(blip_connection_t* connection, uint8_t* data, size_t size, size_t* out_size)
{
    static Byte trailer[4] = {0x00, 0x00, 0xff, 0xff};
    z_stream* decompress_stream = connection->decompress_stream;
    decompress_stream->next_in = data;
    decompress_stream->avail_in = (uInt)size;
    decompress_stream->next_out = connection->decompress_buffer;
    decompress_stream->avail_out = (uInt)16384;

    const uLong start = decompress_stream->total_out;
    int err = inflate(decompress_stream, Z_NO_FLUSH);
    if (err < 0) {
        printf("Error decompressing first step: %d\n", err);
        return NULL;
    }

    decompress_stream->next_in = trailer;
    decompress_stream->avail_in = 4;
    err = inflate(decompress_stream, Z_SYNC_FLUSH);
    if (err < 0) {
        printf("Error decompressing second step: %d\n", err);
        return NULL;
    }

    *out_size = decompress_stream->total_out - start;
    uint8_t* decompressed = malloc(*out_size);
    memcpy(decompressed, connection->decompress_buffer, *out_size);
    return decompressed;
}

static void transform_properties(uint8_t* data, size_t size, bool reverse)
{
    if(size == 0) {
        return;
    }

    uint8_t to = ':';
    uint8_t from = 0;
    if(reverse) {
        to = 0;
        from = ':';
    }

    for(size_t i = 0; i < size - 1; i++) {
        if(data[i] == from) {
            data[i] = to;
        }
    }
}

int handle_normal_msg(blip_message_t* msg, uint8_t* data, size_t size)
{
    blip_connection_t* connection = (blip_connection_t*)msg->private[0];
    const int isFound = blip_connection_saw_msg(connection, msg);
    if (isFound < 0) {
        return isFound;
    }

    const bool isCompressed = msg->flags & kCompressed;
    uint8_t* data_to_use = data;
    size_t size_to_use = size;
    if (isCompressed) {
        data_to_use = decompress_body(connection, data, size - BLIP_BODY_CHECKSUM_SIZE, &size_to_use);
        msg->private[1] = (uint64_t)data_to_use;
    } else {
        msg->private[1] = 0ULL;
    }

    uint8_t* data_pos = data_to_use;
    size_t remaining = size_to_use;
    uint64_t properties_length;
    if (isFound == 0) {
        data_pos = get_varint(data_pos, &remaining, &properties_length);
        msg->properties = properties_length > 0 ? data_pos : NULL;
    } else {
        msg->properties = 0;
    }

    data_pos += properties_length;
    remaining -= properties_length;
    if (!isCompressed && remaining >= BLIP_BODY_CHECKSUM_SIZE) {
        remaining -= BLIP_BODY_CHECKSUM_SIZE;
    }

    msg->body = data_pos;
    msg->body_size = remaining;

    int* checksum_area = (int *)(data + size - BLIP_BODY_CHECKSUM_SIZE);
    msg->checksum = _decBig32(*checksum_area);
    const size_t checksum_exclusion = isCompressed ? 0 : BLIP_BODY_CHECKSUM_SIZE;
    msg->calculated_checksum = crc32(connection->crc, data_to_use, (uInt)(size_to_use - checksum_exclusion));
    connection->crc = msg->checksum;

    // This needs to happen last, after the checksum is calculated since it changes the data
    transform_properties(msg->properties, (size_t)properties_length, false);

    return 0;
}

uint8_t* serialize_normal_msg(blip_message_t* msg, size_t* out_size) {
    size_t prop_size = msg->properties ? strlen((const char*)msg->properties) + 1 : 0;
    *out_size += SizeOfVarInt(prop_size) + prop_size + msg->body_size + 4;
    uint8_t* writeBuf = (uint8_t*)malloc(*out_size);
    uint8_t* pos = put_varint(msg->msg_no, writeBuf);
    uint8_t* payload = pos = put_varint(msg->flags | msg->type, pos);
    pos = put_varint(prop_size, pos);
    transform_properties(msg->properties, prop_size, true);
    memcpy(pos, msg->properties, prop_size);
    pos += prop_size;
    memcpy(pos, msg->body, msg->body_size);
    pos += msg->body_size;
    if(msg->flags & kCompressed) {
        size_t finalSize;
        uint8_t* compressed = compress_body((blip_connection_t *)msg->private[0], payload, pos - payload, &finalSize);
        *out_size = payload - writeBuf + finalSize + 4;
        uint8_t* finalWriteBuf = pos = (uint8_t*)malloc(*out_size);
        memcpy(pos, writeBuf, payload - writeBuf);
        pos += (payload - writeBuf);
        memcpy(pos, compressed, finalSize);
        pos += finalSize;
        free(writeBuf);
        writeBuf = finalWriteBuf;
    }

    (*(int*)pos) = _encBig32(msg->checksum);
    msg->private[2] = (uint64_t)writeBuf;
    return writeBuf;
}
