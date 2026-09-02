# Kastle-chan voice bank

`manifest.json` is the source vocabulary for six personality banks. The
generator synthesizes each line locally with Kokoro-82M through `kokoro-mlx`,
then uses FFmpeg to trim, filter, normalize, and render a strong bright formant
shift as clear 20 kHz mono 16-bit PCM for Kastle 2 flash. The fixed treatment
preserves twice the voice bandwidth of the former live dual-formant bank.

- Inference code: `kokoro-mlx` 0.1.2, MIT
- Model weights: `hexgrad/Kokoro-82M`, Apache-2.0
- Selected base voice: `af_heart`, driven with English phonemes
- Model weights and intermediate WAVs are not committed
- Generated Kastle-chan clips are original project assets and do not imitate or
  contain Hatsune Miku voice data

To the extent the project authors have rights in the generated voice clips,
they are dedicated to the public domain under CC0-1.0. The firmware code keeps
its upstream MIT licensing.
