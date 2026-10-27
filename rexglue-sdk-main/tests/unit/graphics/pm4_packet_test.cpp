#include <catch2/catch_test_macros.hpp>

#include <rex/graphics/pm4_packet.h>

using rex::graphics::DecodePM4PacketHeader;
using rex::graphics::PM4PacketType;

TEST_CASE("PM4 header decode preserves packet type, count, opcode, and predicate",
          "[graphics][pm4]") {
  SECTION("type 0 fields") {
    const auto header = DecodePM4PacketHeader(0x012A8001u);
    CHECK(header.type == PM4PacketType::kType0);
    CHECK(header.count == 0x12Bu);
    CHECK(header.opcode == 0x00u);
    CHECK(header.predicate);
  }

  SECTION("type 1 fields") {
    const auto header = DecodePM4PacketHeader(0x40000000u);
    CHECK(header.type == PM4PacketType::kType1);
    CHECK(header.count == 1u);
    CHECK(header.opcode == 0u);
    CHECK_FALSE(header.predicate);
  }

  SECTION("type 2 fields") {
    const auto header = DecodePM4PacketHeader(0x80000001u);
    CHECK(header.type == PM4PacketType::kType2);
    CHECK(header.count == 1u);
    CHECK(header.opcode == 0u);
    CHECK(header.predicate);
  }

  SECTION("type 3 maximum count and opcode") {
    const auto header = DecodePM4PacketHeader(0xFFFF7F00u);
    CHECK(header.type == PM4PacketType::kType3);
    CHECK(header.count == 0x4000u);
    CHECK(header.opcode == 0x7Fu);
    CHECK_FALSE(header.predicate);
  }
}
