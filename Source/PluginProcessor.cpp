/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::String SimpleEQAudioProcessor::paramPeakFreq("Peak Freq");
juce::String SimpleEQAudioProcessor::paramPeakGain("Peak Gain");
juce::String SimpleEQAudioProcessor::paramLowCutFreq( "LowCut Freq");
juce::String SimpleEQAudioProcessor::paramHighCutFreq( "HighCut Freq");
juce::String SimpleEQAudioProcessor::paramPeakQuality("Peak Quality");
juce::String SimpleEQAudioProcessor::paramLowCutSlope("LowCut Slope");
juce::String SimpleEQAudioProcessor::paramHighCutSlope("HighCut Slope");
juce::String SimpleEQAudioProcessor::paramLowCutBypassed("LowCut Bypassed");
juce::String SimpleEQAudioProcessor::paramPeakBypassed("Peak Bypassed");
juce::String SimpleEQAudioProcessor::paramHighCutBypassed("HighCut Bypassed");
juce::String SimpleEQAudioProcessor::paramAnalyzerEnabled("Analyzer Enabled");


//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
	return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName(int index)
{
	return {};
}

void SimpleEQAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..

	juce::dsp::ProcessSpec spec;

	spec.maximumBlockSize = samplesPerBlock;

	spec.numChannels = 1;

	leftChain.prepare(spec);
	rightChain.prepare(spec);

	updateFilters();

	leftChannelFifo.prepare(samplesPerBlock);
	rightChannelFifo.prepare(samplesPerBlock);

	//osc.initialise([](float x) { return std::sin(x); });

	//spec.numChannels = getTotalNumOutputChannels();
	//osc.prepare(spec);

	//osc.setFrequency(2000);
}


void SimpleEQAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
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

void SimpleEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
		buffer.clear(i, 0, buffer.getNumSamples());

	// This is the place where you'd normally do the guts of your plugin's
	// audio processing...
	// Make sure to reset the state if your inner loop is processing
	// the samples and the outer loop is handling the channels.
	// Alternatively, you can process the samples with the channels
	// interleaved by keeping the same state.

	updateFilters();

	juce::dsp::AudioBlock<float> block(buffer);

	//buffer.clear();

	//for (int i = 0; i < buffer.getNumSamples(); i++)
	//{
	//	buffer.setSample(0, i, osc.processSample(0));
	//}

	//juce::dsp::ProcessContextReplacing<float> stereoContext(block);
	//osc.process(stereoContext);


	

	auto leftBlock = block.getSingleChannelBlock(0);
	auto rightBlock = block.getSingleChannelBlock(1);

	juce::dsp::ProcessContextReplacing<float>leftContext(leftBlock);
	juce::dsp::ProcessContextReplacing<float>rightContext(rightBlock);

	leftChain.process(leftContext);
	rightChain.process(rightContext);

	leftChannelFifo.update(buffer);
	rightChannelFifo.update(buffer);


}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
	return new SimpleEQAudioProcessorEditor(*this);

	//return new juce::GenericAudioProcessorEditor(*this);

}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.

	juce::MemoryOutputStream mos(destData, true);
	apvts.state.writeToStream(mos);
}

void SimpleEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.

	auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
	if (tree.isValid())
	{
		apvts.replaceState(tree);
		updateFilters();
	}
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
	ChainSettings settings;

	using SEP = SimpleEQAudioProcessor;

	

	settings.lowCutFreq = apvts.getRawParameterValue(SEP::paramLowCutFreq)->load();
	settings.highCutFreq = apvts.getRawParameterValue(SEP::paramHighCutFreq)->load();
	settings.peakFreq = apvts.getRawParameterValue(SEP::paramPeakFreq)->load();
	settings.peakGainInDecibels = apvts.getRawParameterValue(SEP::paramPeakGain)->load();
	settings.peakQuality = apvts.getRawParameterValue(SEP::paramPeakQuality)->load();
	settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue(SEP::paramLowCutSlope)->load());
	settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue(SEP::paramHighCutSlope)->load());

	//Bypassed
	settings.lowCutBypassed = apvts.getRawParameterValue(SEP::paramLowCutBypassed)->load() > 0.5f;
	settings.peakBypassed = apvts.getRawParameterValue(SEP::paramPeakBypassed)->load() > 0.5f;
	settings.highCutBypassed = apvts.getRawParameterValue(SEP::paramHighCutBypassed)->load() > 0.5f;


	return settings;


}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{

	return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
		sampleRate,
		chainSettings.peakFreq,
		chainSettings.peakQuality,
		juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{

	auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());

	leftChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
	rightChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);

	updateCoefficients(leftChain.get < ChainPositions::Peak>().coefficients, peakCoefficients);
	updateCoefficients(rightChain.get < ChainPositions::Peak>().coefficients, peakCoefficients);
}

void updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
	*old = *replacements;
}

void SimpleEQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
	auto cutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());

	auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
	auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();


	leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
	rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);

	
	updateCutFilter(leftLowCut, cutCoefficients, chainSettings.lowCutSlope);
	updateCutFilter(rightLowCut, cutCoefficients, chainSettings.lowCutSlope);
}

void SimpleEQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
	auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

	auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
	auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

	leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
	rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

	updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
	updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void SimpleEQAudioProcessor::updateFilters()
{
	auto& chainSettings = getChainSettings(apvts);

	updateLowCutFilters(chainSettings);
	updatePeakFilter(chainSettings);
	updateHighCutFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
	juce::AudioProcessorValueTreeState::ParameterLayout layout;

	layout.add(std::make_unique<juce::AudioParameterFloat>(
		paramLowCutFreq,
		paramLowCutFreq,
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));

	layout.add(std::make_unique<juce::AudioParameterFloat>(
		paramHighCutFreq,
		paramHighCutFreq,
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));

	layout.add(std::make_unique<juce::AudioParameterFloat>(
		paramPeakFreq,
		paramPeakFreq,
		juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));

	layout.add(std::make_unique<juce::AudioParameterFloat>(
		paramPeakGain,
		paramPeakGain,
		juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));

	layout.add(std::make_unique<juce::AudioParameterFloat>(
		paramPeakQuality,
		paramPeakQuality,
		juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));



	juce::StringArray stringArray;
	for (int i = 0; i < 4; i++)
	{
		juce::String str;
		str << (12 + i * 12);
		str << " db/Oct";

		stringArray.add(str);
	}

	layout.add(std::make_unique<juce::AudioParameterChoice>(paramLowCutSlope, paramLowCutSlope, stringArray, 0));
	layout.add(std::make_unique<juce::AudioParameterChoice>(paramHighCutSlope, paramHighCutSlope, stringArray, 0));


	//Bypassed
	layout.add(std::make_unique<juce::AudioParameterBool>(paramLowCutBypassed, paramLowCutBypassed, false));
	layout.add(std::make_unique<juce::AudioParameterBool>(paramPeakBypassed, paramPeakBypassed, false));
	layout.add(std::make_unique<juce::AudioParameterBool>(paramHighCutBypassed, paramHighCutBypassed, false));
	layout.add(std::make_unique<juce::AudioParameterBool>(paramAnalyzerEnabled, paramAnalyzerEnabled, true));

	return layout;

}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new SimpleEQAudioProcessor();
}
