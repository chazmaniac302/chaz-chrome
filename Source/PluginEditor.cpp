#include "PluginEditor.h"

//==============================================================================
MacroStrip::MacroStrip (juce::AudioProcessorValueTreeState& s)
{
    const char* ids[] = { "heat", "space", "width", "edge", "chaos" };
    const char* names[] = { "HEAT", "SPACE", "WIDTH", "EDGE", "CHAOS" };
    const juce::Colour fills[] = {
        chazui::col::accent2, chazui::col::accent3, chazui::col::accent,
        chazui::col::good, chazui::col::dim
    };
    for (int i = 0; i < 5; ++i)
    {
        auto* sl = knobs.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag,
                                                juce::Slider::TextBoxBelow));
        sl->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 14);
        sl->setColour (juce::Slider::rotarySliderFillColourId, fills[i]);
        addAndMakeVisible (sl);
        auto* lb = labels.add (new juce::Label ({}, names[i]));
        lb->setJustificationType (juce::Justification::centred);
        lb->setColour (juce::Label::textColourId, chazui::col::dim);
        lb->setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        addAndMakeVisible (lb);
        atts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (s, ids[i], *sl));
    }
}

void MacroStrip::resized()
{
    auto r = getLocalBounds();
    const int n = knobs.size();
    if (n == 0) return;
    const int w = r.getWidth() / n;
    for (int i = 0; i < n; ++i)
    {
        auto cell = r.removeFromLeft (w).reduced (4);
        labels[i]->setBounds (cell.removeFromTop (16));
        knobs[i]->setBounds (cell);
    }
}

//==============================================================================
ChazChromeEditor::ChazChromeEditor (ChazChromeProcessor& p)
    : AudioProcessorEditor (p), proc (p), macros (p.apvts)
{
    setLookAndFeel (&lnf);
    setSize (900, 580);
    setResizable (false, false);

    title.setText ("CHAZ CHROME", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, chazui::col::accent);
    addAndMakeVisible (title);

    subtitle.setText ("Chzm  ·  melodic trap / hip-hop lead  ·  whistle · flute · chrome pluck",
                      juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (13.0f)));
    subtitle.setColour (juce::Label::textColourId, chazui::col::dim);
    addAndMakeVisible (subtitle);

    addAndMakeVisible (macros);

    refreshPresetBox();
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int i = presetBox.getSelectedItemIndex();
        if (i >= 0) proc.applyPreset (i);
    };

    prevBtn.onClick = [this]
    {
        int i = juce::jmax (0, proc.getCurrentProgram() - 1);
        proc.applyPreset (i);
        presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
    };
    nextBtn.onClick = [this]
    {
        int i = juce::jmin (proc.getNumPrograms() - 1, proc.getCurrentProgram() + 1);
        proc.applyPreset (i);
        presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
    };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    bindLayerToggle (whistleBtn, "whistleOn");
    bindLayerToggle (fluteBtn, "fluteOn");
    bindLayerToggle (pluckBtn, "pluckOn");
    bindLayerToggle (bellBtn, "bellOn");
    bindLayerToggle (monoBtn, "monoLegato");
    addAndMakeVisible (whistleBtn);
    addAndMakeVisible (fluteBtn);
    addAndMakeVisible (pluckBtn);
    addAndMakeVisible (bellBtn);
    addAndMakeVisible (monoBtn);

    auto sect = [&] (juce::Label& l, const char* t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        l.setColour (juce::Label::textColourId, chazui::col::accent3);
        addAndMakeVisible (l);
    };
    sect (sectionLead, "WHISTLE / FLUTE");
    sect (sectionPluck, "PLUCK / BELL");
    sect (sectionAmp, "AMP / FILTER / GLIDE");

    buildKnob (whBreath, whBreathL, "BREATH", "whistleBreath");
    buildKnob (whTone, whToneL, "W.TONE", "whistleTone");
    buildKnob (whVib, whVibL, "VIB", "whistleVib");
    buildKnob (flTone, flToneL, "F.TONE", "fluteTone", chazui::col::accent3);
    buildKnob (flPulse, flPulseL, "PULSE", "flutePulse", chazui::col::accent3);
    buildKnob (flNoise, flNoiseL, "WOOD", "fluteNoise", chazui::col::accent3);

    buildKnob (plSharp, plSharpL, "SHARP", "pluckSharp", chazui::col::accent2);
    buildKnob (plBody, plBodyL, "BODY", "pluckBody", chazui::col::accent2);
    buildKnob (plDecay, plDecayL, "DECAY", "pluckDecay", chazui::col::accent2);
    buildKnob (plTone, plToneL, "P.TONE", "pluckTone", chazui::col::accent2);
    buildKnob (bellMix, bellMixL, "BELL", "bellMix", chazui::col::good);
    buildKnob (bellDec, bellDecL, "B.DEC", "bellDecay", chazui::col::good);

    buildKnob (ampA, ampAL, "ATK", "ampAttack");
    buildKnob (ampD, ampDL, "DEC", "ampDecay");
    buildKnob (ampS, ampSL, "SUS", "ampSustain");
    buildKnob (ampR, ampRL, "REL", "ampRelease");
    buildKnob (cutoff, cutoffL, "CUT", "cutoff");
    buildKnob (reso, resoL, "RES", "resonance");
    buildKnob (porta, portaL, "PORTA", "portaTime", chazui::col::accent3);
    buildKnob (master, masterL, "OUT", "masterGainDb", chazui::col::good);

    status.setColour (juce::Label::textColourId, chazui::col::dim);
    status.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (status);

    syncLayerButtons();
    startTimerHz (24);
}

ChazChromeEditor::~ChazChromeEditor()
{
    setLookAndFeel (nullptr);
}

void ChazChromeEditor::buildKnob (juce::Slider& s, juce::Label& l, const char* name, const char* paramId,
                                  juce::Colour fill)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 14);
    s.setColour (juce::Slider::rotarySliderFillColourId, fill);
    addAndMakeVisible (s);
    l.setText (name, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId, chazui::col::dim);
    l.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    addAndMakeVisible (l);
    sliderAtts.add (new juce::AudioProcessorValueTreeState::SliderAttachment (proc.apvts, paramId, s));
}

void ChazChromeEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& ps = chaz::getFactoryPresets();
    for (int i = 0; i < (int) ps.size(); ++i)
        presetBox.addItem (ps[(size_t) i].name, i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
}

void ChazChromeEditor::bindLayerToggle (juce::ToggleButton& btn, const char* paramId)
{
    btn.setClickingTogglesState (true);
    btn.onClick = [this, &btn, paramId]
    {
        if (auto* p = proc.apvts.getParameter (paramId))
            p->setValueNotifyingHost (btn.getToggleState() ? 1.0f : 0.0f);
    };
}

void ChazChromeEditor::syncLayerButtons()
{
    auto on = [&] (const char* id) -> bool
    {
        if (auto* p = proc.apvts.getRawParameterValue (id))
            return p->load() >= 0.5f;
        return false;
    };
    whistleBtn.setToggleState (on ("whistleOn"), juce::dontSendNotification);
    fluteBtn.setToggleState (on ("fluteOn"), juce::dontSendNotification);
    pluckBtn.setToggleState (on ("pluckOn"), juce::dontSendNotification);
    bellBtn.setToggleState (on ("bellOn"), juce::dontSendNotification);
    monoBtn.setToggleState (on ("monoLegato"), juce::dontSendNotification);
}

void ChazChromeEditor::timerCallback()
{
    meterSmooth = meterSmooth * 0.85f + proc.engine.getMeter() * 0.15f;
    syncLayerButtons();
    status.setText ("voices " + juce::String (proc.engine.getActiveVoices())
                    + "  ·  meter " + juce::String (meterSmooth, 3)
                    + "  ·  " + juce::String (proc.getNumPrograms()) + " presets",
                    juce::dontSendNotification);
    repaint (getLocalBounds().removeFromBottom (28));
}

void ChazChromeEditor::paint (juce::Graphics& g)
{
    g.fillAll (chazui::col::bg);
    auto header = getLocalBounds().removeFromTop (70).toFloat().reduced (8.0f, 4.0f);
    g.setColour (chazui::col::panel);
    g.fillRoundedRectangle (header, 10.0f);
    g.setColour (chazui::col::accent.withAlpha (0.45f));
    g.drawRoundedRectangle (header, 10.0f, 1.5f);

    auto meterR = juce::Rectangle<float> ((float) getWidth() - 120.0f, 18.0f, 100.0f, 10.0f);
    g.setColour (chazui::col::panel2);
    g.fillRoundedRectangle (meterR, 3.0f);
    g.setColour (chazui::col::accent);
    g.fillRoundedRectangle (meterR.withWidth (meterR.getWidth() * juce::jlimit (0.0f, 1.0f, meterSmooth * 2.0f)), 3.0f);
}

void ChazChromeEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto top = r.removeFromTop (56);
    title.setBounds (top.removeFromLeft (230).reduced (4));
    subtitle.setBounds (top.removeFromLeft (380).reduced (4));
    nextBtn.setBounds (top.removeFromRight (36).reduced (2));
    prevBtn.setBounds (top.removeFromRight (36).reduced (2));
    presetBox.setBounds (top.reduced (4));

    auto layers = r.removeFromTop (36);
    whistleBtn.setBounds (layers.removeFromLeft (90).reduced (2));
    fluteBtn.setBounds (layers.removeFromLeft (80).reduced (2));
    pluckBtn.setBounds (layers.removeFromLeft (80).reduced (2));
    bellBtn.setBounds (layers.removeFromLeft (70).reduced (2));
    monoBtn.setBounds (layers.removeFromLeft (70).reduced (2));

    macros.setBounds (r.removeFromTop (110));

    auto body = r.removeFromTop (290);
    auto colW = body.getWidth() / 3;

    auto placeRow = [] (juce::Rectangle<int> area, std::initializer_list<std::pair<juce::Slider*, juce::Label*>> items)
    {
        const int n = (int) items.size();
        if (n == 0) return;
        const int w = area.getWidth() / n;
        for (auto& it : items)
        {
            auto cell = area.removeFromLeft (w).reduced (2);
            it.second->setBounds (cell.removeFromTop (14));
            it.first->setBounds (cell);
        }
    };

    {
        auto c = body.removeFromLeft (colW).reduced (4);
        sectionLead.setBounds (c.removeFromTop (18));
        placeRow (c, {
            { &whBreath, &whBreathL }, { &whTone, &whToneL }, { &whVib, &whVibL },
            { &flTone, &flToneL }, { &flPulse, &flPulseL }, { &flNoise, &flNoiseL }
        });
    }
    {
        auto c = body.removeFromLeft (colW).reduced (4);
        sectionPluck.setBounds (c.removeFromTop (18));
        placeRow (c, {
            { &plSharp, &plSharpL }, { &plBody, &plBodyL }, { &plDecay, &plDecayL },
            { &plTone, &plToneL }, { &bellMix, &bellMixL }, { &bellDec, &bellDecL }
        });
    }
    {
        auto c = body.reduced (4);
        sectionAmp.setBounds (c.removeFromTop (18));
        placeRow (c, {
            { &ampA, &ampAL }, { &ampD, &ampDL }, { &ampS, &ampSL }, { &ampR, &ampRL },
            { &cutoff, &cutoffL }, { &reso, &resoL }, { &porta, &portaL }, { &master, &masterL }
        });
    }

    status.setBounds (r.removeFromBottom (22));
}
