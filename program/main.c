// 
//  main.c
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/*
 * A simple program that reads BLIP packets from a directory and deserializes them.
 * Used to check the correctness of the library
 */

static uint8_t* read_file(const char* path, size_t* length)
{
    FILE* fin = fopen(path, "rb");
    if (fin == NULL) {
        return NULL;
    }

    fseek(fin, 0, SEEK_END);
    *length = ftell(fin);
    rewind(fin);

    uint8_t* buffer = malloc(*length);
    fread(buffer, *length, 1, fin);
    fclose(fin);
    return buffer;
}

static char* flags_to_str(FrameFlags flags)
{
    char* retVal = malloc(38);
    size_t current_size = 0;
    if (flags & kCompressed) {
        current_size = strlen("Compressed");
        strcpy(retVal, "Compressed");
    }

    if (flags & kUrgent) {
        if (current_size == 0) {
            current_size = strlen("Urgent");
            strcpy(retVal, "Urgent");
        } else {
            strcpy(retVal + current_size, "|Urgent");
            current_size += strlen("|Urgent");
        }
    }

    if (flags & kNoReply) {
        if (current_size == 0) {
            current_size = strlen("NoReply");
            strcpy(retVal, "NoReply");
        } else {
            strcpy(retVal + current_size, "|NoReply");
            current_size += strlen("|NoReply");
        }
    }

    if (flags & kMoreComing) {
        if (current_size == 0) {
            strcpy(retVal, "MoreComing");
        } else {
            strcpy(retVal + current_size, "|MoreComing");
        }
    }

    if (current_size == 0) {
        strcpy(retVal, "None");
    }

    return retVal;
}

static char* gets_nonewline(char* buffer, int size)
{
    if(!fgets(buffer, size, stdin)) {
        return NULL;
    }

    char* pos;
    if((pos = strchr(buffer, '\n')) != NULL) {
        *pos = 0;
    }

    return buffer;
}

int main(int argc, char** argv)
{
    blip_connection_t* connection = blip_connection_new();
    if(connection == NULL) {
        return -1;
    }

    int i = 1;
    char buffer[1040];
    char packet_dir[1024];
    printf("Enter the directory with BLIP Packets: ");
    gets_nonewline(packet_dir, 1024);
    
    while (true) {
        const int l = sprintf(buffer, "%s\\BLIP_Packet%d", packet_dir, i);
        buffer[l] = 0;

        size_t length;
        uint8_t* data = read_file(buffer, &length);
        if (!data) {
            break;
        }

        blip_message_t* msg = blip_message_new(connection, data, length);
        if(!msg) {
            return -2;
        }

        char* flags_str = flags_to_str(msg->flags);

        printf("Message Number:\t\t%llu\n", msg->msg_no);
        printf("Message Flags:\t\t%s\n", flags_str);
        printf("Message Type:\t\t%s\n", blip_get_message_type(msg));
        printf("Message Properties:\t%s\n", msg->properties);
        printf("Message Body:\t\t%.*s\n", msg->body_size, msg->body);
        printf("Message Checksum:\t%u\n", msg->checksum);
        if(msg->calculated_checksum == msg->checksum) {
            printf("Checksum OK\n\n");
        } else {
            printf("Checksum mismatch, got %d but expected %d\n\n", msg->calculated_checksum, msg->checksum);
        }

        free(flags_str);
        size_t reencoded_length;
        uint8_t* reencoded = blip_message_serialize(msg, &reencoded_length);
        if(length != reencoded_length) {
            printf("Mismatch data length during reencode (original %li, result %"PRIu64, length, reencoded_length);
        }
        int result = memcmp(data, reencoded, length);
        if(result != 0) {
            for(size_t i = 0; i < length; i++) {
                if(data[i] != reencoded[i]) {
                    printf("%02X != %02X at %"PRIu64, data[i], reencoded[i], i);
                }
            }
        } else {
            printf("Successful reencode\r\n");
        }
        blip_message_free(msg);
        i++;
    }

    blip_connection_free(connection);
}
