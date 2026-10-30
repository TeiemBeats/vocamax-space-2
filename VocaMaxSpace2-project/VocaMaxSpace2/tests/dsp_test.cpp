/*
    Headless checks for the VocaMax Space 2 DSP core.
    Build + run:  g++ -std=c++17 -O2 -Wall -Wextra tests/dsp_test.cpp -o /tmp/dsp_test && /tmp/dsp_test
*/

#include "../Source/dsp/VocaMaxDSP.h"

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

namespace
{
int failures = 0;

void check (bool condition, const char* name, double detail = 0.0)
{
    if (condition)
    {
        std::printf ("  PASS  %s\n", name);
    }
    else
    {
        std::printf ("  FAIL  %s (%.6f)\n", name, detail);
        ++failures;
    }
}

struct Stereo
{
    std::vector<float> l, r;
    int n() const { return (int) l.size(); }
};

Stereo makeTone (double sr, double freq, int numSamples, float ampL = 0.25f, float ampR = 0.25f)
{
    Stereo s;
    s.l.resize ((size_t) numSamples);
    s.r.resize ((size_t) numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const float v = (float) (std::sin (2.0 * vocamax::kPi * freq * i / sr));
        s.l[(size_t) i] = v * ampL;
        s.r[(size_t) i] = v * ampR;
    }
    return s;
}

Stereo makeUncorrelated (int numSamples, unsigned seed)
{
    Stereo s;
    s.l.resize ((size_t) numSamples);
    s.r.resize ((size_t) numSamples);

    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        s.l[(size_t) i] = 0.2f * dist (rng);
        s.r[(size_t) i] = 0.2f * dist (rng);
    }
    return s;
}

double rms (const std::vector<float>& v, int skip = 0)
{
    double sum = 0.0;
    int count = 0;

    for (size_t i = (size_t) skip; i < v.size(); ++i)
    {
        sum += (double) v[i] * (double) v[i];
        ++count;
    }
    return count > 0 ? std::sqrt (sum / count) : 0.0;
}

double sideRms (const Stereo& s, int skip = 0)
{
    double sum = 0.0;
    int count = 0;

    for (int i = skip; i < s.n(); ++i)
    {
        const double side = 0.5 * ((double) s.l[(size_t) i] - (double) s.r[(size_t) i]);
        sum += side * side;
        ++count;
    }
    return count > 0 ? std::sqrt (sum / count) : 0.0;
}

double midRms (const Stereo& s, int skip = 0)
{
    double sum = 0.0;
    int count = 0;

    for (int i = skip; i < s.n(); ++i)
    {
        const double mid = 0.5 * ((double) s.l[(size_t) i] + (double) s.r[(size_t) i]);
        sum += mid * mid;
        ++count;
    }
    return count > 0 ? std::sqrt (sum / count) : 0.0;
}

/*  Rough band energy via a one pole split: returns RMS of a band-limited copy. */
double bandRms (const std::vector<float>& in, double sr, double cutoff, bool highPass)
{
    // Two cascaded one pole filters (12 dB/oct) for a gentle measurement split.
    const double a = std::exp (-2.0 * vocamax::kPi * cutoff / sr);
    double z1 = 0.0, z2 = 0.0;
    double sum = 0.0;

    for (float x : in)
    {
        const double low1 = (1.0 - a) * (double) x + a * z1;
        z1 = low1;
        const double low2 = (1.0 - a) * low1 + a * z2;
        z2 = low2;

        const double y = highPass ? (double) x - low2 : low2;
        sum += y * y;
    }
    return std::sqrt (sum / std::max<size_t> (1, in.size()));
}

void runChain (Stereo& s, const vocamax::Parameters& p, double sr)
{
    vocamax::Core core;
    core.prepare (sr);
    core.setParameters (p);
    core.snapToTargets();
    core.process (s.l.data(), s.r.data(), s.n());
}

} // namespace

int main()
{
    const double sr = 44100.0;
    const int n = 44100;

    std::printf ("VocaMax Space 2 DSP checks\n");

    // 1. Power at zero must be effectively transparent.
    {
        auto dry = makeTone (sr, 1000.0, n, 0.3f, 0.2f);
        auto wet = dry;
        vocamax::Parameters p;
        p.power = 0.0f;
        p.space = 0.0f;
        p.low = 0.0f;
        p.high = 0.0f;
        runChain (wet, p, sr);

        double maxErr = 0.0;
        for (int i = 2000; i < n; ++i)
            maxErr = std::max (maxErr, std::abs ((double) wet.l[(size_t) i] - (double) dry.l[(size_t) i]));

        check (maxErr < 2.0e-4, "power 0% is transparent", maxErr);
    }

    // 2. Stability with every parameter at both extremes, all sample rates.
    {
        const double rates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
        bool stable = true;
        double worst = 0.0;

        for (double rate : rates)
        {
            for (int mode = 0; mode < 4; ++mode)
            {
                auto s = makeUncorrelated ((int) (rate * 0.5), 1234u + (unsigned) mode);
                vocamax::Parameters p;
                p.power = 1.0f;
                p.space = (mode % 2 == 0) ? 1.0f : 0.0f;
                p.low = (mode < 2) ? 1.0f : 0.0f;
                p.high = (mode % 2 == 0) ? 1.0f : 0.0f;
                p.focus = mode;
                runChain (s, p, rate);

                for (float v : s.l) { if (! std::isfinite (v)) stable = false; worst = std::max (worst, (double) std::abs (v)); }
                for (float v : s.r) { if (! std::isfinite (v)) stable = false; worst = std::max (worst, (double) std::abs (v)); }
            }
        }

        check (stable, "output is always finite");
        check (worst < 2.0, "extreme settings stay bounded (< 2.0)", worst);
    }

    // 3. Space knob widens / narrows the stereo image.
    {
        const int len = 22050;

        auto narrowIn = makeUncorrelated (len, 77u);
        auto wideIn = narrowIn;

        vocamax::Parameters tight;
        tight.power = 1.0f; tight.space = 0.0f; tight.low = 0.0f; tight.high = 0.5f; tight.focus = 1;
        vocamax::Parameters open;
        open.power = 1.0f; open.space = 1.0f; open.low = 0.0f; open.high = 0.5f; open.focus = 1;

        runChain (narrowIn, tight, sr);
        runChain (wideIn, open, sr);

        auto ref = makeUncorrelated (len, 77u);
        const double refSide = sideRms (ref, 1000);
        const double tightSide = sideRms (narrowIn, 1000);
        const double openSide = sideRms (wideIn, 1000);

        check (tightSide < refSide * 0.5, "space 0% collapses the sides", tightSide / refSide);
        check (openSide > refSide * 1.5, "space 100% widens the sides", openSide / refSide);
        check (openSide > tightSide * 3.0, "space range is wide", openSide / std::max (1e-9, tightSide));
    }

    // 4. Low knob removes low end and leaves the top alone.
    {
        auto bass = makeTone (sr, 50.0, n);
        auto bassRef = bass;
        vocamax::Parameters p;
        p.power = 1.0f; p.space = 0.5f; p.low = 1.0f; p.high = 0.5f; p.focus = 0;
        runChain (bass, p, sr);

        const double before = bandRms (bassRef.l, sr, 400.0, false);
        const double after = bandRms (bass.l, sr, 400.0, false);
        check (after < before * 0.25, "low 100% tightens 50 Hz hard", after / before);

        auto top = makeTone (sr, 5000.0, n);
        auto topRef = top;
        runChain (top, p, sr);
        const double topBefore = bandRms (topRef.l, sr, 400.0, true);
        const double topAfter = bandRms (top.l, sr, 400.0, true);
        check (std::abs (topAfter / topBefore - 1.0) < 0.1, "low knob keeps 5 kHz intact", topAfter / topBefore);
    }

    // 5. High knob brightens / darkens.
    {
        auto makeAndMeasure = [&] (float highKnob)
        {
            auto s = makeTone (sr, 11000.0, n);
            const double before = bandRms (s.l, sr, 6000.0, true);
            vocamax::Parameters p;
            p.power = 1.0f; p.space = 0.5f; p.low = 0.0f; p.high = highKnob; p.focus = 0;
            runChain (s, p, sr);
            return bandRms (s.l, sr, 6000.0, true) / before;
        };

        const double bright = makeAndMeasure (1.0f);
        const double dark = makeAndMeasure (0.0f);

        check (bright > 1.6, "high 100% boosts 11 kHz", bright);
        check (dark < 0.6, "high 0% cuts 11 kHz", dark);
    }

    // 6. Presets / focus modes stay stable and audible.
    {
        bool ok = true;
        double maxAbs = 0.0;

        for (int focus = 0; focus < 4; ++focus)
        {
            auto s = makeUncorrelated (22050, 900u + (unsigned) focus);
            vocamax::Parameters p;
            p.power = 1.0f; p.space = 0.65f; p.low = 0.45f; p.high = 0.55f; p.focus = focus;
            runChain (s, p, sr);

            for (float v : s.l) { if (! std::isfinite (v)) ok = false; maxAbs = std::max (maxAbs, (double) std::abs (v)); }
            for (float v : s.r) { if (! std::isfinite (v)) ok = false; maxAbs = std::max (maxAbs, (double) std::abs (v)); }
        }

        check (ok, "all focus modes finite");
        check (maxAbs < 1.5, "all focus modes bounded", maxAbs);
    }

    // 7. Parameter changes are smoothed (no clicks when jumping).
    {
        const int blockSize = 4096;
        const int blocks = 7;
        std::vector<float> sig ((size_t) (blockSize * blocks));
        std::vector<float> sigR ((size_t) (blockSize * blocks));

        for (size_t i = 0; i < sig.size(); ++i)
        {
            sig[i] = 0.5f * (float) std::sin (2.0 * vocamax::kPi * 220.0 * (double) i / sr);
            sigR[i] = sig[i];
        }

        vocamax::Core core;
        core.prepare (sr);
        vocamax::Parameters quiet;
        quiet.power = 1.0f; quiet.space = 0.0f; quiet.low = 0.0f; quiet.high = 0.0f; quiet.focus = 1;
        core.setParameters (quiet);
        core.snapToTargets();

        core.process (sig.data(), sigR.data(), blockSize);

        vocamax::Parameters loud;
        loud.power = 1.0f; loud.space = 1.0f; loud.low = 1.0f; loud.high = 1.0f; loud.focus = 2;
        core.setParameters (loud);

        double maxJump = 0.0;
        float prev = sig[(size_t) (blockSize - 1)];

        for (int b = 1; b < blocks; ++b)
        {
            float* l = sig.data() + (size_t) (b * blockSize);
            float* r = sigR.data() + (size_t) (b * blockSize);
            core.process (l, r, blockSize);

            for (int i = 0; i < blockSize; ++i)
            {
                maxJump = std::max (maxJump, (double) std::abs (l[i] - prev));
                prev = l[i];
            }
        }

        // A 220 Hz sine at 0.5 amplitude moves about 0.031 per sample.
        check (maxJump < 0.06, "knob jumps are smoothed", maxJump);
    }

    // 8. Mono processing is safe.
    {
        auto s = makeTone (sr, 440.0, 8192, 0.4f, 0.4f);
        vocamax::Core core;
        core.prepare (sr);
        vocamax::Parameters p;
        p.power = 1.0f; p.space = 1.0f; p.low = 1.0f; p.high = 1.0f; p.focus = 2;
        core.setParameters (p);
        core.snapToTargets();
        core.process (s.l.data(), nullptr, s.n());

        bool finite = true;
        for (float v : s.l)
            if (! std::isfinite (v))
                finite = false;

        check (finite, "mono processing is finite");
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
