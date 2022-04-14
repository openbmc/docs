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

    /**
     * @brief Get the offset of the first property in a dictionary.
     *
     * @return the offset to the first property.
     */
    uint16_t bejDictGetPropertyHeadOffset();

    /**
     * @brief Get the offset of the first annotated property in an annoation
     * dictionary.
     *
     * @return the offset to the first annotated property in an annoation
     * dictionary.
     */
    uint16_t bejDictGetFirstAnnotatedPropertyOffset();

    /**
     * @brief Get the property related to the given sequence number.
     *
     * @param dictionary - dictionary containing the sequence number.
     * @param startingPropertyOffset - offset of the starting property for the
     * search.
     * @param sequenceNumber - sequence number of the property.
     * @param property - if the search is successful, this will point to a valid
     * property.
     * @return 0 if successful.
     */
    int bejDictGetProperty(const uint8_t* dictionary,
                           uint16_t startingPropertyOffset,
                           uint16_t sequenceNumber,
                           const struct BejDictionaryProperty** property);

    /**
     * @brief Get the name of a property.
     *
     * @param dictionary - dictionary containing the property.
     * @param nameOffset - dictionary offset of the name.
     * @param nameLength - length of the name.
     * @return a NULL terminated string. If the nameLength is 0, this will
     * return an empty string.
     */
    const char* bejDictGetPropertyName(const uint8_t* dictionary,
                                       uint16_t nameOffset, uint8_t nameLength);

#ifdef __cplusplus
}
#endif