#pragma once

#include "StackProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

namespace stack::ui
{

// Dark slate panel with cyan accents.
class StackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    StackLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

// A knob with its name underneath, attached to one parameter (re-attachable).
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& text);
    void attachTo (const juce::String& paramId);
    void setTooltip (const juce::String& tip) { slider.setTooltip (tip); }
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// A drop-down with its name underneath.
class Choice : public juce::Component
{
public:
    Choice (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& text);
    void attachTo (const juce::String& paramId);
    void resized() override;
    juce::ComboBox box;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

// A lamp-style on/off switch.
class Toggle : public juce::ToggleButton
{
public:
    Toggle (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& text);
    void attachTo (const juce::String& paramId);

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

// 16-step pattern editor used by the arpeggiator (with velocity bars) and the gate.
class StepGrid : public juce::Component
{
public:
    StepGrid (LayerStackProcessor&, bool isArp);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    LayerStackProcessor& processor;
    bool arp;
    bool dragSetsOn = true;
    int lastToggledStep = -1;
    juce::Rectangle<float> cellArea (int step) const;
    juce::Rectangle<float> velocityArea (int step) const;
    int stepAt (juce::Point<float>) const;
    int activeSteps() const;
    void handle (const juce::MouseEvent&, bool isDown);
};

class Page : public juce::Component
{
public:
    virtual void refresh() {}   // after a preset or state load
    virtual void tick() {}      // ten times a second while visible
protected:
    struct Section { juce::String title; juce::Rectangle<int> area; };
    std::vector<Section> sections;
    void paintSections (juce::Graphics&);
};

class LayersPage;
class ArpPage;
class GatePage;
class FxPage;
class ModPage;
class BrowserPage;

class StackPanel : public juce::Component
{
public:
    static constexpr int designWidth = 1120;
    static constexpr int designHeight = 680;

    explicit StackPanel (LayerStackProcessor&);
    ~StackPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();
    void tick();
    void showPage (int index);
    void stepPreset (int delta);

private:
    LayerStackProcessor& processor;
    StackLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };

    std::vector<std::unique_ptr<juce::TextButton>> tabs;
    std::vector<std::unique_ptr<Page>> pages;
    BrowserPage* browser = nullptr;
    juce::Label presetLabel;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    std::unique_ptr<Knob> volume;
    int currentPage = 0;
};

} // namespace stack::ui

class LayerStackEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit LayerStackEditor (LayerStackProcessor&);
    void resized() override;

private:
    LayerStackProcessor& processor;
    stack::ui::StackPanel panel;
    int seenVersion = -1;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayerStackEditor)
};
