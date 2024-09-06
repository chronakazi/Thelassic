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
class ThelassicAudioProcessorEditor  : public juce::AudioProcessorEditor,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
public:
    ThelassicAudioProcessorEditor (ThelassicAudioProcessor&);
    ~ThelassicAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    void parameterValueChanged (int parameterIndex, float newValue) override;

    /** Indicates that a parameter change gesture has started.

        E.g. if the user is dragging a slider, this would be called with gestureIsStarting
        being true when they first press the mouse button, and it will be called again with
        gestureIsStarting being false when they release it.

        IMPORTANT NOTE: This will be called synchronously, and many audio processors will
        call it during their audio callback. This means that not only has your handler code
        got to be completely thread-safe, but it's also got to be VERY fast, and avoid
        blocking. If you need to handle this event on your message thread, use this callback
        to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
        message thread.
    */
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}
    
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ThelassicAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> parametersChanged {false};
    
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
