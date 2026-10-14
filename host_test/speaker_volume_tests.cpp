#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "keyboard/speaker_volume.h"

namespace {

using ai_keyboard::clamp_speaker_volume_percent;
using ai_keyboard::kDefaultBootSoundVolumePercent;
using ai_keyboard::kMaxSpeakerVolumePercent;
using ai_keyboard::scale_pcm_sample;
using ai_keyboard::scale_pcm_samples;

constexpr std::int16_t kMin = std::numeric_limits<std::int16_t>::min();
constexpr std::int16_t kMax = std::numeric_limits<std::int16_t>::max();

void default_boot_sound_volume_is_muted() {
  assert(kDefaultBootSoundVolumePercent == 0);
  assert(kMaxSpeakerVolumePercent == 100);
}

void zero_percent_is_silence() {
  assert(scale_pcm_sample(0, 0) == 0);
  assert(scale_pcm_sample(1, 0) == 0);
  assert(scale_pcm_sample(-1, 0) == 0);
  assert(scale_pcm_sample(kMax, 0) == 0);
  assert(scale_pcm_sample(kMin, 0) == 0);
}

void one_hundred_percent_is_bit_exact() {
  for (int value = kMin; value <= kMax; value += 97) {
    const auto sample = static_cast<std::int16_t>(value);
    assert(scale_pcm_sample(sample, 100) == sample);
  }
  assert(scale_pcm_sample(kMin, 100) == kMin);
  assert(scale_pcm_sample(kMax, 100) == kMax);
}

void thirty_percent_rounds_to_nearest_and_is_symmetric() {
  // 1000 * 0.3 = 300; 1001 * 0.3 = 300.3 -> 300; 1005 * 0.3 = 301.5 -> 302.
  assert(scale_pcm_sample(1000, 30) == 300);
  assert(scale_pcm_sample(1001, 30) == 300);
  assert(scale_pcm_sample(1005, 30) == 302);
  assert(scale_pcm_sample(-1000, 30) == -300);
  assert(scale_pcm_sample(-1001, 30) == -300);
  assert(scale_pcm_sample(-1005, 30) == -302);
  // Full-scale extremes must not overflow.
  assert(scale_pcm_sample(kMax, 30) == 9830);    // 9830.1
  assert(scale_pcm_sample(kMin, 30) == -9830);   // -9830.4
}

void out_of_range_percent_is_saturated() {
  assert(clamp_speaker_volume_percent(-1) == 0);
  assert(clamp_speaker_volume_percent(-500) == 0);
  assert(clamp_speaker_volume_percent(30) == 30);
  assert(clamp_speaker_volume_percent(100) == 100);
  assert(clamp_speaker_volume_percent(101) == 100);
  assert(clamp_speaker_volume_percent(100000) == 100);
  // Never amplifies, never wraps.
  assert(scale_pcm_sample(kMax, 250) == kMax);
  assert(scale_pcm_sample(kMin, 250) == kMin);
  assert(scale_pcm_sample(1234, -7) == 0);
}

void scaling_never_increases_magnitude() {
  for (int value = kMin; value <= kMax; value += 251) {
    const auto sample = static_cast<std::int16_t>(value);
    const int magnitude = value < 0 ? -value : value;
    int previous = 0;
    for (int percent = 0; percent <= 100; ++percent) {
      const int scaled = scale_pcm_sample(sample, percent);
      const int scaled_magnitude = scaled < 0 ? -scaled : scaled;
      assert(scaled_magnitude <= magnitude);
      assert(scaled_magnitude >= previous);
      // Sign is preserved (or the sample becomes silent).
      assert(scaled == 0 || (scaled < 0) == (value < 0));
      previous = scaled_magnitude;
    }
  }
}

void buffer_scaling_matches_per_sample_and_handles_edges() {
  std::array<std::int16_t, 6> samples{{0, 1000, -1000, kMax, kMin, 1005}};
  scale_pcm_samples(samples.data(), samples.size(), 30);
  assert(samples[0] == 0);
  assert(samples[1] == 300);
  assert(samples[2] == -300);
  assert(samples[3] == 9830);
  assert(samples[4] == -9830);
  assert(samples[5] == 302);

  // Null or empty buffers are ignored safely.
  scale_pcm_samples(nullptr, 10, 30);
  std::array<std::int16_t, 2> untouched{{1000, -1000}};
  scale_pcm_samples(untouched.data(), 0, 30);
  assert(untouched[0] == 1000);
  assert(untouched[1] == -1000);

  // 100% leaves the buffer unchanged.
  std::array<std::int16_t, 3> unity{{123, -456, kMin}};
  scale_pcm_samples(unity.data(), unity.size(), 100);
  assert(unity[0] == 123);
  assert(unity[1] == -456);
  assert(unity[2] == kMin);
}

}  // namespace

int main() {
  default_boot_sound_volume_is_muted();
  zero_percent_is_silence();
  one_hundred_percent_is_bit_exact();
  thirty_percent_rounds_to_nearest_and_is_symmetric();
  out_of_range_percent_is_saturated();
  scaling_never_increases_magnitude();
  buffer_scaling_matches_per_sample_and_handles_edges();
  return 0;
}
