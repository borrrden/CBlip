// 
//  msg_handler.h
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

#pragma once
#include "cblip.h"

/**
 * Processes a non-ACK style BLIP message
 * @param msg   The message received over the wire
 * @param data  The data contained in the frame body
 * @param size  The size of the data contained in the frame body
 * @returns     0 on success, negative values on failure
 */
int handle_normal_msg(blip_message_t* msg, uint8_t* data, size_t size);

/**
 * Serializes a non-ACK type BLIP message
 * @param connection    The connection to use during serialization (CRC / GZIP)
 * @param msg           The message to serialize
 * @param out_size      Holds the size of the returned data on completion
 * @return              The serialized byte data of the message
 */
uint8_t* serialize_normal_msg(blip_connection_t* connection, blip_message_t* msg, size_t* out_size);