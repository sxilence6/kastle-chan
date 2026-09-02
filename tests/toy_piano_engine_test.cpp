#include <cassert>
#include <cstdint>
#include <iostream>

#include "ToyPianoEngine.hpp"

using kastle2::ToyPianoEngine;

int main()
{
    constexpr uint32_t sample_rate = 44100;
    ToyPianoEngine piano;
    piano.Init(sample_rate);
    piano.SetControls(0, 3, 1.0f, 0.0f, 0.0f);
    assert(piano.TriggerArpeggio());
    assert(!piano.TriggerArpeggio());

    uint32_t active_frames = 0;
    uint32_t nonzero_frames = 0;
    while (piano.IsActive() && active_frames < sample_rate)
    {
        nonzero_frames += piano.Process() != 0;
        ++active_frames;
    }
    assert(!piano.IsActive());
    assert(active_frames < sample_rate / 2);
    assert(nonzero_frames > 1000);
    assert(piano.GetStartedNotes() == ToyPianoEngine::kNotesPerArpeggio);

    for (uint8_t style = 0; style < ToyPianoEngine::kStyleCount; ++style)
    {
        for (uint8_t variation = 0;
             variation < ToyPianoEngine::kVariationCount; ++variation)
        {
            piano.SetControls(
                style, variation, variation & 1u ? 0.20f : 4.0f,
                variation & 1u ? 1.0f : 0.0f,
                static_cast<float>(style) /
                    (ToyPianoEngine::kStyleCount - 1));
            assert(piano.GetPreset() == style % 4);
            assert(piano.GetProgression() == style / 2);
            assert(piano.TriggerArpeggio());
            active_frames = 0;
            while (piano.IsActive() && active_frames < sample_rate * 2)
            {
                piano.Process();
                ++active_frames;
            }
            assert(!piano.IsActive());
            assert(active_frames < sample_rate * 3 / 2);
            assert(
                piano.GetStartedNotes() ==
                ToyPianoEngine::kNotesPerArpeggio);
        }
    }

    // Music-box preset exposes the full AMOUNT range: a crisp minimum and a
    // dreamy but still finite 1.2-second maximum.
    piano.SetControls(1, 0, 1.0f, 0.0f, 0.0f);
    assert(piano.TriggerArpeggio());
    assert(piano.GetDecaySeconds() >= 0.059f);
    while (piano.IsActive())
        piano.Process();
    piano.SetControls(1, 0, 1.0f, 1.0f, 1.0f);
    assert(piano.TriggerArpeggio());
    assert(piano.GetDecaySeconds() > 1.19f);
    active_frames = 0;
    while (piano.IsActive() && active_frames < sample_rate * 2)
    {
        piano.Process();
        ++active_frames;
    }
    assert(!piano.IsActive());
    assert(active_frames > sample_rate);
    assert(active_frames < sample_rate * 3 / 2);

    size_t replacements = 0;
    for (size_t i = 0; i < 2000; ++i)
    {
        replacements += piano.ShouldReplaceWord();
    }
    assert(replacements > 600);
    assert(replacements < 850);

    std::cout << "ToyPianoEngine tests passed\n";
}
