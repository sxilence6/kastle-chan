#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "ToyPianoEngine.hpp"

using kastle2::ToyPianoEngine;

static void WriteU16(std::ofstream &out, uint16_t value)
{
    out.put(static_cast<char>(value & 0xFF));
    out.put(static_cast<char>((value >> 8) & 0xFF));
}

static void WriteU32(std::ofstream &out, uint32_t value)
{
    WriteU16(out, static_cast<uint16_t>(value & 0xFFFF));
    WriteU16(out, static_cast<uint16_t>(value >> 16));
}

int main(int argc, char **argv)
{
    const std::string path = argc > 1 ? argv[1] : "toy-piano-preview.wav";
    constexpr uint32_t sample_rate = 44100;
    constexpr uint32_t seconds = 16;
    constexpr uint32_t frames = sample_rate * seconds;

    std::ofstream out(path, std::ios::binary);
    out.write("RIFF", 4);
    WriteU32(out, 36 + frames * 4);
    out.write("WAVEfmt ", 8);
    WriteU32(out, 16);
    WriteU16(out, 1);
    WriteU16(out, 2);
    WriteU32(out, sample_rate);
    WriteU32(out, sample_rate * 4);
    WriteU16(out, 4);
    WriteU16(out, 16);
    out.write("data", 4);
    WriteU32(out, frames * 4);

    ToyPianoEngine piano;
    piano.Init(sample_rate);
    std::vector<float> delay_left(5999);
    std::vector<float> delay_right(7993);
    size_t left_pos = 0;
    size_t right_pos = 0;
    float damp_left = 0.0f;
    float damp_right = 0.0f;

    for (uint32_t frame = 0; frame < frames; ++frame)
    {
        if (frame % (sample_rate * 2) == 0)
        {
            const uint32_t chord = frame / (sample_rate * 2);
            const float pitch = chord < 4 ? 1.0f : 1.5f;
            piano.SetControls(
                static_cast<uint8_t>(chord % ToyPianoEngine::kStyleCount),
                static_cast<uint8_t>((chord * 3) % ToyPianoEngine::kVariationCount),
                pitch, chord < 4 ? 0.35f : 1.0f,
                static_cast<float>(chord) / 7.0f);
            piano.TriggerArpeggio();
        }

        const float dry = piano.Process() / 32768.0f;
        const float delayed_left = delay_left[left_pos];
        const float delayed_right = delay_right[right_pos];
        damp_left = damp_left * 0.78f + delayed_left * 0.22f;
        damp_right = damp_right * 0.78f + delayed_right * 0.22f;
        delay_left[left_pos] = dry * 0.38f + damp_right * 0.76f;
        delay_right[right_pos] = dry * 0.38f + damp_left * 0.76f;
        left_pos = (left_pos + 1) % delay_left.size();
        right_pos = (right_pos + 1) % delay_right.size();

        const float left = std::clamp(dry * 0.40f + delayed_left * 0.60f, -1.0f, 1.0f);
        const float right = std::clamp(dry * 0.40f + delayed_right * 0.60f, -1.0f, 1.0f);
        WriteU16(out, static_cast<uint16_t>(static_cast<int16_t>(left * 32767.0f)));
        WriteU16(out, static_cast<uint16_t>(static_cast<int16_t>(right * 32767.0f)));
    }
}
