#pragma once
#include <juceheader.h>

class PreAmp
{
public:
	PreAmp() {}
	~PreAmp() {}

	double processAudioSample(double xnH, double xnL, float k)
	{
		double yn = 0.0;
		float negK = k / 3;

		xnL = inputHPF.processSample(xnL);

		// high band distortion
		if (xnH > 0.0)
		{
			xnH = std::atan(k * xnH) / std::atan(k);			
		}
		else
		{
			xnH = std::atan(negK * xnH) / std::atan(negK);	
		}

		// low band distortion
		if (xnL > 0.0)
		{
			xnL = std::atan(k * xnL) / std::atan(k);			
		}
		else
		{
			xnL = std::atan(negK * xnL) / std::atan(negK);
		}

		yn = xnL + xnH;

		yn = dcRemoval.processSample(yn);

		yn = lowShelf.processSample(yn);

		return yn;
	}


	juce::dsp::IIR::Filter<double> inputHPF;
	juce::dsp::IIR::Filter<double> dcRemoval;
	juce::dsp::IIR::Filter<double> lowShelf;
		
};
