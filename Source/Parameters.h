#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Engine/ChromeEngine.h"

namespace chaz
{

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using AP = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addF = [&] (const char* id, const char* name, Range r, float def)
    {
        p.push_back (std::make_unique<AP> (juce::ParameterID { id, 1 }, name, r, def));
    };

    // Discrete 0/1 floats (not AudioParameterBool): pluginval state restore is reliable
    addF ("whistleOn", "Whistle On", Range (0.0f, 1.0f, 1.0f), 1.0f);
    addF ("fluteOn",   "Flute On",   Range (0.0f, 1.0f, 1.0f), 0.0f);
    addF ("pluckOn",   "Pluck On",   Range (0.0f, 1.0f, 1.0f), 1.0f);
    addF ("bellOn",    "Bell On",    Range (0.0f, 1.0f, 1.0f), 0.0f);
    addF ("monoLegato","Mono Legato",Range (0.0f, 1.0f, 1.0f), 0.0f);

    addF ("whistleGainDb", "Whistle Gain", Range (-24.0f, 12.0f, 0.1f), 0.0f);
    addF ("fluteGainDb",   "Flute Gain",   Range (-24.0f, 12.0f, 0.1f), -3.0f);
    addF ("pluckGainDb",   "Pluck Gain",   Range (-24.0f, 12.0f, 0.1f), -2.0f);
    addF ("bellGainDb",    "Bell Gain",    Range (-24.0f, 12.0f, 0.1f), -6.0f);

    addF ("whistleBreath",  "Whistle Breath", Range (0.0f, 1.0f, 0.001f), 0.35f);
    addF ("whistleTone",    "Whistle Tone",   Range (0.0f, 1.0f, 0.001f), 0.70f);
    addF ("whistleVib",     "Whistle Vib",    Range (0.0f, 1.0f, 0.001f), 0.45f);
    addF ("whistleVibRate", "Vib Rate",       Range (0.5f, 10.0f, 0.01f), 5.2f);

    addF ("fluteTone",   "Flute Tone",   Range (0.0f, 1.0f, 0.001f), 0.40f);
    addF ("flutePulse",  "Flute Pulse",  Range (0.0f, 1.0f, 0.001f), 0.55f);
    addF ("fluteNoise",  "Flute Noise",  Range (0.0f, 1.0f, 0.001f), 0.18f);
    addF ("fluteAttack", "Flute Attack", Range (0.005f, 0.5f, 0.001f), 0.04f);

    addF ("pluckSharp", "Pluck Sharp", Range (0.0f, 1.0f, 0.001f), 0.75f);
    addF ("pluckBody",  "Pluck Body",  Range (0.0f, 1.0f, 0.001f), 0.45f);
    addF ("pluckDecay", "Pluck Decay", Range (0.05f, 2.0f, 0.001f), 0.35f);
    addF ("pluckTone",  "Pluck Tone",  Range (0.0f, 1.0f, 0.001f), 0.65f);

    addF ("bellMix",   "Bell Mix",   Range (0.0f, 1.0f, 0.001f), 0.55f);
    addF ("bellDecay", "Bell Decay", Range (0.05f, 2.0f, 0.001f), 0.40f);
    addF ("bellTone",  "Bell Tone",  Range (0.0f, 1.0f, 0.001f), 0.80f);

    addF ("ampAttack",  "Amp Atk", Range (0.001f, 1.0f, 0.001f), 0.005f);
    addF ("ampDecay",   "Amp Dec", Range (0.01f, 3.0f, 0.001f), 0.28f);
    addF ("ampSustain", "Amp Sus", Range (0.0f, 1.0f, 0.001f), 0.55f);
    addF ("ampRelease", "Amp Rel", Range (0.01f, 4.0f, 0.001f), 0.35f);

    addF ("cutoff",    "Cutoff",    Range (0.0f, 1.0f, 0.001f), 0.78f);
    addF ("resonance", "Resonance", Range (0.0f, 1.0f, 0.001f), 0.22f);

    addF ("portaTime", "Porta",      Range (0.0f, 1.0f, 0.001f), 0.08f);
    addF ("bendRange", "Bend Range", Range (0.0f, 12.0f, 1.0f), 2.0f);

    addF ("heat",  "Heat",  Range (0.0f, 1.0f, 0.001f), 0.35f);
    addF ("space", "Space", Range (0.0f, 1.0f, 0.001f), 0.28f);
    addF ("width", "Width", Range (0.0f, 1.0f, 0.001f), 0.40f);
    addF ("edge",  "Edge",  Range (0.0f, 1.0f, 0.001f), 0.55f);
    addF ("chaos", "Chaos", Range (0.0f, 1.0f, 0.001f), 0.12f);
    addF ("masterGainDb", "Master", Range (-24.0f, 12.0f, 0.1f), -3.0f);

    return { p.begin(), p.end() };
}

inline ChromeParams readChromeParams (juce::AudioProcessorValueTreeState& apvts)
{
    auto f = [&] (const char* id) -> float
    {
        if (auto* p = apvts.getRawParameterValue (id)) return p->load();
        return 0.0f;
    };
    auto b = [&] (const char* id) -> bool { return f (id) >= 0.5f; };

    ChromeParams cp;
    cp.whistleOn = b ("whistleOn"); cp.fluteOn = b ("fluteOn");
    cp.pluckOn = b ("pluckOn"); cp.bellOn = b ("bellOn");
    cp.monoLegato = b ("monoLegato");
    cp.whistleGainDb = f ("whistleGainDb"); cp.fluteGainDb = f ("fluteGainDb");
    cp.pluckGainDb = f ("pluckGainDb"); cp.bellGainDb = f ("bellGainDb");
    cp.whistleBreath = f ("whistleBreath"); cp.whistleTone = f ("whistleTone");
    cp.whistleVib = f ("whistleVib"); cp.whistleVibRate = f ("whistleVibRate");
    cp.fluteTone = f ("fluteTone"); cp.flutePulse = f ("flutePulse");
    cp.fluteNoise = f ("fluteNoise"); cp.fluteAttack = f ("fluteAttack");
    cp.pluckSharp = f ("pluckSharp"); cp.pluckBody = f ("pluckBody");
    cp.pluckDecay = f ("pluckDecay"); cp.pluckTone = f ("pluckTone");
    cp.bellMix = f ("bellMix"); cp.bellDecay = f ("bellDecay"); cp.bellTone = f ("bellTone");
    cp.ampAttack = f ("ampAttack"); cp.ampDecay = f ("ampDecay");
    cp.ampSustain = f ("ampSustain"); cp.ampRelease = f ("ampRelease");
    cp.cutoff = f ("cutoff"); cp.resonance = f ("resonance");
    cp.portaTime = f ("portaTime"); cp.bendRange = f ("bendRange");
    cp.heat = f ("heat"); cp.space = f ("space"); cp.width = f ("width");
    cp.edge = f ("edge"); cp.chaos = f ("chaos"); cp.masterGainDb = f ("masterGainDb");
    return cp;
}

} // namespace chaz
