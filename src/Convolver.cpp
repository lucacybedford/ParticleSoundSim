#include "Convolver.hpp"
#include <algorithm>
#include <complex>
#include <cstddef>

#include "pocketfft_hdronly.h"

// namespace for helper functions only used in this file
namespace {

std::size_t next_pow2(std::size_t n) {
  std::size_t p = 1;
  while (p < n)
    p <<= 1;
  return p;
}

// Real-to-complex forward FFT of `in` (length nfft) into `out` (nfft/2+1 bins).
void rfft(const std::vector<double> &in,
          std::vector<std::complex<double>> &out) {
  const std::size_t nfft = in.size();
  pocketfft::shape_t shape{nfft};
  pocketfft::stride_t stride_in{static_cast<ptrdiff_t>(sizeof(double))};
  pocketfft::stride_t stride_out{
      static_cast<ptrdiff_t>(sizeof(std::complex<double>))};
  pocketfft::r2c(shape, stride_in, stride_out, /*axis=*/0, pocketfft::FORWARD,
                 in.data(), out.data(), 1.0);
}

// Complex-to-real inverse FFT, normalised by 1/nfft so it is a true inverse.
void irfft(const std::vector<std::complex<double>> &in,
           std::vector<double> &out) {
  const std::size_t nfft = out.size();
  pocketfft::shape_t shape{nfft};
  pocketfft::stride_t stride_in{
      static_cast<ptrdiff_t>(sizeof(std::complex<double>))};
  pocketfft::stride_t stride_out{static_cast<ptrdiff_t>(sizeof(double))};
  pocketfft::c2r(shape, stride_in, stride_out, /*axis=*/0, pocketfft::BACKWARD,
                 in.data(), out.data(), 1.0 / static_cast<double>(nfft));
}

} // namespace

std::vector<float> convolve(const std::vector<float> &x,
                            const std::vector<float> &h) {
  const std::size_t N = x.size();
  const std::size_t M = h.size();
  if (N == 0 || M == 0)
    return {};

  // FFT length: comfortably larger than the IR so each block carries a useful
  // chunk of input. Each block convolves B input samples with the full IR
  // without circular wraparound, which requires nfft >= B + M - 1.
  std::size_t nfft = std::max<std::size_t>(2048, next_pow2(2 * M));
  const std::size_t B = nfft - M + 1; // input samples consumed per block
  const std::size_t ncplx = nfft / 2 + 1;

  // Transform the IR once.
  std::vector<double> buf(nfft, 0.0);
  std::copy(h.begin(), h.end(), buf.begin());
  std::vector<std::complex<double>> H(ncplx);
  rfft(buf, H);

  std::vector<float> y(N + M - 1, 0.0f);
  std::vector<std::complex<double>> X(ncplx);

  for (std::size_t start = 0; start < N; start += B) {
    const std::size_t len = std::min(B, N - start);

    std::fill(buf.begin(), buf.end(), 0.0);
    for (std::size_t i = 0; i < len; ++i)
      buf[i] = x[start + i];

    rfft(buf, X);
    for (std::size_t i = 0; i < ncplx; ++i)
      X[i] *= H[i];
    irfft(X, buf); // buf now holds this block's contribution, length nfft

    for (std::size_t i = 0; i < nfft; ++i) {
      const std::size_t idx = start + i;
      if (idx < y.size())
        y[idx] += static_cast<float>(buf[i]);
    }
  }

  return y;
}
