#pragma once

#include "json.hpp"

#include <span>

namespace rde
{

struct BejTestInputFiles
{
    const char* jsonFile;
    const char* schemaDictionaryFile;
    const char* annotationDictionaryFile;
    const char* errorDictionaryFile;
    const char* encodedStreamFile;
};

struct BejTestInputs
{
    const nlohmann::json expectedJson;
    const uint8_t* schemaDictionary;
    const uint8_t* annotationDictionary;
    const uint8_t* errorDictionary;
    std::span<uint8_t> encodedStream;
};

const BejTestInputFiles driveOemTestFiles = {
    .jsonFile = "../test/json/drive_oem.json",
    .schemaDictionaryFile = "../test/dictionaries/drive_oem_dict.bin",
    .annotationDictionaryFile = "../test/dictionaries/annotation_dict.bin",
    .errorDictionaryFile = "",
    .encodedStreamFile = "../test/encoded/drive_oem_enc.bin",
};

const BejTestInputFiles circuitTestFiles = {
    .jsonFile = "../test/json/circuit.json",
    .schemaDictionaryFile = "../test/dictionaries/circuit_dict.bin",
    .annotationDictionaryFile = "../test/dictionaries/annotation_dict.bin",
    .errorDictionaryFile = "",
    .encodedStreamFile = "../test/encoded/circuit_enc.bin",
};

const BejTestInputFiles storageTestFiles = {
    .jsonFile = "../test/json/storage.json",
    .schemaDictionaryFile = "../test/dictionaries/storage_dict.bin",
    .annotationDictionaryFile = "../test/dictionaries/annotation_dict.bin",
    .errorDictionaryFile = "",
    .encodedStreamFile = "../test/encoded/storage_enc.bin",
};

const BejTestInputFiles dummySimpleTestFiles = {
    .jsonFile = "../test/json/dummysimple.json",
    .schemaDictionaryFile = "../test/dictionaries/dummy_simple_dict.bin",
    .annotationDictionaryFile = "../test/dictionaries/annotation_dict.bin",
    .errorDictionaryFile = "",
    .encodedStreamFile = "../test/encoded/dummy_simple_enc.bin",
};

} // namespace rde