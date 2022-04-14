#include "bej_decoder_core.h"

#include "bej_dictionary.h"
#include "stdio.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

const uint32_t supportedBejVersions[] = {0xF1F0F000};

/**
 * @brief Get the integer value from BEJ byte stream.
 *
 * @param bytes - valid pointer to a byte stream in little-endian format.
 * @param numOfBytes - number of bytes belongs to the value.
 * @return signed 64bit representation of the value.
 */
static int64_t bejGetIntegerValue(const uint8_t* bytes, uint8_t numOfBytes)
{
    if (numOfBytes == 0)
    {
        return 0;
    }
    uint64_t value = rdeGetUnsignedInteger(bytes, numOfBytes);
    uint8_t bitsInVal = numOfBytes * 8;
    // Since numOfBytes > 0, bitsInVal is non negative.
    uint64_t mask = 1U << (uint8_t)(bitsInVal - 1);
    return (value ^ mask) - mask;
}

/**
 * @brief Get offsets of SFLV fields with respect to the enSegment start.
 *
 * @param enSegment - a valid pointer to a start of a bejTuple.
 * @param offsets - this will hold the local offsets.
 */
static void bejGetLocalBejSFLVOffsets(const uint8_t* enSegment,
                                      struct BejSFLVOffset* offsets)
{
    // Structure of the SFLV.
    //   [Number of bytes need to represent the sequence number] - uint8_t
    //   [SequenceNumber] - multi byte
    //   [Format] - uint8_t
    //   [Number of bytes need to represent the value length] - uint8_t
    //   [Value length] - multi byte

    // Number of bytes need to represent the sequence number.
    const uint8_t seqSize = *enSegment;
    // Start of format.
    const uint32_t formatOffset = sizeof(uint8_t) + seqSize;
    // Start of length of the value-length bytes.
    const uint32_t valueLenNnintOffset = formatOffset + sizeof(uint8_t);
    // Number of bytes need to represent the value length.
    const uint8_t valueLengthSize = *(enSegment + valueLenNnintOffset);
    // Start of the Value.
    const uint32_t valueOffset =
        valueLenNnintOffset + sizeof(uint8_t) + valueLengthSize;

    offsets->formatOffset = formatOffset;
    offsets->valueLenNnintOffset = valueLenNnintOffset;
    offsets->valueOffset = valueOffset;
}

/**
 * @brief Initialize sflv struct in params struct.
 *
 * @param params - a valid BejHandleTypeFuncParam struct with
 * params->state.encodedSubStream pointing to the start of the encoded stream
 * and params->state.encodedStreamOffset pointing to the current bejTuple.
 */
static void bejInitSFLVStruct(struct BejHandleTypeFuncParam* params)
{
    struct BejSFLVOffset localOffset;
    // Get offsets of different SFLV fields with respect to start of the encoded
    // segment.
    bejGetLocalBejSFLVOffsets(params->state.encodedSubStream, &localOffset);
    struct BejSFLV* sflv = &params->sflv;
    const uint32_t valueLength = (uint32_t)(rdeGetNnint(
        params->state.encodedSubStream + localOffset.valueLenNnintOffset));
    // Sequence number itself should be 16bits. Using 32bits for
    // [sequence_number + schema_type].
    uint32_t tupleS = (uint32_t)(rdeGetNnint(params->state.encodedSubStream));
    sflv->tupleS.schema = (uint8_t)(tupleS & DICTIONARY_TYPE_MASK);
    sflv->tupleS.sequenceNumber =
        (uint16_t)((tupleS & (~DICTIONARY_TYPE_MASK)) >> DICTIONARY_TYPE_SHIFT);
    sflv->format = *(struct BejTupleF*)(params->state.encodedSubStream +
                                        localOffset.formatOffset);
    sflv->valueLength = valueLength;
    sflv->valueEndOffset = params->state.encodedStreamOffset +
                           localOffset.valueOffset + valueLength;
    sflv->value = params->state.encodedSubStream + localOffset.valueOffset;
}

/**
 * @brief Get the offset to the first tuple of a bejArray or bejSet. The first
 * part of the value of a bejArray or a bejSet contains an nnint providing the
 * number of elements/tuples. Offset is with respect to the start of the encoded
 * stream.
 *
 * @param params - a valid BejHandleTypeFuncParam struct.
 * @return offset with respect to the start of the encoded stream.
 */
static uint32_t
    bejGetFirstTupleOffset(const struct BejHandleTypeFuncParam* params)
{
    struct BejSFLVOffset localOffset;
    // Get the offset of the value with respect to the current encoded segment
    // being decoded.
    bejGetLocalBejSFLVOffsets(params->state.encodedSubStream, &localOffset);
    return params->state.encodedStreamOffset + localOffset.valueOffset +
           rdeGetNnintSize(params->sflv.value);
}

/**
 * @brief Get the correct property and the dictionary it belongs to.
 *
 * @param params - a BejHandleTypeFuncParam struct pointing to valid
 * dictionaries.
 * @param schemaType - indicate whether to use the annotation dictionary or
 * the main schema dictionary.
 * @param sequenceNumber - sequence number to use for property search. Not
 * using the params->sflv.tupleS.sequenceNumber from the provided params struct.
 * @param dictionary - if the function is successful, this will point to a
 * valid dictionary to be used.
 * @param prop - if the function is successful, this will point to a valid
 * property in a dictionary.
 * @return 0 if successful.
 */
static int
    bejGetDictionaryAndProperty(const struct BejHandleTypeFuncParam* params,
                                uint8_t schemaType, uint32_t sequenceNumber,
                                const uint8_t** dictionary,
                                const struct BejDictionaryProperty** prop)
{
    uint16_t dictPropOffset;
    // We need to pick the correct dictionary.
    if (schemaType == BejPrimary)
    {
        *dictionary = params->mainDictionary;
        dictPropOffset = params->state.mainDictPropOffset;
    }
    else if (schemaType == BejAnnotation)
    {
        *dictionary = params->annotDictionary;
        dictPropOffset = params->state.annoDictPropOffset;
    }
    else
    {
        fprintf(stderr, "Failed to select a dictionary. schema type: %u\n",
                schemaType);
        return -1;
    }

    int ret =
        bejDictGetProperty(*dictionary, dictPropOffset, sequenceNumber, prop);
    if (ret != 0)
    {
        fprintf(stderr, "Failed to get dictionary property for offset: %u\n",
                dictPropOffset);
        return -1;
    }
    return 0;
}

/**
 * @brief Find and return the property name of the current encoded segment.
 *
 * @param params - a valid populated BejHandleTypeFuncParam.
 * @return 0 if successful.
 */
static const char* bejFindPropName(struct BejHandleTypeFuncParam* params)
{
    const uint8_t* dictionary;
    const struct BejDictionaryProperty* prop;
    int ret = bejGetDictionaryAndProperty(params, params->sflv.tupleS.schema,
                                          params->sflv.tupleS.sequenceNumber,
                                          &dictionary, &prop);
    if (ret != 0)
    {
        return "";
    }
    return bejDictGetPropertyName(dictionary, prop->NameOffset,
                                  prop->NameLength);
}

/**
 * @brief This figures out whether the current encoded segment marks a section
 * ending. If so, this function will update the decoder state and pop the stack
 * used to memorize endings. This function should be called after updating the
 * encodedStreamOffset to the end of decoded SFLV tuple.
 *
 * @param params - a valid BejHandleTypeFuncParam which contains the decoder
 * state.
 * @param canBeEmpty - if true, the stack being empty is not an error. If
 * false, stack cannot be empty.
 * @return 0 if successful.
 */
static int bejProcessEnding(struct BejHandleTypeFuncParam* params,
                            bool canBeEmpty)
{
    if (params->stackCallback->stackEmpty(params->stackDataPtr) &&
        (!canBeEmpty))
    {
        // If bejProcessEnding has been called after adding an appropriate JSON
        // property, then stack cannot be empty.
        fprintf(stderr, "Ending stack cannot be empty.\n");
        return -1;
    }

    while (!params->stackCallback->stackEmpty(params->stackDataPtr))
    {
        const struct BejStackProperty* const ending =
            params->stackCallback->stackPeek(params->stackDataPtr);
        // Check whether the current offset location matches the expected ending
        // offset. If so, we are done with that section.
        if (params->state.encodedStreamOffset == ending->streamEndOffset)
        {
            // Since we are going out of a section, we need to reset the
            // dictionary property offsets to this section's parent property
            // start.
            params->state.mainDictPropOffset = ending->mainDictPropOffset;
            params->state.annoDictPropOffset = ending->annoDictPropOffset;
            params->state.addPropertyName = ending->addPropertyName;

            if (ending->sectionType == BejSectionSet)
            {
                params->decodedCallback->callbackSetEnd(
                    params->callbaksDataPtr);
            }
            else if (ending->sectionType == BejSectionArray)
            {
                params->decodedCallback->callbackArrayEnd(
                    params->callbaksDataPtr);
            }
            params->stackCallback->stackPop(params->stackDataPtr);
        }
        else
        {
            params->decodedCallback->callbackPropertyEnd(
                params->callbaksDataPtr);
            // Do not change the parent dictionary property offset since we are
            // still inside the same section.
            return 0;
        }
    }
    return 0;
}

/**
 * @brief Check whether the current encoded segment being decoded is an array
 * element.
 *
 * @param params - a valid BejHandleTypeFuncParam struct.
 * @return true if the encoded segment is an array element. Else false.
 */
static bool bejIsArrayElement(const struct BejHandleTypeFuncParam* params)
{
    // If the encoded segment enters an array section, we are adding a
    // BejSectionArray to the stack. Therefore if the stack is empty, encoded
    // segment cannot be an array element.
    if (params->stackCallback->stackEmpty(params->stackDataPtr))
    {
        return false;
    }
    const struct BejStackProperty* const ending =
        params->stackCallback->stackPeek(params->stackDataPtr);
    // If the stack top element holds a BejSectionArray, encoded segment is
    // an array element.
    return ending->sectionType == BejSectionArray;
}

static int bejHandleBejSet(struct BejHandleTypeFuncParam* params)
{
    uint16_t sequenceNumber = params->sflv.tupleS.sequenceNumber;
    // Check whether this BejSet is an array element or not.
    if (bejIsArrayElement(params))
    {
        // Dictionary only contains an entry for element 0.
        sequenceNumber = 0;
    }
    const uint8_t* dictionary;
    const struct BejDictionaryProperty* prop;
    RETURN_IF_IERROR(
        bejGetDictionaryAndProperty(params, params->sflv.tupleS.schema,
                                    sequenceNumber, &dictionary, &prop));

    const char* propName;
    if (params->state.addPropertyName)
    {
        propName = bejDictGetPropertyName(dictionary, prop->NameOffset,
                                          prop->NameLength);
    }
    else
    {
        propName = "";
    }

    RETURN_IF_IERROR(params->decodedCallback->callbackSetStart(
        propName, params->callbaksDataPtr));

    uint64_t elements = rdeGetNnint(params->sflv.value);
    // If its an empty set, we are done here.
    if (elements == 0)
    {
        RETURN_IF_IERROR(
            params->decodedCallback->callbackSetEnd(params->callbaksDataPtr));
        goto bejEndSet;
    }

    // Update the states for the next encoding segment.
    struct BejStackProperty newEnding = {
        .sectionType = BejSectionSet,
        .addPropertyName = params->state.addPropertyName,
        .mainDictPropOffset = params->state.mainDictPropOffset,
        .annoDictPropOffset = params->state.annoDictPropOffset,
        .streamEndOffset = params->sflv.valueEndOffset,
    };
    RETURN_IF_IERROR(
        params->stackCallback->stackPush(&newEnding, params->stackDataPtr));
    params->state.addPropertyName = true;
    if (params->sflv.tupleS.schema == BejAnnotation)
    {
        // Since this set is an annotated type, we need to advance the
        // annotation dictionary for decoding the next segment.
        params->state.annoDictPropOffset = prop->ChildPointerOffset;
    }
    else
    {
        params->state.mainDictPropOffset = prop->ChildPointerOffset;
    }
bejEndSet:
    params->state.encodedStreamOffset = bejGetFirstTupleOffset(params);
    return 0;
}

static int bejHandleBejArray(struct BejHandleTypeFuncParam* params)
{
    const uint8_t* dictionary;
    const struct BejDictionaryProperty* prop;
    RETURN_IF_IERROR(bejGetDictionaryAndProperty(
        params, params->sflv.tupleS.schema, params->sflv.tupleS.sequenceNumber,
        &dictionary, &prop));

    const char* propName;
    if (params->state.addPropertyName)
    {
        propName = bejDictGetPropertyName(dictionary, prop->NameOffset,
                                          prop->NameLength);
    }
    else
    {
        propName = "";
    }

    RETURN_IF_IERROR(params->decodedCallback->callbackArrayStart(
        propName, params->callbaksDataPtr));

    uint64_t elements = rdeGetNnint(params->sflv.value);
    // If its an empty array, we are done here.
    if (elements == 0)
    {
        RETURN_IF_IERROR(
            params->decodedCallback->callbackArrayEnd(params->callbaksDataPtr));
        goto bejEndArray;
    }

    // Update the state for next segment decoding.
    struct BejStackProperty newEnding = {
        .sectionType = BejSectionArray,
        .addPropertyName = params->state.addPropertyName,
        .mainDictPropOffset = params->state.mainDictPropOffset,
        .annoDictPropOffset = params->state.annoDictPropOffset,
        .streamEndOffset = params->sflv.valueEndOffset,
    };
    RETURN_IF_IERROR(
        params->stackCallback->stackPush(&newEnding, params->stackDataPtr));
    // We do not add property names for array elements.
    params->state.addPropertyName = false;
    if (params->sflv.tupleS.schema == BejAnnotation)
    {
        // Since this array is an annotated type, we need to advance the
        // annotation dictionary for decoding the next segment.
        params->state.annoDictPropOffset = prop->ChildPointerOffset;
    }
    else
    {
        params->state.mainDictPropOffset = prop->ChildPointerOffset;
    }

bejEndArray:
    params->state.encodedStreamOffset = bejGetFirstTupleOffset(params);
    return 0;
}

static int bejHandleBejNull(struct BejHandleTypeFuncParam* params)
{
    const char* propName;
    if (params->state.addPropertyName)
    {
        propName = bejFindPropName(params);
    }
    else
    {
        propName = "";
    }
    RETURN_IF_IERROR(params->decodedCallback->callbackNull(
        propName, params->callbaksDataPtr));
    params->state.encodedStreamOffset = params->sflv.valueEndOffset;
    return bejProcessEnding(params, /*canBeEmpty=*/false);
}

static int bejHandleBejInteger(struct BejHandleTypeFuncParam* params)
{
    const char* propName;
    if (params->state.addPropertyName)
    {
        propName = bejFindPropName(params);
    }
    else
    {
        propName = "";
    }
    params->decodedCallback->callbackInteger(
        propName,
        bejGetIntegerValue(params->sflv.value, params->sflv.valueLength),
        params->callbaksDataPtr);
    params->state.encodedStreamOffset = params->sflv.valueEndOffset;
    return bejProcessEnding(params, /*canBeEmpty=*/false);
}

static int bejHandleBejEnum(struct BejHandleTypeFuncParam* params)
{
    uint16_t sequenceNumber = params->sflv.tupleS.sequenceNumber;
    if (bejIsArrayElement(params))
    {
        sequenceNumber = 0;
    }
    const uint8_t* dictionary;
    const struct BejDictionaryProperty* prop;
    RETURN_IF_IERROR(
        bejGetDictionaryAndProperty(params, params->sflv.tupleS.schema,
                                    sequenceNumber, &dictionary, &prop));

    const char* propName;
    if (params->state.addPropertyName)
    {
        propName = bejDictGetPropertyName(dictionary, prop->NameOffset,
                                          prop->NameLength);
    }
    else
    {
        propName = "";
    }

    // Get the string for enum value.
    uint16_t enumValueSequenceN = (uint16_t)(rdeGetNnint(params->sflv.value));
    const struct BejDictionaryProperty* enumValueProp;
    RETURN_IF_IERROR(bejDictGetProperty(dictionary, prop->ChildPointerOffset,
                                        enumValueSequenceN, &enumValueProp));
    const char* enumValueName = bejDictGetPropertyName(
        dictionary, enumValueProp->NameOffset, enumValueProp->NameLength);

    params->decodedCallback->callbackEnum(propName, enumValueName,
                                          params->callbaksDataPtr);
    params->state.encodedStreamOffset = params->sflv.valueEndOffset;
    return bejProcessEnding(params, /*canBeEmpty=*/false);
}

static int bejHandleBejString(struct BejHandleTypeFuncParam* params)
{
    // TODO: Handle deferred bindings.
    const char* propName;
    if (params->state.addPropertyName)
    {
        propName = bejFindPropName(params);
    }
    else
    {
        propName = "";
    }
    params->decodedCallback->callbackString(
        propName, (const char*)(params->sflv.value), params->callbaksDataPtr);
    params->state.encodedStreamOffset = params->sflv.valueEndOffset;
    return bejProcessEnding(params, /*canBeEmpty=*/false);
}

static int bejHandleBejReal(struct BejHandleTypeFuncParam* params)
{
    const char* propName;
    if (params->state.addPropertyName)
    {
        propName = bejFindPropName(params);
    }
    else
    {
        propName = "";
    }

    // Real value has the following format.
    // nnint      - Length of whole
    // bejInteger - whole (includes sign for the overall real number)
    // nnint      - Leading zero count for fract
    // nnint      - fract
    // nnint      - Length of exp
    // bejInteger - exp (includes sign for the exponent)
    uint8_t wholeByteLen = (uint8_t)rdeGetNnint(params->sflv.value);
    const uint8_t* wholeBejInt =
        params->sflv.value + rdeGetNnintSize(params->sflv.value);
    const uint8_t* fractZeroCountNnint = wholeBejInt + wholeByteLen;
    const uint8_t* fractNnint =
        fractZeroCountNnint + rdeGetNnintSize(fractZeroCountNnint);
    const uint8_t* lenExpNnint = fractNnint + rdeGetNnintSize(fractNnint);
    const uint8_t* expBejInt = lenExpNnint + rdeGetNnintSize(lenExpNnint);

    struct BejReal realValue;
    realValue.whole = bejGetIntegerValue(wholeBejInt, wholeByteLen);
    realValue.zeroCount = rdeGetNnint(fractZeroCountNnint);
    realValue.fract = rdeGetNnint(fractNnint);
    realValue.expLen = (uint8_t)rdeGetNnint(lenExpNnint);
    if (realValue.expLen != 0)
    {
        realValue.exp =
            bejGetIntegerValue(expBejInt, (uint8_t)rdeGetNnint(lenExpNnint));
    }
    params->decodedCallback->callbackReal(propName, &realValue,
                                          params->callbaksDataPtr);
    params->state.encodedStreamOffset = params->sflv.valueEndOffset;
    return bejProcessEnding(params, /*canBeEmpty=*/false);
}

static int bejHandleBejBoolean(struct BejHandleTypeFuncParam* params)
{
    const char* propName;
    if (params->state.addPropertyName)
    {
        propName = bejFindPropName(params);
    }
    else
    {
        propName = "";
    }
    RETURN_IF_IERROR(params->decodedCallback->callbackBool(
        propName, *(params->sflv.value) > 0, params->callbaksDataPtr));
    params->state.encodedStreamOffset = params->sflv.valueEndOffset;
    return bejProcessEnding(params, /*canBeEmpty=*/false);
}

static int bejHandleBejPropertyAnnotation(struct BejHandleTypeFuncParam* params)
{
    // Property annotation has the form OuterProperty@Annotation. First
    // processing the outer property name.
    const uint8_t* outerDictionary;
    const struct BejDictionaryProperty* outerProp;
    RETURN_IF_IERROR(bejGetDictionaryAndProperty(
        params, params->sflv.tupleS.schema, params->sflv.tupleS.sequenceNumber,
        &outerDictionary, &outerProp));

    const char* propName = bejDictGetPropertyName(
        outerDictionary, outerProp->NameOffset, outerProp->NameLength);
    RETURN_IF_IERROR(params->decodedCallback->callbackAnnotation(
        propName, params->callbaksDataPtr));

    // Mark the ending of the property annotation.
    struct BejStackProperty newEnding = {
        .sectionType = BejSectionNoType,
        .addPropertyName = params->state.addPropertyName,
        .mainDictPropOffset = params->state.mainDictPropOffset,
        .annoDictPropOffset = params->state.annoDictPropOffset,
        .streamEndOffset = params->sflv.valueEndOffset,
    };
    // Update the states for the next encoding segment.
    RETURN_IF_IERROR(
        params->stackCallback->stackPush(&newEnding, params->stackDataPtr));
    params->state.addPropertyName = true;
    // Can there be a property annotation to a annotation property? This assumes
    // it doesn't.
    params->state.mainDictPropOffset = outerProp->ChildPointerOffset;
    // Point to the start of the value for next decoding.
    params->state.encodedStreamOffset =
        params->sflv.valueEndOffset - params->sflv.valueLength;
    return 0;
}

/**
 * @brief Decodes an encoded bej stream.
 *
 * @param schemaDictionary - main schema dictionary to use.
 * @param annotationDictionary - annotation dictionary
 * @param enStream - encoded stream without the PLDM header.
 * @param streamLen - length of the enStream.
 * @param stackCallback - callbacks for stack handlers.
 * @param decodedCallback - callbacks for extracting decoded properties.
 * @param callbaksDataPtr - data pointer to pass to decoded callbacks. This can
 * be used pass additional data.
 * @param stackDataPtr - data pointer to pass to stack callbacks. This can be
 * used pass additional data.
 * @return 0 if successful.
 */
static int bejDecode(const uint8_t* schemaDictionary,
                     const uint8_t* annotationDictionary,
                     const uint8_t* enStream, uint32_t streamLen,
                     const struct BejStackCallback* stackCallback,
                     const struct BejDecodedCallback* decodedCallback,
                     void* callbaksDataPtr, void* stackDataPtr)
{
    struct BejHandleTypeFuncParam params = {0};

    params.stackCallback = stackCallback;
    params.decodedCallback = decodedCallback;
    params.callbaksDataPtr = callbaksDataPtr;
    params.stackDataPtr = stackDataPtr;

    // We only add names of set properties. We don't use names for array
    // properties. Here we are omitting the name of the root set.
    params.state.addPropertyName = false;
    // Current location of the encoded segment we are processing.
    params.state.encodedStreamOffset = 0;
    // At start, parent property from the main dictionary is the first property.
    params.state.mainDictPropOffset = bejDictGetPropertyHeadOffset();
    params.state.annoDictPropOffset = bejDictGetFirstAnnotatedPropertyOffset();

    params.annotDictionary = annotationDictionary;
    params.mainDictionary = schemaDictionary;

    while (params.state.encodedStreamOffset < streamLen)
    {
        // Go to the next encoded segment in the encoded stream.
        params.state.encodedSubStream =
            enStream + params.state.encodedStreamOffset;
        bejInitSFLVStruct(&params);

        switch (params.sflv.format.principalDataType)
        {
            case BejSet:
                RETURN_IF_IERROR(bejHandleBejSet(&params));
                break;
            case BejArray:
                RETURN_IF_IERROR(bejHandleBejArray(&params));
                break;
            case BejNull:
                RETURN_IF_IERROR(bejHandleBejNull(&params));
                break;
            case BejInteger:
                RETURN_IF_IERROR(bejHandleBejInteger(&params));
                break;
            case BejEnum:
                RETURN_IF_IERROR(bejHandleBejEnum(&params));
                break;
            case BejString:
                RETURN_IF_IERROR(bejHandleBejString(&params));
                break;
            case BejReal:
                RETURN_IF_IERROR(bejHandleBejReal(&params));
                break;
            case BejBoolean:
                RETURN_IF_IERROR(bejHandleBejBoolean(&params));
                break;
            case BejBytestring:
                // TODO: Add support for BejBytestring decoding.
                fprintf(stderr, "No BejBytestring support\n");
                params.state.encodedStreamOffset = params.sflv.valueEndOffset;
                break;
            case BejChoice:
                // TODO: Add support for BejChoice decoding.
                fprintf(stderr, "No BejChoice support\n");
                params.state.encodedStreamOffset = params.sflv.valueEndOffset;
                break;
            case BejPropertyAnnotation:
                RETURN_IF_IERROR(bejHandleBejPropertyAnnotation(&params));
                break;
            case BejResourceLink:
                // TODO: Add support for BejResourceLink decoding.
                fprintf(stderr, "No BejResourceLink support\n");
                params.state.encodedStreamOffset = params.sflv.valueEndOffset;
                break;
            case BejResourceLinkExpansion:
                // TODO: Add support for BejResourceLinkExpansion decoding.
                fprintf(stderr, "No BejResourceLinkExpansion support\n");
                params.state.encodedStreamOffset = params.sflv.valueEndOffset;
                break;
            default:
                break;
        };
    };
    RETURN_IF_IERROR(bejProcessEnding(&params, /*canBeEmpty=*/true));
    if (!params.stackCallback->stackEmpty(params.stackDataPtr))
    {
        fprintf(
            stderr,
            "Ending stack should be empty but its not. Something must have gone wrong with the encoding\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Check if a bej version is supported by this decoder
 *
 * @param bejVersion - the bej version in the received encoded stream
 * @return 0 if supported.
 */
static int bejIsSupported(uint32_t bejVersion)
{
    for (uint32_t i = 0; i < sizeof(supportedBejVersions) / sizeof(uint32_t);
         ++i)
    {
        if (bejVersion == supportedBejVersions[i])
        {
            return 0;
        }
    }
    fprintf(stderr, "Bej decoder doesn't support the bej version: %u\n",
            bejVersion);
    return -1;
}

int bejDecodePldmBlock(const struct BejDictionaries* dictionaries,
                       const uint8_t* encodedPldmBlock, uint32_t blockLength,
                       const struct BejStackCallback* stackCallback,
                       const struct BejDecodedCallback* decodedCallback,
                       void* callbaksDataPtr, void* stackDataPtr)
{
    uint32_t pldmHeaderSize = sizeof(struct BejPldmBlockHeader);
    if (blockLength < pldmHeaderSize)
    {
        fprintf(stderr, "Invalid pldm block size: %u\n", blockLength);
        return -1;
    }

    const struct BejPldmBlockHeader* pldmHeader =
        (const struct BejPldmBlockHeader*)encodedPldmBlock;
    RETURN_IF_IERROR(bejIsSupported(pldmHeader->bejVersion));
    if (pldmHeader->schemaClass == BejAnnotaionSchemaClass)
    {
        fprintf(stderr,
                "Encoder schema class cannot be BejAnnotaionSchemaClass\n");
        return -1;
    }
    // TODO: Add support for CollectionMemberType schema class.
    if (pldmHeader->schemaClass == BejCollectionMemberTypeSchemaClass)
    {
        fprintf(
            stderr,
            "Decoder doesn't support BejCollectionMemberTypeSchemaClass yet.\n");
        return -1;
    }
    // TODO: Add support for Error schema class.
    if (pldmHeader->schemaClass == BejErrorSchemaClass)
    {
        fprintf(stderr, "Decoder doesn't support BejErrorSchemaClass yet.\n");
        return -1;
    }

    // Skip the PLDM header.
    const uint8_t* enStream = encodedPldmBlock + pldmHeaderSize;
    uint32_t streamLen = blockLength - pldmHeaderSize;
    return bejDecode(dictionaries->schemaDictionary,
                     dictionaries->annotationDictionary, enStream, streamLen,
                     stackCallback, decodedCallback, callbaksDataPtr,
                     stackDataPtr);
}