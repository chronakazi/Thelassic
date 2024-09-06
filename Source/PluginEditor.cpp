/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics & g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider)
{
    using namespace juce;
    
    auto bounds = Rectangle<float>(x, y, width, height);
    
//    Rotary slider bg color
    g.setColour(Colours::oldlace);
    g.fillEllipse(bounds);
    
//    Rotary slider border color.
    g.setColour(Colours::darkorange);
    g.drawEllipse(bounds, 1.f);
    
    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
//        Indicator
        Path p;
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        
        p.addRoundedRectangle(r, 2.f);
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f,
                                 rotaryStartAngle, rotaryEndAngle);
        
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
        
        g.setColour(Colours::burlywood);
        g.fillPath(p);
        
//        Param text
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        
        g.setColour(Colours::oldlace);
        g.fillRect(r);
        
        g.setColour(Colours::black);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }

}

//====================================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
    g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);
    
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng, endAng, *this);
    
    
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    
    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    return juce::String(getValue());
}

//====================================================================================
ResponseCurveComponent::ResponseCurveComponent(ThelassicAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }

}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //params updated?
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Mid>().coefficients, peakCoefficients);
        
        auto loCutCoefficients = makeLoCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto hiCutCoefficients = makeHiCutFilter(chainSettings, audioProcessor.getSampleRate());
        
        updateCutFilter(monoChain.get<ChainPositions::LoCut>(), loCutCoefficients, chainSettings.loCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HiCut>(), hiCutCoefficients, chainSettings.hiCutSlope);
        
        //trigger redraw
        repaint();
    }
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    auto responseArea = getLocalBounds();
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

//==============================================================================
ThelassicAudioProcessorEditor::ThelassicAudioProcessorEditor (ThelassicAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    midFreqSlider(*audioProcessor.apvts.getParameter("Mid Freq"), "hz"),
    midGainSlider(*audioProcessor.apvts.getParameter("Mid Gain"), "db"),
    midQSlider(*audioProcessor.apvts.getParameter("Mid Q"), ""),
    loCutFreqSlider(*audioProcessor.apvts.getParameter("Lo Cut Freq"), "hz"),
    hiCutFreqSlider(*audioProcessor.apvts.getParameter("Hi Cut Freq"), "hz"),
    loCutSlopeSlider(*audioProcessor.apvts.getParameter("Lo Cut Slope"), "db/oct"),
    hiCutSlopeSlider(*audioProcessor.apvts.getParameter("Hi Cut Slope"), "db/oct"),

    responseCurveComponent(audioProcessor),
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

}

void ThelassicAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.50);
    
    responseCurveComponent.setBounds(responseArea);
    
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
        &hiCutSlopeSlider,
        &responseCurveComponent
    };
}
