#pragma once

#include "DSPUtils.h"
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <mutex>

class SpectralAnalyzer
{
public:
    static constexpr int FFT_ORDER = 12;
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;  // 4096
    static constexpr int NUM_BINS = FFT_SIZE / 2;
    static constexpr int NUM_BANDS = 32;

    SpectralAnalyzer()
        : fft(FFT_ORDER)
    {
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        currentSampleRate = sampleRate;
        reset();
    }

    void reset()
    {
        fftBuffer.fill(0.0f);
        fftBufferIndex = 0;

        magnitudeSpectrum.fill(-100.0f);
        smoothedSpectrum.fill(-100.0f);
        peakSpectrum.fill(-100.0f);

        for (auto& band : bandEnergies)
            band.store(-100.0f);

        spectralCentroid.store(0.0f);
        spectralSlope.store(0.0f);
        spectralFlatness.store(0.0f);
    }

    void pushSamples(const float* samples, int numSamples)
    {
        // Mono mix input
        for (int i = 0; i < numSamples; ++i)
        {
            fftBuffer[fftBufferIndex] = samples[i];
            fftBufferIndex++;

            if (fftBufferIndex >= FFT_SIZE)
            {
                processFFT();
                fftBufferIndex = 0;
            }
        }
    }

    void pushStereoSamples(const float* left, const float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            fftBuffer[fftBufferIndex] = (left[i] + right[i]) * 0.5f;
            fftBufferIndex++;

            if (fftBufferIndex >= FFT_SIZE)
            {
                processFFT();
                fftBufferIndex = 0;
            }
        }
    }

    // Get smoothed magnitude spectrum for UI display
    std::array<float, NUM_BINS> getMagnitudeSpectrum() const
    {
        std::lock_guard<std::mutex> lock(spectrumMutex);
        return smoothedSpectrum;
    }

    // Get peak-held spectrum for UI
    std::array<float, NUM_BINS> getPeakSpectrum() const
    {
        std::lock_guard<std::mutex> lock(spectrumMutex);
        return peakSpectrum;
    }

    // Get 32-band energy distribution
    std::array<float, NUM_BANDS> getBandEnergies() const
    {
        std::array<float, NUM_BANDS> result;
        for (int i = 0; i < NUM_BANDS; ++i)
            result[i] = bandEnergies[i].load();
        return result;
    }

    // Spectral features
    float getSpectralCentroid() const { return spectralCentroid.load(); }
    float getSpectralSlope() const { return spectralSlope.load(); }
    float getSpectralFlatness() const { return spectralFlatness.load(); }

    DSPUtils::SpectralFeatures getSpectralFeatures() const
    {
        std::lock_guard<std::mutex> lock(spectrumMutex);
        return lastFeatures;
    }

    // Utility: frequency to bin index
    int frequencyToBin(float frequency) const
    {
        return static_cast<int>(frequency * FFT_SIZE / currentSampleRate);
    }

    float binToFrequency(int bin) const
    {
        return static_cast<float>(bin) * currentSampleRate / FFT_SIZE;
    }

private:
    void processFFT()
    {
        // Copy and apply window
        std::array<float, FFT_SIZE * 2> fftData;
        fftData.fill(0.0f);

        for (int i = 0; i < FFT_SIZE; ++i)
        {
            // Blackman-Harris window for better frequency resolution
            float window = 0.35875f
                         - 0.48829f * std::cos(DSPUtils::TWO_PI * i / (FFT_SIZE - 1))
                         + 0.14128f * std::cos(2.0f * DSPUtils::TWO_PI * i / (FFT_SIZE - 1))
                         - 0.01168f * std::cos(3.0f * DSPUtils::TWO_PI * i / (FFT_SIZE - 1));
            fftData[i] = fftBuffer[i] * window;
        }

        // Perform FFT
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        // Calculate magnitude spectrum
        std::array<float, NUM_BINS> newMagnitudes;
        for (int i = 0; i < NUM_BINS; ++i)
        {
            float magnitude = fftData[i] / FFT_SIZE * 2.0f;  // Normalize
            newMagnitudes[i] = magnitude;
        }

        // Calculate spectral features
        DSPUtils::SpectralFeatures features = DSPUtils::calculateSpectralFeatures(
            newMagnitudes.data(), FFT_SIZE, currentSampleRate);

        // Update atomic feature values
        spectralCentroid.store(features.centroid);
        spectralSlope.store(features.slope);
        spectralFlatness.store(features.flatness);

        // Update band energies
        for (int i = 0; i < NUM_BANDS; ++i)
            bandEnergies[i].store(features.bandEnergies[i]);

        // Update spectrum with thread safety
        {
            std::lock_guard<std::mutex> lock(spectrumMutex);

            lastFeatures = features;

            for (int i = 0; i < NUM_BINS; ++i)
            {
                float magDB = DSPUtils::linearToDecibels(newMagnitudes[i]);
                magnitudeSpectrum[i] = magDB;

                // Smoothing
                float smoothing = 0.7f;
                smoothedSpectrum[i] = smoothedSpectrum[i] * smoothing + magDB * (1.0f - smoothing);

                // Peak hold with decay
                if (magDB > peakSpectrum[i])
                    peakSpectrum[i] = magDB;
                else
                    peakSpectrum[i] = peakSpectrum[i] * 0.995f + magDB * 0.005f;
            }
        }
    }

    double currentSampleRate = 44100.0;

    // FFT
    juce::dsp::FFT fft;
    std::array<float, FFT_SIZE> fftBuffer;
    int fftBufferIndex = 0;

    // Spectrum data (protected by mutex)
    mutable std::mutex spectrumMutex;
    std::array<float, NUM_BINS> magnitudeSpectrum;
    std::array<float, NUM_BINS> smoothedSpectrum;
    std::array<float, NUM_BINS> peakSpectrum;
    DSPUtils::SpectralFeatures lastFeatures;

    // Thread-safe atomic band energies
    std::array<std::atomic<float>, NUM_BANDS> bandEnergies;

    // Atomic spectral features
    std::atomic<float> spectralCentroid { 0.0f };
    std::atomic<float> spectralSlope { 0.0f };
    std::atomic<float> spectralFlatness { 0.0f };
};
