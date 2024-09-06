/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ThelassicAudioProcessorEditor::ThelassicAudioProcessorEditor (ThelassicAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    
    setSize (600, 600);
}

ThelassicAudioProcessorEditor::~ThelassicAudioProcessorEditor()
{
}

//==============================================================================
void ThelassicAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void ThelassicAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.50);
    
    auto loCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto hiCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    loCutFreqSlider.setBounds(loCutArea.removeFromTop(loCutArea.getHeight() * 0.5));
    loCutSlopeSlider.setBounds(loCutArea);
    hiCutFreqSlider.setBounds(hiCutArea.removeFromTop(hiCutArea.getHeight() * 0.5));
    hiCutSlopeSlider.setBounds(hiCutArea);
    
    midFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    midGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    midQSlider.setBounds(bounds);
    
}

std::vector<juce::Component*> ThelassicAudioProcessorEditor::getComps()
{
    return
    {
        &midFreqSlider,
        &midGainSlider,
        &midQSlider,
        &loCutFreqSlider,
        &hiCutFreqSlider,
        &loCutSlopeSlider,
        &hiCutSlopeSlider
    };
}
