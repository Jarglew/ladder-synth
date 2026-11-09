#pragma once

#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <map>
#include <memory>
#include <vector>

// Dark panel, cream legends, amber accents.
class PanelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PanelLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool down, int buttonX, int buttonY,
                       int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

// The fixed-size front panel. The editor scales it to whatever size the window is.
class SynthPanel : public juce::Component
{
public:
    static constexpr int designWidth = 1120;
    static constexpr int designHeight = 520;

    explicit SynthPanel (LadderSynthProcessor&);
    ~SynthPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Choice
    {
        std::unique_ptr<juce::ComboBox> box;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    struct Toggle
    {
        std::unique_ptr<juce::ToggleButton> button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    LadderSynthProcessor& processor;
    PanelLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };

    std::map<juce::String, Knob> knobs;
    std::map<juce::String, Choice> choices;
    std::map<juce::String, Toggle> toggles;
    juce::ComboBox presetBox;

    struct Section { juce::String title; juce::Rectangle<int> area; };
    std::vector<Section> sections;
    std::vector<std::pair<juce::String, juce::Rectangle<int>>> rowLabels;

    void addKnob (const char* paramId, const juce::String& text);
    void addChoice (const char* paramId, const juce::String& text);
    void addToggle (const char* paramId, const juce::String& text);
    void placeKnob (const char* paramId, juce::Rectangle<int> cell, int knobSize);
    void placeChoice (const char* paramId, juce::Rectangle<int> cell);
    void refreshPresetBox();
};

class LadderSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit LadderSynthEditor (LadderSynthProcessor&);
    void resized() override;

private:
    SynthPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LadderSynthEditor)
};
