#pragma once
#include <JuceHeader.h>

class TS9
{
public:
	TS9() {}
	~TS9() {}

	void prepare (const juce::dsp::ProcessSpec &spec) noexcept
	{
		HPF.prepare(spec);
		LPF.prepare(spec);
		LPF_2.prepare(spec);

		HPF.setType(juce::dsp::FirstOrderTPTFilterType::highpass);
		LPF.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
		LPF_2.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
		
		HPF.reset();
		LPF.reset();
		LPF_2.reset();
	}

	void updateFilters()
	{

		HPF.setCutoffFrequency (720.0);
		LPF.setCutoffFrequency (5600.0);
		LPF_2.setCutoffFrequency (723.4);
	}

	double processAudioSample(double x, float drive)
	{
		
		double yn = 0.0;
		xDry = x;

		while (drive == 0) {
			return x;
		}

		k = 2 * drive;

		x *= drive / 2;

		x = HPF.processSample(0, x);

		x = LPF.processSample(0, x);

		x = std::tanh(k * x) / std::tanh(k);

		x = LPF_2.processSample(0, x);
	
		yn = ((x * (drive / 10.f)) + (xDry * (1.f - (drive / 10.f))));
		return yn;

	}

private:	
	double xDry = 0.0;

	juce::dsp::FirstOrderTPTFilter<double> HPF;
	juce::dsp::FirstOrderTPTFilter<double> LPF;
	juce::dsp::FirstOrderTPTFilter<double> LPF_2;
	float k = 0.0;
};
