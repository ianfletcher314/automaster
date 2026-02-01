#pragma once

#include "FeatureExtractor.h"
#include "../DSP/ParameterGenerator.h"
#include <memory>

// Abstract interface for AI model inference
class ModelInterface
{
public:
    virtual ~ModelInterface() = default;

    // Initialize the model
    virtual bool initialize() = 0;

    // Check if model is ready
    virtual bool isReady() const = 0;

    // Run inference: features -> parameters
    virtual ParameterGenerator::GeneratedParameters infer(
        const FeatureExtractor::FeatureVector& features) = 0;

    // Get model confidence (0-1)
    virtual float getConfidence() const = 0;

    // Get model name/version
    virtual std::string getModelName() const = 0;
};

// Null model (returns default parameters)
class NullModel : public ModelInterface
{
public:
    bool initialize() override { return true; }
    bool isReady() const override { return true; }

    ParameterGenerator::GeneratedParameters infer(
        const FeatureExtractor::FeatureVector& /*features*/) override
    {
        // Return neutral parameters
        ParameterGenerator::GeneratedParameters params;
        params.confidence = 0.0f;
        return params;
    }

    float getConfidence() const override { return 0.0f; }
    std::string getModelName() const override { return "None"; }
};
