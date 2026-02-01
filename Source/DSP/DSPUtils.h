#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <array>
#include <atomic>

namespace DSPUtils
{
    // Constants
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float MINUS_INFINITY_DB = -100.0f;

    // Conversion functions
    inline float linearToDecibels(float linear)
    {
        return linear > 0.0f ? 20.0f * std::log10(linear) : MINUS_INFINITY_DB;
    }

    inline float decibelsToLinear(float dB)
    {
        return dB > MINUS_INFINITY_DB ? std::pow(10.0f, dB / 20.0f) : 0.0f;
    }

    inline float frequencyToMel(float freq)
    {
        return 2595.0f * std::log10(1.0f + freq / 700.0f);
    }

    inline float melToFrequency(float mel)
    {
        return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
    }

    // Biquad filter coefficients
    struct BiquadCoeffs
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;

        void makeBypass()
        {
            b0 = 1.0f; b1 = 0.0f; b2 = 0.0f;
            a1 = 0.0f; a2 = 0.0f;
        }

        void makeLowPass(double sampleRate, float frequency, float Q)
        {
            float w0 = TWO_PI * frequency / static_cast<float>(sampleRate);
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);

            float a0 = 1.0f + alpha;
            b0 = ((1.0f - cosw0) / 2.0f) / a0;
            b1 = (1.0f - cosw0) / a0;
            b2 = ((1.0f - cosw0) / 2.0f) / a0;
            a1 = (-2.0f * cosw0) / a0;
            a2 = (1.0f - alpha) / a0;
        }

        void makeHighPass(double sampleRate, float frequency, float Q)
        {
            float w0 = TWO_PI * frequency / static_cast<float>(sampleRate);
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);

            float a0 = 1.0f + alpha;
            b0 = ((1.0f + cosw0) / 2.0f) / a0;
            b1 = (-(1.0f + cosw0)) / a0;
            b2 = ((1.0f + cosw0) / 2.0f) / a0;
            a1 = (-2.0f * cosw0) / a0;
            a2 = (1.0f - alpha) / a0;
        }

        void makePeaking(double sampleRate, float frequency, float gainDB, float Q)
        {
            float A = std::pow(10.0f, gainDB / 40.0f);
            float w0 = TWO_PI * frequency / static_cast<float>(sampleRate);
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);

            float a0 = 1.0f + alpha / A;
            b0 = (1.0f + alpha * A) / a0;
            b1 = (-2.0f * cosw0) / a0;
            b2 = (1.0f - alpha * A) / a0;
            a1 = (-2.0f * cosw0) / a0;
            a2 = (1.0f - alpha / A) / a0;
        }

        void makeLowShelf(double sampleRate, float frequency, float gainDB, float Q)
        {
            float A = std::pow(10.0f, gainDB / 40.0f);
            float w0 = TWO_PI * frequency / static_cast<float>(sampleRate);
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);
            float sqrtA = std::sqrt(A);

            float a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
            b0 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha) / a0;
            b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0) / a0;
            b2 = A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha) / a0;
            a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0) / a0;
            a2 = ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha) / a0;
        }

        void makeHighShelf(double sampleRate, float frequency, float gainDB, float Q)
        {
            float A = std::pow(10.0f, gainDB / 40.0f);
            float w0 = TWO_PI * frequency / static_cast<float>(sampleRate);
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);
            float sqrtA = std::sqrt(A);

            float a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
            b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha) / a0;
            b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0) / a0;
            b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha) / a0;
            a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0) / a0;
            a2 = ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha) / a0;
        }

        void makeAllPass(double sampleRate, float frequency, float Q)
        {
            float w0 = TWO_PI * frequency / static_cast<float>(sampleRate);
            float cosw0 = std::cos(w0);
            float sinw0 = std::sin(w0);
            float alpha = sinw0 / (2.0f * Q);

            float a0 = 1.0f + alpha;
            b0 = (1.0f - alpha) / a0;
            b1 = (-2.0f * cosw0) / a0;
            b2 = (1.0f + alpha) / a0;
            a1 = (-2.0f * cosw0) / a0;
            a2 = (1.0f - alpha) / a0;
        }
    };

    // Biquad filter state
    struct BiquadState
    {
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;

        void reset()
        {
            x1 = x2 = y1 = y2 = 0.0f;
        }

        float process(float input, const BiquadCoeffs& coeffs)
        {
            float output = coeffs.b0 * input + coeffs.b1 * x1 + coeffs.b2 * x2
                         - coeffs.a1 * y1 - coeffs.a2 * y2;
            x2 = x1;
            x1 = input;
            y2 = y1;
            y1 = output;
            return output;
        }
    };

    // Smoothed value for parameter ramping
    class SmoothedValue
    {
    public:
        void reset(double sampleRate, float rampTimeMs = 20.0f)
        {
            if (rampTimeMs <= 0.0f)
            {
                smoothingCoeff = 1.0f;
            }
            else
            {
                float rampSamples = static_cast<float>(sampleRate * rampTimeMs / 1000.0);
                smoothingCoeff = 1.0f - std::exp(-1.0f / rampSamples);
            }
        }

        void setTargetValue(float newTarget)
        {
            targetValue = newTarget;
        }

        void setCurrentAndTargetValue(float value)
        {
            currentValue = value;
            targetValue = value;
        }

        float getNextValue()
        {
            currentValue += smoothingCoeff * (targetValue - currentValue);
            return currentValue;
        }

        float getCurrentValue() const { return currentValue; }
        float getTargetValue() const { return targetValue; }
        bool isSmoothing() const { return std::abs(targetValue - currentValue) > 1e-6f; }

    private:
        float currentValue = 0.0f;
        float targetValue = 0.0f;
        float smoothingCoeff = 1.0f;
    };

    // Envelope follower for dynamics processing
    class EnvelopeFollower
    {
    public:
        void prepare(double sampleRate)
        {
            this->sampleRate = sampleRate;
            updateCoefficients();
        }

        void setAttackTime(float attackMs)
        {
            this->attackMs = attackMs;
            updateCoefficients();
        }

        void setReleaseTime(float releaseMs)
        {
            this->releaseMs = releaseMs;
            updateCoefficients();
        }

        void reset()
        {
            envelope = 0.0f;
        }

        float process(float input)
        {
            float absInput = std::abs(input);
            if (absInput > envelope)
                envelope = attackCoeff * (envelope - absInput) + absInput;
            else
                envelope = releaseCoeff * (envelope - absInput) + absInput;
            return envelope;
        }

        float processRMS(float inputSquared)
        {
            if (inputSquared > envelopeSquared)
                envelopeSquared = attackCoeff * (envelopeSquared - inputSquared) + inputSquared;
            else
                envelopeSquared = releaseCoeff * (envelopeSquared - inputSquared) + inputSquared;
            return std::sqrt(envelopeSquared);
        }

    private:
        void updateCoefficients()
        {
            if (sampleRate > 0.0)
            {
                attackCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * attackMs / 1000.0f));
                releaseCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * releaseMs / 1000.0f));
            }
        }

        double sampleRate = 44100.0;
        float attackMs = 10.0f;
        float releaseMs = 100.0f;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        float envelope = 0.0f;
        float envelopeSquared = 0.0f;
    };

    // Linkwitz-Riley crossover filter (4th order)
    class LinkwitzRileyCrossover
    {
    public:
        void prepare(double sampleRate)
        {
            this->sampleRate = sampleRate;
        }

        void setCrossoverFrequency(float frequency)
        {
            crossoverFreq = frequency;
            updateCoefficients();
        }

        void reset()
        {
            lpState1.reset();
            lpState2.reset();
            hpState1.reset();
            hpState2.reset();
        }

        void process(float input, float& lowOut, float& highOut)
        {
            // 4th order LR = 2x cascaded 2nd order Butterworth
            float lpTemp = lpState1.process(input, lpCoeffs);
            lowOut = lpState2.process(lpTemp, lpCoeffs);

            float hpTemp = hpState1.process(input, hpCoeffs);
            highOut = hpState2.process(hpTemp, hpCoeffs);
        }

    private:
        void updateCoefficients()
        {
            // Q = 0.707 for Butterworth (cascaded twice for LR4)
            lpCoeffs.makeLowPass(sampleRate, crossoverFreq, 0.707f);
            hpCoeffs.makeHighPass(sampleRate, crossoverFreq, 0.707f);
        }

        double sampleRate = 44100.0;
        float crossoverFreq = 1000.0f;

        BiquadCoeffs lpCoeffs, hpCoeffs;
        BiquadState lpState1, lpState2;
        BiquadState hpState1, hpState2;
    };

    // Window functions for FFT analysis
    inline void applyHannWindow(float* data, int size)
    {
        for (int i = 0; i < size; ++i)
        {
            float window = 0.5f * (1.0f - std::cos(TWO_PI * i / (size - 1)));
            data[i] *= window;
        }
    }

    inline void applyBlackmanHarrisWindow(float* data, int size)
    {
        const float a0 = 0.35875f;
        const float a1 = 0.48829f;
        const float a2 = 0.14128f;
        const float a3 = 0.01168f;

        for (int i = 0; i < size; ++i)
        {
            float angle = TWO_PI * i / (size - 1);
            float window = a0 - a1 * std::cos(angle) + a2 * std::cos(2.0f * angle) - a3 * std::cos(3.0f * angle);
            data[i] *= window;
        }
    }

    // Spectral features
    struct SpectralFeatures
    {
        float centroid = 0.0f;        // Brightness indicator (Hz)
        float spread = 0.0f;          // Spectral width
        float flatness = 0.0f;        // Tonality (0=tonal, 1=noisy)
        float slope = 0.0f;           // Spectral tilt (dB/octave)
        float rolloff = 0.0f;         // Frequency below which 85% of energy
        std::array<float, 32> bandEnergies = {};  // 32-band energy distribution
    };

    // Calculate spectral features from FFT magnitude data
    inline SpectralFeatures calculateSpectralFeatures(const float* magnitudes, int fftSize, double sampleRate)
    {
        SpectralFeatures features;
        int numBins = fftSize / 2;
        float binWidth = static_cast<float>(sampleRate) / fftSize;

        // Calculate total energy and weighted frequency sum for centroid
        float totalEnergy = 0.0f;
        float weightedFreqSum = 0.0f;

        for (int i = 1; i < numBins; ++i)
        {
            float freq = i * binWidth;
            float energy = magnitudes[i] * magnitudes[i];
            totalEnergy += energy;
            weightedFreqSum += freq * energy;
        }

        if (totalEnergy > 0.0f)
        {
            features.centroid = weightedFreqSum / totalEnergy;

            // Spectral spread (standard deviation around centroid)
            float spreadSum = 0.0f;
            for (int i = 1; i < numBins; ++i)
            {
                float freq = i * binWidth;
                float energy = magnitudes[i] * magnitudes[i];
                float diff = freq - features.centroid;
                spreadSum += diff * diff * energy;
            }
            features.spread = std::sqrt(spreadSum / totalEnergy);
        }

        // Spectral flatness (geometric mean / arithmetic mean)
        float logSum = 0.0f;
        float linSum = 0.0f;
        int validBins = 0;

        for (int i = 1; i < numBins; ++i)
        {
            if (magnitudes[i] > 1e-10f)
            {
                logSum += std::log(magnitudes[i]);
                linSum += magnitudes[i];
                validBins++;
            }
        }

        if (validBins > 0 && linSum > 0.0f)
        {
            float geometricMean = std::exp(logSum / validBins);
            float arithmeticMean = linSum / validBins;
            features.flatness = geometricMean / arithmeticMean;
        }

        // Spectral slope (linear regression in log-frequency space)
        float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f;
        int slopePoints = 0;

        for (int i = 1; i < numBins; ++i)
        {
            float freq = i * binWidth;
            if (freq > 20.0f && freq < 20000.0f && magnitudes[i] > 1e-10f)
            {
                float logFreq = std::log2(freq);
                float logMag = linearToDecibels(magnitudes[i]);
                sumX += logFreq;
                sumY += logMag;
                sumXY += logFreq * logMag;
                sumX2 += logFreq * logFreq;
                slopePoints++;
            }
        }

        if (slopePoints > 1)
        {
            features.slope = (slopePoints * sumXY - sumX * sumY) /
                            (slopePoints * sumX2 - sumX * sumX);
        }

        // Spectral rolloff (frequency at 85% cumulative energy)
        float cumulativeEnergy = 0.0f;
        float rolloffThreshold = totalEnergy * 0.85f;

        for (int i = 1; i < numBins; ++i)
        {
            cumulativeEnergy += magnitudes[i] * magnitudes[i];
            if (cumulativeEnergy >= rolloffThreshold)
            {
                features.rolloff = i * binWidth;
                break;
            }
        }

        // 32-band energy distribution (logarithmic spacing)
        float minFreq = 20.0f;
        float maxFreq = 20000.0f;
        float logMin = std::log2(minFreq);
        float logMax = std::log2(maxFreq);
        float logStep = (logMax - logMin) / 32.0f;

        for (int band = 0; band < 32; ++band)
        {
            float lowFreq = std::pow(2.0f, logMin + band * logStep);
            float highFreq = std::pow(2.0f, logMin + (band + 1) * logStep);
            int lowBin = static_cast<int>(lowFreq / binWidth);
            int highBin = static_cast<int>(highFreq / binWidth);

            lowBin = std::max(1, std::min(lowBin, numBins - 1));
            highBin = std::max(lowBin + 1, std::min(highBin, numBins));

            float bandEnergy = 0.0f;
            for (int i = lowBin; i < highBin; ++i)
                bandEnergy += magnitudes[i] * magnitudes[i];

            features.bandEnergies[band] = linearToDecibels(std::sqrt(bandEnergy / (highBin - lowBin + 1)));
        }

        return features;
    }

    // Soft clipper for gentle saturation
    inline float softClip(float input, float threshold = 0.9f)
    {
        if (std::abs(input) < threshold)
            return input;

        float sign = input > 0.0f ? 1.0f : -1.0f;
        float absInput = std::abs(input);
        float excess = absInput - threshold;
        float softRegion = 1.0f - threshold;

        return sign * (threshold + softRegion * std::tanh(excess / softRegion));
    }

    // True peak detection using 4x oversampling
    class TruePeakDetector
    {
    public:
        void prepare(double sampleRate)
        {
            // Simple 4x oversampling filter coefficients
            // Low-pass at Nyquist/4 to prevent aliasing
            float cutoff = static_cast<float>(sampleRate) * 0.24f;
            lpCoeffs.makeLowPass(sampleRate * 4.0, cutoff, 0.707f);
            reset();
        }

        void reset()
        {
            for (auto& s : states)
                s.reset();
            for (auto& b : buffer)
                b = 0.0f;
            bufferIndex = 0;
            peakValue = 0.0f;
        }

        float process(float input)
        {
            // Store input in circular buffer for interpolation
            buffer[bufferIndex] = input;

            // 4x oversampling with linear interpolation
            float samples[4];
            int prevIndex = (bufferIndex + 3) % 4;
            float prev = buffer[prevIndex];

            for (int i = 0; i < 4; ++i)
            {
                float t = i / 4.0f;
                float interp = prev + t * (input - prev);
                samples[i] = states[i].process(interp, lpCoeffs);
                peakValue = std::max(peakValue, std::abs(samples[i]));
            }

            bufferIndex = (bufferIndex + 1) % 4;
            return peakValue;
        }

        float getPeakValue() const { return peakValue; }
        void resetPeak() { peakValue = 0.0f; }

    private:
        BiquadCoeffs lpCoeffs;
        std::array<BiquadState, 4> states;
        std::array<float, 4> buffer = {};
        int bufferIndex = 0;
        float peakValue = 0.0f;
    };

} // namespace DSPUtils
