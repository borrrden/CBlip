//
// endian.h
//
// Copyright (c) 2015 Couchbase, Inc All rights reserved.
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

#include "forestdb_endian.h"

#ifndef _LITTLE_ENDIAN
    // convert to little endian
    #define _encLittle64(v) bitswap64(v)
    #define _decLittle64(v) bitswap64(v)
    #define _encLittle32(v) bitswap32(v)
    #define _decLittle32(v) bitswap32(v)
    #define _encLittle16(v) bitswap16(v)
    #define _decLittle16(v) bitswap16(v)
    #define _encBig64(v) (v)
    #define _decBig64(v) (v)
    #define _encBig32(v) (v)
    #define _decBig32(v) (v)
    #define _encBig16(v) (v)
    #define _decBig16(v) (v)
#else
    #define _encLittle64(v) (v)
    #define _decLittle64(v) (v)
    #define _encLittle32(v) (v)
    #define _decLittle32(v) (v)
    #define _encLittle16(v) (v)
    #define _decLittle16(v) (v)
    #define _encBig64(v) bitswap64(v)
    #define _decBig64(v) bitswap64(v)
    #define _encBig32(v) bitswap32(v)
    #define _decBig32(v) bitswap32(v)
    #define _encBig16(v) bitswap16(v)
    #define _decBig16(v) bitswap16(v)
#endif
