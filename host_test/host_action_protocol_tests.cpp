#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "keyboard/host_action_protocol.h"
#include "keyboard/status_hid_protocol.h"

namespace {

void accepts_canonical_lowercase_uuid_without_semantic_uuid_restrictions() {
  const std::vector<std::string> accepted{
      "host_action:123e4567-e89b-12d3-a456-426614174000",
      "host_action:00000000-0000-0000-0000-000000000000",
      "host_action:ffffffff-ffff-ffff-ffff-ffffffffffff",
  };

  for (const auto& action : accepted) {
    assert(ai_keyboard::is_canonical_host_action_value(action));
  }
}

void rejects_noncanonical_uuid_and_leaves_output_untouched() {
  const std::vector<std::string> rejected{
      "host_action:123E4567-e89b-12d3-a456-426614174000",
      "host_action:123e4567-e89b-12d3-a456-42661417400",
      "host_action:123e456-7e89b-12d3-a456-426614174000",
      "host_action:123e4567-e89b-12d3-a456-42661417400g",
      "host-action:123e4567-e89b-12d3-a456-426614174000",
      // Too long by one character.
      "host_action:123e4567-e89b-12d3-a456-4266141740000",
      // Prefix only: zero-length UUID.
      "host_action:",
      // Correct positions but a non-hyphen separator.
      "host_action:123e4567_e89b-12d3-a456-426614174000",
      // The prefix is not case-normalized either.
      "HOST_ACTION:123e4567-e89b-12d3-a456-426614174000",
  };

  for (const auto& action : rejected) {
    assert(!ai_keyboard::is_canonical_host_action_value(action));

    ai_keyboard::HostActionV1Report report;
    report.report_id = 0xAA;
    report.payload.fill(0xAA);
    assert(!ai_keyboard::encode_host_action_v1(action, &report));
    assert(report.report_id == 0xAA);
    for (const auto byte : report.payload) {
      assert(byte == 0xAA);
    }
  }
}

void encodes_the_frozen_v1_report_without_the_config_prefix() {
  static_assert(ai_keyboard::kHostActionV1ReportId == 0x11);
  static_assert(ai_keyboard::kHostActionV1CommandKind == 0x05);
  static_assert(ai_keyboard::kHostActionV1ChunkIndex == 0);
  static_assert(ai_keyboard::kHostActionV1TotalChunks == 1);
  static_assert(ai_keyboard::kHostActionV1UuidLen == 36);
  static_assert(ai_keyboard::kHostActionV1PayloadLen == 63);
  static_assert(ai_keyboard::kStatusResponseCommandKind == 0x04);
  static_assert(ai_keyboard::kHostActionV1CommandKind !=
                ai_keyboard::kStatusResponseCommandKind);

  const std::string action =
      "host_action:123e4567-e89b-12d3-a456-426614174000";
  const std::string uuid = "123e4567-e89b-12d3-a456-426614174000";
  ai_keyboard::HostActionV1Report report;

  assert(ai_keyboard::encode_host_action_v1(action, &report));
  assert(report.report_id == 0x11);
  assert(report.payload[0] == 0x05);
  assert(report.payload[1] == 0);
  assert(report.payload[2] == 1);
  assert(report.payload[3] == 36);
  const std::string encoded_uuid(
      reinterpret_cast<const char*>(report.payload.data() + 4),
      ai_keyboard::kHostActionV1UuidLen);
  assert(encoded_uuid == uuid);
  assert(encoded_uuid.find("host_action:") == std::string::npos);
  for (std::size_t index = 40; index < report.payload.size(); ++index) {
    assert(report.payload[index] == 0);
  }
}

// Models the argument split used by both USB and BLE adapters
// (payload[0], payload[1], payload[2], payload.data() + header, payload[3])
// and the zero-initialised 63-byte App Command builder they share. The
// rebuilt container must equal the encoded report byte for byte, so neither
// transport can add, drop or reorder bytes relative to the shared encoding.
std::array<std::uint8_t, ai_keyboard::kHostActionV1PayloadLen>
rebuild_app_command_container(std::uint8_t command_kind,
                              std::uint8_t chunk_index,
                              std::uint8_t total_chunks,
                              const std::uint8_t* data,
                              std::size_t len) {
  std::array<std::uint8_t, ai_keyboard::kHostActionV1PayloadLen> report{};
  report[0] = command_kind;
  report[1] = chunk_index;
  report[2] = total_chunks;
  report[3] = static_cast<std::uint8_t>(len);
  for (std::size_t index = 0; index < len; ++index) {
    report[ai_keyboard::kHostActionV1HeaderLen + index] = data[index];
  }
  return report;
}

void adapter_argument_split_rebuilds_the_identical_container() {
  for (const char* action : {
           "host_action:123e4567-e89b-12d3-a456-426614174000",
           "host_action:00000000-0000-0000-0000-000000000000",
           "host_action:ffffffff-ffff-ffff-ffff-ffffffffffff",
       }) {
    ai_keyboard::HostActionV1Report report;
    assert(ai_keyboard::encode_host_action_v1(action, &report));
    assert(report.report_id == 0x11);
    const auto& p = report.payload;
    assert(p[3] == ai_keyboard::kHostActionV1UuidLen);
    const auto rebuilt = rebuild_app_command_container(
        p[0], p[1], p[2], p.data() + ai_keyboard::kHostActionV1HeaderLen,
        p[3]);
    assert(rebuilt == p);
    assert(rebuilt[0] == 0x05);
    assert(rebuilt[0] != ai_keyboard::kStatusResponseCommandKind);
  }
}

}  // namespace

int main() {
  accepts_canonical_lowercase_uuid_without_semantic_uuid_restrictions();
  rejects_noncanonical_uuid_and_leaves_output_untouched();
  encodes_the_frozen_v1_report_without_the_config_prefix();
  adapter_argument_split_rebuilds_the_identical_container();
  return 0;
}
