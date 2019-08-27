//
// varint.c
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

#include "varint.h"
#ifndef MIN
# define MIN(A,B) ((A)<(B)?(A):(B))
#endif
#ifndef MAX
# define MAX(A,B) ((A)>(B)?(A):(B))
#endif

size_t SizeOfVarInt(uint64_t n) {
    size_t size = 1;
    while (n >= 0x80) {
        size++;
        n >>= 7;
    }
    return size;
}

size_t _GetUVarInt(uint8_t* buf, size_t size, uint64_t *n) {
    // NOTE: The public inline function GetUVarInt already decodes 1-byte varints,
    // so if we get here we can assume the varint is at least 2 bytes.
    uint8_t* pos = buf;
    uint8_t* end = pos + MIN(size, (size_t)kMaxVarintLen64);
    uint64_t result = *pos++ & 0x7F;
    int shift = 7;
    while (pos < end) {
        const uint8_t byte = *pos++;
        if (byte >= 0x80) {
            result |= (uint64_t)(byte & 0x7F) << shift;
            shift += 7;
        } else {
            result |= (uint64_t)byte << shift;
            *n = result;
            size_t nBytes = pos - buf;
            if (nBytes == kMaxVarintLen64 && byte > 1)
                nBytes = 0; // Numeric overflow
            return nBytes;
        }
    }
    return 0; // buffer too short
}

size_t _GetUVarInt32(uint8_t* buf, size_t size, uint32_t *n) {
    uint64_t n64;
    const size_t s = _GetUVarInt(buf, size, &n64);
    if (s == 0 || n64 > UINT32_MAX) // Numeric overflow
        return 0;
    *n = (uint32_t)n64;
    return s;
}

size_t PutUVarInt(void *buf, uint64_t n) {
    uint8_t* dst = (uint8_t*)buf;
    while (n >= 0x80) {
        *dst++ = (n & 0xFF) | 0x80;
        n >>= 7;
    }
    *dst++ = (uint8_t)n;
    return dst - (uint8_t*)buf;
}
