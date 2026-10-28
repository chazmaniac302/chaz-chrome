#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ChazLookAndFeel.h"

class MacroStrip final : public juce::Component
{
public:
    explicit MacroStrip (juce::AudioProcessorValueTreeState& s);
    void resized() override;
private:
    juce::OwnedArray<juce::Slider> knobs;
    juce::OwnedArray<juce::Label> labels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> atts;
};

class ChazChromeEditor final : public juce::AudioProcessorEditor,
                               public juce::Timer
{
public:
    explicit ChazChromeEditor (ChazChromeProcessor&);
    ~ChazChromeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void buildKnob (juce::Slider& s, juce::Label& l, const char* name, const char* paramId,
                    juce::Colour fill = chazui::col::accent);
    void refreshPresetBox();
    void syncLayerButtons();
    void bindLayerToggle (juce::ToggleButton& btn, const char* paramId);

    ChazChromeProcessor& proc;
    chazui::ChazLookAndFeel lnf;
    MacroStrip macros;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::ToggleButton whistleBtn { "WHISTLE" }, fluteBtn { "FLUTE" },
                       pluckBtn { "PLUCK" }, bellBtn { "BELL" }, monoBtn { "MONO" };
    juce::Label title, subtitle, status, sectionLead, sectionPluck, sectionAmp;

    juce::Slider whBreath, whTone, whVib, flTone, flPulse, flNoise;
    juce::Label  whBreathL, whToneL, whVibL, flToneL, flPulseL, flNoiseL;

    juce::Slider plSharp, plBody, plDecay, plTone, bellMix, bellDec;
    juce::Label  plSharpL, plBodyL, plDecayL, plToneL, bellMixL, bellDecL;

    juce::Slider ampA, ampD, ampS, ampR, cutoff, reso, porta, master;
    juce::Label  ampAL, ampDL, ampSL, ampRL, cutoffL, resoL, portaL, masterL;

    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtts;

    float meterSmooth = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChazChromeEditor)
};
