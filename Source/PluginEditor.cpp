/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
	int x,
	int y,
	int width,
	int height,
	float sliderPosPrportional,
	float rotaryStartAngle,
	float endAngle,
	juce::Slider& slider)
{
	using namespace juce;

	auto bounds = juce::Rectangle<float>(x, y, width, height);
	auto enabled = slider.isEnabled();

	g.setColour(enabled ? Colour(97u, 18u, 167u): Colours::darkgrey);
	g.fillEllipse(bounds);

	g.setColour(enabled ? Colour(255u, 154u, 1u) : Colours::grey);
	g.drawEllipse(bounds, 1.f);

	if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
	{
		auto center = bounds.getCentre();

		Path p;

		Rectangle<float>r;
		r.setLeft(center.getX() - 2);
		r.setRight(center.getX() + 2);
		r.setTop(bounds.getY());
		r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);


		p.addRoundedRectangle(r, 2.f);

		jassert(rotaryStartAngle < endAngle);

		auto sliderAngRad = jmap(sliderPosPrportional, 0.0f, 1.f, rotaryStartAngle, endAngle);
		p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));


		g.fillPath(p);

		g.setFont(rswl->getTextHeight());
		auto text = rswl->getDisplayString();
		auto strWidth = g.getCurrentFont().getStringWidth(text);

		r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
		r.setCentre(bounds.getCentre());

		g.setColour(Colours::black);
		g.fillRect(r);

		g.setColour(Colours::white);
		g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
	}
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
	juce::ToggleButton& toggleButton,
	bool 	shouldDrawButtonAsHighlighted,
	bool 	shouldDrawButtonAsDown)
{
	using namespace juce;

	if( auto* pb = dynamic_cast<PowerButton*>(&toggleButton))
	{
		Path powerButton;

		auto bounds = toggleButton.getLocalBounds();
		auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
		auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

		float ang = 30.f;

		size -= 6;
		powerButton.addCentredArc(r.getCentreX(),
			r.getCentreY(),
			size * 0.5,
			size * 0.5,
			0.f,
			degreesToRadians(ang),
			radiansToDegrees(360.f - ang),
			true);

		powerButton.startNewSubPath(r.getCentreX(), r.getY());
		powerButton.lineTo(r.getCentre());

		PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

		auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
		g.setColour(color);

		g.strokePath(powerButton, pst);
		g.drawEllipse(r, 2.f);
	}
	else if (auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton))
	{
		auto color = !toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
		g.setColour(color);

		auto bounds = toggleButton.getLocalBounds();
		g.drawRect(bounds);

		
		

		g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
	}
}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
	using namespace juce;

	auto startAng = degreesToRadians(180.f + 45.f);
	auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

	auto range = getRange();
	auto sliderBounds = getSliderBounds();


		getLookAndFeel().drawRotarySlider(
			g,
			sliderBounds.getX(),
			sliderBounds.getY(),
			sliderBounds.getWidth(),
			sliderBounds.getHeight(),
			jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
			startAng,
			endAng,
			*this);

		auto center = sliderBounds.toFloat().getCentre();
		auto radius = sliderBounds.getWidth() * 0.5f;

		g.setColour(Colour(0u, 172u, 1u));
		g.setFont(getTextHeight());

		auto numChoices = labels.size();
		for (int i = 0; i < numChoices; i++)
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

	r.setY(2);

	return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
	if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
	{
		return choiceParam->getCurrentChoiceName();
	}

	juce::String str;
	bool addK = false;

	if (auto* choiceParam = dynamic_cast<juce::AudioParameterFloat*>(param))
	{
		float val = getValue();

		if (val >= 1000.f)
		{
			val /= 1000.f;
			addK = true;
		}

		str = juce::String(val, (addK ? 2 : 0));

	}
	else
	{
		jassertfalse; // This shouldn't happen!!
	}

	if (suffix.isNotEmpty())
	{
		str << " ";
		if (addK)
		{
			str << "k";

		}

		str << suffix;
	}

	return str;
}
//=============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) :
	audioProcessor(p),
	leftPathProducer(audioProcessor.leftChannelFifo),
	rightPathProducer(audioProcessor.rightChannelFifo)
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

void ResponseCurveComponent::updateChain()
{
	auto chainSettings = getChainSettings(audioProcessor.apvts);

	monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
	monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
	monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

	auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
	updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

	auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
	auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

	updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
	updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);

}
void ResponseCurveComponent::paint(juce::Graphics& g)
{
	using namespace juce;
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(Colours::black);
	g.drawImage(background, getLocalBounds().toFloat());

	auto responseArea = getAnalysisArea();

	auto w = responseArea.getWidth();

	auto& lowCut = monoChain.get<ChainPositions::LowCut>();
	auto& peak = monoChain.get<ChainPositions::Peak>();
	auto& highCut = monoChain.get<ChainPositions::HighCut>();

	auto sampleRate = audioProcessor.getSampleRate();

	std::vector<double> mags;
	mags.resize(w);

	for (int i = 0; i < w; i++)
	{
		double mag = 1.f;

		auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

		if (!monoChain.isBypassed<ChainPositions::Peak>())
		{
			mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
		}

		//LowCut
		if (!monoChain.isBypassed<ChainPositions::LowCut>())
		{
			if (!lowCut.isBypassed<0>())
			{
				mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

			}
			if (!lowCut.isBypassed<1>())
			{
				mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			}
			if (!lowCut.isBypassed<2>())
			{
				mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			}
			if (!lowCut.isBypassed<3>())
			{
				mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			}
		}


		//HighCut
		if (!monoChain.isBypassed<ChainPositions::HighCut>())
		{
			if (!highCut.isBypassed<0>())
			{
				mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

			}
			if (!highCut.isBypassed<1>())
			{
				mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			}
			if (!highCut.isBypassed<2>())
			{
				mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			}
			if (!highCut.isBypassed<3>())
			{
				mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
			}
		}
		mags[i] = Decibels::gainToDecibels(mag);
	}

	Path responsiveCurve;

	const double outputMin = responseArea.getBottom();
	const double outputMax = responseArea.getY();

	auto map = [outputMin, outputMax](double input)
	{
		return jmap(input, -24.0, 24.0, outputMin, outputMax);
	};

	responsiveCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

	for (size_t i = 0; i < mags.size(); i++)
	{
		responsiveCurve.lineTo(responseArea.getX() + i, map(mags[i]));
	}

	if (shouldShowFFTAnalysis)
	{

		auto leftChannelFFTPath = leftPathProducer.getPath();
		leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

		g.setColour(Colours::skyblue);
		g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));


		auto rightChannelFFTPath = rightPathProducer.getPath();
		rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));

		g.setColour(Colours::yellow);
		g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
	}



	g.setColour(Colours::orange);
	g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

	g.setColour(Colours::white);
	g.strokePath(responsiveCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized()
{
	using namespace juce;

	background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

	Graphics g(background);
	Array<float> freqs{
		20.f,  50.f, 100.f,
		200.f, 500.f, 1000.f,
		2000.f,  5000.f, 1000.f,
		20000.f
	};

	auto renderArea = getAnalysisArea();
	auto left = renderArea.getX();
	auto right = renderArea.getRight();
	auto top = renderArea.getY();
	auto bottom = renderArea.getBottom();
	auto width = renderArea.getWidth();

	Array<float>xs;
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

	Array<float> gain{
	-24.f, -12.f, 0.f, 12.f, 24.f
	};


	for (auto gDb : gain)
	{
		auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));


		g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
		g.drawHorizontalLine(y, left, right);
	}



	g.setColour(Colours::lightgrey);
	const int fontHeight = 10;
	g.setFont(fontHeight);

	for (int i = 0; i < freqs.size(); i++)
	{
		auto f = freqs[i];
		auto x = xs[i];


		bool addK = false;

		//FREQUENCY LABELS
		String str;
		if (f >= 1000.f)
		{
			addK = true;
			f /= 1000.f;
		}

		str << f;
		if (addK)
		{
			str << "k";
		}


		str << "Hz";

		auto textWidth = g.getCurrentFont().getStringWidth(str);

		Rectangle<int> r;
		r.setSize(textWidth, fontHeight);
		r.setCentre(x, 0);
		r.setY(1);

		g.drawFittedText(str, r, juce::Justification::centred, 1);


		//GAIN LABELS
		for (auto gDb : gain)
		{
			auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

			String str;
			if (gDb >= 0)
			{
				str << "+";
			}




			str << gDb;


			auto textWidth = g.getCurrentFont().getStringWidth(str);

			Rectangle<int> r;
			r.setSize(textWidth, fontHeight);

			r.setX(getWidth() - textWidth);
			r.setCentre(r.getCentreX(), y);



			g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);

			g.drawFittedText(str, r, juce::Justification::centred, 1);

			//GAIN LABLES ON LEFT SIDE
			str.clear();
			str << gDb - 24;

			r.setX(1);
			textWidth = g.getCurrentFont().getStringWidth(str);
			r.setSize(textWidth, fontHeight);
			g.setColour(Colours::lightgrey);

			g.drawFittedText(str, r, juce::Justification::centred, 1);

		}
	}
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
	auto bounds = getLocalBounds();


	bounds.removeFromTop(12);
	bounds.removeFromBottom(2);
	bounds.removeFromLeft(20);
	bounds.removeFromRight(20);

	return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
	auto bounds = getRenderArea();


	bounds.removeFromTop(4);
	bounds.removeFromBottom(4);

	return bounds;
}


void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	parametersChanged.set(true);

}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
	juce::AudioBuffer<float> tempIncomingBuffer;

	while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
	{
		if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
		{
			auto size = tempIncomingBuffer.getNumSamples();

			juce::FloatVectorOperations::copy(
				monoBuffer.getWritePointer(0, 0),
				monoBuffer.getReadPointer(0, size),
				monoBuffer.getNumSamples() - size);

			juce::FloatVectorOperations::copy(
				monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
				tempIncomingBuffer.getReadPointer(0, 0),
				size);

			leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);

		}


	}

	/*
* if there are FFT data buffers to pull
*    if we can pull a buffef
*       generate a path
*/

	const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();

	/*
	48000 / 2048 = 23Hz <-- this is the bin width
	*/
	const auto binWidth = sampleRate / (double)fftSize;


	while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
	{
		std::vector<float>fftData;

		if (leftChannelFFTDataGenerator.getFFTData(fftData))
		{
			pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
		}

	}

	/*
	While there are paths that can be pulled,
	pull as many as can be pulled.

	display the most recent path

	*/

	while (pathProducer.getNumPathsAvailable())
	{
		pathProducer.getPath(leftChannelFFTPath);
	}

}
void ResponseCurveComponent::timerCallback()
{
	if (shouldShowFFTAnalysis)
	{
		auto fftBounds = getAnalysisArea().toFloat();
		auto sampleRate = audioProcessor.getSampleRate();

		leftPathProducer.process(fftBounds, sampleRate);
		rightPathProducer.process(fftBounds, sampleRate);

	}
	if (parametersChanged.compareAndSetBool(false, true))
	{
		updateChain();



	}

	repaint();
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p),
	responseCurveComponent(p),
	peakFreqSlider(*audioProcessor.apvts.getParameter(audioProcessor.paramPeakFreq), "Hz"),
	peakGainSlider(*audioProcessor.apvts.getParameter(audioProcessor.paramPeakGain), "dB"),
	peakQualitySlider(*audioProcessor.apvts.getParameter(audioProcessor.paramPeakQuality), ""),
	lowCutFreqSlider(*audioProcessor.apvts.getParameter(audioProcessor.paramLowCutFreq), "Hz"),
	highCutFreqSlider(*audioProcessor.apvts.getParameter(audioProcessor.paramHighCutFreq), "Hz"),
	lowCutSlopeSlider(*audioProcessor.apvts.getParameter(audioProcessor.paramLowCutSlope), "db/Oct"),
	highCutSlopeSlider(*audioProcessor.apvts.getParameter(audioProcessor.paramHighCutSlope), "db/Oct"),
	peakFreqSliderAttachment(audioProcessor.apvts, audioProcessor.paramPeakFreq, peakFreqSlider),
	peakGainSliderAttachment(audioProcessor.apvts, audioProcessor.paramPeakGain, peakGainSlider),
	peakQualitySliderAttachment(audioProcessor.apvts, audioProcessor.paramPeakQuality, peakQualitySlider),
	lowCutFreqSliderAttachment(audioProcessor.apvts, audioProcessor.paramLowCutFreq, lowCutFreqSlider),
	highCutFreqSliderAttachment(audioProcessor.apvts, audioProcessor.paramHighCutFreq, highCutFreqSlider),
	lowCutSlopeSliderAttachment(audioProcessor.apvts, audioProcessor.paramLowCutSlope, lowCutSlopeSlider),
	highCutSlopeSliderAttachment(audioProcessor.apvts, audioProcessor.paramHighCutSlope, highCutSlopeSlider),
	lowcutBypassButtonAttachment(audioProcessor.apvts, audioProcessor.paramLowCutBypassed, lowcutBypassButton),
	peakBypassButtonAttachment(audioProcessor.apvts, audioProcessor.paramPeakBypassed, peakBypassButton),
	highcutBypassButtonAttachment(audioProcessor.apvts, audioProcessor.paramHighCutBypassed, highcutBypassButton),
	analyzerEnabledButtonAttachment(audioProcessor.apvts, audioProcessor.paramAnalyzerEnabled, analyzerEnabledButton)

{
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.

	peakFreqSlider.labels.add({ 0.f, "20Hz" });
	peakFreqSlider.labels.add({ 1.f, "20kHz" });

	peakGainSlider.labels.add({ 0.f, "-24dB" });
	peakGainSlider.labels.add({ 1.f, "+24dB" });

	peakQualitySlider.labels.add({ 0.f, "0.1" });
	peakQualitySlider.labels.add({ 1.f, "10.0" });

	lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
	lowCutFreqSlider.labels.add({ 1.f, "20kHz" });

	highCutFreqSlider.labels.add({ 0.f, "20Hz" });
	highCutFreqSlider.labels.add({ 1.f, "20kHz" });

	lowCutSlopeSlider.labels.add({ 0.f, "12" });
	lowCutSlopeSlider.labels.add({ 1.f, "48" });

	highCutSlopeSlider.labels.add({ 0.f, "12" });
	highCutSlopeSlider.labels.add({ 1.f, "48" });



	for (auto* comp : getComps())
	{
		addAndMakeVisible(comp);
	}

	lowcutBypassButton.setLookAndFeel(&lnf);
	peakBypassButton.setLookAndFeel(&lnf);
	highcutBypassButton.setLookAndFeel(&lnf);
	analyzerEnabledButton.setLookAndFeel(&lnf);

	auto safePtr = juce::Component::SafePointer<SimpleEQAudioProcessorEditor>(this);
	peakBypassButton.onClick = [safePtr]()
	{
		if (auto* comp = safePtr.getComponent())
		{
			auto bypassed = comp->peakBypassButton.getToggleState();

			comp->peakFreqSlider.setEnabled(!bypassed);
			comp->peakGainSlider.setEnabled(!bypassed);
			comp->peakQualitySlider.setEnabled(!bypassed);
		}
	};

	lowcutBypassButton.onClick = [safePtr]()
	{
		if (auto* comp = safePtr.getComponent())
		{
			auto bypassed = comp->lowcutBypassButton.getToggleState();

			comp->lowCutFreqSlider.setEnabled(!bypassed);
			comp->lowCutSlopeSlider.setEnabled(!bypassed);
		}
	};

	highcutBypassButton.onClick = [safePtr]()
	{
		if (auto* comp = safePtr.getComponent())
		{
			auto bypassed = comp->highcutBypassButton.getToggleState();

			comp->highCutFreqSlider.setEnabled(!bypassed);
			comp->highCutSlopeSlider.setEnabled(!bypassed);
		}
	};

	analyzerEnabledButton.onClick = [safePtr]()
	{
		if (auto* comp = safePtr.getComponent())
		{
			auto enabled = comp->analyzerEnabledButton.getToggleState();

			comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
		}
	};


	setSize(600, 480);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
	lowcutBypassButton.setLookAndFeel(nullptr);
	peakBypassButton.setLookAndFeel(nullptr);
	highcutBypassButton.setLookAndFeel(nullptr);
	analyzerEnabledButton.setLookAndFeel(nullptr);
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics& g)
{
	using namespace juce;
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(Colours::black);





}

void SimpleEQAudioProcessorEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..

	auto bounds = getLocalBounds();

	auto analyzerEnableArea = bounds.removeFromTop(25);
	analyzerEnableArea.setWidth(100);
	analyzerEnableArea.setX(5);
	analyzerEnableArea.removeFromTop(2);

	analyzerEnabledButton.setBounds(analyzerEnableArea);
	bounds.removeFromTop(5);

	auto hRatio = 25.f / 100.f; //JUCE_LIVE_CONSTANT(33) / 100.f;

	auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

	responseCurveComponent.setBounds(responseArea);

	bounds.removeFromTop(5);

	auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
	auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

	lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
	lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
	lowCutSlopeSlider.setBounds(lowCutArea);

	highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
	highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
	highCutSlopeSlider.setBounds(highCutArea);

	peakBypassButton.setBounds(bounds.removeFromTop(25));
	peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
	peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
	peakQualitySlider.setBounds(bounds);
}




std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
	return
	{
		&peakFreqSlider,
		&peakGainSlider,
		&peakQualitySlider,
		&lowCutFreqSlider,
		&highCutFreqSlider,
		&lowCutSlopeSlider,
		&highCutSlopeSlider,
		&responseCurveComponent,

		&lowcutBypassButton,
		&peakBypassButton,
		&highcutBypassButton,
		&analyzerEnabledButton
	};
}