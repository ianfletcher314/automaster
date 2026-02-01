#pragma once

#include "DSPUtils.h"
#include <array>

class MasteringEQ
{
public:
    static constexpr int NUM_BANDS = 4;
    static constexpr int RESPONSE_SIZE = 512;

    MasteringEQ() = default;

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        currentBlockSize = samplesPerBlock;
        updateAllFilters();
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int stage = 0; stage < 4; ++stage)
            {
                hpfState[ch][stage].reset();
                lpfState[ch][stage].reset();
            }
            lowShelfState[ch].reset();
            highShelfState[ch].reset();
            for (int band = 0; band < NUM_BANDS; ++band)
                bandState[ch][band].reset();
        }
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (bypassed)
            return;

        const int numSamples = buffer.getNumSamples();
        const int numChannels = std::min(buffer.getNumChannels(), 2);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float x = buffer.getSample(ch, sample);

                // HPF (up to 4 cascaded stages for 24dB/oct)
                if (hpfEnabled)
                {
                    for (int stage = 0; stage < hpfOrder; ++stage)
                        x = hpfState[ch][stage].process(x, hpfCoeffs);
                }

                // Low shelf
                if (std::abs(lowShelfGain) > 0.01f)
                    x = lowShelfState[ch].process(x, lowShelfCoeffs);

                // Parametric bands
                for (int band = 0; band < NUM_BANDS; ++band)
                {
                    if (bandEnabled[band] && std::abs(bandGain[band]) > 0.01f)
                        x = bandState[ch][band].process(x, bandCoeffs[band]);
                }

                // High shelf
                if (std::abs(highShelfGain) > 0.01f)
                    x = highShelfState[ch].process(x, highShelfCoeffs);

                // LPF (up to 4 cascaded stages for 24dB/oct)
                if (lpfEnabled)
                {
                    for (int stage = 0; stage < lpfOrder; ++stage)
                        x = lpfState[ch][stage].process(x, lpfCoeffs);
                }

                buffer.setSample(ch, sample, x);
            }
        }
    }

    // HPF controls
    void setHPFFrequency(float freqHz)
    {
        hpfFreq = juce::jlimit(20.0f, 500.0f, freqHz);
        updateHPF();
    }

    void setHPFEnabled(bool enabled) { hpfEnabled = enabled; }

    void setHPFSlope(int dbPerOctave)
    {
        // 6, 12, 18, or 24 dB/octave
        hpfOrder = juce::jlimit(1, 4, dbPerOctave / 6);
        updateHPF();
    }

    // LPF controls
    void setLPFFrequency(float freqHz)
    {
        lpfFreq = juce::jlimit(5000.0f, 20000.0f, freqHz);
        updateLPF();
    }

    void setLPFEnabled(bool enabled) { lpfEnabled = enabled; }

    void setLPFSlope(int dbPerOctave)
    {
        lpfOrder = juce::jlimit(1, 4, dbPerOctave / 6);
        updateLPF();
    }

    // Low shelf controls
    void setLowShelfFrequency(float freqHz)
    {
        lowShelfFreq = juce::jlimit(20.0f, 500.0f, freqHz);
        updateLowShelf();
    }

    void setLowShelfGain(float gainDB)
    {
        lowShelfGain = juce::jlimit(-12.0f, 12.0f, gainDB);
        updateLowShelf();
    }

    // High shelf controls
    void setHighShelfFrequency(float freqHz)
    {
        highShelfFreq = juce::jlimit(2000.0f, 16000.0f, freqHz);
        updateHighShelf();
    }

    void setHighShelfGain(float gainDB)
    {
        highShelfGain = juce::jlimit(-12.0f, 12.0f, gainDB);
        updateHighShelf();
    }

    // Parametric band controls
    void setBandFrequency(int band, float freqHz)
    {
        if (band >= 0 && band < NUM_BANDS)
        {
            bandFreq[band] = juce::jlimit(20.0f, 20000.0f, freqHz);
            updateBand(band);
        }
    }

    void setBandGain(int band, float gainDB)
    {
        if (band >= 0 && band < NUM_BANDS)
        {
            bandGain[band] = juce::jlimit(-12.0f, 12.0f, gainDB);
            updateBand(band);
        }
    }

    void setBandQ(int band, float q)
    {
        if (band >= 0 && band < NUM_BANDS)
        {
            bandQ[band] = juce::jlimit(0.1f, 10.0f, q);
            updateBand(band);
        }
    }

    void setBandEnabled(int band, bool enabled)
    {
        if (band >= 0 && band < NUM_BANDS)
            bandEnabled[band] = enabled;
    }

    // Global controls
    void setBypass(bool shouldBypass) { bypassed = shouldBypass; }
    bool isBypassed() const { return bypassed; }

    // Get magnitude response for UI visualization
    std::array<float, RESPONSE_SIZE> getMagnitudeResponse() const
    {
        std::array<float, RESPONSE_SIZE> response;

        for (int i = 0; i < RESPONSE_SIZE; ++i)
        {
            // Logarithmic frequency mapping: 20Hz to 20kHz
            float normalizedX = static_cast<float>(i) / (RESPONSE_SIZE - 1);
            float freq = 20.0f * std::pow(1000.0f, normalizedX);

            float magnitude = 1.0f;

            // HPF response
            if (hpfEnabled)
            {
                float hpfMag = getFilterMagnitude(hpfCoeffs, freq);
                for (int stage = 0; stage < hpfOrder; ++stage)
                    magnitude *= hpfMag;
            }

            // Low shelf response
            if (std::abs(lowShelfGain) > 0.01f)
                magnitude *= getFilterMagnitude(lowShelfCoeffs, freq);

            // Band responses
            for (int band = 0; band < NUM_BANDS; ++band)
            {
                if (bandEnabled[band] && std::abs(bandGain[band]) > 0.01f)
                    magnitude *= getFilterMagnitude(bandCoeffs[band], freq);
            }

            // High shelf response
            if (std::abs(highShelfGain) > 0.01f)
                magnitude *= getFilterMagnitude(highShelfCoeffs, freq);

            // LPF response
            if (lpfEnabled)
            {
                float lpfMag = getFilterMagnitude(lpfCoeffs, freq);
                for (int stage = 0; stage < lpfOrder; ++stage)
                    magnitude *= lpfMag;
            }

            response[i] = DSPUtils::linearToDecibels(magnitude);
        }

        return response;
    }

    // Getters for current settings
    float getHPFFrequency() const { return hpfFreq; }
    float getLPFFrequency() const { return lpfFreq; }
    float getLowShelfFrequency() const { return lowShelfFreq; }
    float getLowShelfGain() const { return lowShelfGain; }
    float getHighShelfFrequency() const { return highShelfFreq; }
    float getHighShelfGain() const { return highShelfGain; }
    float getBandFrequency(int band) const { return band >= 0 && band < NUM_BANDS ? bandFreq[band] : 0.0f; }
    float getBandGain(int band) const { return band >= 0 && band < NUM_BANDS ? bandGain[band] : 0.0f; }
    float getBandQ(int band) const { return band >= 0 && band < NUM_BANDS ? bandQ[band] : 1.0f; }

private:
    void updateAllFilters()
    {
        updateHPF();
        updateLPF();
        updateLowShelf();
        updateHighShelf();
        for (int band = 0; band < NUM_BANDS; ++band)
            updateBand(band);
    }

    void updateHPF()
    {
        hpfCoeffs.makeHighPass(currentSampleRate, hpfFreq, 0.707f);
    }

    void updateLPF()
    {
        lpfCoeffs.makeLowPass(currentSampleRate, lpfFreq, 0.707f);
    }

    void updateLowShelf()
    {
        lowShelfCoeffs.makeLowShelf(currentSampleRate, lowShelfFreq, lowShelfGain, 0.707f);
    }

    void updateHighShelf()
    {
        highShelfCoeffs.makeHighShelf(currentSampleRate, highShelfFreq, highShelfGain, 0.707f);
    }

    void updateBand(int band)
    {
        if (band >= 0 && band < NUM_BANDS)
            bandCoeffs[band].makePeaking(currentSampleRate, bandFreq[band], bandGain[band], bandQ[band]);
    }

    float getFilterMagnitude(const DSPUtils::BiquadCoeffs& coeffs, float freq) const
    {
        // Calculate magnitude response at given frequency
        float w = DSPUtils::TWO_PI * freq / static_cast<float>(currentSampleRate);
        float cosw = std::cos(w);
        float cos2w = std::cos(2.0f * w);
        float sinw = std::sin(w);
        float sin2w = std::sin(2.0f * w);

        // Numerator: b0 + b1*z^-1 + b2*z^-2 at z = e^(jw)
        float numReal = coeffs.b0 + coeffs.b1 * cosw + coeffs.b2 * cos2w;
        float numImag = -coeffs.b1 * sinw - coeffs.b2 * sin2w;

        // Denominator: 1 + a1*z^-1 + a2*z^-2 at z = e^(jw)
        float denReal = 1.0f + coeffs.a1 * cosw + coeffs.a2 * cos2w;
        float denImag = -coeffs.a1 * sinw - coeffs.a2 * sin2w;

        float numMag = std::sqrt(numReal * numReal + numImag * numImag);
        float denMag = std::sqrt(denReal * denReal + denImag * denImag);

        return denMag > 0.0f ? numMag / denMag : 0.0f;
    }

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool bypassed = false;

    // HPF
    float hpfFreq = 30.0f;
    int hpfOrder = 2;  // 12dB/oct
    bool hpfEnabled = false;
    DSPUtils::BiquadCoeffs hpfCoeffs;
    DSPUtils::BiquadState hpfState[2][4];

    // LPF
    float lpfFreq = 18000.0f;
    int lpfOrder = 2;
    bool lpfEnabled = false;
    DSPUtils::BiquadCoeffs lpfCoeffs;
    DSPUtils::BiquadState lpfState[2][4];

    // Low shelf
    float lowShelfFreq = 100.0f;
    float lowShelfGain = 0.0f;
    DSPUtils::BiquadCoeffs lowShelfCoeffs;
    DSPUtils::BiquadState lowShelfState[2];

    // High shelf
    float highShelfFreq = 8000.0f;
    float highShelfGain = 0.0f;
    DSPUtils::BiquadCoeffs highShelfCoeffs;
    DSPUtils::BiquadState highShelfState[2];

    // Parametric bands
    std::array<float, NUM_BANDS> bandFreq = { 200.0f, 800.0f, 2500.0f, 6000.0f };
    std::array<float, NUM_BANDS> bandGain = { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, NUM_BANDS> bandQ = { 1.0f, 1.0f, 1.0f, 1.0f };
    std::array<bool, NUM_BANDS> bandEnabled = { true, true, true, true };
    std::array<DSPUtils::BiquadCoeffs, NUM_BANDS> bandCoeffs;
    DSPUtils::BiquadState bandState[2][NUM_BANDS];
};
