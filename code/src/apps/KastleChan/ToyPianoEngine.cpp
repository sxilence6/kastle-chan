/*
MIT License

Copyright (c) 2026 Matteo Ruggiero
*/

#include "ToyPianoEngine.hpp"

#include <algorithm>
#include <array>
#include <cmath>

using namespace kastle2;

namespace
{
// Four cute J-pop-flavoured loops. Each TYPE pair shares one progression.
constexpr std::array<std::array<int8_t, 4>, 4> kProgressions = {{
    {{0, 7, 9, 5}},  // I V vi IV
    {{0, 9, 5, 7}},  // I vi IV V
    {{5, 7, 4, 9}},  // IV V iii vi
    {{0, 4, 5, 7}},  // I iii IV V
}};

constexpr std::array<std::array<uint8_t, 4>, 8> kPatterns = {{
    {{0, 1, 2, 3}}, {{0, 2, 1, 3}}, {{3, 2, 1, 0}}, {{0, 1, 3, 2}},
    {{0, 2, 3, 1}}, {{1, 0, 2, 3}}, {{0, 3, 2, 1}}, {{2, 1, 0, 3}},
}};
}

void ToyPianoEngine::Init(float sample_rate)
{
    sample_rate_ = sample_rate;
    oscillator_.Init(sample_rate);
    oscillator_.SetWaveform(Oscillator::Waveform::TRI);
    active_ = false;
}

void ToyPianoEngine::SetControls(
    uint8_t style, uint8_t variation, float pitch_multiplier,
    float decay, float excitation)
{
    style_ = std::min<uint8_t>(style, kStyleCount - 1);
    variation_ = std::min<uint8_t>(variation, kVariationCount - 1);
    preset_ = style_ & 0x03u;
    progression_ = style_ / 2;
    pitch_multiplier_ = std::clamp(pitch_multiplier, 0.20f, 4.0f);
    decay_control_ = std::clamp(decay, 0.0f, 1.0f);
    excitation_ = std::clamp(excitation, 0.0f, 1.0f);
}

bool ToyPianoEngine::TriggerArpeggio()
{
    if (active_)
    {
        return false;
    }

    const uint8_t progression_step = chord_counter_++ & 0x03u;
    chord_root_ = kProgressions[progression_][progression_step];
    chord_minor_ = chord_root_ == 2 || chord_root_ == 4 || chord_root_ == 9;

    // Every preset spans a clearly plucked minimum to the same dreamy 1.2 s
    // maximum, so AMOUNT remains obvious in all eight TYPE banks.
    static constexpr std::array<float, 4> kMinimumDecay = {
        0.045f, 0.060f, 0.060f, 0.045f};
    const float minimum_decay = kMinimumDecay[preset_];
    decay_seconds_ = minimum_decay + decay_control_ * decay_control_ *
                                         (1.200f - minimum_decay);

    // 58--93 ms between notes. Earlier notes hand their tails to the delay;
    // the final note receives the full AMOUNT decay.
    note_gap_frames_ = static_cast<uint32_t>(
        sample_rate_ * (0.058f + variation_ * 0.005f));
    max_note_frames_ = static_cast<uint32_t>(
        sample_rate_ * decay_seconds_);
    max_total_frames_ =
        note_gap_frames_ * (kNotesPerArpeggio - 1) + max_note_frames_;

    total_frame_ = 0;
    note_frame_ = 0;
    next_note_ = 0;
    started_notes_ = 0;
    active_ = true;
    StartNote(next_note_++);
    return true;
}

bool ToyPianoEngine::ShouldReplaceWord()
{
    return Random() % 100 < 36;
}

void ToyPianoEngine::StartNote(uint8_t note_index)
{
    const uint8_t pattern_note = kPatterns[variation_][note_index];
    const int8_t third = chord_minor_ ? 3 : 4;
    const std::array<int8_t, 4> chord = {
        0, third, 7,
        static_cast<int8_t>((progression_ & 1u) ? 14 : 12)};
    const float semitones = static_cast<float>(chord_root_ + chord[pattern_note]);
    static constexpr std::array<int8_t, 4> kPresetOctaves = {0, 12, 12, -12};
    const float frequency = std::clamp(
        220.0f * pitch_multiplier_ * std::pow(
            2.0f, (semitones + kPresetOctaves[preset_]) / 12.0f),
        55.0f, 2400.0f);

    static constexpr std::array<Oscillator::Waveform, 4> kWaveforms = {
        Oscillator::Waveform::TRI, Oscillator::Waveform::SINE,
        Oscillator::Waveform::SAW, Oscillator::Waveform::SQUARE};
    oscillator_.Reset(Q31_ZERO);
    oscillator_.SetWaveform(kWaveforms[preset_]);
    oscillator_.SetFrequency(frequency);
    amplitude_ = Q31_MAX;
    amplitude_decay_ = DecayCoefficient(decay_seconds_, sample_rate_);
    click_envelope_ = Q31_MAX;
    click_decay_ = DecayCoefficient(
        0.003f + excitation_ * 0.005f, sample_rate_);
    static constexpr std::array<float, 4> kPresetClick = {
        0.045f, 0.070f, 0.100f, 0.025f};
    static constexpr std::array<float, 4> kPresetGain = {
        0.48f, 0.52f, 0.28f, 0.24f};
    click_level_ = float_to_q15(std::min(
        0.28f, kPresetClick[preset_] + excitation_ * 0.16f));
    output_gain_ = float_to_q15(std::min(
        0.62f, kPresetGain[preset_] + excitation_ * 0.08f));
    note_frame_ = 0;
    ++started_notes_;
}

FASTCODE q15_t ToyPianoEngine::Process()
{
    if (!active_)
    {
        return Q15_ZERO;
    }

    if (next_note_ < kNotesPerArpeggio &&
        total_frame_ >= note_gap_frames_ * next_note_)
    {
        StartNote(next_note_++);
    }

    const q15_t tone = q15_mult(
        q31_to_q15(oscillator_.Process()), q31_to_q15(amplitude_));
    const q15_t click = q15_mult(
        q15_mult(NextNoise(), q31_to_q15(click_envelope_)), click_level_);
    const q15_t output = q15_mult(
        q15_add(tone, click), output_gain_);

    amplitude_ = q31_mult(amplitude_, amplitude_decay_);
    click_envelope_ = q31_mult(click_envelope_, click_decay_);
    ++note_frame_;
    ++total_frame_;

    if (total_frame_ >= max_total_frames_ ||
        (next_note_ == kNotesPerArpeggio && note_frame_ >= max_note_frames_))
    {
        active_ = false;
        amplitude_ = Q31_ZERO;
        click_envelope_ = Q31_ZERO;
    }

    return output;
}

q31_t ToyPianoEngine::DecayCoefficient(float seconds, float sample_rate)
{
    seconds = std::max(seconds, 0.001f);
    return float_to_q31(
        std::exp(std::log(0.001f) / (seconds * sample_rate)));
}

uint32_t ToyPianoEngine::Random()
{
    random_state_ ^= random_state_ << 13;
    random_state_ ^= random_state_ >> 17;
    random_state_ ^= random_state_ << 5;
    return random_state_;
}

FASTCODE q15_t ToyPianoEngine::NextNoise()
{
    noise_state_ ^= noise_state_ << 13;
    noise_state_ ^= noise_state_ >> 17;
    noise_state_ ^= noise_state_ << 5;
    return static_cast<q15_t>(noise_state_ >> 16);
}
