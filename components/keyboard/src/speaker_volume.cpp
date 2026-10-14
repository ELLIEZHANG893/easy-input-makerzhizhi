#include "keyboard/speaker_volume.h"

namespace ai_keyboard {

std::uint8_t clamp_speaker_volume_percent(int percent) {
  if (percent <= 0) {
    return 0;
  }
  if (percent >= kMaxSpeakerVolumePercent) {
    return kMaxSpeakerVolumePercent;
  }
  return static_cast<std::uint8_t>(percent);
}

std::int16_t scale_pcm_sample(std::int16_t sample, int percent) {
  const std::int32_t clamped = clamp_speaker_volume_percent(percent);
  const std::int32_t product = static_cast<std::int32_t>(sample) * clamped;
  const std::int32_t half = kMaxSpeakerVolumePercent / 2;
  // Integer division truncates toward zero, so bias by half in the sample's
  // direction to round to nearest with halves away from zero.
  const std::int32_t rounded =
      (product >= 0 ? product + half : product - half) /
      kMaxSpeakerVolumePercent;
  return static_cast<std::int16_t>(rounded);
}

void scale_pcm_samples(std::int16_t* samples,
                       std::size_t sample_count,
                       int percent) {
  if (samples == nullptr || sample_count == 0U) {
    return;
  }
  const auto clamped = clamp_speaker_volume_percent(percent);
  if (clamped == kMaxSpeakerVolumePercent) {
    return;
  }
  for (std::size_t index = 0; index < sample_count; ++index) {
    samples[index] = scale_pcm_sample(samples[index], clamped);
  }
}

}  // namespace ai_keyboard
