#include <cstring>
#include <iostream>

#include "catch2/catch.hpp"

#include "base/guid.h"

namespace base {

TEST_CASE("Generating or validating a GUID", "[GUID]") {
  SECTION("generate a guid") {
    auto guid = GenerateGUID();
    REQUIRE_FALSE(guid.empty());
    std::cout << guid << std::endl;
  }

  SECTION("validate a guid") {
    auto valid_guid = GenerateGUID();
    REQUIRE(IsValidGUID(valid_guid));

    std::string invalid_guid = "8B6099ED-1517-4735-81F2-3FECEEE56CG8";
    REQUIRE_FALSE(IsValidGUID(invalid_guid));
  }
}

}  // namespace base
