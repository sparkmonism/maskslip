#pragma once

#include "Analyzer.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

// MaskSlip (Songcraft Audio). The audio passes through untouched. Each instance
// analyses its track and shares the band levels with every other instance in the
// same host process, so any instance can show which frequencies other tracks
// are masking (or being masked by). An optional sidechain input adds one more
// track for hosts that keep plugins apart.
class MaskProcessor : public juce::AudioProcessor
{
public:
    MaskProcessor();
    ~MaskProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MaskSlip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Registry slot of this instance (-1 if all slots are taken).
    int getSlot() const { return slot; }
    ms::TrackData& ownData() { return slot >= 0 ? ms::Registry::get().slot (slot).data : scData; }

    juce::String getTrackName() const;
    void setTrackName (const juce::String&);

    bool sidechainActive() const { return scActive.load(); }

    juce::AudioProcessorValueTreeState apvts;

    ms::TrackData scData;                                   // the sidechain input, analysed locally
    std::atomic<bool> scActive { false };
    std::array<std::atomic<bool>, ms::kSlots + 1> excluded {}; // UI switches; [kSlots] = sidechain

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void publishName();

    int slot = -1;
    ms::Analyzer mainAnalyzer, scAnalyzer;
    int lastSpeed = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaskProcessor)
};
