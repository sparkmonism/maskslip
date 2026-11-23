#include "MaskProcessor.h"
#include "PluginEditor.h"

namespace
{
using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
}

Layout MaskProcessor::createLayout()
{
    Layout l;

    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "View",
                                                         juce::StringArray { "What masks me", "What I mask" }, 0));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sens", 1 }, "Sensitivity",
                                                        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 60.0f));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mindepth", 1 }, "Min Depth",
                                                        juce::NormalisableRange<float> (1.0f, 12.0f, 0.1f), 3.0f,
                                                        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "speed", 1 }, "Speed",
                                                         juce::StringArray { "Fast", "Medium", "Slow" }, 1));
    return l;
}

MaskProcessor::MaskProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto& e : excluded)
        e.store (false);

    slot = ms::Registry::get().claim();

    if (! apvts.state.hasProperty ("trackName"))
        apvts.state.setProperty ("trackName", slot >= 0 ? "Track " + juce::String (slot + 1) : juce::String ("Track"), nullptr);

    publishName();
}

MaskProcessor::~MaskProcessor()
{
    ms::Registry::get().release (slot);
}

bool MaskProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out || (in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono()))
        return false;

    const auto& sc = layouts.getChannelSet (true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::stereo() || sc == juce::AudioChannelSet::mono();
}

juce::String MaskProcessor::getTrackName() const
{
    return apvts.state.getProperty ("trackName").toString();
}

void MaskProcessor::setTrackName (const juce::String& n)
{
    apvts.state.setProperty ("trackName", n.substring (0, ms::kNameLen - 1), nullptr);
    publishName();
}

void MaskProcessor::publishName()
{
    if (slot < 0)
        return;

    std::string s;
    for (auto c : getTrackName().substring (0, ms::kNameLen - 1))
        s.push_back (c < 128 && c >= 32 ? (char) c : '?');

    ms::Registry::get().slot (slot).setName (s);
}

void MaskProcessor::prepareToPlay (double sampleRate, int)
{
    mainAnalyzer.prepare (sampleRate);
    scAnalyzer.prepare (sampleRate);
    lastSpeed = -1;
}

void MaskProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // The audio is left exactly as it is; we only listen.
    const int speed = (int) std::lround (apvts.getRawParameterValue ("speed")->load());
    if (speed != lastSpeed)
    {
        mainAnalyzer.setSpeed (speed);
        scAnalyzer.setSpeed (speed);
        lastSpeed = speed;
    }

    const int n = buffer.getNumSamples();

    {
        auto main = getBusBuffer (buffer, true, 0);
        const float* l = main.getReadPointer (0);
        const float* r = main.getNumChannels() > 1 ? main.getReadPointer (1) : l;
        auto& data = ownData();
        for (int i = 0; i < n; ++i)
            mainAnalyzer.push (0.5f * (l[i] + r[i]), data);
    }

    if (auto* bus = getBus (true, 1); bus != nullptr && bus->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0)
        {
            const float* l = sc.getReadPointer (0);
            const float* r = sc.getNumChannels() > 1 ? sc.getReadPointer (1) : l;
            for (int i = 0; i < n; ++i)
                scAnalyzer.push (0.5f * (l[i] + r[i]), scData);
            scActive.store (true);
            return;
        }
    }

    scActive.store (false);
}

juce::AudioProcessorEditor* MaskProcessor::createEditor()
{
    return new MaskEditor (*this);
}

void MaskProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MaskProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            publishName();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MaskProcessor();
}
