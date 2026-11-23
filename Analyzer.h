#pragma once

#include "MaskCore.h"
#include <juce_dsp/juce_dsp.h>

namespace ms
{
// Streams samples in, publishes smoothed log-band levels every hop.
class Analyzer
{
public:
    static constexpr int kOrder = 12;
    static constexpr int kSize = 1 << kOrder;
    static constexpr int kHop = 1024;

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        for (int i = 0; i < kSize; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * 3.14159265f * (float) i / (float) kSize);

        const float binHz = (float) fs / (float) kSize;
        for (int b = 0; b < kBands; ++b)
        {
            lo[(size_t) b] = bandEdgeHz ((float) b) / binHz;
            hi[(size_t) b] = bandEdgeHz ((float) b + 1.0f) / binHz;
        }

        setSpeed (1);
        reset();
    }

    void reset()
    {
        ring.fill (0.0f);
        pos = count = 0;
        level.fill (kFloorDb);
    }

    // 0 fast, 1 medium, 2 slow
    void setSpeed (int s)
    {
        static const float tau[3] = { 0.15f, 0.5f, 1.5f };
        const float hopSec = (float) kHop / (float) fs;
        release = 1.0f - std::exp (-hopSec / tau[juce::jlimit (0, 2, s)]);
    }

    void push (float x, TrackData& out)
    {
        ring[(size_t) pos] = x;
        pos = (pos + 1) & (kSize - 1);

        if (++count >= kHop)
        {
            count = 0;
            analyse();
            out.write (level.data());
        }
    }

    const std::array<float, kBands>& levels() const { return level; }

private:
    void analyse()
    {
        for (int i = 0; i < kSize; ++i)
            data[(size_t) i] = ring[(size_t) ((pos + i) & (kSize - 1))] * window[(size_t) i];
        std::fill (data.begin() + kSize, data.end(), 0.0f);

        fft.performFrequencyOnlyForwardTransform (data.data());

        // data[0..kSize/2] now holds magnitudes; turn them into power with a sine of
        // amplitude A reading about A in its peak bin (Hann window gain 0.5).
        constexpr int nb = kSize / 2;
        for (int i = 0; i <= nb; ++i)
        {
            const float a = data[(size_t) i] * 4.0f / (float) kSize;
            power[(size_t) i] = a * a;
        }

        const float nyq = (float) fs * 0.5f * 0.98f;
        for (int b = 0; b < kBands; ++b)
        {
            float target = kFloorDb;

            if (bandEdgeHz ((float) b) < nyq)
            {
                const float l = lo[(size_t) b], h = std::min (hi[(size_t) b], (float) nb - 1.0f);
                const float width = h - l;
                float e;

                if (width < 4.0f)
                {
                    // narrow band: integrate the interpolated spectrum
                    constexpr int n = 8;
                    float acc = 0.0f;
                    for (int k = 0; k < n; ++k)
                    {
                        const float x = l + width * ((float) k + 0.5f) / (float) n;
                        const int i = (int) std::floor (x);
                        const float f = x - (float) i;
                        acc += power[(size_t) i] * (1.0f - f) + power[(size_t) std::min (i + 1, nb)] * f;
                    }
                    e = acc / (float) n * std::max (width, 0.0f);
                }
                else
                {
                    e = 0.0f;
                    const int i0 = (int) std::ceil (l), i1 = (int) std::floor (h);
                    for (int i = i0; i <= i1; ++i)
                        e += power[(size_t) i];
                }

                target = std::max (kFloorDb, 10.0f * std::log10 (std::max (e, 1.0e-13f)));
            }

            float& cur = level[(size_t) b];
            cur = target > cur ? cur + (target - cur) * 0.6f : cur + (target - cur) * release;
        }
    }

    double fs = 44100.0;
    float release = 0.2f;

    juce::dsp::FFT fft { kOrder };
    std::array<float, kSize> ring {}, window {};
    std::array<float, 2 * kSize> data {};
    std::array<float, kSize / 2 + 1> power {};
    std::array<float, kBands> lo {}, hi {}, level {};
    int pos = 0, count = 0;
};
} // namespace ms
