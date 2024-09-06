/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ThelassicAudioProcessorEditor::ThelassicAudioProcessorEditor (ThelassicAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    midFreqSliderAttachment(audioProcessor.apvts, "Mid Freq", midFreqSlider),
    midGainSliderAttachment(audioProcessor.apvts, "Mid Gain", midGainSlider),
    midQSliderAttachment(audioProcessor.apvts, "Mid Q", midQSlider),
    loCutFreqSliderAttachment(audioProcessor.apvts, "Lo Cut Freq", loCutFreqSlider),
    hiCutFreqSliderAttachment(audioProcessor.apvts, "Hi Cut Freq", hiCutFreqSlider),
    loCutSlopeSliderAttachment(audioProcessor.apvts, "Lo Cut Slope", loCutSlopeSlider),
    hiCutSlopeSliderAttachment(audioProcessor.apvts, "Hi Cut Slope", hiCutSlopeSlider)
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
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::darkslategrey);

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.50);
    auto w = responseArea.getWidth();
    
    auto& locut = monoChain.get<ChainPositions::LoCut>();
    auto& mid = monoChain.get<ChainPositions::Mid>();
    auto& hicut = monoChain.get<ChainPositions::HiCut>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    mags.resize(w);
    
    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
        if (!monoChain.isBypassed<ChainPositions::Mid>())
            mag *= mid.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if (!locut.isBypassed<0>())
            mag *= locut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!locut.isBypassed<1>())
            mag *= locut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!locut.isBypassed<2>())
            mag *= locut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!locut.isBypassed<3>())
            mag *= locut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if (!hicut.isBypassed<0>())
            mag *= hicut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hicut.isBypassed<1>())
            mag *= hicut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hicut.isBypassed<2>())
            mag *= hicut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!hicut.isBypassed<3>())
            mag *= hicut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        mags [i] = Decibels::gainToDecibels(mag);
    }
    
    Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    
    g.setColour(Colours::darkorange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    
    g.setColour(Colours::ghostwhite);
    g.strokePath(responseCurve, PathStrokeType(2.f));
    
    
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

void ThelassicAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ThelassicAudioProcessorEditor::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //params updated?
        //trigger redraw
    }
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
