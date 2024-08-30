/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ThelassicAudioProcessor::ThelassicAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

ThelassicAudioProcessor::~ThelassicAudioProcessor()
{
}

//==============================================================================
const juce::String ThelassicAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ThelassicAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ThelassicAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ThelassicAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ThelassicAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ThelassicAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ThelassicAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ThelassicAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ThelassicAudioProcessor::getProgramName (int index)
{
    return {};
}

void ThelassicAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ThelassicAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    auto chainSettings = getChainSettings(apvts);
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.midFreq, chainSettings.midQ, juce::Decibels::decibelsToGain(chainSettings.midGain));
    
    *leftChain.get<ChainPositions::Mid>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Mid>().coefficients = *peakCoefficients;
    
}

void ThelassicAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ThelassicAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ThelassicAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumInputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto chainSettings = getChainSettings(apvts);
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.midFreq, chainSettings.midQ, juce::Decibels::decibelsToGain(chainSettings.midGain));
    
    *leftChain.get<ChainPositions::Mid>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Mid>().coefficients = *peakCoefficients;
    
    auto block = juce::dsp::AudioBlock<float>(buffer);
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(leftBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
}

//==============================================================================
bool ThelassicAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ThelassicAudioProcessor::createEditor()
{
//    return new ThelassicAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void ThelassicAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ThelassicAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    settings.loCutFreq = apvts.getRawParameterValue("Lo Cut Freq")->load();
    settings.hiCutFreq = apvts.getRawParameterValue("Hi Cut Freq")->load();
    settings.midFreq = apvts.getRawParameterValue("Mid Freq")->load();
    settings.midGain = apvts.getRawParameterValue("Mid Gain")->load();
    settings.midQ = apvts.getRawParameterValue("Mid Q")->load();
    settings.loCutSlope = apvts.getRawParameterValue("Lo Cut Slope")->load();
    settings.hiCutSlope = apvts.getRawParameterValue("Hi Cut Slope")->load();
    
    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout
    ThelassicAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
        layout.add(std::make_unique<juce::AudioParameterFloat>("Lo Cut Freq",
                                                                "Lo Cut Freq",
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                                20.f));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Hi Cut Freq",
                                                                "Hi Cut Freq",
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                                20000.f));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Mid Freq",
                                                                "Mid Freq",
                                                                juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.22f),
                                                                1000.f));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Mid Gain",
                                                                "Mid Gain",
                                                                juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                                0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>("Mid Q",
                                                                "Mid Q",
                                                                juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                                1.f));
        
        juce::StringArray stringArray;
        for( int i = 0; i < 4; ++i )
        {
            juce::String str;
            str << (12 + i*12);
            str << " db/oct";
            stringArray.add(str);
        }
        
        layout.add(std::make_unique<juce::AudioParameterChoice>("Lo Cut Slope", "Lo Cut Slope", stringArray, 0));
        layout.add(std::make_unique<juce::AudioParameterChoice>("Hi Cut Slope", "Hi Cut Slope", stringArray, 0));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ThelassicAudioProcessor();
}
