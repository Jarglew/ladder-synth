#include "StackEditor.h"

namespace stack::ui
{

namespace
{
const juce::Colour background (0xff121418);
const juce::Colour panel (0xff1a1d23);
const juce::Colour panelLight (0xff242831);
const juce::Colour line (0xff313643);
const juce::Colour text (0xffe6ebf2);
const juce::Colour dim (0xff8a93a3);
const juce::Colour accent (0xff3fd0e0);
const juce::Colour accent2 (0xff9b7cff);

juce::Font font (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

// Lays widgets out left to right in equal cells.
void row (juce::Rectangle<int> area, std::initializer_list<juce::Component*> items)
{
    const int w = area.getWidth() / juce::jmax (1, (int) items.size());
    for (auto* c : items)
        if (c != nullptr)
            c->setBounds (area.removeFromLeft (w).reduced (4, 0));
        else
            area.removeFromLeft (w);
}
} // namespace

// ================================================================ look and feel

StackLookAndFeel::StackLookAndFeel()
{
    setColour (juce::Label::textColourId, text);
    setColour (juce::ComboBox::backgroundColourId, panelLight);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::PopupMenu::backgroundColourId, panelLight);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::TextButton::buttonColourId, panelLight);
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    setColour (juce::ListBox::backgroundColourId, panel);
    setColour (juce::ListBox::outlineColourId, line);
    setColour (juce::TextEditor::backgroundColourId, panelLight);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, line);
    setColour (juce::TextEditor::focusedOutlineColourId, accent);
    setColour (juce::CaretComponent::caretColourId, accent);
    setColour (juce::BubbleComponent::backgroundColourId, panelLight);
    setColour (juce::BubbleComponent::outlineColourId, accent);
    setColour (juce::TooltipWindow::backgroundColourId, panelLight);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::AlertWindow::outlineColourId, line);
    setColour (juce::ScrollBar::thumbColourId, line.brighter (0.3f));
}

void StackLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start,
                                         float end, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = start + pos * (end - start);
    const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
    const float zeroAngle = bipolar ? start + (float) ((0.0 - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum())) * (end - start) : start;

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius * 0.85f, radius * 0.85f, 0.0f, start, end, true);
    g.setColour (line);
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius * 0.85f, radius * 0.85f, 0.0f, juce::jmin (zeroAngle, angle),
                         juce::jmax (zeroAngle, angle), true);
    g.setColour (slider.isEnabled() ? accent : dim);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float bodyR = radius * 0.62f;
    g.setColour (panelLight.brighter (0.15f));
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (centre));
    g.setColour (text);
    g.drawLine ({ centre.getPointOnCircumference (bodyR * 0.3f, angle), centre.getPointOnCircumference (bodyR * 0.95f, angle) }, 2.0f);
}

void StackLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto bounds = b.getLocalBounds().toFloat();
    const auto lamp = juce::Rectangle<float> (26.0f, 14.0f).withCentre ({ bounds.getX() + 15.0f, bounds.getCentreY() });
    const bool on = b.getToggleState();

    g.setColour (on ? accent : panelLight);
    g.fillRoundedRectangle (lamp, 7.0f);
    g.setColour (highlighted ? text : line.brighter (0.3f));
    g.drawRoundedRectangle (lamp, 7.0f, 1.0f);
    g.setColour (on ? juce::Colours::black : dim);
    const float knobX = on ? lamp.getRight() - 7.0f : lamp.getX() + 7.0f;
    g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ knobX, lamp.getCentreY() }));

    g.setColour (on ? text : dim);
    g.setFont (font (13.0f));
    g.drawText (b.getButtonText(), bounds.withTrimmedLeft (34.0f), juce::Justification::centredLeft);
}

void StackLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& colour,
                                             bool highlighted, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
    auto c = b.getToggleState() ? accent : colour;
    if (down)
        c = c.brighter (0.2f);
    else if (highlighted)
        c = c.brighter (0.08f);
    g.setColour (c);
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (b.getToggleState() ? accent : line);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
}

void StackLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<int> (w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (box.hasKeyboardFocus (true) ? accent : line);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
    juce::Path arrow;
    const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (accent);
    g.fillPath (arrow);
}

juce::Font StackLookAndFeel::getComboBoxFont (juce::ComboBox&) { return font (13.5f); }
juce::Font StackLookAndFeel::getPopupMenuFont() { return font (14.0f); }
juce::Font StackLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return font (13.5f, true); }

void StackLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& l)
{
    l.setBounds (6, 1, box.getWidth() - 22, box.getHeight() - 2);
    l.setFont (getComboBoxFont (box));
}

// ================================================================ widgets

Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText)
    : apvts (state)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    addAndMakeVisible (slider);
    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (font (12.5f));
    label.setColour (juce::Label::textColourId, dim);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
    attachTo (paramId);
}

void Knob::attachTo (const juce::String& paramId)
{
    attachment.reset();
    if (auto* param = apvts.getParameter (paramId))
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromBottom (18));
    const int size = juce::jmin (r.getWidth(), r.getHeight(), 76);
    slider.setBounds (r.withSizeKeepingCentre (size, size));
    label.setBounds (label.getBounds().withY (slider.getBottom() + 2));
}

Choice::Choice (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText)
    : apvts (state)
{
    addAndMakeVisible (box);
    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (font (12.5f));
    label.setColour (juce::Label::textColourId, dim);
    addAndMakeVisible (label);
    attachTo (paramId);
}

void Choice::attachTo (const juce::String& paramId)
{
    attachment.reset();
    box.clear (juce::dontSendNotification);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (paramId)))
        box.addItemList (param->choices, 1);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, paramId, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromBottom (18));
    box.setBounds (r.withSizeKeepingCentre (r.getWidth(), juce::jmin (28, r.getHeight())));
}

Toggle::Toggle (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText)
    : juce::ToggleButton (labelText), apvts (state)
{
    attachTo (paramId);
}

void Toggle::attachTo (const juce::String& paramId)
{
    attachment.reset();
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, paramId, *this);
}

// ================================================================ step grid

StepGrid::StepGrid (LayerStackProcessor& p, bool isArp) : processor (p), arp (isArp) {}

int StepGrid::activeSteps() const
{
    const auto* param = processor.apvts.getRawParameterValue (arp ? GlobalParam::arpSteps : GlobalParam::gateSteps);
    return juce::jlimit (1, maxSteps, (int) param->load());
}

juce::Rectangle<float> StepGrid::cellArea (int step) const
{
    const float w = (float) getWidth() / (float) maxSteps;
    const float top = arp ? (float) getHeight() - 46.0f : 22.0f;
    return { w * (float) step + 3.0f, top, w - 6.0f, arp ? 30.0f : (float) getHeight() - 30.0f };
}

juce::Rectangle<float> StepGrid::velocityArea (int step) const
{
    const float w = (float) getWidth() / (float) maxSteps;
    return { w * (float) step + 3.0f, 22.0f, w - 6.0f, (float) getHeight() - 76.0f };
}

int StepGrid::stepAt (juce::Point<float> p) const
{
    return juce::jlimit (0, maxSteps - 1, (int) (p.x / ((float) getWidth() / (float) maxSteps)));
}

void StepGrid::paint (juce::Graphics& g)
{
    const int steps = activeSteps();
    g.setFont (font (11.0f));

    for (int i = 0; i < maxSteps; ++i)
    {
        const bool inUse = i < steps;
        const bool on = arp ? processor.getArpStepOn (i) : processor.getGateStepOn (i);
        const float alpha = inUse ? 1.0f : 0.3f;
        const auto cell = cellArea (i);

        g.setColour (dim.withAlpha (alpha));
        g.drawText (juce::String (i + 1), juce::Rectangle<float> (cell.getX(), 2.0f, cell.getWidth(), 16.0f),
                    juce::Justification::centred);

        if (i % 4 == 0 && i > 0)
        {
            g.setColour (line);
            g.drawVerticalLine ((int) (cell.getX() - 3.0f), 18.0f, (float) getHeight());
        }

        if (arp)
        {
            const auto area = velocityArea (i);
            g.setColour (panelLight.withAlpha (alpha));
            g.fillRoundedRectangle (area, 3.0f);
            const float vel = processor.getArpStepVelocity (i);
            const auto bar = area.withTop (area.getBottom() - area.getHeight() * vel);
            g.setColour ((on ? accent2 : dim.darker (0.6f)).withAlpha (alpha));
            g.fillRoundedRectangle (bar, 3.0f);
        }

        g.setColour ((on ? accent : panelLight).withAlpha (alpha));
        g.fillRoundedRectangle (cell, 4.0f);
        g.setColour (line.withAlpha (alpha));
        g.drawRoundedRectangle (cell, 4.0f, 1.0f);
    }
}

void StepGrid::handle (const juce::MouseEvent& e, bool isDown)
{
    const auto p = e.position;
    const int step = stepAt (p);

    if (arp && p.y < cellArea (0).getY() - 4.0f)
    {
        // Velocity bar: height sets the step's velocity and switches it on.
        const auto area = velocityArea (step);
        const float vel = juce::jlimit (0.05f, 1.0f, (area.getBottom() - p.y) / area.getHeight());
        processor.setArpStep (step, true, vel);
    }
    else
    {
        // Cells: click toggles; dragging paints the same state across steps.
        if (isDown)
        {
            const bool current = arp ? processor.getArpStepOn (step) : processor.getGateStepOn (step);
            dragSetsOn = ! current;
            lastToggledStep = -1;
        }
        if (step != lastToggledStep)
        {
            if (arp)
                processor.setArpStep (step, dragSetsOn, processor.getArpStepVelocity (step));
            else
                processor.setGateStep (step, dragSetsOn);
            lastToggledStep = step;
        }
    }
    repaint();
}

void StepGrid::mouseDown (const juce::MouseEvent& e) { handle (e, true); }
void StepGrid::mouseDrag (const juce::MouseEvent& e) { handle (e, false); }

// ================================================================ pages

void Page::paintSections (juce::Graphics& g)
{
    for (const auto& s : sections)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (s.area.toFloat(), 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (s.area.toFloat().reduced (0.5f), 8.0f, 1.0f);
        g.setColour (accent);
        g.setFont (font (12.0f, true));
        g.drawText (s.title, s.area.withHeight (26).reduced (12, 0), juce::Justification::centredLeft);
    }
}

// ---------------------------------------------------------------- layers

class LayerCard : public juce::Component
{
public:
    LayerCard (LayerStackProcessor& p, int layerIndex)
        : processor (p), index (layerIndex),
          power (p.apvts, layerId (layerIndex, LayerParam::on), ""),
          level (p.apvts, layerId (layerIndex, LayerParam::level), "Level"),
          pan (p.apvts, layerId (layerIndex, LayerParam::pan), "Pan")
    {
        addAndMakeVisible (power);
        addAndMakeVisible (level);
        addAndMakeVisible (pan);
        power.setTooltip ("Switch layer " + juce::String (index + 1) + " on or off");
    }

    std::function<void (int)> onSelect;
    bool selected = false;

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced (1.0f);
        const bool on = power.getToggleState();
        g.setColour (selected ? panelLight : panel);
        g.fillRoundedRectangle (b, 8.0f);
        g.setColour (selected ? accent : line);
        g.drawRoundedRectangle (b, 8.0f, selected ? 2.0f : 1.0f);

        g.setColour (on ? text : dim);
        g.setFont (font (15.0f, true));
        g.drawText ("LAYER " + juce::String (index + 1), 50, 10, 120, 20, juce::Justification::centredLeft);

        const int wave = (int) processor.apvts.getRawParameterValue (layerId (index, LayerParam::wave))->load();
        auto waveText = waveNames()[wave];
        if (wave == (int) LayerWave::sample)
        {
            const auto name = processor.getSampleName (index);
            waveText = name.isEmpty() ? "Sample (none loaded)" : name;
        }
        g.setColour (on ? accent : dim);
        g.setFont (font (13.0f));
        g.drawText (waveText, 14, 36, getWidth() / 2 - 10, 20, juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        power.setBounds (8, 8, 40, 24);
        auto r = getLocalBounds().reduced (8);
        r.removeFromLeft (r.getWidth() / 2);
        level.setBounds (r.removeFromLeft (r.getWidth() / 2));
        pan.setBounds (r);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        if (onSelect)
            onSelect (index);
    }

private:
    LayerStackProcessor& processor;
    int index;
    Toggle power;
    Knob level, pan;
};

class LayersPage : public Page
{
public:
    explicit LayersPage (LayerStackProcessor& p)
        : processor (p),
          wave (p.apvts, layerId (0, LayerParam::wave), "Wave"),
          octave (p.apvts, layerId (0, LayerParam::octave), "Octave"),
          semi (p.apvts, layerId (0, LayerParam::semitone), "Semitone"),
          fine (p.apvts, layerId (0, LayerParam::fine), "Fine"),
          detune (p.apvts, layerId (0, LayerParam::detune), "Detune"),
          width (p.apvts, layerId (0, LayerParam::width), "Width"),
          root (p.apvts, layerId (0, LayerParam::root), "Root Note"),
          loop (p.apvts, layerId (0, LayerParam::loop), "Loop"),
          filterType (p.apvts, layerId (0, LayerParam::filterType), "Type"),
          cutoff (p.apvts, layerId (0, LayerParam::cutoff), "Cutoff"),
          reso (p.apvts, layerId (0, LayerParam::resonance), "Resonance"),
          fenv (p.apvts, layerId (0, LayerParam::filterEnv), "Env Amount"),
          keyTrack (p.apvts, layerId (0, LayerParam::keyTrack), "Key Track"),
          fA (p.apvts, layerId (0, LayerParam::fAttack), "Attack"),
          fD (p.apvts, layerId (0, LayerParam::fDecay), "Decay"),
          fS (p.apvts, layerId (0, LayerParam::fSustain), "Sustain"),
          fR (p.apvts, layerId (0, LayerParam::fRelease), "Release"),
          aA (p.apvts, layerId (0, LayerParam::aAttack), "Attack"),
          aD (p.apvts, layerId (0, LayerParam::aDecay), "Decay"),
          aS (p.apvts, layerId (0, LayerParam::aSustain), "Sustain"),
          aR (p.apvts, layerId (0, LayerParam::aRelease), "Release"),
          velocity (p.apvts, layerId (0, LayerParam::velocity), "Velocity")
    {
        for (int i = 0; i < numLayers; ++i)
        {
            auto card = std::make_unique<LayerCard> (p, i);
            card->onSelect = [this] (int index) { select (index); };
            addAndMakeVisible (*card);
            cards.push_back (std::move (card));
        }

        for (juce::Component* c : std::initializer_list<juce::Component*> {
                 &wave, &octave, &semi, &fine, &detune, &width, &root, &loop, &filterType, &cutoff, &reso, &fenv,
                 &keyTrack, &fA, &fD, &fS, &fR, &aA, &aD, &aS, &aR, &velocity, &loadButton, &clearButton, &sampleLabel })
            addAndMakeVisible (c);

        detune.setTooltip ("Supersaw only: how far apart its seven saws are tuned.");
        width.setTooltip ("Supersaw only: how widely its saws spread across the stereo field.");
        root.setTooltip ("The note your sample was recorded at, so it plays in tune.");
        loadButton.setTooltip ("Load a WAV, AIFF, FLAC or Ogg file into this layer.");
        sampleLabel.setColour (juce::Label::textColourId, dim);
        sampleLabel.setFont (font (13.0f));
        loadButton.onClick = [this] { chooseSample(); };
        clearButton.onClick = [this] { processor.clearSample (selected); refresh(); };
        select (0);
    }

    void select (int index)
    {
        selected = index;
        for (size_t i = 0; i < cards.size(); ++i)
        {
            cards[i]->selected = ((int) i == index);
            cards[i]->repaint();
        }
        using namespace LayerParam;
        wave.attachTo (layerId (index, LayerParam::wave));
        octave.attachTo (layerId (index, LayerParam::octave));
        semi.attachTo (layerId (index, semitone));
        fine.attachTo (layerId (index, LayerParam::fine));
        detune.attachTo (layerId (index, LayerParam::detune));
        width.attachTo (layerId (index, LayerParam::width));
        root.attachTo (layerId (index, LayerParam::root));
        loop.attachTo (layerId (index, LayerParam::loop));
        filterType.attachTo (layerId (index, LayerParam::filterType));
        cutoff.attachTo (layerId (index, LayerParam::cutoff));
        reso.attachTo (layerId (index, resonance));
        fenv.attachTo (layerId (index, filterEnv));
        keyTrack.attachTo (layerId (index, LayerParam::keyTrack));
        fA.attachTo (layerId (index, fAttack));
        fD.attachTo (layerId (index, fDecay));
        fS.attachTo (layerId (index, fSustain));
        fR.attachTo (layerId (index, fRelease));
        aA.attachTo (layerId (index, aAttack));
        aD.attachTo (layerId (index, aDecay));
        aS.attachTo (layerId (index, aSustain));
        aR.attachTo (layerId (index, aRelease));
        velocity.attachTo (layerId (index, LayerParam::velocity));
        refresh();
        repaint();
    }

    void tick() override
    {
        for (auto& c : cards)
            c->repaint();
    }

    void refresh() override
    {
        const auto name = processor.getSampleName (selected);
        sampleLabel.setText (name.isEmpty() ? "No sample loaded" : name, juce::dontSendNotification);
        clearButton.setEnabled (name.isNotEmpty());
        for (auto& c : cards)
            c->repaint();
    }

    void paint (juce::Graphics& g) override
    {
        paintSections (g);
        g.setColour (dim);
        g.setFont (font (12.0f));
        g.drawText ("Editing layer " + juce::String (selected + 1) + ". Click a layer above to edit it.",
                    getLocalBounds().removeFromBottom (22).reduced (14, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        sections.clear();
        auto area = getLocalBounds().reduced (12, 8);
        area.removeFromBottom (18);

        auto cardRow = area.removeFromTop (118);
        const int cardW = cardRow.getWidth() / numLayers;
        for (auto& c : cards)
            c->setBounds (cardRow.removeFromLeft (cardW).reduced (4, 0));
        area.removeFromTop (10);

        auto source = area.removeFromLeft (460).reduced (4, 0);
        auto filter = area.removeFromLeft (300).reduced (4, 0);
        auto envs = area.reduced (4, 0);
        sections.push_back ({ "SOURCE", source });
        sections.push_back ({ "FILTER", filter });
        sections.push_back ({ "ENVELOPES", envs });

        auto s = source.reduced (10, 0).withTrimmedTop (30);
        auto r1 = s.removeFromTop (140);
        row (r1.removeFromLeft (120).withSizeKeepingCentre (120, 60), { &wave });
        row (r1, { &octave, &semi, &fine, &detune, &width });
        s.removeFromTop (16);
        sampleHeader = s.removeFromTop (20);
        auto r2 = s.removeFromTop (140);
        auto buttons = r2.removeFromLeft (110).withSizeKeepingCentre (100, 64);
        loadButton.setBounds (buttons.removeFromTop (30));
        buttons.removeFromTop (4);
        clearButton.setBounds (buttons.removeFromTop (30));
        auto rightSide = r2.removeFromRight (190).withSizeKeepingCentre (190, 110);
        row (rightSide, { &root, &loop });
        loop.setBounds (loop.getBounds().withSizeKeepingCentre (loop.getWidth(), 28));
        sampleLabel.setBounds (r2.reduced (8, 0));

        auto f = filter.reduced (10, 0).withTrimmedTop (30);
        auto fr1 = f.removeFromTop (160);
        row (fr1, { &filterType, &cutoff });
        filterType.setBounds (filterType.getBounds().withSizeKeepingCentre (filterType.getWidth(), 60));
        f.removeFromTop (16);
        row (f.removeFromTop (150), { &reso, &fenv, &keyTrack });

        auto e = envs.reduced (10, 0).withTrimmedTop (30);
        filterEnvHeader = e.removeFromTop (18);
        row (e.removeFromTop (150), { &fA, &fD, &fS, &fR });
        e.removeFromTop (14);
        ampEnvHeader = e.removeFromTop (18);
        row (e.removeFromTop (150), { &aA, &aD, &aS, &aR, &velocity });
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        g.setColour (dim);
        g.setFont (font (11.0f, true));
        g.drawText ("SAMPLE", sampleHeader, juce::Justification::centredLeft);
        g.drawText ("FILTER ENVELOPE", filterEnvHeader, juce::Justification::centredLeft);
        g.drawText ("AMP ENVELOPE", ampEnvHeader, juce::Justification::centredLeft);
    }

private:
    LayerStackProcessor& processor;
    int selected = 0;
    std::vector<std::unique_ptr<LayerCard>> cards;
    Choice wave;
    Knob octave, semi, fine, detune, width, root;
    Toggle loop;
    Choice filterType;
    Knob cutoff, reso, fenv, keyTrack, fA, fD, fS, fR, aA, aD, aS, aR, velocity;
    juce::TextButton loadButton { "Load..." }, clearButton { "Clear" };
    juce::Label sampleLabel;
    juce::Rectangle<int> sampleHeader, filterEnvHeader, ampEnvHeader;
    std::unique_ptr<juce::FileChooser> chooser;

    void chooseSample()
    {
        chooser = std::make_unique<juce::FileChooser> ("Load a sample into layer " + juce::String (selected + 1),
                                                       juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                       "*.wav;*.aif;*.aiff;*.flac;*.ogg");
        const int layer = selected;
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, layer] (const juce::FileChooser& fc)
        {
            const auto file = fc.getResult();
            if (file == juce::File())
                return;
            juce::String error;
            if (processor.loadSample (layer, file, error))
            {
                // Loading a sample switches the layer to play it.
                if (auto* param = processor.apvts.getParameter (layerId (layer, LayerParam::wave)))
                    param->setValueNotifyingHost (param->convertTo0to1 ((float) LayerWave::sample));
                if (auto* on = processor.apvts.getParameter (layerId (layer, LayerParam::on)))
                    on->setValueNotifyingHost (1.0f);
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't load sample", error);
            }
            refresh();
        });
    }
};

// ---------------------------------------------------------------- arp and gate

class ArpPage : public Page
{
public:
    explicit ArpPage (LayerStackProcessor& p)
        : on (p.apvts, GlobalParam::arpOn, "Arpeggiator"),
          mode (p.apvts, GlobalParam::arpMode, "Mode"),
          rate (p.apvts, GlobalParam::arpRate, "Rate"),
          octaves (p.apvts, GlobalParam::arpOctaves, "Octaves"),
          gate (p.apvts, GlobalParam::arpGate, "Gate"),
          swing (p.apvts, GlobalParam::arpSwing, "Swing"),
          latch (p.apvts, GlobalParam::arpLatch, "Latch"),
          steps (p.apvts, GlobalParam::arpSteps, "Steps"),
          grid (p, true)
    {
        for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &mode, &rate, &octaves, &gate, &swing, &latch, &steps, &grid })
            addAndMakeVisible (c);
        latch.setTooltip ("Keep arpeggiating after you let go of the keys.");
        swing.setTooltip ("Delay every second step for a shuffled groove.");
    }

    void paint (juce::Graphics& g) override
    {
        paintSections (g);
        g.setColour (dim);
        g.setFont (font (12.0f));
        g.drawText ("Click a step to switch it on or off. Drag a bar to set that step's velocity. Steps follow Logic's tempo.",
                    getLocalBounds().removeFromBottom (24).reduced (16, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        sections.clear();
        auto area = getLocalBounds().reduced (16, 10);
        area.removeFromBottom (20);
        auto controls = area.removeFromTop (130);
        sections.push_back ({ "ARPEGGIATOR", controls });
        auto c = controls.reduced (12, 0).withTrimmedTop (30).withTrimmedBottom (8);
        on.setBounds (c.removeFromLeft (150).withSizeKeepingCentre (150, 30));
        latch.setBounds (c.removeFromRight (110).withSizeKeepingCentre (110, 30));
        row (c, { &mode, &rate, &octaves, &gate, &swing, &steps });
        mode.setBounds (mode.getBounds().withSizeKeepingCentre (130, mode.getHeight()));
        rate.setBounds (rate.getBounds().withSizeKeepingCentre (100, rate.getHeight()));

        area.removeFromTop (12);
        sections.push_back ({ "PATTERN", area });
        grid.setBounds (area.reduced (14, 0).withTrimmedTop (30).withTrimmedBottom (12));
    }

    void tick() override { grid.repaint(); }

private:
    Toggle on;
    Choice mode, rate;
    Knob octaves, gate, swing;
    Toggle latch;
    Knob steps;
    StepGrid grid;
};

class GatePage : public Page
{
public:
    explicit GatePage (LayerStackProcessor& p)
        : on (p.apvts, GlobalParam::gateOn, "Trance Gate"),
          rate (p.apvts, GlobalParam::gateRate, "Rate"),
          steps (p.apvts, GlobalParam::gateSteps, "Steps"),
          depth (p.apvts, GlobalParam::gateDepth, "Depth"),
          attack (p.apvts, GlobalParam::gateAttack, "Attack"),
          release (p.apvts, GlobalParam::gateRelease, "Release"),
          grid (p, false)
    {
        for (juce::Component* c : std::initializer_list<juce::Component*> { &on, &rate, &steps, &depth, &attack, &release, &grid })
            addAndMakeVisible (c);
        depth.setTooltip ("How far closed steps turn the sound down.");
    }

    void paint (juce::Graphics& g) override
    {
        paintSections (g);
        g.setColour (dim);
        g.setFont (font (12.0f));
        g.drawText ("Lit steps let the sound through; dark steps chop it. Click or drag across steps. Locked to Logic's tempo.",
                    getLocalBounds().removeFromBottom (24).reduced (16, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        sections.clear();
        auto area = getLocalBounds().reduced (16, 10);
        area.removeFromBottom (20);
        auto controls = area.removeFromTop (130);
        sections.push_back ({ "TRANCE GATE", controls });
        auto c = controls.reduced (12, 0).withTrimmedTop (30).withTrimmedBottom (8);
        on.setBounds (c.removeFromLeft (150).withSizeKeepingCentre (150, 30));
        row (c, { &rate, &steps, &depth, &attack, &release });
        rate.setBounds (rate.getBounds().withSizeKeepingCentre (100, rate.getHeight()));

        area.removeFromTop (12);
        sections.push_back ({ "PATTERN", area });
        grid.setBounds (area.reduced (14, 0).withTrimmedTop (30).withTrimmedBottom (40));
    }

    void tick() override { grid.repaint(); }

private:
    Toggle on;
    Choice rate;
    Knob steps, depth, attack, release;
    StepGrid grid;
};

// ---------------------------------------------------------------- effects

class FxRow : public juce::Component
{
public:
    FxRow (LayerStackProcessor& p, FxType t) : type (t)
    {
        using namespace GlobalParam;
        auto& s = p.apvts;
        auto knob = [&] (const char* id, const char* name) { controls.push_back (std::make_unique<Knob> (s, id, name)); };

        switch (t)
        {
            case FxType::distortion:
                enable = std::make_unique<Toggle> (s, distOn, "Distortion");
                knob (distDrive, "Drive"); knob (distTone, "Tone"); knob (distMix, "Mix");
                break;
            case FxType::chorus:
                enable = std::make_unique<Toggle> (s, chorusOn, "Chorus");
                knob (chorusRate, "Rate"); knob (chorusDepth, "Depth"); knob (chorusMix, "Mix");
                break;
            case FxType::phaser:
                enable = std::make_unique<Toggle> (s, phaserOn, "Phaser");
                knob (phaserRate, "Rate"); knob (phaserDepth, "Depth"); knob (phaserFeedback, "Feedback"); knob (phaserMix, "Mix");
                break;
            case FxType::delay:
                enable = std::make_unique<Toggle> (s, delayOn, "Delay");
                controls.push_back (std::make_unique<Choice> (s, delayTime, "Time"));
                knob (delayFeedback, "Feedback"); knob (delayMix, "Mix");
                controls.push_back (std::make_unique<Toggle> (s, delayPingPong, "Ping-Pong"));
                break;
            case FxType::reverb:
                enable = std::make_unique<Toggle> (s, reverbOn, "Reverb");
                knob (reverbSize, "Size"); knob (reverbDamping, "Damping"); knob (reverbWidth, "Width"); knob (reverbMix, "Mix");
                break;
        }

        addAndMakeVisible (*enable);
        for (auto& c : controls)
            addAndMakeVisible (*c);
        addAndMakeVisible (up);
        addAndMakeVisible (down);
        up.setTooltip ("Move earlier in the chain");
        down.setTooltip ("Move later in the chain");
    }

    FxType type;
    juce::TextButton up { "^" }, down { "v" };

    void paint (juce::Graphics& g) override
    {
        g.setColour (panel);
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 8.0f);
        g.setColour (enable->getToggleState() ? accent.withAlpha (0.6f) : line);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 8.0f, 1.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10, 6);
        auto arrows = r.removeFromLeft (34);
        up.setBounds (arrows.removeFromTop (arrows.getHeight() / 2).reduced (2));
        down.setBounds (arrows.reduced (2));
        enable->setBounds (r.removeFromLeft (170).withSizeKeepingCentre (160, 30));
        const int w = 110;
        for (auto& c : controls)
        {
            auto cell = r.removeFromLeft (w).reduced (6, 0);
            if (dynamic_cast<Toggle*> (c.get()) != nullptr)
                cell = cell.withSizeKeepingCentre (cell.getWidth(), 30);
            else if (dynamic_cast<Choice*> (c.get()) != nullptr)
                cell = cell.withSizeKeepingCentre (cell.getWidth(), 50);
            c->setBounds (cell);
        }
    }

private:
    std::unique_ptr<Toggle> enable;
    std::vector<std::unique_ptr<juce::Component>> controls;
};

class FxPage : public Page
{
public:
    explicit FxPage (LayerStackProcessor& p) : processor (p)
    {
        for (int t = 0; t < numFx; ++t)
        {
            auto r = std::make_unique<FxRow> (p, (FxType) t);
            r->up.onClick = [this, t] { move (t, -1); };
            r->down.onClick = [this, t] { move (t, 1); };
            addAndMakeVisible (*r);
            rows.push_back (std::move (r));
        }
    }

    void refresh() override { resized(); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (dim);
        g.setFont (font (12.0f));
        g.drawText ("Effects run top to bottom. Use the arrows to change the order.",
                    getLocalBounds().removeFromBottom (24).reduced (16, 0), juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (16, 10);
        area.removeFromBottom (20);
        const int h = area.getHeight() / numFx;
        const auto order = processor.getFxOrder();
        for (int pos = 0; pos < numFx; ++pos)
            rows[(size_t) order[(size_t) pos]]->setBounds (area.removeFromTop (h).reduced (0, 4));
    }

private:
    LayerStackProcessor& processor;
    std::vector<std::unique_ptr<FxRow>> rows;

    void move (int type, int delta)
    {
        const auto order = processor.getFxOrder();
        for (int pos = 0; pos < numFx; ++pos)
            if (order[(size_t) pos] == type)
                processor.moveFx (pos, delta);
        resized();
    }
};

// ---------------------------------------------------------------- modulation

class ModPage : public Page
{
public:
    explicit ModPage (LayerStackProcessor& p)
        : mode (p.apvts, GlobalParam::voiceMode, "Voices"),
          glide (p.apvts, GlobalParam::glide, "Glide"),
          bend (p.apvts, GlobalParam::bendRange, "Bend Range"),
          lfo1Shape (p.apvts, GlobalParam::lfo1Shape, "Shape"),
          lfo1Rate (p.apvts, GlobalParam::lfo1Rate, "Rate"),
          lfo1Cutoff (p.apvts, GlobalParam::lfo1Cutoff, "To Cutoff"),
          lfo2Shape (p.apvts, GlobalParam::lfo2Shape, "Shape"),
          lfo2Rate (p.apvts, GlobalParam::lfo2Rate, "Rate"),
          lfo2Pitch (p.apvts, GlobalParam::lfo2Pitch, "To Pitch"),
          wheel (p.apvts, GlobalParam::modWheelVibrato, "Mod Wheel")
    {
        for (juce::Component* c : std::initializer_list<juce::Component*> { &mode, &glide, &bend, &lfo1Shape, &lfo1Rate, &lfo1Cutoff,
                                                                          &lfo2Shape, &lfo2Rate, &lfo2Pitch, &wheel })
            addAndMakeVisible (c);
        lfo1Cutoff.setTooltip ("LFO 1 sweeps every layer's filter. Left of centre inverts it.");
        wheel.setTooltip ("How much vibrato the mod wheel adds through LFO 2.");
    }

    void paint (juce::Graphics& g) override { paintSections (g); }

    void resized() override
    {
        sections.clear();
        auto area = getLocalBounds().reduced (16, 10);
        auto top = area.removeFromTop (area.getHeight() / 2);
        auto voice = top.removeFromLeft (top.getWidth() / 2).reduced (6);
        tipArea = top.reduced (6);
        auto lfo1 = area.removeFromLeft (area.getWidth() / 2).reduced (6);
        auto lfo2 = area.reduced (6);
        sections.push_back ({ "VOICE", voice });
        sections.push_back ({ "LFO 1  >  FILTER", lfo1 });
        sections.push_back ({ "LFO 2  >  VIBRATO", lfo2 });

        auto content = [] (juce::Rectangle<int> r) { return r.reduced (14, 0).withTrimmedTop (34).withSizeKeepingCentre (r.getWidth() - 28, 100); };
        row (content (voice), { &mode, &glide, &bend });
        row (content (lfo1), { &lfo1Shape, &lfo1Rate, &lfo1Cutoff });
        row (content (lfo2), { &lfo2Shape, &lfo2Rate, &lfo2Pitch, &wheel });
        for (auto* c : { &mode, &lfo1Shape, &lfo2Shape })
            c->setBounds (c->getBounds().withSizeKeepingCentre (110, 54));
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        g.setColour (panel);
        g.fillRoundedRectangle (tipArea.toFloat(), 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (tipArea.toFloat().reduced (0.5f), 8.0f, 1.0f);
        g.setColour (dim);
        g.setFont (font (13.0f));
        g.drawFittedText ("Mono plays one note at a time and glides between held notes.\n"
                          "Poly plays up to 16 notes; glide then slides each new note in.\n\n"
                          "Double-click any knob to reset it.",
                          tipArea.reduced (18), juce::Justification::centredLeft, 6);
    }

private:
    Choice mode;
    Knob glide, bend;
    Choice lfo1Shape;
    Knob lfo1Rate, lfo1Cutoff;
    Choice lfo2Shape;
    Knob lfo2Rate, lfo2Pitch, wheel;
    juce::Rectangle<int> tipArea;
};

// ---------------------------------------------------------------- browser

class BrowserPage : public Page,
                    private juce::ListBoxModel
{
public:
    explicit BrowserPage (LayerStackProcessor& p) : processor (p)
    {
        categories.add ("All");
        categories.addArray (presetCategories());
        categories.add ("User");

        categoryModel.owner = this;   // before setModel, which immediately asks for rows
        categoryList.setModel (&categoryModel);
        categoryList.setRowHeight (28);
        presetList.setModel (this);
        presetList.setRowHeight (28);
        search.setTextToShowWhenEmpty ("Search presets", dim);
        search.setJustification (juce::Justification::centredLeft);
        search.setIndents (10, 0);
        search.onTextChange = [this] { filter(); };
        saveButton.onClick = [this] { savePreset(); };
        rescanButton.onClick = [this] { rebuild(); };
        for (juce::Component* c : std::initializer_list<juce::Component*> { &categoryList, &presetList, &search, &saveButton, &rescanButton })
            addAndMakeVisible (c);
        rebuild();
        categoryList.selectRow (0);
    }

    struct Entry
    {
        juce::String name, category;
        int factoryIndex = -1;
        juce::File file;
    };

    void refresh() override { presetList.repaint(); }

    void rebuild()
    {
        all.clear();
        const auto& factory = factoryPresets();
        for (size_t i = 0; i < factory.size(); ++i)
            all.push_back ({ factory[i].name, factory[i].category, (int) i, {} });
        for (const auto& u : scanUserPresets())
            all.push_back ({ u.name, "User", -1, u.file });
        filter();
    }

    // Steps through every preset in browser order (used by the header arrows).
    void step (int delta)
    {
        if (all.empty())
            return;
        int current = -1;
        for (size_t i = 0; i < all.size(); ++i)
            if (all[i].name == processor.getPresetName())
                current = (int) i;
        const int next = (current + delta + (int) all.size()) % (int) all.size();
        load (all[(size_t) next]);
    }

    void paint (juce::Graphics& g) override
    {
        paintSections (g);
    }

    void resized() override
    {
        sections.clear();
        auto area = getLocalBounds().reduced (16, 10);
        auto left = area.removeFromLeft (220);
        sections.push_back ({ "CATEGORY", left });
        categoryList.setBounds (left.reduced (8).withTrimmedTop (24));

        area.removeFromLeft (12);
        sections.push_back ({ "PRESETS", area });
        auto inner = area.reduced (10).withTrimmedTop (24);
        auto top = inner.removeFromTop (34);
        saveButton.setBounds (top.removeFromRight (130).reduced (2));
        rescanButton.setBounds (top.removeFromRight (90).reduced (2));
        search.setBounds (top.reduced (2));
        inner.removeFromTop (6);
        presetList.setBounds (inner);
    }

private:
    LayerStackProcessor& processor;
    juce::StringArray categories;
    std::vector<Entry> all, visible;
    juce::ListBox categoryList, presetList;
    juce::TextEditor search;
    juce::TextButton saveButton { "Save Preset..." }, rescanButton { "Rescan" };
    juce::String category { "All" };

    struct CategoryModel : public juce::ListBoxModel
    {
        BrowserPage* owner = nullptr;
        int getNumRows() override { return owner != nullptr ? owner->categories.size() : 0; }
        void paintListBoxItem (int rowNumber, juce::Graphics& g, int w, int h, bool selected) override
        {
            if (selected)
            {
                g.setColour (panelLight);
                g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f);
            }
            g.setColour (selected ? accent : text);
            g.setFont (font (14.0f, selected));
            g.drawText (owner->categories[rowNumber], 10, 0, w - 20, h, juce::Justification::centredLeft);
        }
        void selectedRowsChanged (int lastRowSelected) override
        {
            if (lastRowSelected >= 0)
            {
                owner->category = owner->categories[lastRowSelected];
                owner->filter();
            }
        }
    } categoryModel;

    void filter()
    {
        visible.clear();
        const auto query = search.getText().trim();
        for (const auto& e : all)
            if ((category == "All" || e.category == category)
                && (query.isEmpty() || e.name.containsIgnoreCase (query)))
                visible.push_back (e);
        presetList.updateContent();
        presetList.repaint();
    }

    void load (const Entry& e)
    {
        if (e.factoryIndex >= 0)
            processor.loadFactoryPreset (e.factoryIndex);
        else
            processor.loadUserPreset (e.file);
        presetList.repaint();
    }

    int getNumRows() override { return (int) visible.size(); }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int w, int h, bool) override
    {
        if (! juce::isPositiveAndBelow (rowNumber, (int) visible.size()))
            return;
        const auto& e = visible[(size_t) rowNumber];
        const bool current = e.name == processor.getPresetName();
        if (current)
        {
            g.setColour (panelLight);
            g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f);
        }
        g.setColour (current ? accent : text);
        g.setFont (font (14.0f, current));
        g.drawText (e.name, 12, 0, w - 140, h, juce::Justification::centredLeft, true);
        g.setColour (dim);
        g.setFont (font (12.0f));
        g.drawText (e.category, w - 130, 0, 118, h, juce::Justification::centredRight);
    }

    void listBoxItemClicked (int rowNumber, const juce::MouseEvent&) override
    {
        if (juce::isPositiveAndBelow (rowNumber, (int) visible.size()))
            load (visible[(size_t) rowNumber]);
    }

    void savePreset()
    {
        auto* window = new juce::AlertWindow ("Save Preset", "Name your sound and pick a category.", juce::MessageBoxIconType::NoIcon);
        window->addTextEditor ("name", processor.getPresetName(), "Name");
        auto cats = presetCategories();
        cats.add ("User");
        window->addComboBox ("category", cats, "Category");
        if (auto* box = window->getComboBoxComponent ("category"))
            box->setSelectedItemIndex (cats.size() - 1);
        window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        window->enterModalState (true, juce::ModalCallbackFunction::create ([this, window] (int result)
        {
            if (result == 1)
            {
                const auto name = window->getTextEditorContents ("name");
                const auto* box = window->getComboBoxComponent ("category");
                const auto cat = box != nullptr ? box->getText() : juce::String ("User");
                juce::String error;
                if (processor.saveUserPreset (name, cat, error))
                    rebuild();
                else
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Couldn't save", error);
            }
        }), true);
    }
};

// ================================================================ panel

StackPanel::StackPanel (LayerStackProcessor& p) : processor (p)
{
    setLookAndFeel (&lookAndFeel);

    pages.push_back (std::make_unique<LayersPage> (p));
    pages.push_back (std::make_unique<ArpPage> (p));
    pages.push_back (std::make_unique<GatePage> (p));
    pages.push_back (std::make_unique<FxPage> (p));
    pages.push_back (std::make_unique<ModPage> (p));
    auto browserPage = std::make_unique<BrowserPage> (p);
    browser = browserPage.get();
    pages.push_back (std::move (browserPage));

    const char* names[] = { "LAYERS", "ARP", "GATE", "FX", "MOD", "BROWSE" };
    for (int i = 0; i < (int) pages.size(); ++i)
    {
        auto tab = std::make_unique<juce::TextButton> (names[i]);
        tab->setClickingTogglesState (false);
        tab->onClick = [this, i] { showPage (i); };
        addAndMakeVisible (*tab);
        tabs.push_back (std::move (tab));
        addChildComponent (*pages[(size_t) i]);
    }

    presetLabel.setJustificationType (juce::Justification::centred);
    presetLabel.setFont (font (17.0f, true));
    presetLabel.setColour (juce::Label::textColourId, text);
    presetLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (presetLabel);
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    prevButton.setTooltip ("Previous preset");
    nextButton.setTooltip ("Next preset");
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);
    volume = std::make_unique<Knob> (p.apvts, GlobalParam::volume, "Volume");
    addAndMakeVisible (*volume);

    showPage (0);
    refresh();
    setSize (designWidth, designHeight);
}

StackPanel::~StackPanel()
{
    setLookAndFeel (nullptr);
}

void StackPanel::showPage (int index)
{
    currentPage = index;
    for (size_t i = 0; i < pages.size(); ++i)
    {
        pages[i]->setVisible ((int) i == index);
        tabs[i]->setToggleState ((int) i == index, juce::dontSendNotification);
    }
}

void StackPanel::stepPreset (int delta)
{
    browser->step (delta);
}

void StackPanel::refresh()
{
    presetLabel.setText (processor.getPresetName(), juce::dontSendNotification);
    for (auto& page : pages)
        page->refresh();
    tick();
}

void StackPanel::tick()
{
    // Things automation can change without a preset load: layer cards and step grids.
    pages[(size_t) currentPage]->tick();
}

void StackPanel::paint (juce::Graphics& g)
{
    g.fillAll (background);
    auto header = getLocalBounds().removeFromTop (64);
    g.setGradientFill (juce::ColourGradient (panel, 0.0f, 0.0f, background, 0.0f, 64.0f, false));
    g.fillRect (header);

    g.setColour (accent);
    g.setFont (font (24.0f, true));
    g.drawText ("LAYER", header.withTrimmedLeft (22), juce::Justification::centredLeft);
    g.setColour (accent2);
    g.drawText ("STACK", header.withTrimmedLeft (106), juce::Justification::centredLeft);

    auto presetBox = juce::Rectangle<int> (designWidth / 2 - 190, 14, 380, 36).toFloat();
    g.setColour (panelLight);
    g.fillRoundedRectangle (presetBox, 6.0f);
    g.setColour (line);
    g.drawRoundedRectangle (presetBox, 6.0f, 1.0f);

    g.setColour (line);
    g.drawHorizontalLine (64 + 40, 0.0f, (float) getWidth());
}

void StackPanel::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop (64);
    volume->setBounds (header.removeFromRight (90).reduced (8, 2));
    auto presetBox = juce::Rectangle<int> (designWidth / 2 - 190, 14, 380, 36);
    prevButton.setBounds (presetBox.removeFromLeft (36).reduced (4));
    nextButton.setBounds (presetBox.removeFromRight (36).reduced (4));
    presetLabel.setBounds (presetBox);

    auto tabRow = area.removeFromTop (40).reduced (16, 5);
    for (auto& tab : tabs)
        tab->setBounds (tabRow.removeFromLeft (110).reduced (3, 0));

    for (auto& page : pages)
        page->setBounds (area);
}

} // namespace stack::ui

// ================================================================ editor

LayerStackEditor::LayerStackEditor (LayerStackProcessor& p) : AudioProcessorEditor (&p), processor (p), panel (p)
{
    using P = stack::ui::StackPanel;
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (P::designWidth * 6 / 10, P::designHeight * 6 / 10, P::designWidth * 2, P::designHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) P::designWidth / P::designHeight);
    setSize (P::designWidth, P::designHeight);
    startTimerHz (10);
}

void LayerStackEditor::resized()
{
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) stack::ui::StackPanel::designWidth));
}

void LayerStackEditor::timerCallback()
{
    // Presets and state loads change things the panel caches; otherwise only
    // refresh the live parts of the visible page.
    const int version = processor.stateVersion.load();
    if (version != seenVersion)
    {
        seenVersion = version;
        panel.refresh();
    }
    else
    {
        panel.tick();
    }
}
