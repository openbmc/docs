#include "bej_dictionary.h"

#include <stdbool.h>
#include <stdio.h>

uint16_t bejDictGetPropertyHeadOffset()
{
    // First property is present soon after the dictionary header.
    return sizeof(struct BejDictionaryHeader);
}

uint16_t bejDictGetFirstAnnotatedPropertyOffset()
{
    // First property is present is "Annotations" set. Next immediate property
    // is the First valid property.
    return sizeof(struct BejDictionaryHeader) +
           sizeof(struct BejDictionaryProperty);
}

static uint16_t bejGetPropertyEntryIndex(uint16_t propertyOffset)
{
    return (propertyOffset - bejDictGetPropertyHeadOffset()) /
           sizeof(struct BejDictionaryProperty);
}

static bool bejValidatePropertyOffset(uint16_t propertyOffset)
{
    // propertyOffset should be a multiple of sizeof(BejDictionaryProperty)
    // starting from first property within the dictionary.
    if ((propertyOffset - bejDictGetPropertyHeadOffset()) %
        sizeof(struct BejDictionaryProperty))
    {
        fprintf(stderr, "Invalid property offset.\n");
        return false;
    }
    return true;
}

int bejDictGetProperty(const uint8_t* dictionary,
                       uint16_t startingPropertyOffset, uint16_t sequenceNumber,
                       const struct BejDictionaryProperty** property)
{
    uint16_t propertyOffset = startingPropertyOffset;
    const struct BejDictionaryHeader* header =
        (const struct BejDictionaryHeader*)dictionary;

    if (!bejValidatePropertyOffset(propertyOffset))
    {
        return -1;
    }
    uint16_t propertyIndex = bejGetPropertyEntryIndex(propertyOffset);

    for (uint16_t index = propertyIndex; index < header->entryCount; ++index)
    {
        const struct BejDictionaryProperty* p =
            (const struct BejDictionaryProperty*)(dictionary + propertyOffset);
        if (p->SequenceNumber == sequenceNumber)
        {
            *property = p;
            return 0;
        }
        propertyOffset += sizeof(struct BejDictionaryProperty);
    }
    return -1;
}

const char* bejDictGetPropertyName(const uint8_t* dictionary,
                                   uint16_t nameOffset, uint8_t nameLength)
{
    if (!nameLength)
    {
        return "";
    }
    return (const char*)(dictionary + nameOffset);
}