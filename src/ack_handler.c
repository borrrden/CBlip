// 
//  ack_handler.c
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

#include "ack_handler.h"
#include <stdlib.h>

void handle_ack_msg(blip_message_t* msg, uint8_t* data, size_t size)
{
    // Nothing to do here except store the ack bytes size
    uint64_t ack_size;
    uint8_t* pos = data;
    size_t rem = size;
    pos = get_varint(pos, &rem, &ack_size);
    msg->private[1] = ack_size;
}

uint8_t* serialize_ack_msg(blip_message_t* msg, size_t* out_size) {
    *out_size += SizeOfVarInt(msg->private[1]);
    uint8_t* writeBuf = (uint8_t*)malloc(*out_size);
    uint8_t* pos = put_varint(msg->msg_no, writeBuf);
    pos = put_varint(msg->flags | msg->type, pos);
    pos = put_varint(msg->private[1], pos);
    msg->private[2] = (uint64_t)writeBuf;

    return writeBuf;
}
