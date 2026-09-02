/*
MIT License

Copyright (c) 2026 Matteo Ruggiero
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace kastle2
{

class GrammarEngine
{
public:
    static constexpr uint8_t kPersonalityCount = 6;
    static constexpr uint8_t kRoleCount = 4;
    static constexpr uint8_t kChoicesPerRole = 6;
    static constexpr uint8_t kFragmentCount = kRoleCount * kChoicesPerRole;
    static constexpr uint8_t kExpressiveBase = kFragmentCount;
    static constexpr uint8_t kSampleCount = 32;
    static constexpr uint8_t kLalaPersonality = 3;

    enum class EventKind : uint8_t
    {
        PLAY,
        SILENCE
    };

    struct Event
    {
        EventKind kind = EventKind::SILENCE;
        uint8_t sample = 0;
        int8_t semitone_offset = 0;
        bool schedule_extra = false;
        uint16_t extra_delay_ms = 0;
        bool fast_follow = false;
    };

    explicit GrammarEngine(uint32_t seed = 0x4B324348);

    void Reset(uint8_t personality, bool introduce = true);
    Event Next(uint8_t direct_choice, uint8_t panic, uint8_t glitch_amount = 0);

    uint8_t GetPersonality() const { return personality_; }
    uint8_t GetCurrentRole() const;

private:
    static constexpr std::array<uint8_t, 5> kTemplateLengths = {4, 2, 3, 5, 6};
    static constexpr std::array<std::array<uint8_t, 6>, 5> kTemplates = {{
        {{0, 1, 2, 3, 0, 0}},
        {{0, 3, 0, 0, 0, 0}},
        {{0, 1, 3, 0, 0, 0}},
        {{0, 1, 2, 2, 3, 0}},
        {{0, 1, 2, 1, 2, 3}},
    }};

    uint32_t Random();
    uint8_t RandomPercent();
    uint8_t SelectTemplate();
    Event MakePlay(uint8_t sample, uint8_t effective_panic);

    uint32_t random_state_;
    uint8_t personality_ = 0;
    uint8_t template_ = 0;
    uint8_t template_position_ = 0;
    uint8_t pause_remaining_ = 0;
    uint8_t glitch_remaining_ = 0;
    uint8_t last_sample_ = 0;
    bool has_last_sample_ = false;
    bool introduction_pending_ = true;
    bool reaction_pending_ = false;
};

} // namespace kastle2
