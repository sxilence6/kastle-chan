/*
MIT License

Copyright (c) 2026 Matteo Ruggiero
*/

#include "GrammarEngine.hpp"

#include <algorithm>

namespace kastle2
{

GrammarEngine::GrammarEngine(uint32_t seed) : random_state_(seed == 0 ? 1 : seed)
{
}

void GrammarEngine::Reset(uint8_t personality, bool introduce)
{
    personality_ = personality % kPersonalityCount;
    template_ = SelectTemplate();
    template_position_ = 0;
    pause_remaining_ = 0;
    glitch_remaining_ = 0;
    has_last_sample_ = false;
    introduction_pending_ = introduce;
    reaction_pending_ = false;
}

uint8_t GrammarEngine::GetCurrentRole() const
{
    const uint8_t position = template_position_ < kTemplateLengths[template_]
                                 ? template_position_
                                 : 0;
    return kTemplates[template_][position];
}

uint32_t GrammarEngine::Random()
{
    // xorshift32: tiny, deterministic, and sufficient for musical variation.
    uint32_t value = random_state_;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    random_state_ = value;
    return value;
}

uint8_t GrammarEngine::RandomPercent()
{
    return static_cast<uint8_t>(Random() % 100);
}

uint8_t GrammarEngine::SelectTemplate()
{
    const uint8_t roll = RandomPercent();
    if (roll < 58)
    {
        return 0; // Four fragments is the recognizable home position.
    }
    if (roll < 70)
    {
        return 1;
    }
    if (roll < 82)
    {
        return 2;
    }
    if (roll < 93)
    {
        return 3;
    }
    return 4;
}

GrammarEngine::Event GrammarEngine::MakePlay(uint8_t sample, uint8_t effective_panic)
{
    Event event;
    event.kind = EventKind::PLAY;
    event.sample = sample % kSampleCount;
    event.fast_follow = effective_panic >= 82;

    if (effective_panic >= 20 && RandomPercent() < effective_panic - 8)
    {
        static constexpr std::array<int8_t, 7> jumps = {0, 3, 5, 7, 7, 12, 12};
        event.semitone_offset = jumps[Random() % jumps.size()];
    }

    if (effective_panic >= 40 && RandomPercent() < (effective_panic - 30) / 2)
    {
        event.schedule_extra = true;
        event.extra_delay_ms = static_cast<uint16_t>(70 + Random() % 111);
    }

    last_sample_ = event.sample;
    has_last_sample_ = true;
    return event;
}

GrammarEngine::Event GrammarEngine::Next(
    uint8_t direct_choice, uint8_t panic, uint8_t glitch_amount)
{
    panic = std::min<uint8_t>(panic, 100);
    glitch_amount = std::min<uint8_t>(glitch_amount, 100);

    // LENGTH controls the chance of a brief 2--3-event recognizable glitch.
    // The burst finishes quickly and then returns to ordinary speech.
    uint8_t effective_panic = panic;
    bool glitching = false;
    if (glitch_remaining_ > 0)
    {
        glitching = true;
        effective_panic = std::max<uint8_t>(effective_panic, 88 + Random() % 13);
        --glitch_remaining_;
    }
    else if (RandomPercent() < 2 + glitch_amount / 7)
    {
        glitching = true;
        glitch_remaining_ = 1 + Random() % 2;
        effective_panic = std::max<uint8_t>(effective_panic, 88 + Random() % 13);
    }

    if (introduction_pending_)
    {
        introduction_pending_ = false;
        const uint8_t introduction_count = personality_ == 0 ? 5 : 2;
        return MakePlay(kExpressiveBase + Random() % introduction_count, effective_panic);
    }

    if (pause_remaining_ > 0)
    {
        --pause_remaining_;
        return Event{};
    }

    if (reaction_pending_)
    {
        reaction_pending_ = false;
        return MakePlay(kExpressiveBase + 4, effective_panic);
    }

    if (glitching)
    {
        // Use short compatible fragments or the two noise clips rather than a
        // hero sentence, keeping the entire seizure recognisable and brief.
        if (RandomPercent() < 78)
        {
            return MakePlay(
                (Random() % kRoleCount) * kChoicesPerRole,
                effective_panic);
        }
        return MakePlay(kExpressiveBase + 6 + Random() % 2, effective_panic);
    }

    // At maximum cuteness she starts catching on the same word.
    if (effective_panic >= 85 && has_last_sample_ && RandomPercent() < 25)
    {
        return MakePlay(last_sample_, effective_panic);
    }

    // Vocal noises appear late enough that normal speech remains readable.
    if (effective_panic >= 70 && RandomPercent() < (effective_panic - 55) / 2)
    {
        return MakePlay(kExpressiveBase + 6 + Random() % 2, effective_panic);
    }

    if (template_position_ >= kTemplateLengths[template_])
    {
        template_ = SelectTemplate();
        template_position_ = 0;

        // Questions leave two trigger steps for the owner to answer, then react.
        if (RandomPercent() < 14)
        {
            pause_remaining_ = 2;
            reaction_pending_ = true;
            return MakePlay(kExpressiveBase + 2 + Random() % 2, effective_panic);
        }

        if (RandomPercent() < 12)
        {
            return MakePlay(kExpressiveBase + 5, effective_panic);
        }
    }

    // LA-LA POP spends about 70% of its time in combinable sung fragments.
    if (personality_ == kLalaPersonality && RandomPercent() < 30)
    {
        return MakePlay(kExpressiveBase + 5 + Random() % 3, effective_panic);
    }

    uint8_t role = kTemplates[template_][template_position_];
    ++template_position_;

    if (effective_panic >= 55 && RandomPercent() < (effective_panic - 35) / 2)
    {
        role = Random() % kRoleCount;
    }

    const uint8_t choice = direct_choice % kChoicesPerRole;
    return MakePlay(role * kChoicesPerRole + choice, effective_panic);
}

} // namespace kastle2
