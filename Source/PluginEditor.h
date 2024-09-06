/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                  juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }
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
    
    CustomRotarySlider midFreqSlider,
                       midGainSlider,
                       midQSlider,
                       loCutFreqSlider,
                       hiCutFreqSlider,
                       loCutSlopeSlider,
                       hiCutSlopeSlider;
    
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
    
    MonoChain monoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThelassicAudioProcessorEditor)
};
