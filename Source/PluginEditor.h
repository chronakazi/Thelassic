/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum ColorPalette
{
    Primary = 0xff222831,
    Secondary = 0xff31363f,
    Accent = 0xff76abae,
    Tertiary = 0xffeeeeee,
};

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider&) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
    juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                 juce::Slider::TextEntryBoxPosition::NoTextBox),
    param(&rap),
    suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }
    
    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }
    
    struct LabelPos
    {
        float pos;
        juce::String label;
    };
    
    juce::Array<LabelPos> labels;
    
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const {return 14;}
    juce::String getDisplayString() const;
    
private:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct ResponseCurveComponent: juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    ResponseCurveComponent(ThelassicAudioProcessor&);
    ~ResponseCurveComponent();
    
    void parameterValueChanged (int parameterIndex, float newValue) override;

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    
    void resized() override;
private:
    ThelassicAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged {false};
    
    MonoChain monoChain;
    
    void updateChain();
    
    juce::Image background;
    
    juce::Rectangle<int> getRenderArea();
    
//    juce::Rectangle<int> getAnalysisArea();
};

//==============================================================================
/**
*/
class ThelassicAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ThelassicAudioProcessorEditor (ThelassicAudioProcessor&);
    ~ThelassicAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ThelassicAudioProcessor& audioProcessor;
    
    RotarySliderWithLabels midFreqSlider,
                           midGainSlider,
                           midQSlider,
                           loCutFreqSlider,
                           hiCutFreqSlider,
                           loCutSlopeSlider,
                           hiCutSlopeSlider;
    
    ResponseCurveComponent responseCurveComponent;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment midFreqSliderAttachment,
               midGainSliderAttachment,
               midQSliderAttachment,
               loCutFreqSliderAttachment,
               hiCutFreqSliderAttachment,
               loCutSlopeSliderAttachment,
               hiCutSlopeSliderAttachment;
    
    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThelassicAudioProcessorEditor)
};
