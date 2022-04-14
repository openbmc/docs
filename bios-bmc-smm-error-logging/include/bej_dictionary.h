#pragma once

#include "rde_common.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define DICTIONARY_TYPE_MASK 0x01
#define DICTIONARY_TYPE_SHIFT 1

    enum BejDictionaryType
    {
        BejPrimary = 0,
        BejAnnotation = 1,
    };

    struct BejDictionaryProperty
    {
        struct BejTupleF format;
        uint16_t SequenceNumber;
        uint16_t ChildPointerOffset;
        uint16_t ChildCount;
        uint8_t NameLength;
        uint16_t NameOffset;
    } __attribute__((__packed__));

    struct BejDictionaryHeader
    {
        uint8_t versionTag;
        uint8_t truncationFlag : 1;
        uint8_t reservedFlags : 7;
        uint16_t entryCount;
        uint32_t schemaVersion;
        uint32_t dictionarySize;
    } __attribute__((__packed__));

    uint16_t bejDictGetPropertyHeadOffset();
    uint16_t bejDictGetFirstAnnotatedPropertyOffset();
    int bejDictGetProperty(const uint8_t* dictionary,
                           uint16_t startingPropertyOffset,
                           uint16_t sequenceNumber,
                           const struct BejDictionaryProperty** property);
    const char* bejDictGetPropertyName(const uint8_t* dictionary,
                                       uint16_t nameOffset, uint8_t nameLength);

#ifdef __cplusplus
}
#endif