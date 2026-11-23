#pragma once

#include "MaskProcessor.h"
#include <functional>
#include <memory>
#include <vector>

// Songcraft Audio palette for MaskSlip: blue-black with a coral "masked" accent.
namespace mk
{
inline const juce::Colour kBg      { 0xff0e131a };
inline const juce::Colour kPanel   { 0xff151c26 };
inline const juce::Colour kPanelHi { 0xff1f2937 };
inline const juce::Colour kLine    { 0xff2d3a4d };
inline const juce::Colour kTrack   { 0xff263244 };
inline const juce::Colour kText    { 0xffe8eef7 };
inline const juce::Colour kMuted   { 0xff8796ab };
inline const juce::Colour kAccent  { 0xffff5e7e };   // masked
inline const juce::Colour kYou     { 0xff7fe3ff };   // this track
} // namespace mk

using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

//==============================================================================
class MaskLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MaskLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

//==============================================================================
struct TrackView
{
    int slot = -1;                 // registry slot, kSlots for the sidechain, -1 for this track when it has none
    juce::String name;
    juce::Colour colour;
    std::array<float, ms::kBands> db {};
    bool alive = false;
    bool excluded = false;
};

struct ViewModel
{
    struct Region { float f0, f1, depth; int who; };

    std::vector<TrackView> tracks;                 // [0] is this track
    ms::MaskResult result;                         // who[] indexes tracks
    std::vector<Region> regions;
    bool masker = false;                           // false: what masks me, true: what I mask
    float minDepth = 3.0f;
    int numOthers = 0;                             // other tracks taking part
};

//==============================================================================
class GraphView : public juce::Component
{
public:
    explicit GraphView (const ViewModel& m) : model (m) {}

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> plot() const;
    float freqToX (float f) const;
    float dbToY (float db) const;

    const ViewModel& model;
    float hoverX = -1.0f;
};

class TrackList : public juce::Component
{
public:
    TrackList (const ViewModel& m, std::function<void (int)> toggle) : model (m), onToggle (std::move (toggle)) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    const ViewModel& model;
    std::function<void (int)> onToggle;
};

class ReportPanel : public juce::Component
{
public:
    explicit ReportPanel (const ViewModel& m) : model (m) {}
    void paint (juce::Graphics&) override;

private:
    const ViewModel& model;
};

//==============================================================================
class KnobCell : public juce::Component
{
public:
    KnobCell (juce::AudioProcessorValueTreeState&, const juce::String& title, const juce::String& paramId,
              const juce::String& unit, int decimals);
    void resized() override;

    juce::Slider slider;
    juce::Label label;

private:
    std::unique_ptr<SliderAttachment> att;
};

//==============================================================================
class MaskEditor : public juce::AudioProcessorEditor,
                   private juce::Timer
{
public:
    explicit MaskEditor (MaskProcessor&);
    ~MaskEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateModel();
    void setMode (int mode);

    MaskProcessor& proc;
    MaskLookAndFeel lnf;

    ViewModel model;
    std::array<uint32_t, ms::kSlots + 1> lastBeat {};
    std::array<int, ms::kSlots + 1> staleTicks {};

    GraphView graph;
    TrackList list;
    ReportPanel report;

    juce::TextButton victimBtn { "WHAT MASKS ME" }, maskerBtn { "WHAT I MASK" };
    juce::TextEditor nameEdit;
    KnobCell sensK, depthK;
    juce::ComboBox speedBox;
    std::unique_ptr<ComboAttachment> speedAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaskEditor)
};
