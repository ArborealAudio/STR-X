#pragma once
#include <JuceHeader.h>

class TS9
{
public:
	TS9() {}
	~TS9() {}

	void prepare(const dsp::ProcessSpec& spec) noexcept
	{
		HPF.prepare(spec);
		LPF.prepare(spec);
		LPF_2.prepare(spec);

		HPF.setType(dsp::FirstOrderTPTFilterType::highpass);
		LPF.setType(dsp::FirstOrderTPTFilterType::lowpass);
		LPF_2.setType(dsp::FirstOrderTPTFilterType::lowpass);

		HPF.setCutoffFrequency(720.0);
		LPF.setCutoffFrequency(5600.0);
		LPF_2.setCutoffFrequency(723.4);

		HPF.reset();
		LPF.reset();
		LPF_2.reset();
	}

	void reset()
	{
		HPF.reset();
		LPF.reset();
		LPF_2.reset();
	}

	inline double processAudioSample(double x, float drive)
	{

		double yn = 0.0;
		xDry = x;
		float currentGain = drive;

		if (currentGain != lastGain)
			lastGain = 0.001f * currentGain + (1.0 - 0.001f) * lastGain;
		else
			lastGain = drive;

		k = 2 * drive;

		x *= drive / 2;

		x = HPF.processSample(0, x);

		x = LPF.processSample(0, x);

		x = std::tanh(k * x) / std::tanh(k);

		x = LPF_2.processSample(0, x);

		yn = ((x * (drive / 10.f)) + (xDry * (1.f - (drive / 10.f))));

		HPF.snapToZero();
		LPF.snapToZero();
		LPF_2.snapToZero();

		return yn;

	}

private:
	double xDry = 0.0;
	float lastGain = 0.0;

	dsp::FirstOrderTPTFilter<double> HPF, LPF, LPF_2;

	float k = 0.0;
};

//==================================================================

class PreAmp
{
public:
	PreAmp() {}
	~PreAmp() {}

	void prepare(const dsp::ProcessSpec& spec, double sampleRate) noexcept
	{
		lowBand.prepare(spec);
		hiBand.prepare(spec);

		inputHPF.prepare(spec);
		dcRemoval.prepare(spec);
		lowShelf.prepare(spec);

		lowBand.setType(dsp::LinkwitzRileyFilterType::lowpass);
		hiBand.setType(dsp::LinkwitzRileyFilterType::highpass);

		dcRemoval.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(sampleRate, 10.0));
		lowShelf.coefficients = (dsp::IIR::Coefficients<double>::makeLowShelf(sampleRate, 185.0, 1.8, 0.5));
		inputHPF.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(sampleRate, 65.0));

		inputHPF.reset();
		dcRemoval.reset();
		lowShelf.reset();
		lowBand.reset();
		hiBand.reset();
	}

	void reset()
	{
		inputHPF.reset();
		dcRemoval.reset();
		lowShelf.reset();
		lowBand.reset();
		hiBand.reset();
	}

	inline void updateCrossover(float crossover)
	{
		float mode = crossover;

		if (mode == 0) {
			lowBand.setCutoffFrequency(100.0);
			hiBand.setCutoffFrequency(100.0);
		}
		else if (mode == 1) {
			lowBand.setCutoffFrequency(250.0);
			hiBand.setCutoffFrequency(250.0);
		}
		else {
			lowBand.setCutoffFrequency(400.0);
			hiBand.setCutoffFrequency(400.0);
		}
	}

	inline double processAudioSample(double xn, float inputGain)
	{
	    float currentInputGain = inputGain;
		if (currentInputGain != lastInputGain)
			lastInputGain = 0.001f * currentInputGain + (1.0 - 0.001f) * lastInputGain;
		else
			lastInputGain = inputGain;
			
		float gain = lastInputGain * 8;
		double yn = 0.0;
		float k = lastInputGain / 3;
		float negK = k / 0.9;

		xn *= gain;

		double xnL = lowBand.processSample(0, xn);
		double xnH = hiBand.processSample(0, xn);

		xnL = inputHPF.processSample(xnL);

		// high band distortion
		if (xnH > 0.0)
		{
			xnH = std::atan(k * xnH) / std::atan(k);
		}
		else
		{
			xnH = 0.9 * (std::atan(negK * xnH) / std::atan(negK));
		}

		// low band distortion
		if (xnL > 0.0)
		{
			xnL = std::atan(k * xnL) / std::atan(k);
		}
		else
		{
			xnL = 0.9 * (std::atan(negK * xnL) / std::atan(negK));
		}

		yn = xnL + xnH;

		yn = dcRemoval.processSample(yn);

		yn = lowShelf.processSample(yn);

		lowBand.snapToZero();
		hiBand.snapToZero();
		inputHPF.snapToZero();
		dcRemoval.snapToZero();
		lowShelf.snapToZero();

		return yn;

	}

	dsp::LinkwitzRileyFilter<double> lowBand, hiBand;

	dsp::IIR::Filter<double> inputHPF, dcRemoval, lowShelf;
	
private:

    float lastInputGain = 0.0;

};

//========================================================

class ClassBValvePair
{
public:
	ClassBValvePair() {}
	~ClassBValvePair() {}

public:

	void prepare(const dsp::ProcessSpec& spec, double sampleRate) noexcept
	{
		dcRemoval.prepare(spec);
		dcRemoval.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(sampleRate, 10.0));
		dcRemoval.reset();
	}

	void reset()
	{
		dcRemoval.reset();
	}

	inline double processAudioSample(double xn, float outGain)
	{
	    float currentGain = outGain;
		if (currentGain != lastGain)
			lastGain = 0.001f * currentGain + (1.0 - 0.001f) * lastGain;
		else
			lastGain = outGain;
	    
		float gain = lastGain * 0.6;
		double yn = 0.0;

		xn *= gain;

		// --- asymmetrical waveshaping
		double yn_pos = waveShaper(xn, 1.70, 23.6, 1.01);
		double yn_neg = waveShaper(xn, 1.70, 1.01, 23.6);

		yn_pos = dcRemoval.processSample(yn_pos);
		yn_neg = dcRemoval.processSample(yn_neg);

		// --- symmetrical waveshaping
		yn_pos = waveShaper(yn_pos, 4.0, 1.01, 1.01);
		yn_neg = waveShaper(yn_neg, 4.0, 1.01, 1.01);

		yn = yn_pos + yn_neg;

		yn *= 0.1767;

		dcRemoval.snapToZero();

		return yn;
	}

	dsp::IIR::Filter<double> dcRemoval;

private:

    float lastGain = 0.0;

	inline double waveShaper(double xn, double g, double Ln, double Lp)
	{
		double yn = 0.0;
		if (xn <= 0)
			yn = (g * xn) / (1.0 - ((g * xn) / Ln));
		else
			yn = (g * xn) / (1.0 + ((g * xn) / Lp));
		return yn;
	}

};

//=====================================================================

class AmpProcessor
{
public:
	AmpProcessor() {}
	~AmpProcessor() {}

	std::atomic<float>* inputGain = nullptr;
	std::atomic<float>* crossover = nullptr;
	std::atomic<float>* outGain = nullptr;
	std::atomic<float>* tsXGain = nullptr;
	std::atomic<float>* bright = nullptr;

	dsp::IIR::Filter<double> highPass, bandPass, lowPass;

	dsp::IIR::Filter<double> bass, mid, treble, presence, brightShelf;


	void prepare(const dsp::ProcessSpec& spec, double sampleRate) noexcept
	{
		ts9.prepare(spec);
		highPass.prepare(spec);
		bandPass.prepare(spec);
		lowPass.prepare(spec);
		preAmp.prepare(spec, sampleRate);
		bass.prepare(spec);
		mid.prepare(spec);
		treble.prepare(spec);
		presence.prepare(spec);
		brightShelf.prepare(spec);
		powerAmp.prepare(spec, sampleRate);

		highPass.coefficients = (dsp::IIR::Coefficients<double>::makeFirstOrderHighPass(sampleRate, 750.f));
		bandPass.coefficients = (dsp::IIR::Coefficients<double>::makeBandPass(sampleRate, 80.f));
		lowPass.coefficients = (dsp::IIR::Coefficients<double>::makeFirstOrderLowPass(sampleRate, 10000.f));
		brightShelf.coefficients = (dsp::IIR::Coefficients<double>::makeHighShelf(sampleRate, 2500.0, 0.707, 2.0));

		highPass.reset();
		bandPass.reset();
		lowPass.reset();
		bass.reset();
		mid.reset();
		treble.reset();
		presence.reset();
		brightShelf.reset();
	}

	void reset()
	{
		ts9.reset();
		preAmp.reset();
		powerAmp.reset();
		highPass.reset();
		bandPass.reset();
		lowPass.reset();
		bass.reset();
		mid.reset();
		treble.reset();
		presence.reset();
		brightShelf.reset();
	}

	inline void updateFilters(double sampleRate, float bassGain, float midGain, float trebleGain,
		float presenceGain) noexcept
	{
	    float currentBass = bassGain;
		float currentMid = midGain;
		float currentTreble = trebleGain;
		float currentPres = presenceGain;

		if (currentBass != lastBass)
			currentBass = 0.1f * currentBass + (1.0 - 0.1f) * lastBass;
		lastBass = currentBass;

		if (currentMid != lastMid)
			currentMid = 0.1f * currentMid + (1.0 - 0.1f) * lastMid;
		lastMid = currentMid;

		if (currentTreble != lastTreble)
			currentTreble = 0.1f * currentTreble + (1.0 - 0.1f) * lastTreble;
		lastTreble = currentTreble;

		if (currentPres != lastPres)
			currentPres = 0.1f * currentPres + (1.0 - 0.1f) * lastPres;
		lastPres = currentPres;
	    
		*bass.coefficients = *(dsp::IIR::Coefficients<double>::makeLowShelf(sampleRate, 150.f, 0.606f,
			bassGain));
		*mid.coefficients = *(dsp::IIR::Coefficients<double>::makePeakFilter(sampleRate, 600.f, 0.5f,
			midGain));
		*treble.coefficients = *(dsp::IIR::Coefficients<double>::makeHighShelf(sampleRate, 1500.f, 0.3f,
			trebleGain));
		*presence.coefficients = *(dsp::IIR::Coefficients<double>::makePeakFilter(sampleRate, 4000.f, 0.6f,
			presenceGain));
	}

	inline void processAmp(const dsp::ProcessContextReplacing<float>& context) noexcept
	{
		const auto& inBlock = context.getInputBlock();
		auto& outBlock = context.getOutputBlock();

		preAmp.updateCrossover(*crossover);

		double yn = 0.0;

		for (int i = 0; i < inBlock.getNumSamples(); ++i)
		{
			auto xn = inBlock.getSample(0, i);

			if (*tsXGain > 0)
				xn = ts9.processAudioSample(xn, *tsXGain);

			xn = preAmp.processAudioSample(xn, *inputGain);
			xn = lowPass.processSample(xn);

			double xHPF = highPass.processSample(xn);
			double xBPF = bandPass.processSample(xn);
			xn = xHPF + xBPF;
			double xB = bass.processSample(xn);
			double xM = mid.processSample(xB);
			double xT = treble.processSample(xM);
			yn = presence.processSample(xT);
			if (*bright == true)
				yn = brightShelf.processSample(yn);
			

			yn = powerAmp.processAudioSample(yn, *outGain);

			outBlock.setSample(0, i, yn);
			outBlock.setSample(1, i, yn);
		}

	}


	TS9 ts9;

	PreAmp preAmp;

	ClassBValvePair powerAmp;
	
private:

    float lastBass = 0.0;
	float lastMid = 0.0;
	float lastTreble = 0.0;
	float lastPres = 0.0;

};

//=======================================================================
