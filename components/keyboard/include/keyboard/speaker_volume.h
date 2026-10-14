#pragma once

#include <cstddef>
#include <cstdint>

namespace ai_keyboard {

// Software volume for decoded PCM. It only scales sample values before they
// reach I2S; it never changes I2S clocks, speaker GPIOs or the shared GPIO8
// peripheral power rail.
constexpr std::uint8_t kDefaultBootSoundVolumePercent = 0;
constexpr std::uint8_t kMaxSpeakerVolumePercent = 100;

// Saturates any requested percentage into 0..100.
std::uint8_t clamp_speaker_volume_percent(int percent);

// Scales one signed 16-bit PCM sample, rounding to nearest with halves away
// from zero. Out-of-range percentages are saturated first, so the result is
// never louder than the input and can never overflow.
std::int16_t scale_pcm_sample(std::int16_t sample, int percent);

// Scales a PCM buffer in place. Null or empty buffers are ignored, and 100%
// leaves the samples bit-exact.
void scale_pcm_samples(std::int16_t* samples,
                       std::size_t sample_count,
                       int percent);

}  // namespace ai_keyboard
