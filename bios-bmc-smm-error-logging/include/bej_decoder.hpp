#pragma once

#include "bej_decoder_core.h"
#include "rde_common.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rde
{

/**
 * @brief Class for decoding RDE BEJ to a JSON output.
 *
 */
class BejDecoderJson
{

  public:
    /**
     * @brief Decode the encoded PLDM block.
     *
     * @param dictionaries - dictionaries needed for decoding.
     * @param encodedPldmBlock - encoded PLDM block.
     * @return 0 if successful.
     */
    int decode(const BejDictionaries& dictionaries,
               const std::span<uint8_t> encodedPldmBlock);

    /**
     * @brief Get the JSON output related to the lastest call to decode.
     *
     * @return std::string_view pointing to a JSON. If the decoding was
     * unsuccessful, this might contain partial data.
     */
    std::string_view getOutput();

  private:
    bool isPrevAnnotated;
    std::string output;
    std::vector<BejStackProperty> stack;
};

} // namespace rde