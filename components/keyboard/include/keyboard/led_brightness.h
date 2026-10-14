#pragma once

#include <cstdint>

namespace ai_keyboard {

// Global WS2812 brightness is applied by scaling RGB values only. It never
// touches GPIO12, RMT timing or the shared GPIO8 peripheral power rail.
constexpr std::uint8_t kDefaultLedBrightnessPercent = 5;
constexpr std::uint8_t kMaxLedBrightnessPercent = 100;

// Saturates any requested percentage into 0..100.
std::uint8_t clamp_led_brightness_percent(int percent);

// Scales one 8-bit colour channel by a brightness percentage, rounding to the
// nearest integer (exact halves round up). Out-of-range percentages are
// saturated first, so the result never exceeds the input value.
std::uint8_t scale_led_channel(std::uint8_t value, int percent);

}  // namespace ai_keyboard
