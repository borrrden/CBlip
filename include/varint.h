//
// varint.h
//
// Copyright (c) 2014 Couchbase, Inc All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Based on varint implementation from the Go language (src/pkg/encoding/binary/varint.go)
// This file implements "varint" encoding of unsigned 64-bit integers.
// The encoding is:
// - unsigned integers are serialized 7 bits at a time, starting with the
//   least significant bits
// - the most significant bit (msb) in each output byte indicates if there
//   is a continuation byte (msb = 1)


/** MaxVarintLenN is the maximum length of a varint-encoded N-bit integer. */
enum {
    kMaxVarintLen16 = 3,
    kMaxVarintLen32 = 5,
    kMaxVarintLen64 = 10,
};

/** Returns the number of bytes needed to encode a specific integer. */
size_t SizeOfVarInt(uint64_t n);

/** Encodes n as a varint, writing it to buf. Returns the number of bytes written. */
size_t PutUVarInt(void *buf, uint64_t n);

size_t _GetUVarInt(uint8_t* buf, size_t size, uint64_t *n);   // do not call directly
size_t _GetUVarInt32(uint8_t* buf, size_t size, uint32_t *n); // do not call directly

/** Decodes a varint from the bytes in buf, storing it into *n.
    Returns the number of bytes read, or 0 if the data is invalid (buffer too short or number
    too long.) */
static inline size_t GetUVarInt(uint8_t* buf, size_t size, uint64_t *n) {
    if (size == 0)
        return 0;
    uint8_t byte = buf[0];
    if (byte < 0x80) {
        *n = byte;
        return 1;
    }
    return _GetUVarInt(buf, size, n);
}

/** Decodes a varint from the bytes in buf, storing it into *n.
    Returns the number of bytes read, or 0 if the data is invalid (buffer too short or number
    too long.) */
static inline size_t GetUVarInt32(uint8_t* buf, size_t size, uint32_t *n) {
    if (size == 0)
        return 0;
    uint8_t byte = buf[0];
    if (byte < 0x80) {
        *n = byte;
        return 1;
    }
    return _GetUVarInt32(buf, size, n);
}