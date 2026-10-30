/*
    VocaMax Space 2 - DSP core
    By Mark33s Beats Lab

    Plain C++17, no framework dependencies, so the audio engine can be compiled
    and tested outside of JUCE (see tests/dsp_test.cpp).

    Signal flow:
        low shelf  ->  24 dB/oct high pass  ->  vocal presence peak  ->  air shelf
                   ->  mid / side matrix (width + vocal focus)
                   ->  stereo out
*/

#pragma once

#include <algorithm>
#include <cmath>

namespace vocamax
{
constexpr double kPi = 3.14159265358979323846;

inline float clamp01 (float v) noexcept
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

inline float clampf (float v, float lo, float hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/*  Vocal focus mode adds a small, fixed character on top of the four knobs. */
struct FocusStyle
{
    float widthScale  = 1.0f;
    float presenceDb  = 0.0f;
    float airDb       = 0.0f;
    float lowShelfDb  = 0.0f;
    float highPassScale = 1.0f;
    float midFocus    = 0.0f;
};

inline FocusStyle focusStyleFor (int index) noexcept
{
    switch (index)
    {
        case 0:  return { 0.95f,  0.0f,  0.0f,  0.0f, 1.00f,  0.00f };  // Natural
        case 1:  return { 0.98f,  2.5f,  1.0f, -0.5f, 1.15f,  0.02f };  // Clear
        case 2:  return { 1.35f,  1.0f,  0.5f,  0.0f, 1.00f, -0.02f };  // Wide
        default: return { 1.12f, -1.0f, -2.5f,  3.0f, 0.80f,  0.04f };  // Deep
    }
}

/*  Space knob -> stereo width multiplier. 0 % = tight/narrow, 50 % = natural,
    100 % = wide open. */
inline float widthForSpace (float space) noexcept
{
    space = clamp01 (space);

    if (space <= 0.5f)
        return 0.15f + 0.85f * (space / 0.5f);

    return 1.0f + (space - 0.5f) / 0.5f;
}

/*  Transposed direct form II biquad. */
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1[2] { 0.0f, 0.0f };
    float z2[2] { 0.0f, 0.0f };

    void reset() noexcept
    {
        z1[0] = z1[1] = 0.0f;
        z2[0] = z2[1] = 0.0f;
    }

    inline float process (int ch, float x) noexcept
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }

    void setCoefficients (double nb0, double nb1, double nb2,
                          double na0, double na1, double na2) noexcept
    {
        const double inv = 1.0 / na0;
        b0 = (float) (nb0 * inv);
        b1 = (float) (nb1 * inv);
        b2 = (float) (nb2 * inv);
        a1 = (float) (na1 * inv);
        a2 = (float) (na2 * inv);
    }

    void setHighpass (double sr, double freq, double q = 0.7071067811865476) noexcept
    {
        freq = std::min (freq, sr * 0.45);
        const double w0 = 2.0 * kPi * freq / sr;
        const double cosw = std::cos (w0);
        const double sinw = std::sin (w0);
        const double alpha = sinw / (2.0 * q);

        const double b0v = (1.0 + cosw) * 0.5;
        const double b1v = -(1.0 + cosw);
        const double b2v = (1.0 + cosw) * 0.5;
        const double a0v = 1.0 + alpha;
        const double a1v = -2.0 * cosw;
        const double a2v = 1.0 - alpha;

        setCoefficients (b0v, b1v, b2v, a0v, a1v, a2v);
    }

    void setLowShelf (double sr, double freq, double gainDb, double q = 0.7071067811865476) noexcept
    {
        freq = std::min (freq, sr * 0.45);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * freq / sr;
        const double cosw = std::cos (w0);
        const double sinw = std::sin (w0);
        const double alpha = sinw / (2.0 * q);
        const double beta = 2.0 * std::sqrt (A) * alpha;

        const double b0v = A * ((A + 1.0) - (A - 1.0) * cosw + beta);
        const double b1v = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw);
        const double b2v = A * ((A + 1.0) - (A - 1.0) * cosw - beta);
        const double a0v = (A + 1.0) + (A - 1.0) * cosw + beta;
        const double a1v = -2.0 * ((A - 1.0) + (A + 1.0) * cosw);
        const double a2v = (A + 1.0) + (A - 1.0) * cosw - beta;

        setCoefficients (b0v, b1v, b2v, a0v, a1v, a2v);
    }

    void setHighShelf (double sr, double freq, double gainDb, double q = 0.7071067811865476) noexcept
    {
        freq = std::min (freq, sr * 0.45);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * freq / sr;
        const double cosw = std::cos (w0);
        const double sinw = std::sin (w0);
        const double alpha = sinw / (2.0 * q);
        const double beta = 2.0 * std::sqrt (A) * alpha;

        const double b0v = A * ((A + 1.0) + (A - 1.0) * cosw + beta);
        const double b1v = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw);
        const double b2v = A * ((A + 1.0) + (A - 1.0) * cosw - beta);
        const double a0v = (A + 1.0) - (A - 1.0) * cosw + beta;
        const double a1v = 2.0 * ((A - 1.0) - (A + 1.0) * cosw);
        const double a2v = (A + 1.0) - (A - 1.0) * cosw - beta;

        setCoefficients (b0v, b1v, b2v, a0v, a1v, a2v);
    }

    void setPeak (double sr, double freq, double gainDb, double q) noexcept
    {
        freq = std::min (freq, sr * 0.45);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * freq / sr;
        const double cosw = std::cos (w0);
        const double alpha = std::sin (w0) / (2.0 * q);

        const double b0v = 1.0 + alpha * A;
        const double b1v = -2.0 * cosw;
        const double b2v = 1.0 - alpha * A;
        const double a0v = 1.0 + alpha / A;
        const double a1v = -2.0 * cosw;
        const double a2v = 1.0 - alpha / A;

        setCoefficients (b0v, b1v, b2v, a0v, a1v, a2v);
    }
};

/*  One pole parameter smoother (per sample). */
struct Smoother
{
    float value = 0.0f;
    float target = 0.0f;
    float coeff = 0.003f;

    void setTime (double sr, double ms) noexcept
    {
        const double samples = std::max (1.0, sr * ms * 0.001);
        coeff = (float) (1.0 - std::exp (-1.0 / samples));
    }

    void snap (float v) noexcept { value = target = v; }
    void setTarget (float v) noexcept { target = v; }
    inline float current() const noexcept { return value; }

    inline float next() noexcept
    {
        value += (target - value) * coeff;
        return value;
    }
};

struct Parameters
{
    float power = 1.0f;    // 0..1 overall depth / intensity
    float space = 0.65f;   // 0..1 vocal separation width
    float low = 0.45f;     // 0..1 tighten / remove
    float high = 0.55f;    // 0..1 brighten / remove
    int focus = 1;         // 0 natural, 1 clear, 2 wide, 3 deep
};

class Core
{
public:
    void prepare (double sampleRate) noexcept
    {
        sr = (sampleRate > 1000.0 ? sampleRate : 48000.0);

        sPower.setTime (sr, 18.0);
        sSpace.setTime (sr, 22.0);
        sLow.setTime (sr, 22.0);
        sHigh.setTime (sr, 22.0);

        snapToTargets();
        updateCoefficients();
        reset();
    }

    void reset() noexcept
    {
        hpA.reset();
        hpB.reset();
        lowShelf.reset();
        presence.reset();
        air.reset();
        sideHp.reset();
        counter = 0;
    }

    void setParameters (const Parameters& p) noexcept
    {
        params = p;
        focus = focusStyleFor (p.focus);
        sPower.setTarget (clamp01 (p.power));
        sSpace.setTarget (clamp01 (p.space));
        sLow.setTarget (clamp01 (p.low));
        sHigh.setTarget (clamp01 (p.high));
        dirty = true;
    }

    void snapToTargets() noexcept
    {
        sPower.snap (clamp01 (params.power));
        sSpace.snap (clamp01 (params.space));
        sLow.snap (clamp01 (params.low));
        sHigh.snap (clamp01 (params.high));
        focus = focusStyleFor (params.focus);
    }

    double getSampleRate() const noexcept { return sr; }

    /*  Stereo in place. If right is null the block is treated as mono. */
    void process (float* left, float* right, int numSamples) noexcept
    {
        if (left == nullptr || numSamples <= 0)
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            if ((counter++ & 31) == 0)
                updateCoefficients();

            const float pwr = sPower.next();
            sLow.next();
            sHigh.next();
            const float spaceAmt = sSpace.next();

            float l = left[i];
            float r = (right != nullptr ? right[i] : l);

            const float dryL = l;
            const float dryR = r;

            l = lowShelf.process (0, l);
            r = lowShelf.process (1, r);
            l = hpA.process (0, l);
            r = hpA.process (1, r);
            l = hpB.process (0, l);
            r = hpB.process (1, r);
            l = presence.process (0, l);
            r = presence.process (1, r);
            l = air.process (0, l);
            r = air.process (1, r);

            float mid  = 0.5f * (l + r);
            float side = 0.5f * (l - r);

            // Keep the low end centred: high pass the side signal as the low
            // knob tightens, faded in with the power knob for transparency.
            const float sideCut = clamp01 (pwr * (0.35f + 0.65f * spaceAmt));
            const float sideFiltered = sideHp.process (0, side);
            side = side + (sideFiltered - side) * sideCut;

            const float width = std::min (2.2f,
                1.0f + (widthForSpace (spaceAmt) * focus.widthScale - 1.0f) * pwr);
            const float midGain = 1.0f + ((focus.midFocus + (1.0f - spaceAmt) * 0.10f) * pwr);

            mid *= midGain;
            side *= width;

            // Power at 0 % is an exact, click free bypass of the whole chain.
            const float bypass = clamp01 (pwr * 20.0f);
            const float outL = mid + side;
            const float outR = mid - side;

            left[i] = dryL + (outL - dryL) * bypass;
            if (right != nullptr)
                right[i] = dryR + (outR - dryR) * bypass;
        }

        dirty = false;
    }

private:
    void updateCoefficients() noexcept
    {
        const float pwr = sPower.current();
        const float lowAmt = sLow.current() * pwr;
        const float highKnob = sHigh.current();

        const double hpFreq = 20.0 * std::pow (12.0, (double) lowAmt) * (double) focus.highPassScale;

        hpA.setHighpass (sr, hpFreq, 0.7071);
        hpB.setHighpass (sr, hpFreq, 0.7071);

        lowShelf.setLowShelf (sr, 130.0, -3.5 * (double) lowAmt + (double) focus.lowShelfDb * (double) pwr);

        presence.setPeak (sr, 3200.0, (double) focus.presenceDb * (double) pwr, 0.9);

        const double airDb = ((double) highKnob - 0.5) * 16.0 * (double) pwr
                              + (double) focus.airDb * (double) pwr;
        air.setHighShelf (sr, 9000.0, airDb, 0.7071);

        sideHp.setHighpass (sr, 60.0 + 160.0 * (double) lowAmt, 0.7071);
    }

    double sr = 48000.0;
    Parameters params;
    FocusStyle focus = focusStyleFor (1);

    Smoother sPower, sSpace, sLow, sHigh;

    Biquad hpA, hpB, lowShelf, presence, air, sideHp;
    int counter = 0;
    bool dirty = false;
};

} // namespace vocamax
