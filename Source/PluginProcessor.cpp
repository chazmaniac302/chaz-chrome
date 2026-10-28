#include "PluginProcessor.h"
#include "PluginEditor.h"

ChazChromeProcessor::ChazChromeProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", chaz::createParameterLayout())
{
    currentProgram = 0;
}

void ChazChromeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    engine.setParams (chaz::readChromeParams (apvts));
}

bool ChazChromeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void ChazChromeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    auto cp = chaz::readChromeParams (apvts);
    engine.setParams (cp);

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            engine.noteOff (msg.getNoteNumber());
        else if (msg.isPitchWheel())
            engine.setPitchBend ((msg.getPitchWheelValue() / 8192.0f) - 1.0f);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            engine.allNotesOff();
    }

    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
    engine.process (L, R, n);
}

juce::AudioProcessorEditor* ChazChromeProcessor::createEditor()
{
    return new ChazChromeEditor (*this);
}

int ChazChromeProcessor::getProgramCount() const
{
    return (int) chaz::getFactoryPresets().size();
}

int ChazChromeProcessor::getNumPrograms()
{
    return juce::jmax (1, getProgramCount());
}

const juce::String ChazChromeProcessor::getProgramName (int index)
{
    const auto& ps = chaz::getFactoryPresets();
    if (index >= 0 && index < (int) ps.size())
        return ps[(size_t) index].name;
    return {};
}

void ChazChromeProcessor::applyPreset (int index)
{
    const auto& ps = chaz::getFactoryPresets();
    if (index < 0 || index >= (int) ps.size()) return;
    currentProgram = index;
    for (const auto& kv : ps[(size_t) index].values)
        if (auto* p = apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
    engine.setParams (chaz::readChromeParams (apvts));
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void ChazChromeProcessor::setCurrentProgram (int index)
{
    applyPreset (index);
}

void ChazChromeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("version", JucePlugin_VersionString, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChazChromeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
            apvts.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChazChromeProcessor();
}
