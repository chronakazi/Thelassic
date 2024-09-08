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
    g.setColour(Colour(ColorPalette::Accent));
    g.fillEllipse(bounds);
    
//    Rotary slider border color.
    g.setColour(Colour(ColorPalette::Tertiary));
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
        
        g.setColour(Colour(ColorPalette::Tertiary));
        g.fillPath(p);
        
//        Param text
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        
        g.setColour(Colour(ColorPalette::Accent));
        g.fillRect(r);
        
        g.setColour(Colour(ColorPalette::Secondary));
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
    
//    bounding box guides
//    g.setColour(Colours::red);
//    g.drawRect(getLocalBounds());
//    g.setColour(Colours::yellow);
//    g.drawRect(sliderBounds);
    
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng, endAng, *this);
    
//    perimeter labels
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * .5f;
    
    g.setColour(Colour(ColorPalette::Tertiary));
    g.setFont(getTextHeight());
    
    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(10);
    
    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();
    
    juce::String str;
    bool addK = false;
    
    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        if (val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }
        
        str = juce::String(val, (addK ? 2 : 0));
        
    }
    else
    {
        jassertfalse; //should never happen
    }
    
    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";
        
        str << suffix;
    }
    
    return str;
    
}

//====================================================================================
ResponseCurveComponent::ResponseCurveComponent(ThelassicAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    
    updateChain();
    
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
        updateChain();
        
        //trigger redraw
        repaint();
    }
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Mid>().coefficients, peakCoefficients);
    
    auto loCutCoefficients = makeLoCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto hiCutCoefficients = makeHiCutFilter(chainSettings, audioProcessor.getSampleRate());
    
    updateCutFilter(monoChain.get<ChainPositions::LoCut>(), loCutCoefficients, chainSettings.loCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HiCut>(), hiCutCoefficients, chainSettings.hiCutSlope);
    
}
void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(ColorPalette::Primary));
    
    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getRenderArea();
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
    
    g.setColour(Colour(ColorPalette::Tertiary));
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
    
    g.setColour(Colour(ColorPalette::Accent));
    g.strokePath(responseCurve, PathStrokeType(2.f));
    
    
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    
    Graphics g(background);
    
    Array<float> freqs
    {
        20, 50, 100,
        200, 500, 1000,
        2000, 5000, 10000,
        20000
    };
    
    auto renderArea = getRenderArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    Array<float> xs;
    for (auto f : freqs)
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }
    
    g.setColour(Colours::dimgrey);
    for (auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }
    
    Array<float> gain
    {
        -24, -18, -12, -6, 0, 6, 12, 18, 24
    };
    
    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        g.setColour(gDb == 0.f ? Colour(ColorPalette::Tertiary) : Colours::dimgrey);
        g.drawHorizontalLine(y, left, right);
    }

    g.setColour(Colours::dimgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    for (int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];
        
        bool addK = false;
        String str;
        if ( f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }
        
        str << f;
        if (addK)
            str << "k";
        str << "hz";
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(bottom + 4);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        String str;
        if (gDb > 0)
            str << "+";
        str << gDb;
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);
        
        g.setColour(Colours::dimgrey);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    
//    bounds.reduce(15, //JUCE_LIVE_CONSTANT(5),
//                  15); //JUCE_LIVE_CONSTANT(5));
    bounds.removeFromTop(5);
    bounds.removeFromBottom(15);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);
    
    return bounds;
}

//juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
//{
//    auto bounds = getRenderArea();
//    bounds.removeFromTop(4);
//    bounds.removeFromBottom(4);
//    return bounds;
//    
//}

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
    
    midFreqSlider.labels.add({0.f, "20 hz"});
    midFreqSlider.labels.add({1.f, "20 khz"});
    
    midGainSlider.labels.add({0.f, "-24 db"});
    midGainSlider.labels.add({1.f, "+24 db"});
    
    midQSlider.labels.add({0.f, "broad"});
    midQSlider.labels.add({1.f, "narrow"});
    
    loCutFreqSlider.labels.add({0.f, "20 hz"});
    loCutFreqSlider.labels.add({1.f, "20 khz"});
    
    loCutSlopeSlider.labels.add({0.f, "gentle"});
    loCutSlopeSlider.labels.add({1.f, "abrupt"});
    
    hiCutFreqSlider.labels.add({0.f, "20 hz"});
    hiCutFreqSlider.labels.add({1.f, "20 khz"});
    
    hiCutSlopeSlider.labels.add({0.f, "gentle"});
    hiCutSlopeSlider.labels.add({1.f, "abrupt"});
    
    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    
    setSize (550, 550);
}

ThelassicAudioProcessorEditor::~ThelassicAudioProcessorEditor()
{
    
}

//==============================================================================
void ThelassicAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colour(ColorPalette::Secondary));

}

void ThelassicAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
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
