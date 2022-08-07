#pragma once
#include "juceheader.h"


class ClassBValvePair
{
public:
	ClassBValvePair() {}
	~ClassBValvePair() {}

public:

	double processAudioSample(double xn)
	{
		double yn = 0.0;
		
		xn *= 3.0;

		// --- asymmetrical waveshaping
		double yn_pos = waveShaper(xn, 1.70, 23.6,
			1.01);
		double yn_neg = waveShaper(xn, 1.70, 1.01,
			23.6);

		yn_pos = dcRemoval.processSample(yn_pos);
		yn_neg = dcRemoval.processSample(yn_neg);

		// --- symmetrical waveshaping
		yn_pos = waveShaper(yn_pos, 4.0, 1.01,
			1.01);
		yn_neg = waveShaper(yn_neg, 4.0, 1.01,
			1.01);

		yn = yn_pos + yn_neg;

		yn *= 0.53;

		return yn;
	}

	juce::dsp::IIR::Filter<double> dcRemoval;

private:

	double waveShaper(double xn, double g, double Ln, double Lp)
	{
		double yn = 0.0;
		if (xn <= 0)
			yn = (g * xn) / (1.0 - ((g * xn) / Ln));
		else
			yn = (g * xn) / (1.0 + ((g * xn) / Lp));
		return yn;
	}

};

