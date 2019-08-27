//
// BLIPProtocol.hh
//
// Copyright (c) 2017 Couchbase, Inc All rights reserved.
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
#include <stdint.h>

// https://github.com/couchbaselabs/BLIP-Cpp/blob/master/docs/BLIP%20Protocol.md

typedef enum {
    kRequestType     = 0,  // A message initiated by a peer
    kResponseType    = 1,  // A response to a Request
    kErrorType       = 2,  // A response indicating failure
    kAckRequestType  = 4,  // Acknowledgement of data received from a Request (internal)
    kAckResponseType = 5,  // Acknowledgement of data received from a Response (internal)
} MessageType;

typedef enum {
    kTypeMask   = 0x07,     // These 3 bits hold a MessageType
    kCompressed = 0x08,     // Message payload is gzip-deflated
    kUrgent     = 0x10,     // Message is given priority delivery
    kNoReply    = 0x20,     // Request only: no response desired
    kMoreComing = 0x40,     // Used only in frames, not in messages
} FrameFlags;


typedef uint64_t MessageNo;
typedef uint64_t MessageSize;