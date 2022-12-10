#pragma once

template <typename Type>
class TS9
{
public:
    TS9() = default;

    void prepare(const dsp::ProcessSpec &spec) noexcept
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

    inline void process(double *x, double drive, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            x[i] = processSample(x[i], drive);
    }

    inline void process(vec *x, vec drive, int numSamples)
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
    PreAmp(strix::FloatParameter *inGain, strix::ChoiceParameter *crossover, strix::ChoiceParameter *channel) : inGain(inGain), xover(crossover), channel(channel)
    {
    }

    void prepare(const dsp::ProcessSpec &spec) noexcept
    {
        inputHPF.prepare(spec);
        dcRemoval.prepare(spec);
        lowShelf.prepare(spec);

        lr.prepare(spec);
        lr.setType(strix::LRFilterType::lowpass);

        dcRemoval.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(spec.sampleRate, 10.0));
        lowShelf.coefficients = (dsp::IIR::Coefficients<double>::makeLowShelf(spec.sampleRate, 185.0, 1.8, 0.5));
        inputHPF.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(spec.sampleRate, 65.0));

        gain.reset(spec.maximumBlockSize);
    }

    void reset()
    {
        inputHPF.reset();
        dcRemoval.reset();
        lowShelf.reset();
        lr.reset();
    }

    std::atomic<bool> needCrossoverUpdate = false;

    void updateCrossover(int crossover)
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

    template <typename Block>
    void process(Block &block)
    {
        if (needCrossoverUpdate)
        {
            updateCrossover(*xover);
            needCrossoverUpdate = false;
        }

        gain.setTargetValue(*inGain);

        if (*channel)
        {
            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
                processHiGain(block.getChannelPointer(ch), block.getNumSamples());
        }
        else
        {
            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
                processLoGain(block.getChannelPointer(ch), block.getNumSamples());
        }
    }

private:
    inline void processHiGain(Type *in, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            in[i] = processSampleHiGain(in[i]);
        }
    }

    inline void processLoGain(Type *in, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            in[i] = processSampleLoGain(in[i]);
        }
    }

    inline Type processSampleHiGain(Type xn)
    {
        Type gain_ = gain.getNextValue() * 8.f;
        Type yn = 0.0, xnL = 0.0, xnH = 0.0;

        xn *= gain_;

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

    inline Type processSampleLoGain(Type xn)
    {
        Type gain_ = gain.getNextValue() * 4.f;
        Type yn = 0.0, xnL = 0.0, xnH = 0.0;

        xn *= gain_;

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

    inline double hiGainSaturation(double x)
    {
        double k = gain.getCurrentValue() / 3.0;
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

    inline vec hiGainSaturation(vec x)
    {
        vec k = gain.getCurrentValue() / 3.0;
        vec nk = k / 0.9;

        return xsimd::select(x > 0.0,
                             xsimd::atan(k * x) / xsimd::atan(k),
                             0.9 * xsimd::atan(nk * x) / xsimd::atan(nk));
    }

    inline vec loGainSaturation(vec x)
    {
        return xsimd::select(x > 0.0, (x / (1.0 + xsimd::abs(x))) * 2.0, (2.0 * x) / (1.0 + xsimd::abs(x * 2.0)));
    }

    strix::LinkwitzRileyFilter<Type> lr;

    dsp::IIR::Filter<Type> inputHPF, dcRemoval, lowShelf;

    strix::FloatParameter *inGain = nullptr;
    strix::ChoiceParameter *xover = nullptr;
    strix::ChoiceParameter *channel = nullptr;

    SmoothedValue<float> gain;
};

//========================================================

template <typename Type>
struct ToneSection
{
    ToneSection(AudioProcessorValueTreeState &v) : apvts(v)
    {
        bass_p = static_cast<strix::FloatParameter *>(apvts.getParameter("bass"));
        mid_p = static_cast<strix::FloatParameter *>(apvts.getParameter("mid"));
        treble_p = static_cast<strix::FloatParameter *>(apvts.getParameter("treble"));
        presence_p = static_cast<strix::FloatParameter *>(apvts.getParameter("presence"));
        bright_p = static_cast<strix::BoolParameter *>(apvts.getParameter("bright"));
        legacy_p = static_cast<strix::BoolParameter *>(apvts.getParameter("legacyTone"));
    }

    dsp::IIR::Filter<Type> highPass, bandPass, lowPass, bass, mid, treble, presence, brightShelf;

    std::vector<dsp::IIR::Filter<Type> *> getFilters()
    {
        return {
            &highPass,
            &bandPass,
            &lowPass,
            &bass,
            &mid,
            &treble,
            &presence,
            &brightShelf};
    }

    SmoothedValue<float> bass_s, mid_s, treble_s, pres_s;
    std::vector<SmoothedValue<float> *> getSmoothers()
    {
        return {
            &bass_s,
            &mid_s,
            &treble_s,
            &pres_s};
    }

    void prepare(const dsp::ProcessSpec &spec)
    {
        SR = spec.sampleRate;

        highPass.coefficients = (dsp::IIR::Coefficients<double>::makeFirstOrderHighPass(spec.sampleRate, 750.f));
        bandPass.coefficients = (dsp::IIR::Coefficients<double>::makeBandPass(spec.sampleRate, 80.f));
        lowPass.coefficients = (dsp::IIR::Coefficients<double>::makeFirstOrderLowPass(spec.sampleRate, 10000.f));
        brightShelf.coefficients = (dsp::IIR::Coefficients<double>::makeHighShelf(spec.sampleRate, 2500.0, 0.707, 2.0));

        updateAllFilters();

        for (auto *s : getSmoothers())
            s->reset(spec.maximumBlockSize);
    }

    void reset()
    {
        for (auto *f : getFilters())
            f->reset();
    }

    void updateAllFilters()
    {
        float bassParam = *bass_p;
        float midParam = *mid_p;
        float trebleParam = *treble_p;
        float presenceParam = *presence_p;

        bassParam /= 10.f;
        midParam /= 10.f;
        trebleParam /= 10.f;
        presenceParam /= 10.f;

        float bassCook = 0.0, midCook = 0.0, trebleCook = 0.0, presenceCook = 0.0;

        if (!*legacy_p)
        {
            // convert 0 - 1 value into a dB value
            bassCook = jmap(bassParam, -12.f, 12.f);
            midCook = jmap(midParam, -7.f, 7.f);
            trebleCook = jmap(trebleParam, -14.f, 14.f);
            presenceCook = jmap(presenceParam, -8.f, 8.f);

            // convert to linear multiplier
            bassCook = pow(10, (bassCook / 20));
            midCook = pow(10, (midCook / 20));
            trebleCook = pow(10, (trebleCook / 20));
            presenceCook = pow(10, (presenceCook / 20));
        }
        else
        {
            // convert 0 - 1 value into legacy linear values
            bassCook = cookParams(bassParam, 0.2f, 1.666f);
            midCook = cookParams(midParam, 0.3f, 2.2f);
            trebleCook = cookParams(trebleParam, 0.2f, 3.0f);
            presenceCook = cookParams(presenceParam, 0.4f, 2.5f);
        }
        bass.coefficients = (dsp::IIR::Coefficients<double>::makeLowShelf(SR, 150.f, 0.606f, bassCook));
        mid.coefficients = (dsp::IIR::Coefficients<double>::makePeakFilter(SR, 600.f, 0.5f, midCook));
        treble.coefficients = (dsp::IIR::Coefficients<double>::makeHighShelf(SR, 1500.f, 0.3f, trebleCook));
        presence.coefficients = (dsp::IIR::Coefficients<double>::makePeakFilter(SR, 4000.f, 0.6f, presenceCook));
    }

    /**
     * @param index bitmask for which filter needs update
     * 1: Bass | 2: Mid | 4: Treble | 8: Presence
     */
    inline void updateFilter(int index, float newValue)
    {
        newValue /= 10.f;

        switch (index)
        {
        case 1:
            if (!*legacy_p)
            {
                newValue = jmap(newValue, -12.f, 12.f);
                newValue = pow(10, (newValue / 20));
            }
            else
                newValue = cookParams(newValue, 0.2f, 1.666f);
            *bass.coefficients = (dsp::IIR::ArrayCoefficients<double>::makeLowShelf(SR, 150.f, 0.606f, newValue));
            break;
        case 1 << 1:
            if (!*legacy_p)
            {
                newValue = jmap(newValue, -7.f, 7.f);
                newValue = pow(10, (newValue / 20));
            }
            else
                newValue = cookParams(newValue, 0.3f, 2.2f);
            *mid.coefficients = (dsp::IIR::ArrayCoefficients<double>::makePeakFilter(SR, 600.f, 0.5f, newValue));
            break;
        case 1 << 2:
            if (!*legacy_p)
            {
                newValue = jmap(newValue, -14.f, 14.f);
                newValue = pow(10, (newValue / 20));
            }
            else
                newValue = cookParams(newValue, 0.2f, 3.0f);
            *treble.coefficients = (dsp::IIR::ArrayCoefficients<double>::makeHighShelf(SR, 1500.f, 0.3f, newValue));
            break;
        case 1 << 3:
            if (!*legacy_p)
            {
                newValue = jmap(newValue, -8.f, 8.f);
                newValue = pow(10, (newValue / 20));
            }
            else
                newValue = cookParams(newValue, 0.4f, 2.5f);
            *presence.coefficients = (dsp::IIR::ArrayCoefficients<double>::makePeakFilter(SR, 4000.f, 0.6f, newValue));
            break;
        }
    }

    template <typename Block>
    void process(Block &block)
    {
        bass_s.setTargetValue(*bass_p);
        mid_s.setTargetValue(*mid_p);
        treble_s.setTargetValue(*treble_p);
        pres_s.setTargetValue(*presence_p);

        bool b = *bright_p;

        auto smoothers = getSmoothers();
        int bitmask = 0;
        for (int i = 0; i < smoothers.size(); ++i)
            if (smoothers[i]->isSmoothing())
                bitmask |= 1 << i;

        if (bitmask > 0)
        {
            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
            {
                auto *in = block.getChannelPointer(ch);
                for (size_t i = 0; i < block.getNumSamples(); ++i)
                {
                    int bit = 1;
                    for (int n = 0; n < smoothers.size(); ++n)
                    {
                        if (bit & bitmask)
                            updateFilter(bit, smoothers[n]->getNextValue());
                        
                        bit <<= 1;
                        // doing it per-channel since we're using SIMD and will only run once...not the best
                    }

                    in[i] = lowPass.processSample(in[i]);

                    Type xHPF = highPass.processSample(in[i]);
                    Type xBPF = bandPass.processSample(in[i]);
                    in[i] = xHPF + xBPF;
                    in[i] = bass.processSample(in[i]);
                    in[i] = mid.processSample(in[i]);
                    in[i] = treble.processSample(in[i]);
                    in[i] = presence.processSample(in[i]);
                    if (b)
                        in[i] = brightShelf.processSample(in[i]);
                }
            }
            return;
        }

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto *in = block.getChannelPointer(ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i)
            {
                in[i] = lowPass.processSample(in[i]);

                Type xHPF = highPass.processSample(in[i]);
                Type xBPF = bandPass.processSample(in[i]);
                in[i] = xHPF + xBPF;
                in[i] = bass.processSample(in[i]);
                in[i] = mid.processSample(in[i]);
                in[i] = treble.processSample(in[i]);
                in[i] = presence.processSample(in[i]);
                if (b)
                    in[i] = brightShelf.processSample(in[i]);
            }
        }
    }

private:
    AudioProcessorValueTreeState &apvts;
    strix::FloatParameter *bass_p, *mid_p, *treble_p, *presence_p;
    strix::BoolParameter *bright_p, *legacy_p;
    double SR = 44100.0;
};

//========================================================

template <typename Type>
class ClassBValvePair
{
public:
    ClassBValvePair(strix::FloatParameter *outGain, strix::ChoiceParameter *chMode) : gain(outGain), channel(chMode)
    {
    }

    void prepare(const dsp::ProcessSpec &spec) noexcept
    {
        dcRemoval.prepare(spec);
        dcRemoval.coefficients = (dsp::IIR::Coefficients<double>::makeHighPass(spec.sampleRate, 10.0));
    }

    void reset()
    {
        dcRemoval.reset();
    }

    template <typename Block>
    void process(Block &block)
    {
        if (*channel)
            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
                processHiGain(block.getChannelPointer(ch), *gain, block.getNumSamples());
        else
            for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
                processLoGain(block.getChannelPointer(ch), *gain, block.getNumSamples());
    }

    inline void processHiGain(Type *in, float gain, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            in[i] = processSampleHiGain(in[i], gain);
    }

    inline void processLoGain(Type *in, float gain, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
            in[i] = processSampleLoGain(in[i], gain);
    }

    inline Type processSampleHiGain(Type xn, float outGain)
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

        return yn;
    }

    inline Type processSampleLoGain(Type xn, float outGain)
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

    strix::FloatParameter *gain = nullptr;
    strix::ChoiceParameter *channel = nullptr;

    float lastGain = 0.0;

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
    std::atomic<float> *inputGain, *tsXGain, *outGain, *channel;

    double SR = 0.0;

    AudioProcessorValueTreeState &vts;

public:
    AmpProcessor(AudioProcessorValueTreeState &v) : vts(v),
                                                    preAmp(static_cast<strix::FloatParameter *>(vts.getParameter("gain")), static_cast<strix::ChoiceParameter *>(vts.getParameter("mode")), static_cast<strix::ChoiceParameter *>(vts.getParameter("channel"))),
                                                    eq(vts),
                                                    powerAmp(static_cast<strix::FloatParameter*>(vts.getParameter("master")), static_cast<strix::ChoiceParameter*>(vts.getParameter("channel")))
    {
        inputGain = vts.getRawParameterValue("gain");
        outGain = vts.getRawParameterValue("master");
        tsXGain = vts.getRawParameterValue("tsXgain");
        channel = vts.getRawParameterValue("channel");
    }

    void prepare(const dsp::ProcessSpec &spec) noexcept
    {
        ts9.prepare(spec);
        preAmp.prepare(spec);
        eq.prepare(spec);
        powerAmp.prepare(spec);

        SR = spec.sampleRate;
    }

    void reset()
    {
        ts9.reset();
        preAmp.reset();
        eq.reset();
        powerAmp.reset();
    }

    template <typename Block>
    inline void processAmp(Block &block)
    {
        auto tsX = tsXGain->load();

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto in = block.getChannelPointer(ch);
            if (tsX > 0.f)
                ts9.process(in, tsX, block.getNumSamples());
        }

        preAmp.process(block);
        eq.process(block);
        powerAmp.process(block);
    }

    TS9<T> ts9;
    PreAmp<T> preAmp;
    ToneSection<T> eq;
    ClassBValvePair<T> powerAmp;
};

//=======================================================================
