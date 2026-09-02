/*
MIT License

Copyright (c) 2026 Matteo Ruggiero
*/

#pragma once

#include <array>
#include <cstdint>

#include "common/dsp/math/qmath.hpp"
#include "common/dsp/synthesis/Oscillator.hpp"
#include "common/fastcode.hpp"

namespace kastle2
{

/**
 * One hard-bounded toy-piano voice. Chords are implied by four sequential
 * notes and the shared delay tail; no oscillator bank runs continuously.
 */
class ToyPianoEngine
{
public:
    static constexpr uint8_t kStyleCount = 8;
    static constexpr uint8_t kVariationCount = 8;
    static constexpr uint8_t kNotesPerArpeggio = 4;

    void Init(float sample_rate);
    void SetControls(
        uint8_t style, uint8_t variation, float pitch_multiplier,
        float decay, float excitation);

    /** Start a finite arpeggio, or reject it while another is active. */
    bool TriggerArpeggio();

    /** Deterministic frequent word-replacement decision (about 36%). */
    bool ShouldReplaceWord();

    FASTCODE q15_t Process();
    bool IsActive() const { return active_; }
    uint8_t GetStartedNotes() const { return started_notes_; }
    uint8_t GetPreset() const { return preset_; }
    uint8_t GetProgression() const { return progression_; }
    float GetDecaySeconds() const { return decay_seconds_; }

private:
    void StartNote(uint8_t note_index);
    static q31_t DecayCoefficient(float seconds, float sample_rate);
    uint32_t Random();
    FASTCODE q15_t NextNoise();

    float sample_rate_ = 44100.0f;
    uint8_t style_ = 0;
    uint8_t variation_ = 0;
    uint8_t preset_ = 0;
    uint8_t progression_ = 0;
    float pitch_multiplier_ = 1.0f;
    float decay_control_ = 0.5f;
    float excitation_ = 0.0f;
    float decay_seconds_ = 0.3f;

    Oscillator oscillator_;
    q31_t amplitude_ = Q31_ZERO;
    q31_t amplitude_decay_ = Q31_ZERO;
    q31_t click_envelope_ = Q31_ZERO;
    q31_t click_decay_ = Q31_ZERO;
    q15_t click_level_ = Q15_ZERO;
    q15_t output_gain_ = q15(0.45f);

    uint32_t total_frame_ = 0;
    uint32_t note_frame_ = 0;
    uint32_t note_gap_frames_ = 0;
    uint32_t max_note_frames_ = 0;
    uint32_t max_total_frames_ = 0;
    uint8_t next_note_ = 0;
    uint8_t started_notes_ = 0;
    uint8_t chord_counter_ = 0;
    int8_t chord_root_ = 0;
    bool chord_minor_ = false;
    bool active_ = false;

    uint32_t random_state_ = 0x5049414Eu;
    uint32_t noise_state_ = 0x71AC39D5u;
};

} // namespace kastle2
