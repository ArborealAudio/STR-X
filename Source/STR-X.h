#pragma once
// #include <JuceHeader.h>

template <typename Type>
class TS9
{
public:
    TS9() = default;

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

	inline Type processAudioSample(Type x, float drive)
	{

		Type yn = 0.0;
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

		return yn;
	}

private:
	Type xDry = 0.0;
	float lastGain = 0.0;

	dsp::FirstOrderTPTFilter<Type> HPF, LPF, LPF_2;

	Type k = 0.0;
};

//==================================================================

template <typename Type>
class PreAmp
{
public:
    PreAmp() = default;

    void prepare(const dsp::ProcessSpec& spec, double sampleRate) noexcept
	{
		lowBand.prepare(spec);
		hiBand.prepare(spec);

		inputHPF.prepare(spec);
		dcRemoval.prepare(spec);
		lowShelf.prepare(spec);

		lowBand.setType(dsp::LinkwitzRileyFilterType::lowpass);
		hiBand.setType(dsp::LinkwitzRileyFilterType::highpass);

		dcRemoval.coefficients = (dsp::IIR::Coefficients<Type>::makeHighPass(sampleRate, 10.0));
		lowShelf.coefficients = (dsp::IIR::Coefficients<Type>::makeLowShelf(sampleRate, 185.0, 1.8, 0.5));
		inputHPF.coefficients = (dsp::IIR::Coefficients<Type>::makeHighPass(sampleRate, 65.0));

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

	inline void updateCrossover(int crossover)
	{
		auto mode = crossover;

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

	inline Type processAudioSample(Type xn, float inputGain)
	{
	    float currentInputGain = inputGain;
		if (currentInputGain != lastInputGain)
			lastInputGain = 0.001f * currentInputGain + (1.0 - 0.001f) * lastInputGain;
		else
			lastInputGain = inputGain;
			
		float gain = lastInputGain * 8;
		Type yn = 0.0;
		Type k = lastInputGain / 3.0;
		Type negK = k / 0.9;

		xn *= gain;

		Type xnL = lowBand.processSample(0, xn);
		Type xnH = hiBand.processSample(0, xn);

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

		return yn;
	}

	dsp::LinkwitzRileyFilter<Type> lowBand, hiBand;

	dsp::IIR::Filter<Type> inputHPF, dcRemoval, lowShelf;
	
private:

    float lastInputGain = 0.0;

};

//========================================================

template <typename Type>
class ClassBValvePair
{
public:
    ClassBValvePair() = default;

public:

	void prepare(const dsp::ProcessSpec& spec, double sampleRate) noexcept
	{
		dcRemoval.prepare(spec);
		dcRemoval.coefficients = (dsp::IIR::Coefficients<Type>::makeHighPass(sampleRate, 10.0));
		dcRemoval.reset();
	}

	void reset()
	{
		dcRemoval.reset();
	}

	inline Type processAudioSample(Type xn, float outGain)
	{
	    float currentGain = outGain;
		if (currentGain != lastGain)
			lastGain = 0.001f * currentGain + (1.0 - 0.001f) * lastGain;
		else
			lastGain = outGain;
	    
		float gain = lastGain * 0.6;
		Type yn = 0.0;

		xn *= gain;

		// --- asymmetrical waveshaping
		Type yn_pos = waveShaper(xn, 1.70, 23.6, 1.01);
		Type yn_neg = waveShaper(xn, 1.70, 1.01, 23.6);

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

	dsp::IIR::Filter<Type> dcRemoval;

private:

    float lastGain = 0.0;

	inline Type waveShaper(Type xn, Type g, Type Ln, Type Lp)
	{
		Type yn = 0.0;
		if (xn <= 0)
			yn = (g * xn) / (1.0 - ((g * xn) / Ln));
		else
			yn = (g * xn) / (1.0 + ((g * xn) / Lp));
		return yn;
	}
};

//=====================================================================

template <typename T>
class AmpProcessor
{
public:
    AmpProcessor() = default;

    std::atomic<float>* inputGain = nullptr;
	std::atomic<float>* crossover = nullptr;
	std::atomic<float>* outGain = nullptr;
	std::atomic<float>* tsXGain = nullptr;
	std::atomic<float>* bright = nullptr;

	dsp::IIR::Filter<T> highPass, bandPass, lowPass;

	dsp::IIR::Filter<T> bass, mid, treble, presence, brightShelf;


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

		highPass.coefficients = (dsp::IIR::Coefficients<T>::makeFirstOrderHighPass(sampleRate, 750.f));
		bandPass.coefficients = (dsp::IIR::Coefficients<T>::makeBandPass(sampleRate, 80.f));
		lowPass.coefficients = (dsp::IIR::Coefficients<T>::makeFirstOrderLowPass(sampleRate, 10000.f));
		brightShelf.coefficients = (dsp::IIR::Coefficients<T>::makeHighShelf(sampleRate, 2500.0, 0.707, 2.0));

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
	    
		*bass.coefficients = *(dsp::IIR::Coefficients<T>::makeLowShelf(sampleRate, 150.f, 0.606f,
			bassGain));
		*mid.coefficients = *(dsp::IIR::Coefficients<T>::makePeakFilter(sampleRate, 600.f, 0.5f,
			midGain));
		*treble.coefficients = *(dsp::IIR::Coefficients<T>::makeHighShelf(sampleRate, 1500.f, 0.3f,
			trebleGain));
		*presence.coefficients = *(dsp::IIR::Coefficients<T>::makePeakFilter(sampleRate, 4000.f, 0.6f,
			presenceGain));
	}

	inline void processAmp(const dsp::ProcessContextReplacing<T>& context) noexcept
	{
		const auto& inBlock = context.getInputBlock();
		auto& outBlock = context.getOutputBlock();

		preAmp.updateCrossover(*crossover);

		T yn = 0.0;

		for (int i = 0; i < inBlock.getNumSamples(); ++i)
		{
			auto xn = inBlock.getSample(0, i);

			if (*tsXGain > 0.f)
				xn = ts9.processAudioSample(xn, *tsXGain);

			xn = preAmp.processAudioSample(xn, *inputGain);
			xn = lowPass.processSample(xn);

			T xHPF = highPass.processSample(xn);
			T xBPF = bandPass.processSample(xn);
			xn = xHPF + xBPF;
			T xB = bass.processSample(xn);
			T xM = mid.processSample(xB);
			T xT = treble.processSample(xM);
			yn = presence.processSample(xT);
			if (*bright == true)
				yn = brightShelf.processSample(yn);
			

			yn = powerAmp.processAudioSample(yn, *outGain);

			outBlock.setSample(0, i, yn);
			outBlock.setSample(1, i, yn);
		}

	}


	TS9<T> ts9;

	PreAmp<T> preAmp;

	ClassBValvePair<T> powerAmp;
	
private:

    float lastBass = 0.0;
	float lastMid = 0.0;
	float lastTreble = 0.0;
	float lastPres = 0.0;

};

//=======================================================================
