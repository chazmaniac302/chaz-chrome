#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace chazui
{
namespace col
{
    // Cold chrome / ice blue on black
    const juce::Colour bg       { 0xff05070a };
    const juce::Colour panel    { 0xff0c1218 };
    const juce::Colour panel2   { 0xff141c26 };
    const juce::Colour outline  { 0xff2a3a4a };
    const juce::Colour text     { 0xffe8f0f8 };
    const juce::Colour dim      { 0xff7a8fa3 };
    const juce::Colour accent   { 0xff7ec8e8 }; // ice blue
    const juce::Colour accent2  { 0xffc0c8d0 }; // chrome silver
    const juce::Colour accent3  { 0xff4aa8d8 }; // deeper ice
    const juce::Colour good     { 0xff5ad4c8 };
    const juce::Colour padIdle  { 0xff0e1620 };
    const juce::Colour padHit   { 0xff3a6a88 };
}

class ChazLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ChazLookAndFeel()
    {
        setColourScheme ({ col::bg, col::panel, col::panel2, col::outline, col::text, col::accent,
                           juce::Colours::black, col::accent, col::text });
        setColour (juce::ResizableWindow::backgroundColourId, col::bg);
        setColour (juce::TextButton::buttonColourId, col::panel2);
        setColour (juce::TextButton::buttonOnColourId, col::accent);
        setColour (juce::TextButton::textColourOffId, col::text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        setColour (juce::ComboBox::backgroundColourId, col::panel2);
        setColour (juce::ComboBox::outlineColourId, col::outline);
        setColour (juce::ComboBox::arrowColourId, col::dim);
        setColour (juce::PopupMenu::backgroundColourId, col::panel);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::accent3.withAlpha (0.85f));
        setColour (juce::Label::textColourId, col::text);
        setColour (juce::Slider::thumbColourId, col::accent);
        setColour (juce::Slider::trackColourId, col::accent.withAlpha (0.55f));
        setColour (juce::Slider::backgroundColourId, col::panel2);
        setColour (juce::Slider::rotarySliderFillColourId, col::accent);
        setColour (juce::Slider::rotarySliderOutlineColourId, col::outline);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, col::panel2);
        setColour (juce::ToggleButton::textColourId, col::text);
        setColour (juce::ToggleButton::tickColourId, col::accent);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour (col::panel2);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        juce::Path arc;
        arc.addCentredArc (centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f,
                           rotaryStartAngle, toAngle, true);
        auto c = slider.findColour (juce::Slider::rotarySliderFillColourId);
        g.setColour (c);
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        juce::Path pointer;
        pointer.addRectangle (-1.5f, -radius + 6.0f, 3.0f, radius * 0.35f);
        g.setColour (col::text);
        g.fillPath (pointer, juce::AffineTransform::rotation (toAngle).translated (centre));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        auto c = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : backgroundColour;
        if (! b.isEnabled()) c = c.withMultipliedAlpha (0.4f);
        if (down) c = c.darker (0.2f);
        else if (highlighted) c = c.brighter (0.12f);
        g.setColour (c);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (b.getToggleState() ? c.brighter (0.15f) : col::outline);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override
    {
        return juce::Font (juce::FontOptions (juce::jmin (13.0f, (float) h * 0.48f), juce::Font::bold));
    }
};
} // namespace chazui
