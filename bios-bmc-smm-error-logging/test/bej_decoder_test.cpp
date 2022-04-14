#include "bej_decoder.hpp"
#include "bej_test_inputs.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <optional>

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace rde
{

struct BejDecoderTestParams
{
    const std::string testName;
    const BejTestInputFiles inputFiles;
};

using BejDecoderTest = testing::TestWithParam<BejDecoderTestParams>;

// Buffer size for storing a single binary file data.
constexpr uint32_t maxBufferSize = 16 * 1024;

std::streamsize readBinaryFile(const char* fileName, uint8_t* buffer,
                               uint32_t bufferSize)
{
    std::ifstream inputStream(fileName, std::ios::binary);
    if (!inputStream.is_open())
    {
        std::cout << "Cannot open file: " << fileName << "\n";
        return 0;
    }
    auto readLength =
        inputStream.readsome(reinterpret_cast<char*>(buffer), bufferSize);
    if (inputStream.peek() != EOF)
    {
        std::cout << "Failed to read the complete file: " << fileName
                  << "  read length: " << readLength << "\n";
        return 0;
    }
    return readLength;
}

std::optional<BejTestInputs> loadInputs(const BejTestInputFiles& files,
                                        bool readErrorDictionary = false)
{
    std::ifstream jsonInput(files.jsonFile);
    if (!jsonInput.is_open())
    {
        std::cout << "Cannot open file: " << files.jsonFile << "\n";
        return std::nullopt;
    }
    nlohmann::json expJson;
    jsonInput >> expJson;

    static uint8_t schemaDictBuffer[maxBufferSize];
    if (readBinaryFile(files.schemaDictionaryFile, schemaDictBuffer,
                       maxBufferSize) == 0)
    {
        return std::nullopt;
    }

    static uint8_t annoDictBuffer[maxBufferSize];
    if (readBinaryFile(files.annotationDictionaryFile, annoDictBuffer,
                       maxBufferSize) == 0)
    {
        return std::nullopt;
    }

    static uint8_t encBuffer[maxBufferSize];
    auto encLen =
        readBinaryFile(files.encodedStreamFile, encBuffer, maxBufferSize);
    if (encLen == 0)
    {
        return std::nullopt;
    }

    static uint8_t errorDict[maxBufferSize];
    if (readErrorDictionary)
    {
        if (readBinaryFile(files.errorDictionaryFile, errorDict,
                           maxBufferSize) == 0)
        {
            return std::nullopt;
        }
    }

    BejTestInputs inputs = {
        .expectedJson = expJson,
        .schemaDictionary = schemaDictBuffer,
        .annotationDictionary = annoDictBuffer,
        .errorDictionary = errorDict,
        .encodedStream = std::span(encBuffer, encLen),
    };
    return inputs;
}

TEST_P(BejDecoderTest, Decode)
{
    const BejDecoderTestParams& test_case = GetParam();
    auto inputsOrErr = loadInputs(test_case.inputFiles);
    EXPECT_TRUE(inputsOrErr);

    BejDictionaries dictionaries = {
        .schemaDictionary = inputsOrErr->schemaDictionary,
        .annotationDictionary = inputsOrErr->annotationDictionary,
        .errorDictionary = inputsOrErr->errorDictionary,
    };

    BejDecoderJson decoder;
    EXPECT_THAT(decoder.decode(dictionaries, inputsOrErr->encodedStream), 0);
    auto decoded = decoder.getOutput();
    nlohmann::json jsonDecoded = nlohmann::json::parse(decoded);
    EXPECT_TRUE(jsonDecoded == inputsOrErr->expectedJson);
}

/**
 * TODO: Add more test cases.
 * - Test Enums inside array elemets
 * - Array inside an array: is this a valid case?
 * - Real numbers with exponent part
 * - Every type inside an array.
 */
INSTANTIATE_TEST_SUITE_P(
    , BejDecoderTest,
    testing::ValuesIn<BejDecoderTestParams>({
        {"DriveOEM", driveOemTestFiles},
        {"Circuit", circuitTestFiles},
        {"Storage", storageTestFiles},
        {"DummySimple", dummySimpleTestFiles},
    }),
    [](const testing::TestParamInfo<BejDecoderTest::ParamType>& info) {
        return info.param.testName;
    });

} // namespace rde