# Kastle-chan toy piano

Kastle-chan combines generated speech with finite toy-piano arpeggios and
stereo delay.

## Personalities

SAMPLE directly selects six voice personalities; the current word finishes
before the change:

1. cyan — WELCOME
2. pink — ROMANCE
3. blue-white — CHATBOT
4. yellow — LA-LA POP
5. green — FRIENDS
6. magenta-red — CUTE ESCAPE

## Familiar controls

| Control | Kastle-chan behavior |
| --- | --- |
| PITCH | Wide slow-low to fast-high voice pitch; transposes piano too |
| PITCH MOD | Attenuates shared PITCH/NOTE CV modulation |
| SAMPLE | Six voice personalities; also soft-to-sharp piano excitation |
| VAR MOD / CV | Eight toy-piano arpeggio shapes |
| AMOUNT (centre) | Short pluck to dreamy ~1.2 s piano decay |
| TYPE / BANK | Four harmony pairs, each with a different synth preset |
| SHIFT button | Manually requests an arpeggio |
| SHIFT + SAMPLE | Familiar DJ filter |
| SHIFT + SAMPLE MOD | Delay to the left, dry centre, reverb to the right |
| SHIFT + AMOUNT | AMOUNT/decay CV amount |
| SHIFT + PITCH | CUTE PANIC, including more rapid glitches |

Kastle-chan speaks autonomously after a fixed greeting. About one word slot in
three becomes a four-note arpeggio. Clock, pattern, trigger, CV and MIDI mostly
advance the piano; every fourth accepted trigger nudges speech. Only one piano
note voice exists at once and every arpeggio has an absolute end.
Audio routing, LEDs, envelope output, and the familiar patching surface remain
based on Wave Bard.

TYPE banks are deliberately paired: 1 wooden / 2 music box share harmony A;
3 glass / 4 rubber share harmony B; 5 wooden / 6 music box share harmony C;
7 glass / 8 rubber share harmony D. SAMPLE adds brightness and attack from
soft WELCOME to sharp CUTE ESCAPE without triggering a note.

## Grammar

Each personality contains 32 clips:

- 0–23: four grammatical roles, six choices per role
- 24–25: introductions
- 26–27: questions
- 28: response after two silent answer beats
- 29: hero phrase
- 30–31: giggles, breaths, and digital/vocal noises

Four-fragment sentences are most common, with shorter fragments and longer
run-ons mixed in. LA-LA POP uses sung syllables about 70% of the time.

## CUTE PANIC

The SHIFT + PITCH layer progresses through melodic jumps, quicker conversation,
broken grammar, vocal noises, and repeated words. Brief recognizable rapid
glitches happen autonomously, and become more frequent as CUTE PANIC rises.

## Build

Generate the original voice bank first:

```sh
uv venv .venv
uv pip install --python .venv/bin/python -r voice/requirements.txt
.venv/bin/python voice/generate_voice_bank.py
```

Then configure the Pico SDK/toolchain as described in the repository's
`TOOLCHAIN_INSTALL.md` and build:

```sh
./configure.sh
cmake --build code/build --target kastle-chan-with-voice
```

The merged UF2 appears at
`code/build/output/kastle2-kastle-chan-with-voice.uf2`.

## Status

This firmware is experimental and has not yet been tested on physical
hardware. Keep an official Bastl firmware available when testing it.

Kastle-chan is derived from Bastl's MIT-licensed Wave Bard application. Its
voice bank does not contain Wave Bard factory samples.
