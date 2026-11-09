#include "PluginEditor.h"
#include "Presets.h"
#include <algorithm>

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

void StepButton::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    const auto r = getLocalBounds().toFloat().reduced (1.5f);
    const bool on = getToggleState();

    g.setColour (on ? accent : juce::Colour (0xff2a2a2f));
    g.fillRoundedRectangle (r, 3.0f);
    if (on)
    {
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRoundedRectangle (r.withTrimmedBottom (r.getHeight() * 0.55f), 3.0f);
    }

    g.setColour (playing ? legendColour : (highlighted ? lineColour.brighter (0.6f) : lineColour.brighter (0.2f)));
    g.drawRoundedRectangle (r, 3.0f, playing ? 2.0f : 1.0f);
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

    // Arpeggiator, trance gate and effects
    addToggle (arpOn, "");
    addChoice (arpMode, "Mode");
    addChoice (arpRate, "Rate");
    addKnob (arpOctaves, "Octaves");
    addKnob (arpGate, "Gate");
    addKnob (arpSwing, "Swing");
    addToggle (arpHold, "Hold");

    addToggle (gateOn, "");
    addChoice (gateRate, "Rate");
    addKnob (gateDepth, "Depth");
    addKnob (gateLength, "Length");
    addKnob (gateSmooth, "Smooth");
    for (int i = 0; i < ladder::GateSettings::numSteps; ++i)
    {
        Step st;
        st.button = std::make_unique<StepButton>();
        st.button->setTooltip ("Gate step " + juce::String (i + 1) + ": lit steps let the sound through.");
        addAndMakeVisible (*st.button);
        st.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, gateSteps[i], *st.button);
        steps.push_back (std::move (st));
    }

    addToggle (chorusOn, "");
    addKnob (chorusRate, "Rate");
    addKnob (chorusDepth, "Depth");
    addKnob (chorusMix, "Mix");

    addToggle (delayOn, "");
    addChoice (delayTime, "Time");
    addToggle (delayPingPong, "Ping-Pong");
    addKnob (delayFeedback, "Feedback");
    addKnob (delayDamp, "Damp");
    addKnob (delayMix, "Mix");

    addToggle (reverbOn, "");
    addKnob (reverbSize, "Size");
    addKnob (reverbDamp, "Damp");
    addKnob (reverbWidth, "Width");
    addKnob (reverbMix, "Mix");

    toggles[arpOn].button->setTooltip ("Arpeggiator on/off. Follows the host tempo; locks to the bar while the host plays.");
    toggles[arpHold].button->setTooltip ("Latch: the pattern keeps playing after you let go. A new chord replaces it.");
    knobs[arpGate].slider->setTooltip ("Note length, as a share of each step.");
    knobs[arpSwing].slider->setTooltip ("Pushes every second step late, for a shuffle.");
    toggles[gateOn].button->setTooltip ("Trance gate on/off: chops the sound to the step pattern below, in time with the host.");
    knobs[gateLength].slider->setTooltip ("How much of each lit step is open. Below 100% even runs of lit steps pulse.");
    knobs[gateSmooth].slider->setTooltip ("Softens the gate's edges.");
    toggles[chorusOn].button->setTooltip ("Chorus on/off");
    toggles[delayOn].button->setTooltip ("Delay on/off. Times follow the host tempo.");
    toggles[delayPingPong].button->setTooltip ("Echoes bounce between left and right.");
    knobs[delayDamp].slider->setTooltip ("Darkens each repeat.");
    toggles[reverbOn].button->setTooltip ("Reverb on/off");

    knobs[masterTune].slider->setTooltip ("Master tune, in cents. Double-click any knob to reset it.");
    knobs[modMix].slider->setTooltip ("Modulation source: left is oscillator 3, right is noise.");
    knobs[modAmount].slider->setTooltip ("Modulation depth, added to your mod wheel.");
    knobs[contour].slider->setTooltip ("How far the filter envelope opens the cutoff.");
    knobs[emphasis].slider->setTooltip ("Resonance. Near the top the filter whistles on its own.");
    toggles[osc3KeyTrack].button->setTooltip ("Off: oscillator 3 drones at a fixed pitch. Use with the Lo range as an LFO.");

    addAndMakeVisible (presetBox);
    presetBox.setTooltip ("Factory presets, by category");
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0 && index != processor.getCurrentProgram())
            processor.setCurrentProgram (index);
    };

    for (auto* b : { &prevPreset, &nextPreset })
    {
        b->setColour (juce::TextButton::buttonColourId, panelLight);
        b->setColour (juce::TextButton::buttonOnColourId, panelLight);
        b->setColour (juce::TextButton::textColourOffId, accent);
        b->setColour (juce::ComboBox::outlineColourId, lineColour);
        addAndMakeVisible (*b);
    }
    prevPreset.setTooltip ("Previous preset");
    nextPreset.setTooltip ("Next preset");
    prevPreset.onClick = [this] { stepPreset (-1); };
    nextPreset.onClick = [this] { stepPreset (1); };

    setSize (designWidth, designHeight);
    startTimerHz (30);
}

SynthPanel::~SynthPanel()
{
    setLookAndFeel (nullptr);
}

void SynthPanel::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    browseOrder.clear();
    const auto& presets = factoryPresets();

    for (const char* category : presetCategories())
    {
        bool headed = false;
        for (size_t i = 0; i < presets.size(); ++i)
        {
            if (juce::String (presets[i].category) != category)
                continue;
            if (! headed)
            {
                presetBox.addSectionHeading (category);
                headed = true;
            }
            presetBox.addItem (presets[i].name, (int) i + 1);
            browseOrder.push_back ((int) i);
        }
    }
    presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
}

void SynthPanel::stepPreset (int delta)
{
    if (browseOrder.empty())
        return;
    const auto it = std::find (browseOrder.begin(), browseOrder.end(), processor.getCurrentProgram());
    const int here = it == browseOrder.end() ? 0 : (int) (it - browseOrder.begin());
    const int size = (int) browseOrder.size();
    const int next = browseOrder[(size_t) (((here + delta) % size + size) % size)];
    processor.setCurrentProgram (next);
    presetBox.setSelectedId (next + 1, juce::dontSendNotification);
}

void SynthPanel::timerCallback()
{
    const int step = processor.gateStepPlaying.load (std::memory_order_relaxed);
    if (step != shownGateStep)
    {
        shownGateStep = step;
        for (int i = 0; i < (int) steps.size(); ++i)
            steps[(size_t) i].button->setPlaying (i == step);
    }

    // The host can change the program too.
    if (presetBox.getSelectedId() != processor.getCurrentProgram() + 1)
        presetBox.setSelectedId (processor.getCurrentProgram() + 1, juce::dontSendNotification);
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
    auto presetArea = top.removeFromRight (310).reduced (16, 14);
    nextPreset.setBounds (presetArea.removeFromRight (28));
    presetArea.removeFromRight (4);
    prevPreset.setBounds (presetArea.removeFromRight (28));
    presetArea.removeFromRight (6);
    presetBox.setBounds (presetArea);
    area.removeFromBottom (10);

    auto upper = area.removeFromTop (454);
    auto lower = area;
    lowerStripTop = lower.getY();

    const int titleH = 26;
    auto* strip = &upper;
    bool rowStart = true;
    auto take = [&] (const juce::String& title, int width)
    {
        auto s = strip->removeFromLeft (width);
        sections.push_back ({ title, s, rowStart });
        rowStart = false;
        s.removeFromTop (titleH);
        return s.reduced (8, 4);
    };
    // Section power switch, at the left of its title.
    auto placePower = [&] (const char* paramId)
    {
        const auto& header = sections.back().area;
        toggles.at (paramId).button->setBounds (header.getX() + 8, header.getY() + 3, 24, 20);
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
        auto s = take ("OUTPUT", upper.getWidth());
        const int cellH = s.getHeight() / 4;
        placeKnob (volume, s.removeFromTop (cellH), 64);
        placeKnob (velAmp, s.removeFromTop (cellH), 48);
        placeKnob (velFilter, s.removeFromTop (cellH), 48);
        placeKnob (bendRange, s, 48);
    }

    // ---- lower strip: performance and effects
    strip = &lower;
    rowStart = true;

    // Arpeggiator
    {
        auto s = take ("ARPEGGIATOR", 290);
        placePower (arpOn);
        s.removeFromTop (6);
        auto row = s.removeFromTop (46);
        placeChoice (arpMode, row.removeFromLeft (112).reduced (4, 0));
        placeChoice (arpRate, row.removeFromLeft (84).reduced (4, 0));
        toggles.at (arpHold).button->setBounds (row.withTrimmedLeft (14).withTrimmedBottom (18));
        s.removeFromTop (12);
        const int cellW = s.getWidth() / 3;
        placeKnob (arpOctaves, s.removeFromLeft (cellW), 56);
        placeKnob (arpGate, s.removeFromLeft (cellW), 56);
        placeKnob (arpSwing, s, 56);
    }

    // Trance gate
    {
        auto s = take ("TRANCE GATE", 360);
        placePower (gateOn);
        auto knobRow = s.removeFromTop (s.getHeight() - 58);
        const int cellW = knobRow.getWidth() / 4;
        placeChoice (gateRate, knobRow.removeFromLeft (cellW).reduced (4, 0).withSizeKeepingCentre (cellW - 8, 46));
        placeKnob (gateDepth, knobRow.removeFromLeft (cellW), 52);
        placeKnob (gateLength, knobRow.removeFromLeft (cellW), 52);
        placeKnob (gateSmooth, knobRow, 52);

        // Four groups of four, with a gap between groups so the beats read at a glance.
        auto stepRow = s.withSizeKeepingCentre (s.getWidth(), 40).translated (0, -6);
        const int gap = 8;
        const int stepW = (stepRow.getWidth() - 3 * gap) / 16;
        int x = stepRow.getX() + (stepRow.getWidth() - (16 * stepW + 3 * gap)) / 2;
        for (int i = 0; i < (int) steps.size(); ++i)
        {
            if (i > 0 && i % 4 == 0)
                x += gap;
            steps[(size_t) i].button->setBounds (x, stepRow.getY(), stepW, stepRow.getHeight());
            x += stepW;
        }
    }

    // Chorus
    {
        auto s = take ("CHORUS", 120);
        placePower (chorusOn);
        auto row = s.removeFromTop (s.getHeight() / 2);
        placeKnob (chorusRate, row.removeFromLeft (row.getWidth() / 2), 40);
        placeKnob (chorusDepth, row, 40);
        placeKnob (chorusMix, s, 40);
    }

    // Delay
    {
        auto s = take ("DELAY", 170);
        placePower (delayOn);
        s.removeFromTop (6);
        auto row = s.removeFromTop (46);
        placeChoice (delayTime, row.removeFromLeft (70).reduced (2, 0));
        toggles.at (delayPingPong).button->setBounds (row.withTrimmedLeft (8).withTrimmedBottom (18));
        s.removeFromTop (12);
        const int cellW = s.getWidth() / 3;
        placeKnob (delayFeedback, s.removeFromLeft (cellW), 42);
        placeKnob (delayDamp, s.removeFromLeft (cellW), 42);
        placeKnob (delayMix, s, 42);
    }

    // Reverb
    {
        auto s = take ("REVERB", lower.getWidth());
        placePower (reverbOn);
        const int cellW = s.getWidth() / 2;
        auto row = s.removeFromTop (s.getHeight() / 2);
        placeKnob (reverbSize, row.removeFromLeft (cellW), 40);
        placeKnob (reverbDamp, row, 40);
        placeKnob (reverbWidth, s.removeFromLeft (cellW), 40);
        placeKnob (reverbMix, s, 40);
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
    g.drawText (juce::String (juce::CharPointer_UTF8 ("24 dB transistor-ladder synthesizer  \xc2\xb7  arp  \xc2\xb7  gate  \xc2\xb7  fx")),
                top.withTrimmedLeft (138), juce::Justification::centredLeft);

    // Rule between the synth voice and the performance/effects strip
    g.setColour (lineColour);
    g.fillRect (body.getX() + 10, lowerStripTop, body.getWidth() - 20, 1);

    // Sections
    for (size_t i = 0; i < sections.size(); ++i)
    {
        const auto& s = sections[i];
        if (! s.startsRow)
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
