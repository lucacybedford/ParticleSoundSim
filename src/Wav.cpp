#include "Wav.hpp"
#include <cmath>
#include <cstdio>
#include <samplerate.h>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

bool wav_read(const std::string &path, Audio &out) {
  drwav wav;
  if (!drwav_init_file(&wav, path.c_str(), nullptr))
    return false;

  const drwav_uint64 frames = wav.totalPCMFrameCount;
  const unsigned channels = wav.channels;
  // dr_wav converts any supported format (8/16/24/32-bit PCM, float, ADPCM)
  // to float in [-1, 1]
  std::vector<float> interleaved(frames * channels);
  drwav_uint64 got =
      drwav_read_pcm_frames_f32(&wav, frames, interleaved.data());
  out.sample_rate = static_cast<int>(wav.sampleRate);
  drwav_uninit(&wav);
  if (got == 0)
    return false;

  out.samples.resize(got);
  for (drwav_uint64 i = 0; i < got; ++i) {
    float acc = 0.0f;
    for (unsigned ch = 0; ch < channels; ++ch)
      acc += interleaved[i * channels + ch];
    out.samples[i] = acc / channels;
  }
  return true;
}

bool wav_write(const std::string &path, const Audio &in) {
  drwav_data_format fmt{};
  fmt.container = drwav_container_riff;
  fmt.format = DR_WAVE_FORMAT_PCM;
  fmt.channels = 1;
  fmt.sampleRate = static_cast<drwav_uint32>(in.sample_rate);
  fmt.bitsPerSample = 16;

  drwav wav;
  if (!drwav_init_file_write(&wav, path.c_str(), &fmt, nullptr))
    return false;

  std::vector<drwav_int16> pcm(in.samples.size());
  for (std::size_t i = 0; i < in.samples.size(); ++i) {
    long s = std::lround(in.samples[i] * 32767.0f);
    if (s > 32767)
      s = 32767;
    if (s < -32768)
      s = -32768;
    pcm[i] = static_cast<drwav_int16>(s);
  }

  drwav_uint64 written =
      drwav_write_pcm_frames(&wav, pcm.size(), pcm.data());
  drwav_uninit(&wav);
  return written == pcm.size();
}

std::vector<float> resample(const std::vector<float> &in, int in_rate,
                            int out_rate) {
  if (in_rate == out_rate || in.empty())
    return in;

  const double ratio = static_cast<double>(out_rate) / in_rate;
  // +1 guards against the rounded estimate being one frame short
  std::vector<float> out(static_cast<std::size_t>(in.size() * ratio) + 1);

  SRC_DATA d{};
  d.data_in = in.data();
  d.input_frames = static_cast<long>(in.size());
  d.data_out = out.data();
  d.output_frames = static_cast<long>(out.size());
  d.src_ratio = ratio;

  int err = src_simple(&d, SRC_SINC_BEST_QUALITY, /*channels=*/1);
  if (err != 0) {
    std::fprintf(stderr, "resample failed: %s\n", src_strerror(err));
    return {};
  }

  out.resize(static_cast<std::size_t>(d.output_frames_gen));
  return out;
}

void normalize_peak(std::vector<float> &samples, float peak) {
  float maxabs = 0.0f;
  for (float v : samples)
    maxabs = std::max(maxabs, std::fabs(v));
  if (maxabs <= 0.0f)
    return;
  float g = peak / maxabs;
  for (float &v : samples)
    v *= g;
}
