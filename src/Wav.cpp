#include "Wav.hpp"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>

// namespace for helper functions only used in this file
namespace {

uint16_t rd_u16(const unsigned char *p) {
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
uint32_t rd_u32(const unsigned char *p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

} // namespace

bool wav_read(const std::string &path, Audio &out) {
  std::ifstream f(path, std::ios::binary);
  if (!f)
    return false;
  std::vector<unsigned char> d((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
  if (d.size() < 44)
    return false;
  // check if the file is in the right format
  if (std::memcmp(d.data(), "RIFF", 4) != 0 ||
      std::memcmp(d.data() + 8, "WAVE", 4) != 0)
    return false;

  uint16_t channels = 0, bits = 0;
  uint32_t rate = 0;
  const unsigned char *data_ptr = nullptr;
  uint32_t data_len = 0;

  // go over the chunks looking for "fmt" and "data"
  std::size_t pos = 12;
  while (pos + 8 <= d.size()) {
    const unsigned char *c = d.data() + pos;
    uint32_t sz = rd_u32(c + 4);
    if (std::memcmp(c, "fmt ", 4) == 0 && pos + 8 + 16 <= d.size()) {
      channels = rd_u16(c + 8 + 2);
      rate = rd_u32(c + 8 + 4);
      bits = rd_u16(c + 8 + 14);
    } else if (std::memcmp(c, "data", 4) == 0) {
      data_ptr = c + 8;
      data_len = sz;
    }
    pos += 8 + sz + (sz & 1); // chunks are word-aligned
  }

  if (!data_ptr || bits != 16 || channels < 1)
    return false;

  out.sample_rate = static_cast<int>(rate);
  std::size_t total = data_len / 2; // int16 samples
  std::size_t frames = total / channels;
  out.samples.resize(frames);
  for (std::size_t i = 0; i < frames; ++i) {
    int acc = 0;
    for (int ch = 0; ch < channels; ++ch) {
      int16_t s;
      std::memcpy(&s, data_ptr + (i * channels + ch) * 2, 2);
      acc += s;
    }
    out.samples[i] = static_cast<float>(acc) / (channels * 32768.0f);
  }
  return true;
}

bool wav_write(const std::string &path, const Audio &in) {
  std::ofstream f(path, std::ios::binary);
  if (!f)
    return false;

  const uint16_t channels = 1, bits = 16;
  const uint32_t rate = static_cast<uint32_t>(in.sample_rate);
  const uint32_t block_align = channels * bits / 8;
  const uint32_t byte_rate = rate * block_align;
  const uint32_t data_len = static_cast<uint32_t>(in.samples.size() * 2);

  auto w16 = [&](uint16_t v) {
    unsigned char b[2] = {static_cast<unsigned char>(v),
                          static_cast<unsigned char>(v >> 8)};
    f.write(reinterpret_cast<char *>(b), 2);
  };
  auto w32 = [&](uint32_t v) {
    unsigned char b[4] = {static_cast<unsigned char>(v),
                          static_cast<unsigned char>(v >> 8),
                          static_cast<unsigned char>(v >> 16),
                          static_cast<unsigned char>(v >> 24)};
    f.write(reinterpret_cast<char *>(b), 4);
  };

  f.write("RIFF", 4);
  w32(36 + data_len);
  f.write("WAVE", 4);
  f.write("fmt ", 4);
  w32(16); // PCM fmt chunk size
  w16(1);  // audio format = PCM
  w16(channels);
  w32(rate);
  w32(byte_rate);
  w16(static_cast<uint16_t>(block_align));
  w16(bits);
  f.write("data", 4);
  w32(data_len);

  for (float v : in.samples) {
    long s = std::lround(v * 32767.0f);
    if (s > 32767)
      s = 32767;
    if (s < -32768)
      s = -32768;
    w16(static_cast<uint16_t>(static_cast<int16_t>(s)));
  }
  return static_cast<bool>(f);
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
