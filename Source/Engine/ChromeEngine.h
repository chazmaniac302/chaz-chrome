// Chaz Chrome — melodic trap / hip-hop lead DSP (pure C++17, no JUCE).
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace chaz
{

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float clamp01 (float v) noexcept { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
inline float softClip (float x) noexcept { return std::tanh (x); }
inline float midiToHz (float note) noexcept
{
    return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
}
inline float fastSin (double x) noexcept { return (float) std::sin (x); }

//==============================================================================
struct ChromeParams
{
    // Layers
    bool  whistleOn = true;
    bool  fluteOn   = false;
    bool  pluckOn   = true;
    bool  bellOn    = false;
    float whistleGainDb = 0.0f;
    float fluteGainDb   = -3.0f;
    float pluckGainDb   = -2.0f;
    float bellGainDb    = -6.0f;

    // Whistle / air
    float whistleBreath = 0.35f; // noise breath mix
    float whistleTone   = 0.70f; // brightness
    float whistleVib    = 0.45f; // vibrato depth
    float whistleVibRate = 5.2f; // Hz

    // Dark flute / wood
    float fluteTone   = 0.40f; // filter brightness
    float flutePulse  = 0.55f; // pulse vs sine mix
    float fluteNoise  = 0.18f; // wood air
    float fluteAttack = 0.04f;

    // Chrome pluck
    float pluckSharp  = 0.75f; // transient
    float pluckBody   = 0.45f; // FM body amount
    float pluckDecay  = 0.35f; // seconds-ish
    float pluckTone   = 0.65f;

    // Bell / ice
    float bellMix    = 0.55f;
    float bellDecay  = 0.40f;
    float bellTone   = 0.80f;

    // Amp ADSR
    float ampAttack  = 0.005f;
    float ampDecay   = 0.28f;
    float ampSustain = 0.55f;
    float ampRelease = 0.35f;

    // Filter
    float cutoff    = 0.78f;
    float resonance = 0.22f;

    // Portamento / mono
    float portaTime   = 0.08f; // seconds 0..1 mapped
    float bendRange   = 2.0f;  // semitones
    bool  monoLegato  = false;

    // Macros
    float heat  = 0.35f; // drive/sat
    float space = 0.28f; // short plate/room
    float width = 0.40f;
    float edge  = 0.55f; // brightness / filter push
    float chaos = 0.12f; // pitch/timbre instability

    float masterGainDb = -3.0f;
    float pitchBend    = 0.0f; // -1..1 from MIDI
};

//==============================================================================
class Smoother
{
public:
    void prepare (double sr, double rampSec) noexcept
    {
        rampLength = std::max (1, (int) std::lround (sr * rampSec));
    }
    void snap (float v) noexcept { current = target = v; remaining = 0; }
    void setTarget (float v) noexcept
    {
        if (v == target) return;
        target = v;
        remaining = rampLength;
        step = (target - current) / (float) rampLength;
    }
    float next() noexcept
    {
        if (remaining <= 0) return target;
        if (--remaining == 0) current = target;
        else current += step;
        return current;
    }
    float get() const noexcept { return current; }
private:
    float current = 0, target = 0, step = 0;
    int rampLength = 1, remaining = 0;
};

//==============================================================================
class OnePoleLP
{
public:
    void reset() noexcept { z = 0; }
    void setCutoff (float hz, double sr) noexcept
    {
        hz = std::max (20.0f, std::min (hz, (float) (sr * 0.45)));
        const float x = (float) std::exp (-2.0 * M_PI * (double) hz / sr);
        a = 1.0f - x;
        b = x;
    }
    float process (float x) noexcept { z = a * x + b * z; return z; }
private:
    float a = 1, b = 0, z = 0;
};

//==============================================================================
class OnePoleHP
{
public:
    void reset() noexcept { z = 0; x1 = 0; }
    void setCutoff (float hz, double sr) noexcept
    {
        hz = std::max (20.0f, std::min (hz, (float) (sr * 0.45)));
        const float x = (float) std::exp (-2.0 * M_PI * (double) hz / sr);
        a = x;
    }
    float process (float in) noexcept
    {
        z = a * (z + in - x1);
        x1 = in;
        return z;
    }
private:
    float a = 0, z = 0, x1 = 0;
};

//==============================================================================
class ChromeEngine
{
public:
    static constexpr int kMaxVoices = 12;

    void prepare (double sampleRate, int /*block*/) noexcept;
    void reset() noexcept;
    void setParams (const ChromeParams& p) noexcept { params = p; applyMacroTargets(); }
    const ChromeParams& getParams() const noexcept { return params; }

    void noteOn (int midiNote, float velocity /*0..1*/) noexcept;
    void noteOff (int midiNote) noexcept;
    void allNotesOff() noexcept;
    void setPitchBend (float norm /*-1..1*/) noexcept { params.pitchBend = std::max (-1.0f, std::min (1.0f, norm)); }

    void process (float* left, float* right, int numSamples) noexcept;

    float getMeter() const noexcept { return meter; }
    int   getActiveVoices() const noexcept;

private:
    struct Voice
    {
        bool active = false;
        int note = 60;
        int targetNote = 60;
        float vel = 1;
        int age = 0;
        bool releasing = false;
        bool legato = false;

        float currentHz = 440.0f;
        float targetHz = 440.0f;

        // Amp ADSR
        float ampEnv = 0;
        int ampStage = 0;

        // Whistle
        double whPhase = 0;
        float vibPhase = 0;
        OnePoleLP whLp;
        OnePoleHP whHp;

        // Flute
        double flPhase = 0;
        float flEnv = 0;
        int flStage = 0;
        OnePoleLP flLp;

        // Pluck
        double plPhase = 0, plMod = 0;
        float plEnv = 0;
        float plClick = 0;
        OnePoleLP plLp;

        // Bell
        double bellPhase = 0, bellMod = 0;
        float bellEnv = 0;

        OnePoleLP voiceLp;
        float chaosPhase = 0;
        float chaosAmt = 0;
    };

    void renderVoice (Voice& v, float& L, float& R) noexcept;
    void applyMacroTargets() noexcept;
    float noise() noexcept;
    Voice* allocVoice (int note, bool preferStealSame) noexcept;
    Voice* findVoice (int note) noexcept;
    void startVoice (Voice& v, int note, float velocity, bool legato) noexcept;

    ChromeParams params;
    double sr = 44100.0;
    float invSr = 1.0f / 44100.0f;

    std::array<Voice, kMaxVoices> voices {};
    int lastNote = -1;
    int heldCount = 0;
    std::array<int, 128> heldNotes {};
    int heldSize = 0;

    OnePoleLP masterLpL, masterLpR;
    OnePoleHP edgeHpL, edgeHpR;
    Smoother heatSm, spaceSm, widthSm, edgeSm, chaosSm, masterSm, cutSm;

    static constexpr int kDelayLen = 16384;
    std::array<float, kDelayLen> delayL {}, delayR {};
    int delayPos = 0;

    uint32_t rng = 0xC4A7C402u;
    float meter = 0;
};

} // namespace chaz
