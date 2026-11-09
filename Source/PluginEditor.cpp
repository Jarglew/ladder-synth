#include "PluginEditor.h"
#include "Presets.h"

namespace
{
const juce::Colour panelColour (0xff17171a);
const juce::Colour panelLight (0xff222227);
const juce::Colour lineColour (0xff3a3a42);
const juce::Colour legendColour (0xffefe6d0);
const juce::Colour dimLegend (0xffa59d8a);
const juce::Colour accent (0xffe8a33d);
const juce::Colour woodDark (0xff4a2a17);
const juce::Colour woodLight (0xff7a4a2a);

juce::Font legendFont (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}
} // namespace

// ============================================================== look and feel

PanelLookAndFeel::PanelLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, legendColour);
    setColour (juce::Label::textColourId, legendColour);
    setColour (juce::ComboBox::backgroundColourId, panelLight);
    setColour (juce::ComboBox::textColourId, legendColour);
    setColour (juce::ComboBox::outlineColourId, lineColour);
    setColour (juce::ComboBox::arrowColourId, accent);
    setColour (juce::PopupMenu::backgroundColourId, panelLight);
    setColour (juce::PopupMenu::textColourId, legendColour);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.85f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::BubbleComponent::backgroundColourId, panelLight);
    setColour (juce::BubbleComponent::outlineColourId, accent);
    setColour (juce::TooltipWindow::backgroundColourId, panelLight);
    setColour (juce::TooltipWindow::textColourId, legendColour);
    setColour (juce::TooltipWindow::outlineColourId, lineColour);
}

void PanelLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (2.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    // Scale ticks around the knob
    g.setColour (dimLegend);
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const auto inner = centre.getPointOnCircumference (radius * 0.86f, a);
        const auto outer = centre.getPointOnCircumference (radius * (i % 5 == 0 ? 1.0f : 0.95f), a);
        g.drawLine ({ inner, outer }, i % 5 == 0 ? 1.4f : 0.9f);
    }

    // Value arc
    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius * 0.78f, radius * 0.78f, 0.0f, startAngle, angle, true);
    g.setColour (accent.withAlpha (0.8f));
    g.strokePath (arc, juce::PathStrokeType (2.0f));

    // Knob body with a soft top-left highlight
    const float bodyR = radius * 0.68f;
    const auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (centre);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3c3c42), centre.translated (-bodyR * 0.5f, -bodyR * 0.6f),
                                             juce::Colour (0xff0b0b0c), centre.translated (bodyR * 0.6f, bodyR * 0.8f), true));
    g.fillEllipse (body);
    g.setColour (juce::Colour (0xff55555c));
    g.drawEllipse (body, 1.0f);

    // Pointer
    g.setColour (legendColour);
    g.drawLine ({ centre.getPointOnCircumference (bodyR * 0.25f, angle), centre.getPointOnCircumference (bodyR * 0.92f, angle) }, 2.2f);
}

void PanelLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto bounds = b.getLocalBounds().toFloat();
    const auto lamp = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ bounds.getX() + 9.0f, bounds.getCentreY() });
    const bool on = b.getToggleState();

    g.setColour (on ? accent : juce::Colour (0xff2c2c31));
    g.fillRoundedRectangle (lamp, 3.0f);
    if (on)
    {
        g.setColour (accent.withAlpha (0.25f));
        g.fillRoundedRectangle (lamp.expanded (3.0f), 5.0f);
    }
    g.setColour (highlighted ? legendColour : lineColour.brighter (0.4f));
    g.drawRoundedRectangle (lamp, 3.0f, 1.0f);

    g.setColour (on ? legendColour : dimLegend);
    g.setFont (legendFont (12.5f));
    g.drawText (b.getButtonText(), bounds.withTrimmedLeft (22.0f), juce::Justification::centredLeft);
}

void PanelLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (width, height).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (box.hasKeyboardFocus (true) ? accent : box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) width - 12.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (accent);
    g.fillPath (arrow);
}

juce::Font PanelLookAndFeel::getComboBoxFont (juce::ComboBox&) { return legendFont (13.0f); }
juce::Font PanelLookAndFeel::getPopupMenuFont() { return legendFont (14.0f); }

void PanelLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 22, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

// ============================================================== panel

SynthPanel::SynthPanel (LadderSynthProcessor& p) : processor (p)
{
    setLookAndFeel (&lookAndFeel);
    using namespace ParamIDs;

    addKnob (masterTune, "Tune");
    addKnob (glide, "Glide");
    addKnob (modMix, "Mod Mix");
    addKnob (modAmount, "Mod Amount");
    addToggle (oscMod, "Osc Mod");
    addToggle (filterMod, "Filter Mod");
    addChoice (voiceMode, "Voices");
    addToggle (legato, "Legato");

    addChoice (osc1Range, "Range");
    addChoice (osc1Wave, "Wave");
    addChoice (osc2Range, "Range");
    addKnob (osc2Tune, "Tune");
    addChoice (osc2Wave, "Wave");
    addChoice (osc3Range, "Range");
    addKnob (osc3Tune, "Tune");
    addChoice (osc3Wave, "Wave");
    addToggle (osc3KeyTrack, "Keyboard");

    addKnob (mixOsc1, "Osc 1");
    addKnob (mixOsc2, "Osc 2");
    addKnob (mixOsc3, "Osc 3");
    addKnob (mixNoise, "Noise");
    addChoice (noiseColour, "");

    addKnob (cutoff, "Cutoff");
    addKnob (emphasis, "Emphasis");
    addKnob (contour, "Contour");
    addKnob (drive, "Drive");
    addChoice (keyTrack, "Key Track");
    addKnob (fAttack, "Attack");
    addKnob (fDecay, "Decay");
    addKnob (fSustain, "Sustain");
    addKnob (fRelease, "Release");
    addKnob (aAttack, "Attack");
    addKnob (aDecay, "Decay");
    addKnob (aSustain, "Sustain");
    addKnob (aRelease, "Release");

    addKnob (volume, "Volume");
    addKnob (velAmp, "Vel > Amp");
    addKnob (velFilter, "Vel > Filter");
    addKnob (bendRange, "Bend Range");

    knobs[masterTune].slider->setTooltip ("Master tune, in cents. Double-click any knob to reset it.");
    knobs[modMix].slider->setTooltip ("Modulation source: left is oscillator 3, right is noise.");
    knobs[modAmount].slider->setTooltip ("Modulation depth, added to your mod wheel.");
    knobs[contour].slider->setTooltip ("How far the filter envelope opens the cutoff.");
    knobs[emphasis].slider->setTooltip ("Resonance. Near the top the filter whistles on its own.");
    toggles[osc3KeyTrack].button->setTooltip ("Off: oscillator 3 drones at a fixed pitch. Use with the Lo range as an LFO.");

    addAndMakeVisible (presetBox);
    presetBox.setTooltip ("Factory presets");
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedItemIndex();
        if (index >= 0 && index != processor.getCurrentProgram())
            processor.setCurrentProgram (index);
    };

    setSize (designWidth, designHeight);
}

SynthPanel::~SynthPanel()
{
    setLookAndFeel (nullptr);
}

void SynthPanel::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetBox.addItem (processor.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (processor.getCurrentProgram(), juce::dontSendNotification);
}

void SynthPanel::addKnob (const char* paramId, const juce::String& text)
{
    Knob k;
    k.slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    k.slider->setPopupDisplayEnabled (true, true, this);
    k.slider->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    if (auto* param = processor.apvts.getParameter (paramId))
        k.slider->setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    addAndMakeVisible (*k.slider);

    k.label = std::make_unique<juce::Label> ("", text);
    k.label->setJustificationType (juce::Justification::centred);
    k.label->setFont (legendFont (12.5f));
    k.label->setInterceptsMouseClicks (false, false);
    addAndMakeVisible (*k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramId, *k.slider);
    knobs[paramId] = std::move (k);
}

void SynthPanel::addChoice (const char* paramId, const juce::String& text)
{
    Choice c;
    c.box = std::make_unique<juce::ComboBox>();
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (processor.apvts.getParameter (paramId)))
        c.box->addItemList (param->choices, 1);
    addAndMakeVisible (*c.box);

    c.label = std::make_unique<juce::Label> ("", text);
    c.label->setJustificationType (juce::Justification::centred);
    c.label->setFont (legendFont (12.5f));
    addAndMakeVisible (*c.label);

    c.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, paramId, *c.box);
    choices[paramId] = std::move (c);
}

void SynthPanel::addToggle (const char* paramId, const juce::String& text)
{
    Toggle t;
    t.button = std::make_unique<juce::ToggleButton> (text);
    addAndMakeVisible (*t.button);
    t.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, paramId, *t.button);
    toggles[paramId] = std::move (t);
}

void SynthPanel::placeKnob (const char* paramId, juce::Rectangle<int> cell, int knobSize)
{
    // Knob with its label directly underneath, the pair centred in the cell.
    auto& k = knobs.at (paramId);
    const int labelH = 18;
    auto pair = cell.withSizeKeepingCentre (cell.getWidth(), knobSize + labelH);
    k.slider->setBounds (pair.removeFromTop (knobSize).withSizeKeepingCentre (knobSize, knobSize));
    k.label->setBounds (pair);
}

void SynthPanel::placeChoice (const char* paramId, juce::Rectangle<int> cell)
{
    auto& c = choices.at (paramId);
    auto labelArea = cell.removeFromBottom (18);
    c.box->setBounds (cell.withSizeKeepingCentre (cell.getWidth(), 24));
    c.label->setBounds (labelArea);
}

void SynthPanel::resized()
{
    using namespace ParamIDs;
    sections.clear();
    rowLabels.clear();

    auto area = getLocalBounds().reduced (28, 0); // wood cheeks
    auto top = area.removeFromTop (56);
    presetBox.setBounds (top.removeFromRight (240).reduced (16, 14));
    area.removeFromBottom (10);

    const int titleH = 26;
    auto take = [&] (const juce::String& title, int width)
    {
        auto s = area.removeFromLeft (width);
        sections.push_back ({ title, s });
        s.removeFromTop (titleH);
        return s.reduced (8, 4);
    };

    // Controllers
    {
        auto s = take ("CONTROLLERS", 190);
        const int cellW = s.getWidth() / 2;
        auto row = s.removeFromTop (104);
        placeKnob (masterTune, row.removeFromLeft (cellW), 60);
        placeKnob (glide, row, 60);
        row = s.removeFromTop (104);
        placeKnob (modMix, row.removeFromLeft (cellW), 60);
        placeKnob (modAmount, row, 60);
        s.removeFromTop (10);
        row = s.removeFromTop (28);
        toggles.at (oscMod).button->setBounds (row.removeFromLeft (cellW));
        toggles.at (filterMod).button->setBounds (row);
        s.removeFromTop (18);
        row = s.removeFromTop (46);
        placeChoice (voiceMode, row.removeFromLeft (cellW).reduced (4, 0));
        toggles.at (legato).button->setBounds (row.withTrimmedLeft (8).withTrimmedBottom (18));
    }

    // Oscillator bank
    {
        auto s = take ("OSCILLATOR BANK", 310);
        const int rowH = s.getHeight() / 3;
        const char* ranges[] = { osc1Range, osc2Range, osc3Range };
        const char* waves[] = { osc1Wave, osc2Wave, osc3Wave };
        const char* tunes[] = { nullptr, osc2Tune, osc3Tune };

        for (int i = 0; i < 3; ++i)
        {
            auto row = s.removeFromTop (rowH).reduced (0, 6);
            rowLabels.push_back ({ juce::String (i + 1), row.removeFromLeft (22) });
            auto rangeCell = row.removeFromLeft (78).reduced (4, 0);
            auto tuneCell = row.removeFromLeft (84);
            auto waveCell = row.reduced (4, 0);

            if (i == 2)
            {
                // Make room for the keyboard switch under the wave selector.
                toggles.at (osc3KeyTrack).button->setBounds (waveCell.removeFromBottom (26).withTrimmedLeft (6));
            }

            placeChoice (ranges[i], rangeCell.withSizeKeepingCentre (rangeCell.getWidth(), 46));
            placeChoice (waves[i], waveCell.withSizeKeepingCentre (waveCell.getWidth(), 46));
            if (tunes[i] != nullptr)
                placeKnob (tunes[i], tuneCell, 56);
        }
    }

    // Mixer
    {
        auto s = take ("MIXER", 120);
        const int cellH = (s.getHeight() - 30) / 4;
        placeKnob (mixOsc1, s.removeFromTop (cellH), 50);
        placeKnob (mixOsc2, s.removeFromTop (cellH), 50);
        placeKnob (mixOsc3, s.removeFromTop (cellH), 50);
        placeKnob (mixNoise, s.removeFromTop (cellH), 50);
        choices.at (noiseColour).box->setBounds (s.removeFromTop (26).reduced (8, 0));
        choices.at (noiseColour).label->setBounds ({});
    }

    // Modifiers
    {
        auto s = take ("MODIFIERS", 330);
        const int rowH = s.getHeight() / 3;
        const int cellW = s.getWidth() / 4;

        auto row = s.removeFromTop (rowH);
        auto header = row.removeFromTop (22);
        rowLabels.push_back ({ "FILTER", header.removeFromLeft (120) });
        auto keyTrackBox = header.removeFromRight (110);
        choices.at (keyTrack).label->setBounds (header.removeFromRight (70));
        choices.at (keyTrack).box->setBounds (keyTrackBox.reduced (0, 0));
        placeKnob (cutoff, row.removeFromLeft (cellW), 64);
        placeKnob (emphasis, row.removeFromLeft (cellW), 56);
        placeKnob (contour, row.removeFromLeft (cellW), 56);
        placeKnob (drive, row, 56);

        row = s.removeFromTop (rowH);
        rowLabels.push_back ({ "FILTER CONTOUR", row.removeFromTop (22) });
        placeKnob (fAttack, row.removeFromLeft (cellW), 50);
        placeKnob (fDecay, row.removeFromLeft (cellW), 50);
        placeKnob (fSustain, row.removeFromLeft (cellW), 50);
        placeKnob (fRelease, row, 50);

        row = s;
        rowLabels.push_back ({ "LOUDNESS CONTOUR", row.removeFromTop (22) });
        placeKnob (aAttack, row.removeFromLeft (cellW), 50);
        placeKnob (aDecay, row.removeFromLeft (cellW), 50);
        placeKnob (aSustain, row.removeFromLeft (cellW), 50);
        placeKnob (aRelease, row, 50);
    }

    // Output
    {
        auto s = take ("OUTPUT", area.getWidth());
        const int cellH = s.getHeight() / 4;
        placeKnob (volume, s.removeFromTop (cellH), 64);
        placeKnob (velAmp, s.removeFromTop (cellH), 48);
        placeKnob (velFilter, s.removeFromTop (cellH), 48);
        placeKnob (bendRange, s, 48);
    }
}

void SynthPanel::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();

    // Wood end cheeks
    auto drawCheek = [&] (juce::Rectangle<int> r)
    {
        g.setGradientFill (juce::ColourGradient (woodLight, (float) r.getX(), 0.0f,
                                                 woodDark, (float) r.getRight(), 0.0f, false));
        g.fillRect (r);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        for (int yy = r.getY() + 7; yy < r.getBottom(); yy += 13)
            g.drawHorizontalLine (yy, (float) r.getX() + 3.0f, (float) r.getRight() - 3.0f);
    };
    drawCheek (bounds.withWidth (28));
    drawCheek (bounds.withLeft (bounds.getRight() - 28));

    auto body = bounds.reduced (28, 0);
    g.setColour (panelColour);
    g.fillRect (body);

    // Top strip with the name
    auto top = body.removeFromTop (56);
    g.setColour (juce::Colour (0xff0e0e10));
    g.fillRect (top);
    g.setColour (legendColour);
    g.setFont (legendFont (26.0f, true));
    g.drawText ("LADDER", top.withTrimmedLeft (22), juce::Justification::centredLeft);
    g.setColour (accent);
    g.setFont (legendFont (14.0f));
    g.drawText ("24 dB transistor-ladder synthesizer", top.withTrimmedLeft (138), juce::Justification::centredLeft);

    // Sections
    for (size_t i = 0; i < sections.size(); ++i)
    {
        const auto& s = sections[i];
        if (i > 0)
        {
            g.setColour (lineColour);
            g.drawVerticalLine (s.area.getX(), (float) s.area.getY() + 8.0f, (float) s.area.getBottom() - 4.0f);
        }
        g.setColour (legendColour);
        g.setFont (legendFont (12.0f, true));
        g.drawText (s.title, s.area.withHeight (26), juce::Justification::centred);
        g.setColour (accent.withAlpha (0.6f));
        g.fillRect (s.area.withHeight (26).withSizeKeepingCentre (30, 2).translated (0, 12));
    }

    g.setColour (dimLegend);
    for (const auto& [text, r] : rowLabels)
    {
        g.setFont (legendFont (text.length() <= 1 ? 18.0f : 11.0f, true));
        g.drawText (text, r, text.length() <= 1 ? juce::Justification::centred : juce::Justification::centredLeft);
    }
}

// ============================================================== editor

LadderSynthEditor::LadderSynthEditor (LadderSynthProcessor& p) : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (SynthPanel::designWidth * 6 / 10, SynthPanel::designHeight * 6 / 10,
                     SynthPanel::designWidth * 2, SynthPanel::designHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) SynthPanel::designWidth / SynthPanel::designHeight);
    setSize (SynthPanel::designWidth, SynthPanel::designHeight);
}

void LadderSynthEditor::resized()
{
    const float scale = (float) getWidth() / (float) SynthPanel::designWidth;
    panel.setTransform (juce::AffineTransform::scale (scale));
}
