#pragma once

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

		HPF.setType(strix::FilterType::firstOrderHighpass);
		LPF.setType(strix::FilterType::firstOrderLowpass);
		LPF_2.setType(strix::FilterType::firstOrderLowpass);

		HPF.setCutoffFreq(720.0);
		LPF.setCutoffFreq(5600.0);
		LPF_2.setCutoffFreq(723.4);
	}

	void reset()
	{
		HPF.reset();
		LPF.reset();
		LPF_2.reset();
	}

    inline void process(double* x, double drive, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            x[i] = processSample(x[i], drive);
    }

    inline void process(vec* x, vec drive, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            x[i] = processSample(x[i], drive);
    }

private:
	inline double processSample(double x, double drive)
	{
		double yn = 0.0;
		double xDry = x;
		double currentGain = drive;

		if (currentGain != lastGain)
			lastGain = 0.001f * currentGain + (1.0 - 0.001f) * lastGain;
		else
			lastGain = drive;

		k = 2.0 * drive;

		x *= drive / 2;

		x = HPF.processSample(0, x);

		x = LPF.processSample(0, x);

		x = std::tanh(k * x) / std::tanh(k);

		x = LPF_2.processSample(0, x);

		yn = ((x * (drive / 10.f)) + (xDry * (1.f - (drive / 10.f))));

		return yn;
	}

    inline vec processSample(vec x, vec drive)
	{
		vec yn = 0.0;
		vec xDry = x;
		vec currentGain = drive;

		if (xsimd::any(currentGain != lastGain))
			lastGain = 0.001f * currentGain + (1.0 - 0.001f) * lastGain;
		else
			lastGain = drive;

		k = 2.0 * drive;

		x *= drive / 2;

		x = HPF.processSample(0, x);

		x = LPF.processSample(0, x);

		x = xsimd::tanh(k * x) / xsimd::tanh(k);

		x = LPF_2.processSample(0, x);

		yn = ((x * (drive / 10.0)) + (xDry * (1.0 - (drive / 10.0))));

		return yn;
	}

	Type lastGain = 0.0;

	strix::SVTFilter<Type> HPF, LPF, LPF_2;

	Type k = 0.0;
};

//==================================================================

template <typename Type>
class PreAmp
{
public:
    PreAmp() = default;

    void prepare(const dsp::ProcessSpec& spec) noexcept
	{
		inputHPF.prepare(spec);
		dcRemoval.prepare(spec);
		lowShelf.prepare(spec);

		lr.prepare(spec);
		lr.setType(strix::LRFilterType::lowpass);

		dcRemoval.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(spec.sampleRate, 10.0));
		lowShelf.coefficients = (dsp::IIR::Coefficients<double>::makeLowShelf(spec.sampleRate, 185.0, 1.8, 0.5));
		inputHPF.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(spec.sampleRate, 65.0));
	}

	void reset()
	{
		inputHPF.reset();
		dcRemoval.reset();
		lowShelf.reset();
		lr.reset();
	}

	inline void updateCrossover(int crossover)
	{
        switch (crossover)
        {
		case 0:
			lr.setCutoffFrequency(100.0);
            break;
        case 1:
            lr.setCutoffFrequency(250.0);
            break;
        case 2:
			lr.setCutoffFrequency(400.0);
            break;
        default:
            lr.setCutoffFrequency(250.0);
            break;
        }
	}

    inline void processHiGain(Type* in, Type inputGain, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            in[i] = processSampleHiGain(in[i], inputGain);
        }
    }

    inline void processLoGain(Type* in, Type inputGain, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            in[i] = processSampleLoGain(in[i], inputGain);
        }
    }
	
private:
    inline Type processSampleHiGain(Type xn, Type inputGain)
	{
	    Type currentInputGain = inputGain;
		if (xsimd::any(currentInputGain != lastInputGain))
			lastInputGain = 0.001f * currentInputGain + (1.0 - 0.001f) * lastInputGain;
		else
			lastInputGain = inputGain;
			
		Type gain = lastInputGain * 8.f;
		Type yn = 0.0, xnL = 0.0, xnH = 0.0;

		xn *= gain;

        lr.processSample(0, xn, xnL, xnH);

        xnL = inputHPF.processSample(xnL);

		// high band distortion
        xnH = hiGainSaturation(xnH);

        // low band distortion
        xnL = hiGainSaturation(xnL);

        yn = xnL + xnH;

		yn = dcRemoval.processSample(yn);

		yn = lowShelf.processSample(yn);

		return yn;
	}

    inline Type processSampleLoGain(Type xn, Type inputGain)
    {
        Type currentInputGain = inputGain;
		if (xsimd::any(currentInputGain != lastInputGain))
			lastInputGain = 0.001f * currentInputGain + (1.0 - 0.001f) * lastInputGain;
		else
			lastInputGain = inputGain;
			
		Type gain = lastInputGain * 4.f;
		Type yn = 0.0, xnL = 0.0, xnH = 0.0;

		xn *= gain;

        lr.processSample(0, xn, xnL, xnH);

        // xnL = inputHPF.processSample(xnL);

		// high band distortion
        xnH = loGainSaturation(xnH);

        // low band distortion
        xnL = loGainSaturation(xnL);

        yn = xnL + xnH;

		yn = dcRemoval.processSample(yn);

		yn = lowShelf.processSample(yn);

		return yn;
    }

    inline double hiGainSaturation (double x)
    {
        double k = lastInputGain / 3.0;
		double nk = k / 0.9;

        if (x > 0.0)
        {
            x = std::atan(k * x) / std::atan(k);
        }
        else
        {
            x = 0.9 * std::atan(nk * x) / std::atan(nk);
        }

        return x;
    }

    inline double loGainSaturation(double x)
    {
        if (x > 0.0)
        {
            x = (x / (1.0 + std::abs(x))) * 2.0;
        }
        else
        {
            x = (2.0 * x) / (1.0 + std::abs(x * 2.0));
        }

        return x;
    }

    inline vec hiGainSaturation (vec x)
    {
        vec k = lastInputGain / 3.0;
		vec nk = k / 0.9;

        return xsimd::select(x > 0.0, xsimd::atan(k * x) / xsimd::atan(k), 0.9 * xsimd::atan(nk * x) / xsimd::atan(nk));
    }

    inline vec loGainSaturation(vec x)
    {
        return xsimd::select(x > 0.0, (x / (1.0 + xsimd::abs(x))) * 2.0, (2.0 * x) / (1.0 + xsimd::abs(x * 2.0)));
    }

	strix::LinkwitzRileyFilter<Type> lr;

	dsp::IIR::Filter<Type> inputHPF, dcRemoval, lowShelf;

    Type lastInputGain = 0.0;
};

//========================================================

template <typename Type>
class ClassBValvePair
{
public:
    ClassBValvePair() = default;

	void prepare(const dsp::ProcessSpec& spec) noexcept
	{
		dcRemoval.prepare(spec);
		dcRemoval.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(spec.sampleRate, 10.0));
	}

	void reset()
	{
		dcRemoval.reset();
	}

    inline void processHiGain(Type* in, Type gain, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            in[i] = processSampleHiGain(in[i], gain);
    }

    inline void processLoGain(Type* in, Type gain, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            in[i] = processSampleLoGain(in[i], gain);
    }

	inline Type processSampleHiGain(Type xn, Type outGain)
	{
	    Type currentGain = outGain;
		if (xsimd::any(currentGain != lastGain))
			lastGain = 0.001f * currentGain + (1.0 - 0.001f) * lastGain;
		else
			lastGain = outGain;
	    
		Type gain = lastGain * 0.6;
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

		return yn;
	}

    inline Type processSampleLoGain(Type xn, Type outGain)
	{
	    Type currentGain = outGain;
		if (xsimd::any(currentGain != lastGain))
			lastGain = 0.001f * currentGain + (1.0 - 0.001f) * lastGain;
		else
			lastGain = outGain;
	    
		Type gain = lastGain * 0.6;
		Type yn = 0.0;

		xn *= gain;

		// --- asymmetrical waveshaping
		Type yn_pos = waveShaper(xn, 1.70, 23.6, 2.01);
		Type yn_neg = waveShaper(xn, 1.70, 2.01, 23.6);

		yn_pos = dcRemoval.processSample(yn_pos);
		yn_neg = dcRemoval.processSample(yn_neg);

		// --- symmetrical waveshaping
		yn_pos = waveShaper(yn_pos, 2.0, 2.01, 2.01);
		yn_neg = waveShaper(yn_neg, 2.0, 2.01, 2.01);

		yn = yn_pos + yn_neg;

		yn *= 0.1767;

		return yn;
	}

private:
	dsp::IIR::Filter<Type> dcRemoval;

    Type lastGain = 0.0;

	inline double waveShaper(double xn, Type g, Type Ln, Type Lp)
	{
		Type yn = 0.0;
		if (xn <= 0)
			yn = (g * xn) / (1.0 - ((g * xn) / Ln));
		else
			yn = (g * xn) / (1.0 + ((g * xn) / Lp));
		return yn;
	}

    inline vec waveShaper(vec xn, Type g, Type Ln, Type Lp)
    {
        return xsimd::select(xn <= 0.0, (g * xn) / (1.0 - ((g * xn) / Ln)), (g * xn) / (1.0 + ((g * xn) / Lp)));
    }
};

//=====================================================================

template <typename T>
class AmpProcessor
{
    std::atomic<float> *inputGain, *tsXGain, *bright, *outGain, *channel;

	dsp::IIR::Filter<T> highPass, bandPass, lowPass,
    bass, mid, treble, presence, brightShelf;

    AudioProcessorValueTreeState &vts;

public:
    AmpProcessor(AudioProcessorValueTreeState& v) : vts(v)
    {
        inputGain = vts.getRawParameterValue("gain");
        outGain = vts.getRawParameterValue("master");
        tsXGain = vts.getRawParameterValue("tsXgain");
        bright = vts.getRawParameterValue("bright");
        channel = vts.getRawParameterValue("channel");
    }

	void prepare(const dsp::ProcessSpec& spec) noexcept
	{
		ts9.prepare(spec);
		highPass.prepare(spec);
		bandPass.prepare(spec);
		lowPass.prepare(spec);
		preAmp.prepare(spec);
		bass.prepare(spec);
		mid.prepare(spec);
		treble.prepare(spec);
		presence.prepare(spec);
		brightShelf.prepare(spec);
		powerAmp.prepare(spec);

		highPass.coefficients = (dsp::IIR::Coefficients<double>::makeFirstOrderHighPass(spec.sampleRate, 750.f));
		bandPass.coefficients = (dsp::IIR::Coefficients<double>::makeBandPass(spec.sampleRate, 80.f));
		lowPass.coefficients = (dsp::IIR::Coefficients<double>::makeFirstOrderLowPass(spec.sampleRate, 10000.f));
		brightShelf.coefficients = (dsp::IIR::Coefficients<double>::makeHighShelf(spec.sampleRate, 2500.0, 0.707, 2.0));
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
	    // float currentBass = bassGain;
		// float currentMid = midGain;
		// float currentTreble = trebleGain;
		// float currentPres = presenceGain;

		// if (currentBass != lastBass)
		// 	currentBass = 0.1f * currentBass + (1.0 - 0.1f) * lastBass;
		// lastBass = currentBass;

		// if (currentMid != lastMid)
		// 	currentMid = 0.1f * currentMid + (1.0 - 0.1f) * lastMid;
		// lastMid = currentMid;

		// if (currentTreble != lastTreble)
		// 	currentTreble = 0.1f * currentTreble + (1.0 - 0.1f) * lastTreble;
		// lastTreble = currentTreble;

		// if (currentPres != lastPres)
		// 	currentPres = 0.1f * currentPres + (1.0 - 0.1f) * lastPres;
		// lastPres = currentPres;
	    
		bass.coefficients = (dsp::IIR::Coefficients<double>::makeLowShelf(sampleRate, 150.f, 0.606f, bassGain));
		mid.coefficients = (dsp::IIR::Coefficients<double>::makePeakFilter(sampleRate, 600.f, 0.5f, midGain));
		treble.coefficients = (dsp::IIR::Coefficients<double>::makeHighShelf(sampleRate, 1500.f, 0.3f, trebleGain));
		presence.coefficients = (dsp::IIR::Coefficients<double>::makePeakFilter(sampleRate, 4000.f, 0.6f, presenceGain));
	}

    template <typename Block>
	inline void processAmp(Block& block)
	{
        auto tsX = tsXGain->load();
        bool b = (bool)bright->load();
        auto inGain = inputGain->load();
        auto out = outGain->load();

        auto in = block.getChannelPointer(0);

        if (tsX > 0.f)
            ts9.process(in, tsX, block.getNumSamples());
        
        if (!*channel)
            preAmp.processLoGain(in, inGain, block.getNumSamples());
        else
            preAmp.processHiGain(in, inGain, block.getNumSamples());

        for (int i = 0; i < block.getNumSamples(); ++i)
        {
            in[i] = lowPass.processSample(in[i]);

            T xHPF = highPass.processSample(in[i]);
            T xBPF = bandPass.processSample(in[i]);
            in[i] = xHPF + xBPF;
            in[i] = bass.processSample(in[i]);
            in[i] = mid.processSample(in[i]);
            in[i] = treble.processSample(in[i]);
            in[i] = presence.processSample(in[i]);
            if (b)
                in[i] = brightShelf.processSample(in[i]);
        }

        if (!*channel)
            powerAmp.processLoGain(in, out, block.getNumSamples());
        else
            powerAmp.processHiGain(in, out, block.getNumSamples());
    }

	TS9<T> ts9;

	PreAmp<T> preAmp;

	ClassBValvePair<T> powerAmp;
	
private:
    float lastBass, lastMid, lastTreble, lastPres = 0.f;
};

//=======================================================================
