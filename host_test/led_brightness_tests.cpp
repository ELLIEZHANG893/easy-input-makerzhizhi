#include <cassert>
#include <cstdint>

#include "keyboard/led_brightness.h"

namespace {

using ai_keyboard::clamp_led_brightness_percent;
using ai_keyboard::kDefaultLedBrightnessPercent;
using ai_keyboard::kMaxLedBrightnessPercent;
using ai_keyboard::scale_led_channel;

void default_brightness_is_five_percent() {
  assert(kDefaultLedBrightnessPercent == 5);
  assert(kMaxLedBrightnessPercent == 100);
}

void zero_percent_turns_every_channel_off() {
  assert(scale_led_channel(0, 0) == 0);
  assert(scale_led_channel(1, 0) == 0);
  assert(scale_led_channel(46, 0) == 0);
  assert(scale_led_channel(255, 0) == 0);
}

void one_hundred_percent_keeps_every_channel_unchanged() {
  for (int value = 0; value <= 255; ++value) {
    const auto channel = static_cast<std::uint8_t>(value);
    assert(scale_led_channel(channel, 100) == channel);
  }
}

void thirty_five_percent_rounds_to_nearest() {
  // 28 * 0.35 = 9.8 -> 10; 46 * 0.35 = 16.1 -> 16; 255 * 0.35 = 89.25 -> 89.
  assert(scale_led_channel(28, 35) == 10);
  assert(scale_led_channel(46, 35) == 16);
  assert(scale_led_channel(255, 35) == 89);
  assert(scale_led_channel(0, 35) == 0);
}

void exact_half_rounds_up() {
  // 10 * 0.35 = 3.5 -> 4; 1 * 0.50 = 0.5 -> 1; 3 * 0.50 = 1.5 -> 2.
  assert(scale_led_channel(10, 35) == 4);
  assert(scale_led_channel(1, 50) == 1);
  assert(scale_led_channel(3, 50) == 2);
}

void below_half_rounds_down_to_zero() {
  // 1 * 0.35 = 0.35 -> 0; 1 * 0.49 = 0.49 -> 0.
  assert(scale_led_channel(1, 35) == 0);
  assert(scale_led_channel(1, 49) == 0);
}

void out_of_range_percent_is_saturated() {
  assert(clamp_led_brightness_percent(-1) == 0);
  assert(clamp_led_brightness_percent(-1000) == 0);
  assert(clamp_led_brightness_percent(0) == 0);
  assert(clamp_led_brightness_percent(35) == 35);
  assert(clamp_led_brightness_percent(100) == 100);
  assert(clamp_led_brightness_percent(101) == 100);
  assert(clamp_led_brightness_percent(100000) == 100);

  // Scaling never brightens beyond the original value and never wraps.
  assert(scale_led_channel(255, 200) == 255);
  assert(scale_led_channel(46, 1000) == 46);
  assert(scale_led_channel(46, -5) == 0);
}

void scaling_never_increases_and_is_monotonic() {
  for (int value = 0; value <= 255; ++value) {
    const auto channel = static_cast<std::uint8_t>(value);
    std::uint8_t previous = 0;
    for (int percent = 0; percent <= 100; ++percent) {
      const auto scaled = scale_led_channel(channel, percent);
      assert(scaled <= channel);
      assert(scaled >= previous);
      previous = scaled;
    }
  }
}

void relative_color_order_is_preserved() {
  // Scaling every channel by one factor keeps brighter channels brighter.
  constexpr std::uint8_t kRed = 30;
  constexpr std::uint8_t kGreen = 12;
  constexpr std::uint8_t kBlue = 0;
  const auto red = scale_led_channel(kRed, kDefaultLedBrightnessPercent);
  const auto green = scale_led_channel(kGreen, kDefaultLedBrightnessPercent);
  const auto blue = scale_led_channel(kBlue, kDefaultLedBrightnessPercent);
  assert(red >= green);
  assert(green >= blue);
  assert(red == 2);    // 1.5 -> 2
  assert(green == 1);  // 0.6 -> 1
  assert(blue == 0);
}

}  // namespace

int main() {
  default_brightness_is_five_percent();
  zero_percent_turns_every_channel_off();
  one_hundred_percent_keeps_every_channel_unchanged();
  thirty_five_percent_rounds_to_nearest();
  exact_half_rounds_up();
  below_half_rounds_down_to_zero();
  out_of_range_percent_is_saturated();
  scaling_never_increases_and_is_monotonic();
  relative_color_order_is_preserved();
  return 0;
}
