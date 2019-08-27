// 
//  cblip.c
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

#include "cblip.h"
#include "ack_handler.h"
#include "msg_handler.h"
#include "hashset.h"
#include "types.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include <endian.h>

// Array mapping MessageType to a short mnemonic like "REQ".
static const char* const kMessageTypeNames[] = {
    "REQ", "RES", "ERR", "?3?",
    "ACKREQ", "AKRES", "?6?", "?7?"
};

blip_connection_t* blip_connection_new()
{
    blip_connection_t* retVal = malloc(sizeof(blip_connection_t));
    if (!retVal) {
        return NULL;
    }

    retVal->decompress_stream = malloc(sizeof(z_stream));
    retVal->recompress_stream = malloc(sizeof(z_stream));
    if (!retVal->decompress_stream || !retVal->recompress_stream) {
        return NULL;
    }

    memset(retVal->decompress_stream, 0, sizeof(z_stream));
    memset(retVal->recompress_stream, 0, sizeof(z_stream));
    int err = inflateInit2(retVal->decompress_stream, -MAX_WBITS);
    if (err != Z_OK) {
        return NULL;
    }

    err = deflateInit2(retVal->recompress_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL,
                         Z_DEFAULT_STRATEGY);
    if (err != Z_OK) {
        return NULL;
    }

    retVal->started_msg_set = hashset_create();
    if (!retVal->started_msg_set) {
        return NULL;
    }

    retVal->crc = 0;
    return retVal;
}

void blip_connection_free(blip_connection_t* connection)
{
    hashset_destroy(connection->started_msg_set);
    inflateEnd(connection->decompress_stream);
    free(connection->decompress_stream);
    free(connection);
}

blip_message_t* blip_message_new(const blip_connection_t* connection, uint8_t* data, size_t size)
{
    blip_message_t* retVal = malloc(sizeof(blip_message_t));
    if (!retVal) {
        return NULL;
    }

    retVal->private[3] = (uint64_t)malloc(size);
    if(!retVal->private[3]) {
        return NULL;
    }

    memcpy((void *)retVal->private[3], data, size);
    retVal->private[0] = (uint64_t)connection;
    retVal->private[2] = 0;
    uint8_t* pos = (uint8_t *)retVal->private[3];
    size_t rem = size;

    pos = get_varint(pos, &rem, &retVal->msg_no);
    uint64_t rawFlags;
    pos = get_varint(pos, &rem, &rawFlags);
    retVal->flags = (FrameFlags)(rawFlags & ~kTypeMask);
    retVal->type = (MessageType)(rawFlags & kTypeMask);
    if (retVal->type >= kAckRequestType) {
        handle_ack_msg(retVal, pos, rem);
    } else {
        const int rc = handle_normal_msg(retVal, pos, rem);
        if(rc < 0) {
            return NULL;
        }
    }

    return retVal;
}

void blip_message_free(blip_message_t* msg)
{
    if (msg->type < kAckRequestType) {
        free((void*)msg->private[1]); // decompressed payload (NULL if uncompressed)
    }

    free((void *)msg->private[2]);
    free((void *)msg->private[3]);
    free(msg);
}

const char* blip_get_message_type(const blip_message_t* msg)
{
    const MessageType type = msg->type;
    return kMessageTypeNames[type];
}

uint64_t blip_get_message_ack_size(const blip_message_t* msg)
{
    const MessageType type = msg->flags & kTypeMask;
    if (type >= kAckRequestType) {
        return msg->private[1];
    }

    return 0UL;
}

const uint8_t* blip_message_serialize(blip_message_t* msg, size_t* out_size) {
    *out_size = SizeOfVarInt(msg->msg_no);
    *out_size += SizeOfVarInt(msg->flags | msg->type);
    if(msg->type >= kAckRequestType) {
        return serialize_ack_msg(msg, out_size);
    }

    return serialize_normal_msg(msg, out_size);
}
