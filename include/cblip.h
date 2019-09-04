// 
//  cblip.h
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
#include "blipprotocol.h"
#include "varint.h"

#ifdef _MSC_VER
#ifdef CBlip_EXPORTS
#define CBLIP_API __declspec(dllexport)
#else
#define CBLIP_API __declspec(dllimport)
#endif
#else
#define CBLIP_API __attribute__ ((visibility ("default")))
#endif

/** A connection handle, created by blip_connection_new() */
typedef struct blip_connection blip_connection_t;

/** A message received from a BLIP connection */
struct blip_message
{
    MessageNo msg_no;               ///< The message number that this message contains info for
    MessageType type;               ///< The type of message
    FrameFlags flags;               ///< The flags on this message (for information such as Urgent, NoReply, etc)
    uint8_t* properties;            ///< The properties data on the object (A colon separated key-value string, null terminated)
    uint8_t* body;                  ///< The message body of this message (JSON string, *not* null terminated)
    size_t body_size;               ///< The size of the message body
    int32_t checksum;               ///< The checksum for this message in the context of the entire connection
    int32_t calculated_checksum;    ///< The checksum that was calculated from the connection (if this doesn't match checksum, this message is not valid)
    uint64_t private[4];            ///< Internal use information
};

/**
 * Extracts a varint type from the given data
 * @param data      The data to extract the varint from
 * @param size      On the way in, stores the length of the passed data.  
 *                  On the way out, stores the length of the returned data.
 * @param outVar    Stores the decoded unsigned 64-bit value
 * @return          A pointer to the remaining data
 */
static inline uint8_t* get_varint(uint8_t* data, size_t* size, uint64_t* outVar)
{
    const size_t c = GetUVarInt(data, *size, outVar);
    *size = *size - c;
    return data + c;
}

/**
 * Encodes a varint type into the given data
 * @param varint    The integer to encode
 * @param data      The data to write to (must have enough space to write!)
 * @return          The pointer to the position after the last byte was written
 */
static inline uint8_t* put_varint(uint64_t varint, uint8_t* data) {
    const size_t c = PutUVarInt(data, varint);
    return data + c;
}

/** A message received from a BLIP connection */
typedef struct blip_message blip_message_t;


/***********************
 * BLIP Connection API *
 **********************/


/**
 * Creates a new BLIP connection object.  All messages going from a given IP to another
 * given IP must be created from the same connection.
 * @return The initialized connection object
 */
CBLIP_API blip_connection_t* blip_connection_new();

/**
 * Frees the memory associated with a BLIP connection object
 * @param connection The connection to free
 */
CBLIP_API void blip_connection_free(blip_connection_t* connection);

/********************
 * BLIP Message API *
 *******************/

/**
 * Creates a new empty BLIP message object
 * @return The created message object
 */
CBLIP_API blip_message_t* blip_message_new();

/**
 * Creates a new BLIP message object
 * @param connection    The connection to derive the message from
 * @param data          The raw data received over the wire
 * @param size          The size of the received data
 * @return              The created message object
 */
CBLIP_API blip_message_t* blip_message_read(const blip_connection_t* connection, uint8_t* data, size_t size);

/**
 * Frees the memory associated with a BLIP message
 * @param msg The message to free
 */
CBLIP_API void blip_message_free(blip_message_t* msg);

/**
 * Gets the message type of a given message as a string
 * @param msg   The message to extract the type string from
 * @return      The type of the BLIP message
 */
CBLIP_API const char* blip_get_message_type(const blip_message_t* msg);

/**
 * Gets the number of acknowledged bytes that an ACK type message reported
 * (Calling this method on a non-ACK type message returns 0)
 * @param msg   The ACK message to read the acknowledged byte count from
 * @return      The acknowledged byte count
 */
CBLIP_API uint64_t blip_get_message_ack_size(const blip_message_t* msg);

/**
 * Serializes a BLIP message into raw bytes for transport
 * @param connection    The connection to use during serialization (CRC / GZIP)
 * @param msg           The message to serialize
 * @param out_size      On successful completion, contains the size of the returned bytes
 * @return              The encoded BLIP message as a pointer to byte data
 */
CBLIP_API const uint8_t* blip_message_serialize(blip_connection_t* connection, blip_message_t* msg, size_t* out_size);