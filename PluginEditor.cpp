#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

using namespace mk;

namespace
{
juce::Font caps (float size, float kerning = 0.15f)
{
    return juce::Font (juce::FontOptions (size).withStyle ("Bold")).withExtraKerningFactor (kerning);
}

constexpr float kDialStart = juce::MathConstants<float>::pi * 1.2f;
constexpr float kDialEnd   = juce::MathConstants<float>::pi * 2.8f;

const juce::Colour kTrackColours[8] = {
    juce::Colour (0xffffc857), juce::Colour (0xff8be28b), juce::Colour (0xffc792ff), juce::Colour (0xffff9f6b),
    juce::Colour (0xff6bb6ff), juce::Colour (0xfff08fc0), juce::Colour (0xffb8d86a), juce::Colour (0xffb0b8c8)
};

juce::String formatHz (float f)
{
    if (f >= 10000.0f) return juce::String (f / 1000.0f, 1) + "k";
    if (f >= 1000.0f)  return juce::String (f / 1000.0f, 2) + "k";
    return juce::String ((int) std::round (f));
}

void caption (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> r, juce::Colour c = kMuted,
              float size = 10.5f, juce::Justification j = juce::Justification::centredLeft)
{
    g.setColour (c);
    g.setFont (caps (size, 0.14f));
    g.drawText (text, r, j, false);
}

void box (juce::Graphics& g, juce::Rectangle<float> r, float corner = 10.0f)
{
    g.setColour (kPanel);
    g.fillRoundedRectangle (r, corner);
    g.setColour (kLine);
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
}
} // namespace

//==============================================================================
MaskLookAndFeel::MaskLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, kBg);
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, kAccent.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, kAccent);

    setColour (juce::ComboBox::backgroundColourId, kPanelHi);
    setColour (juce::ComboBox::outlineColourId, kLine);
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::arrowColourId, kMuted);

    setColour (juce::PopupMenu::backgroundColourId, kPanelHi);
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccent);
    setColour (juce::PopupMenu::highlightedTextColourId, kBg);

    setColour (juce::TextEditor::backgroundColourId, kPanelHi);
    setColour (juce::TextEditor::textColourId, kText);
    setColour (juce::TextEditor::outlineColourId, kLine);
    setColour (juce::TextEditor::focusedOutlineColourId, kYou);
    setColour (juce::TextEditor::highlightColourId, kYou.withAlpha (0.3f));
}

void MaskLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
    const float dia = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto c = bounds.withSizeKeepingCentre (dia, dia).getCentre();

    const float radius = dia * 0.5f, thick = 4.5f, arcR = radius - thick * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (kTrack);
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (angle - startAngle > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (kAccent);
        g.strokePath (val, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float innerR = radius - thick - 8.0f;
    g.setColour (kPanelHi);
    g.fillEllipse (c.x - innerR, c.y - innerR, innerR * 2.0f, innerR * 2.0f);
    g.setColour (kLine);
    g.drawEllipse (c.x - innerR, c.y - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -innerR * 0.9f, 3.0f, innerR * 0.5f, 1.5f);
    g.setColour (kText);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void MaskLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                            bool highlighted, bool down)
{
    const auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const float corner = r.getHeight() * 0.5f;
    const bool on = button.getToggleState();

    g.setColour (on ? kAccent : (down ? kAccent.withAlpha (0.35f) : kPanelHi));
    g.fillRoundedRectangle (r, corner);
    if (highlighted && ! on)
    {
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (r, corner);
    }
    g.setColour (on ? kAccent : kLine);
    g.drawRoundedRectangle (r, corner, 1.0f);
}

void MaskLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setColour (button.getToggleState() ? kBg : kText);
    g.setFont (caps (11.0f, 0.12f));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
}

juce::Label* MaskLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::Font (juce::FontOptions (11.5f)));
    l->setColour (juce::Label::textColourId, kText);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

juce::Font MaskLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::Font (juce::FontOptions (12.0f));
}

//==============================================================================
KnobCell::KnobCell (juce::AudioProcessorValueTreeState& apvts, const juce::String& title,
                    const juce::String& paramId, const juce::String& unit, int decimals)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setRotaryParameters (kDialStart, kDialEnd, true);

    label.setText (title.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (caps (10.0f, 0.14f));
    label.setColour (juce::Label::textColourId, kMuted);
    label.setInterceptsMouseClicks (false, false);

    addAndMakeVisible (slider);
    addAndMakeVisible (label);

    att = std::make_unique<SliderAttachment> (apvts, paramId, slider);
    slider.textFromValueFunction = [unit, decimals] (double v)
    {
        return juce::String (v, decimals) + (unit.isEmpty() ? juce::String() : (unit == "%" ? unit : " " + unit));
    };
    slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    slider.updateText();
}

void KnobCell::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

//==============================================================================
juce::Rectangle<float> GraphView::plot() const
{
    return getLocalBounds().toFloat().withTrimmedLeft (38.0f).withTrimmedRight (14.0f)
                           .withTrimmedTop (28.0f).withTrimmedBottom (50.0f);
}

float GraphView::freqToX (float f) const
{
    const auto p = plot();
    return p.getX() + p.getWidth() * std::log (f / 20.0f) / std::log (1000.0f);
}

float GraphView::dbToY (float db) const
{
    const auto p = plot();
    return p.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 100.0f) / 100.0f) * p.getHeight();
}

void GraphView::mouseMove (const juce::MouseEvent& e)
{
    hoverX = plot().contains (e.position) ? e.position.x : -1.0f;
    repaint();
}

void GraphView::mouseExit (const juce::MouseEvent&)
{
    hoverX = -1.0f;
    repaint();
}

void GraphView::paint (juce::Graphics& g)
{
    box (g, getLocalBounds().toFloat());
    const auto pl = plot();

    // grid
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    for (int db = -90; db <= 0; db += 30)
    {
        const float y = dbToY ((float) db);
        g.setColour (kLine.withAlpha (0.7f));
        g.drawHorizontalLine ((int) y, pl.getX(), pl.getRight());
        g.setColour (kMuted);
        g.drawText (juce::String (db), 4, (int) y - 6, 30, 12, juce::Justification::right, false);
    }

    const float gridF[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    for (float f : gridF)
    {
        const float x = freqToX (f);
        g.setColour (kLine.withAlpha (0.7f));
        g.drawVerticalLine ((int) x, pl.getY(), pl.getBottom());
        g.setColour (kMuted);
        g.drawText (formatHz (f), (int) x - 20, (int) pl.getBottom() + 22, 40, 14, juce::Justification::centred, false);
    }

    const auto& me = model.tracks.empty() ? TrackView() : model.tracks[0];
    const auto& res = model.result;
    const bool have = model.numOthers > 0;

    auto bandLeft  = [this] (int b) { return freqToX (ms::bandEdgeHz ((float) b)); };
    auto bandRight = [this] (int b) { return freqToX (ms::bandEdgeHz ((float) b + 1.0f)); };
    auto bandMid   = [this] (int b) { return freqToX (ms::bandCentreHz (b)); };

    auto curve = [&] (const std::array<float, ms::kBands>& db)
    {
        juce::Path p;
        for (int b = 0; b < ms::kBands; ++b)
        {
            const float y = dbToY (std::max (db[(size_t) b], -100.0f));
            if (b == 0) p.startNewSubPath (bandMid (b), y);
            else p.lineTo (bandMid (b), y);
        }
        return p;
    };

    // masked columns
    if (have)
        for (int b = 0; b < ms::kBands; ++b)
        {
            const float d = res.depth[(size_t) b];
            if (d >= model.minDepth)
            {
                g.setColour (kAccent.withAlpha (0.05f + 0.13f * juce::jlimit (0.0f, 1.0f, d / 20.0f)));
                g.fillRect (juce::Rectangle<float> (bandLeft (b), pl.getY(), bandRight (b) - bandLeft (b), pl.getHeight()));
            }
        }

    // other tracks
    for (size_t i = 1; i < model.tracks.size(); ++i)
    {
        const auto& t = model.tracks[i];
        if (! t.alive)
            continue;
        g.setColour (t.colour.withAlpha (t.excluded ? 0.12f : 0.75f));
        g.strokePath (curve (t.db), juce::PathStrokeType (1.3f));
    }

    // masking wedge between this track and the threshold
    if (have && ! model.masker)
        for (int b = 0; b < ms::kBands; ++b)
        {
            const float d = res.depth[(size_t) b];
            if (d < model.minDepth)
                continue;
            const float yT = dbToY (res.threshold[(size_t) b]), yM = dbToY (me.db[(size_t) b]);
            if (yM > yT)
            {
                g.setColour (kAccent.withAlpha (0.35f + 0.4f * juce::jlimit (0.0f, 1.0f, d / 20.0f)));
                g.fillRect (juce::Rectangle<float> (bandLeft (b), yT, bandRight (b) - bandLeft (b), yM - yT));
            }
        }

    // threshold
    if (have)
    {
        g.setColour (kAccent);
        g.strokePath (curve (res.threshold), juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
    }

    // this track
    if (me.alive)
    {
        auto p = curve (me.db);
        juce::Path fill (p);
        fill.lineTo (bandMid (ms::kBands - 1), pl.getBottom());
        fill.lineTo (bandMid (0), pl.getBottom());
        fill.closeSubPath();
        g.setColour (kYou.withAlpha (0.16f));
        g.fillPath (fill);
        g.setColour (kYou);
        g.strokePath (p, juce::PathStrokeType (1.8f));
    }

    // heat strip
    const float stripY = pl.getBottom() + 6.0f, stripH = 12.0f;
    g.setColour (kTrack);
    g.fillRoundedRectangle (pl.getX(), stripY, pl.getWidth(), stripH, 3.0f);
    if (have)
        for (int b = 0; b < ms::kBands; ++b)
        {
            const float d = res.depth[(size_t) b];
            if (d > 0.5f)
            {
                g.setColour (kAccent.withAlpha (juce::jlimit (0.08f, 1.0f, d / 18.0f)));
                g.fillRect (juce::Rectangle<float> (bandLeft (b), stripY, bandRight (b) - bandLeft (b) + 0.5f, stripH));
            }
        }

    // legend
    g.setColour (kYou);
    g.fillRect (pl.getX() + 6.0f, 12.0f, 14.0f, 3.0f);
    caption (g, "THIS TRACK", { (int) pl.getX() + 26, 6, 100, 14 }, kText, 9.5f);
    g.setColour (kAccent);
    g.fillRect (pl.getX() + 130.0f, 12.0f, 14.0f, 3.0f);
    caption (g, model.masker ? "YOUR MASKING REACH" : "MASKING THRESHOLD", { (int) pl.getX() + 150, 6, 180, 14 }, kText, 9.5f);

    // messages
    if (! me.alive)
        caption (g, "WAITING FOR AUDIO ON THIS TRACK - PLAY SOMETHING", pl.toNearestInt(), kMuted, 12.0f,
                 juce::Justification::centred);
    else if (! have)
        caption (g, "NO OTHER TRACKS YET - PUT MASKSLIP ON ANOTHER TRACK, OR TURN ON THE SIDECHAIN INPUT",
                 pl.toNearestInt().withTrimmedBottom (pl.toNearestInt().getHeight() / 2), kMuted, 11.5f,
                 juce::Justification::centred);

    // hover readout
    if (hoverX >= 0.0f && me.alive)
    {
        g.setColour (kText.withAlpha (0.35f));
        g.drawVerticalLine ((int) hoverX, pl.getY(), pl.getBottom());

        const float f = 20.0f * std::pow (1000.0f, (hoverX - pl.getX()) / pl.getWidth());
        const int b = juce::jlimit (0, ms::kBands - 1, (int) std::floor (std::log (f / 20.0f) / std::log (1000.0f) * ms::kBands));

        juce::String text = formatHz (f) + " Hz";
        if (have)
        {
            const float d = res.depth[(size_t) b];
            const int w = res.who[(size_t) b];
            if (d >= model.minDepth && w >= 0 && w < (int) model.tracks.size())
                text += juce::String (model.masker ? "   you cover " : "   masked by ") + model.tracks[(size_t) w].name
                        + "  " + juce::String (d, 1) + " dB";
            else
                text += "   clear";
        }

        g.setFont (caps (11.0f, 0.06f));
        const int tw = (int) g.getCurrentFont().getStringWidthFloat (text) + 16;
        const int tx = (int) juce::jlimit (pl.getX(), pl.getRight() - (float) tw, hoverX + 10.0f);
        g.setColour (kPanelHi);
        g.fillRoundedRectangle ((float) tx, pl.getY() + 4.0f, (float) tw, 22.0f, 6.0f);
        g.setColour (kText);
        g.drawText (text, tx, (int) pl.getY() + 4, tw, 22, juce::Justification::centred, false);
    }
}

//==============================================================================
void TrackList::paint (juce::Graphics& g)
{
    box (g, getLocalBounds().toFloat());
    caption (g, "TRACKS", { 16, 10, 100, 16 }, kMuted);
    caption (g, "CLICK A TRACK TO INCLUDE OR LEAVE IT OUT", { 70, 10, getWidth() - 86, 16 }, kLine.brighter (0.4f), 8.5f,
             juce::Justification::centredRight);

    int y = 34;
    const int rowH = 26;
    const int maxRows = (getHeight() - y - 8) / rowH;

    for (size_t i = 0; i < model.tracks.size(); ++i)
    {
        if ((int) i >= maxRows)
        {
            caption (g, "+ " + juce::String ((int) model.tracks.size() - (int) i) + " MORE", { 16, y, 120, rowH }, kMuted, 10.0f);
            break;
        }

        const auto& t = model.tracks[i];
        const bool dim = t.excluded || (! t.alive && i > 0);
        const auto row = juce::Rectangle<int> (8, y, getWidth() - 16, rowH - 2);

        if (i == 0)
        {
            g.setColour (kPanelHi);
            g.fillRoundedRectangle (row.toFloat(), 6.0f);
        }

        g.setColour (t.colour.withAlpha (dim ? 0.3f : 1.0f));
        g.fillEllipse ((float) row.getX() + 8.0f, (float) row.getCentreY() - 5.0f, 10.0f, 10.0f);

        g.setColour (dim ? kMuted : kText);
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText (t.name, row.getX() + 26, row.getY(), row.getWidth() - 110, row.getHeight(),
                    juce::Justification::centredLeft, true);

        const juce::String status = i == 0 ? "THIS TRACK" : (t.excluded ? "LEFT OUT" : (t.alive ? "ON" : "IDLE"));
        caption (g, status, { row.getRight() - 90, row.getY(), 82, row.getHeight() },
                 i > 0 && ! dim ? kYou : kMuted, 9.5f, juce::Justification::centredRight);
        y += rowH;
    }

    if (model.tracks.size() <= 1)
        caption (g, "NONE FOUND YET", { 16, y + 4, getWidth() - 32, 18 }, kMuted, 10.0f);
}

void TrackList::mouseDown (const juce::MouseEvent& e)
{
    const int idx = (e.y - 34) / 26;
    if (e.y >= 34 && idx >= 1 && idx < (int) model.tracks.size())
        onToggle (idx);
}

//==============================================================================
void ReportPanel::paint (juce::Graphics& g)
{
    box (g, getLocalBounds().toFloat());
    caption (g, model.masker ? "WHERE YOU COVER OTHER TRACKS" : "WHERE YOU ARE MASKED", { 16, 10, getWidth() - 32, 16 }, kMuted);

    if (model.tracks.empty() || ! model.tracks[0].alive || model.numOthers == 0)
    {
        caption (g, "NOTHING TO COMPARE YET", { 16, 40, getWidth() - 32, 20 }, kMuted, 11.0f);
        return;
    }

    if (model.regions.empty())
    {
        caption (g, "NO SIGNIFICANT MASKING", { 16, 40, getWidth() - 32, 20 }, kYou, 11.5f);
        caption (g, "LOWER THE MIN DEPTH OR RAISE SENSITIVITY TO LOOK CLOSER.", { 16, 62, getWidth() - 32, 16 }, kMuted, 9.0f);
        return;
    }

    int y = 34;
    for (const auto& r : model.regions)
    {
        g.setColour (kText);
        g.setFont (caps (13.0f, 0.04f));
        g.drawText (formatHz (r.f0) + " - " + formatHz (r.f1) + " Hz", 16, y, 130, 24, juce::Justification::centredLeft, false);

        if (r.who >= 0 && r.who < (int) model.tracks.size())
        {
            const auto& t = model.tracks[(size_t) r.who];
            g.setColour (t.colour);
            g.fillEllipse (152.0f, (float) y + 7.0f, 10.0f, 10.0f);
            g.setColour (kText);
            g.setFont (juce::Font (juce::FontOptions (13.0f)));
            g.drawText ((model.masker ? "you cover " : "masked by ") + t.name, 170, y, getWidth() - 170 - 80, 24,
                        juce::Justification::centredLeft, true);
        }

        g.setColour (kAccent);
        g.setFont (caps (13.0f, 0.04f));
        g.drawText ("-" + juce::String (r.depth, 1) + " dB", getWidth() - 84, y, 70, 24, juce::Justification::centredRight, false);
        y += 28;
    }
}

//==============================================================================
MaskEditor::MaskEditor (MaskProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      graph (model),
      list (model, [this] (int idx)
      {
          if (idx > 0 && idx < (int) model.tracks.size())
          {
              const int slot = model.tracks[(size_t) idx].slot;
              if (slot >= 0 && slot <= ms::kSlots)
                  proc.excluded[(size_t) slot].store (! proc.excluded[(size_t) slot].load());
          }
      }),
      report (model),
      sensK (p.apvts, "Sensitivity", "sens", "%", 0),
      depthK (p.apvts, "Min depth", "mindepth", "dB", 1)
{
    setLookAndFeel (&lnf);
    setSize (960, 640);

    for (auto* b : { &victimBtn, &maskerBtn })
        addAndMakeVisible (*b);
    victimBtn.onClick = [this] { setMode (0); };
    maskerBtn.onClick = [this] { setMode (1); };

    nameEdit.setText (proc.getTrackName(), false);
    nameEdit.setInputRestrictions (ms::kNameLen - 1);
    nameEdit.setFont (juce::Font (juce::FontOptions (14.0f)));
    nameEdit.setIndents (10, 5);
    nameEdit.onTextChange = [this] { proc.setTrackName (nameEdit.getText()); };
    addAndMakeVisible (nameEdit);

    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("speed")))
        speedBox.addItemList (c->choices, 1);
    speedAtt = std::make_unique<ComboAttachment> (proc.apvts, "speed", speedBox);

    addAndMakeVisible (graph);
    addAndMakeVisible (list);
    addAndMakeVisible (report);
    addAndMakeVisible (sensK);
    addAndMakeVisible (depthK);
    addAndMakeVisible (speedBox);

    updateModel();
    startTimerHz (20);
}

MaskEditor::~MaskEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void MaskEditor::setMode (int mode)
{
    if (auto* prm = proc.apvts.getParameter ("mode"))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) mode));
        prm->endChangeGesture();
    }
}

void MaskEditor::resized()
{
    victimBtn.setBounds (236, 20, 150, 30);
    maskerBtn.setBounds (392, 20, 130, 30);
    nameEdit.setBounds (720, 24, 224, 28);

    graph.setBounds (16, 72, 928, 346);
    list.setBounds (16, 430, 300, 194);
    report.setBounds (328, 430, 404, 194);
    sensK.setBounds (744, 430, 100, 118);
    depthK.setBounds (844, 430, 100, 118);
    speedBox.setBounds (756, 584, 176, 28);
}

void MaskEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    g.setColour (kText);
    g.setFont (juce::Font (juce::FontOptions (30.0f).withStyle ("Bold")).withExtraKerningFactor (0.01f));
    g.drawText ("MaskSlip", 24, 8, 200, 36, juce::Justification::centredLeft, false);
    caption (g, "SONGCRAFT AUDIO", { 26, 42, 180, 14 }, kAccent, 9.5f);

    caption (g, "TRACK NAME", { 724, 8, 120, 14 }, kMuted, 9.5f);

    box (g, { 732.0f, 430.0f, 212.0f, 194.0f });
    caption (g, "SPEED", { 756, 560, 100, 18 }, kMuted, 9.5f);
}

void MaskEditor::updateModel()
{
    auto& reg = ms::Registry::get();

    auto alive = [this] (int idx, uint32_t beat)
    {
        if (beat != lastBeat[(size_t) idx]) { lastBeat[(size_t) idx] = beat; staleTicks[(size_t) idx] = 0; }
        else staleTicks[(size_t) idx] = std::min (staleTicks[(size_t) idx] + 1, 1000);
        return staleTicks[(size_t) idx] < 16; // about 0.8 s at 20 Hz
    };

    model.tracks.clear();

    // this track
    {
        TrackView t;
        t.slot = proc.getSlot();
        t.name = proc.getTrackName();
        t.colour = kYou;
        auto& data = proc.ownData();
        data.read (t.db.data());
        const int idx = t.slot >= 0 ? t.slot : ms::kSlots;
        t.alive = alive (idx, data.beat.load (std::memory_order_acquire));
        model.tracks.push_back (t);
    }

    // others
    for (int s = 0; s < ms::kSlots; ++s)
    {
        auto& slot = reg.slot (s);
        if (s == proc.getSlot() || slot.used.load() == 0)
            continue;

        TrackView t;
        t.slot = s;
        t.name = juce::String (slot.getName());
        if (t.name.isEmpty()) t.name = "Track " + juce::String (s + 1);
        t.colour = kTrackColours[s % 8];
        slot.data.read (t.db.data());
        t.alive = alive (s, slot.data.beat.load (std::memory_order_acquire));
        t.excluded = proc.excluded[(size_t) s].load();
        model.tracks.push_back (t);
    }

    if (proc.sidechainActive())
    {
        TrackView t;
        t.slot = ms::kSlots;
        t.name = "Sidechain";
        t.colour = kTrackColours[7];
        proc.scData.read (t.db.data());
        t.alive = alive (ms::kSlots, proc.scData.beat.load (std::memory_order_acquire));
        t.excluded = proc.excluded[(size_t) ms::kSlots].load();
        model.tracks.push_back (t);
    }

    // masking
    model.masker = proc.apvts.getRawParameterValue ("mode")->load() > 0.5f;
    model.minDepth = proc.apvts.getRawParameterValue ("mindepth")->load();
    const float offset = 20.0f - 0.2f * proc.apvts.getRawParameterValue ("sens")->load();

    std::vector<const float*> others;
    std::vector<int> otherIdx;
    for (size_t i = 1; i < model.tracks.size(); ++i)
        if (model.tracks[i].alive && ! model.tracks[i].excluded)
        {
            others.push_back (model.tracks[i].db.data());
            otherIdx.push_back ((int) i);
        }

    model.numOthers = (int) others.size();
    model.result = {};
    model.regions.clear();

    if (model.numOthers > 0 && model.tracks[0].alive)
    {
        model.result = model.masker ? ms::maskingOf (model.tracks[0].db.data(), others.data(), model.numOthers, offset)
                                    : ms::maskedBy (model.tracks[0].db.data(), others.data(), model.numOthers, offset);

        for (auto& w : model.result.who)
            w = w >= 0 ? otherIdx[(size_t) w] : -1;

        // group neighbouring masked bands into regions
        int b = 0;
        while (b < ms::kBands)
        {
            if (model.result.depth[(size_t) b] < model.minDepth) { ++b; continue; }

            int e = b;
            float maxD = 0.0f, sum = 0.0f;
            std::vector<int> votes (model.tracks.size(), 0);
            while (e < ms::kBands && model.result.depth[(size_t) e] >= model.minDepth)
            {
                maxD = std::max (maxD, model.result.depth[(size_t) e]);
                sum += model.result.depth[(size_t) e];
                if (model.result.who[(size_t) e] >= 0)
                    ++votes[(size_t) model.result.who[(size_t) e]];
                ++e;
            }

            const int who = (int) (std::max_element (votes.begin(), votes.end()) - votes.begin());
            model.regions.push_back ({ ms::bandEdgeHz ((float) b), ms::bandEdgeHz ((float) e), maxD,
                                       votes[(size_t) who] > 0 ? who : -1 });
            (void) sum;
            b = e;
        }

        std::sort (model.regions.begin(), model.regions.end(),
                   [] (const ViewModel::Region& a, const ViewModel::Region& c)
                   {
                       return a.depth * std::log (a.f1 / a.f0 + 1.0f) > c.depth * std::log (c.f1 / c.f0 + 1.0f);
                   });
        if (model.regions.size() > 5)
            model.regions.resize (5);
    }
}

void MaskEditor::timerCallback()
{
    updateModel();

    victimBtn.setToggleState (! model.masker, juce::dontSendNotification);
    maskerBtn.setToggleState (model.masker, juce::dontSendNotification);

    graph.repaint();
    list.repaint();
    report.repaint();
}
