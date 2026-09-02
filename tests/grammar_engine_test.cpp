#include <cassert>
#include <cstdint>
#include <iostream>

#include "GrammarEngine.hpp"

using kastle2::GrammarEngine;

int main()
{
    GrammarEngine engine(0x12345678);
    engine.Reset(0, true);

    auto event = engine.Next(0, 0);
    assert(event.kind == GrammarEngine::EventKind::PLAY);
    assert(event.sample >= GrammarEngine::kExpressiveBase);
    assert(event.sample < GrammarEngine::kSampleCount);

    bool saw_silence = false;
    bool saw_reaction_after_silence = false;
    uint8_t silence_run = 0;
    size_t spontaneous_cracks = 0;
    for (size_t i = 0; i < 3000; ++i)
    {
        event = engine.Next(static_cast<uint8_t>(i % 6), 0);
        assert(event.sample < GrammarEngine::kSampleCount);
        if (event.semitone_offset > 0 || event.schedule_extra)
        {
            ++spontaneous_cracks;
        }

        if (event.kind == GrammarEngine::EventKind::SILENCE)
        {
            saw_silence = true;
            ++silence_run;
        }
        else
        {
            if (silence_run == 2 && event.sample == GrammarEngine::kExpressiveBase + 4)
            {
                saw_reaction_after_silence = true;
            }
            silence_run = 0;
        }
    }
    assert(saw_silence);
    assert(saw_reaction_after_silence);
    assert(spontaneous_cracks > 0);
    assert(spontaneous_cracks < 300);

    engine.Reset(5, true);
    event = engine.Next(5, 100);
    assert(engine.GetPersonality() == 5);
    assert(event.kind == GrammarEngine::EventKind::PLAY);
    assert(event.sample < GrammarEngine::kSampleCount);

    bool saw_panic_effect = event.semitone_offset > 0 || event.schedule_extra;
    for (size_t i = 0; i < 300 && !saw_panic_effect; ++i)
    {
        event = engine.Next(5, 100);
        saw_panic_effect = event.semitone_offset > 0 || event.schedule_extra;
    }
    assert(saw_panic_effect);

    GrammarEngine glitch_engine(0xC001C0DE);
    glitch_engine.Reset(GrammarEngine::kLalaPersonality, false);
    size_t fast_events = 0;
    for (size_t i = 0; i < 500; ++i)
    {
        event = glitch_engine.Next(0, 0, 100);
        fast_events += event.fast_follow;
    }
    assert(fast_events > 80);
    assert(fast_events < 350);

    std::cout << "GrammarEngine tests passed\n";
    return 0;
}
