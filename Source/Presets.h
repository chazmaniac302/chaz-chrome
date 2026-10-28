#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace chaz
{
struct Preset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
};
const std::vector<Preset>& getFactoryPresets();
}
