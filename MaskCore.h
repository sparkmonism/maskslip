#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>

// Shared pieces for MaskSlip: the cross-instance registry and the masking model.
// Everything here is plain C++ so it can be tested without JUCE.
namespace ms
{
constexpr int kBands = 64;          // log-spaced bands, 20 Hz .. 20 kHz
constexpr int kSlots = 24;          // most instances tracked at once
constexpr int kNameLen = 24;
constexpr float kFloorDb = -120.0f;
constexpr float kSilentDb = -90.0f; // bands below this count as empty

inline float bandEdgeHz (float b)   { return 20.0f * std::pow (1000.0f, b / (float) kBands); }
inline float bandCentreHz (int b)   { return bandEdgeHz ((float) b + 0.5f); }

// Zwicker's critical band rate.
inline float hzToBark (float f)
{
    const float r = f / 7500.0f;
    return 13.0f * std::atan (0.00076f * f) + 3.5f * std::atan (r * r);
}

//==============================================================================
// Band levels of one track, published by its audio thread and read by the UI.
struct TrackData
{
    std::atomic<uint32_t> beat { 0 };
    std::array<std::atomic<float>, kBands> db;

    TrackData() { for (auto& d : db) d.store (kFloorDb, std::memory_order_relaxed); }

    void write (const float* v)
    {
        for (int b = 0; b < kBands; ++b)
            db[(size_t) b].store (v[b], std::memory_order_relaxed);
        beat.fetch_add (1, std::memory_order_release);
    }

    void read (float* out) const
    {
        for (int b = 0; b < kBands; ++b)
            out[b] = db[(size_t) b].load (std::memory_order_relaxed);
    }
};

struct Slot
{
    std::atomic<int> used { 0 };
    std::array<std::atomic<char>, kNameLen> name;
    TrackData data;

    Slot() { for (auto& c : name) c.store (0, std::memory_order_relaxed); }

    void setName (const std::string& s)
    {
        for (int i = 0; i < kNameLen; ++i)
        {
            const char c = i < (int) s.size() && i < kNameLen - 1 ? s[(size_t) i] : 0;
            name[(size_t) i].store (c, std::memory_order_relaxed);
        }
    }

    std::string getName() const
    {
        std::string s;
        for (int i = 0; i < kNameLen; ++i)
        {
            const char c = name[(size_t) i].load (std::memory_order_relaxed);
            if (c == 0) break;
            s.push_back (c);
        }
        return s;
    }
};

// One registry per plugin binary: every MaskSlip instance that the host loads
// into the same process finds the others here.
class Registry
{
public:
    static Registry& get()
    {
        static Registry r;
        return r;
    }

    int claim()
    {
        for (int i = 0; i < kSlots; ++i)
        {
            int expected = 0;
            if (slots[(size_t) i].used.compare_exchange_strong (expected, 1))
                return i;
        }
        return -1;
    }

    void release (int i)
    {
        if (i >= 0 && i < kSlots)
        {
            for (auto& d : slots[(size_t) i].data.db) d.store (kFloorDb, std::memory_order_relaxed);
            slots[(size_t) i].used.store (0);
        }
    }

    Slot& slot (int i) { return slots[(size_t) i]; }

private:
    std::array<Slot, kSlots> slots;
};

//==============================================================================
// Masking model. A masker at band k raises the threshold at band v by the
// Schroeder spreading function (it spreads further upward than downward).
//==============================================================================
struct SpreadMatrix
{
    std::array<std::array<float, kBands>, kBands> gain {}; // [victim][masker], linear power

    SpreadMatrix()
    {
        std::array<float, kBands> bark {};
        for (int b = 0; b < kBands; ++b)
            bark[(size_t) b] = hzToBark (bandCentreHz (b));

        for (int v = 0; v < kBands; ++v)
            for (int k = 0; k < kBands; ++k)
            {
                const float dz = bark[(size_t) v] - bark[(size_t) k];
                const float x = dz + 0.474f;
                const float sf = 15.81f + 7.5f * x - 17.5f * std::sqrt (1.0f + x * x);
                gain[(size_t) v][(size_t) k] = std::pow (10.0f, sf * 0.1f);
            }
    }

    static const SpreadMatrix& get()
    {
        static const SpreadMatrix m;
        return m;
    }
};

// Masking threshold (dB) that a single masker creates in every band.
inline void thresholdFrom (const float* maskerDb, float offsetDb, float* outDb)
{
    const auto& sm = SpreadMatrix::get();
    std::array<float, kBands> p {};
    for (int k = 0; k < kBands; ++k)
        p[(size_t) k] = maskerDb[k] > kSilentDb ? std::pow (10.0f, maskerDb[k] * 0.1f) : 0.0f;

    for (int v = 0; v < kBands; ++v)
    {
        float acc = 0.0f;
        for (int k = 0; k < kBands; ++k)
            acc += p[(size_t) k] * sm.gain[(size_t) v][(size_t) k];
        outDb[v] = acc > 0.0f ? 10.0f * std::log10 (acc) - offsetDb : kFloorDb;
    }
}

struct MaskResult
{
    std::array<float, kBands> threshold {};  // dB
    std::array<float, kBands> depth {};      // dB the victim sits under the threshold (0 = audible)
    std::array<int, kBands> who {};          // index (into the input list) of the dominant track, -1 = none

    MaskResult()
    {
        threshold.fill (kFloorDb);
        depth.fill (0.0f);
        who.fill (-1);
    }
};

// "What masks me": one victim, several maskers combined.
inline MaskResult maskedBy (const float* victimDb, const float* const* maskerDb, int numMaskers, float offsetDb)
{
    MaskResult r;
    std::array<float, kBands> total {}, thr {};
    std::array<float, kBands> best {};
    best.fill (kFloorDb);

    for (int m = 0; m < numMaskers; ++m)
    {
        thresholdFrom (maskerDb[m], offsetDb, thr.data());
        for (int v = 0; v < kBands; ++v)
        {
            total[(size_t) v] += thr[(size_t) v] > kFloorDb ? std::pow (10.0f, thr[(size_t) v] * 0.1f) : 0.0f;
            if (thr[(size_t) v] > best[(size_t) v])
            {
                best[(size_t) v] = thr[(size_t) v];
                r.who[(size_t) v] = m;
            }
        }
    }

    for (int v = 0; v < kBands; ++v)
    {
        r.threshold[(size_t) v] = total[(size_t) v] > 0.0f ? 10.0f * std::log10 (total[(size_t) v]) : kFloorDb;
        if (victimDb[v] > kSilentDb && r.threshold[(size_t) v] > kFloorDb)
            r.depth[(size_t) v] = std::clamp (r.threshold[(size_t) v] - victimDb[v], 0.0f, 40.0f);
    }
    return r;
}

// "What I mask": one masker, several victims; depth is the worst case over the victims.
inline MaskResult maskingOf (const float* maskerDb, const float* const* victimDb, int numVictims, float offsetDb)
{
    MaskResult r;
    thresholdFrom (maskerDb, offsetDb, r.threshold.data());

    for (int v = 0; v < kBands; ++v)
        for (int i = 0; i < numVictims; ++i)
            if (victimDb[i][v] > kSilentDb && r.threshold[(size_t) v] > kFloorDb)
            {
                const float d = std::clamp (r.threshold[(size_t) v] - victimDb[i][v], 0.0f, 40.0f);
                if (d > r.depth[(size_t) v])
                {
                    r.depth[(size_t) v] = d;
                    r.who[(size_t) v] = i;
                }
            }
    return r;
}

} // namespace ms
