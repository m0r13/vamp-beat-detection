#include "BeatDetectionPlugin.h"

#define OUT (std::cout << "[BeatDetection] ")

BeatDetectionPlugin::BeatDetectionPlugin(float inputSampleRate) :
    Plugin(inputSampleRate), mInputSampleRate(inputSampleRate),
    mStepSize(0), mDownSampleCounter(0.0), mBeatCounter(0), mBeatStatus(false) {
    mProcessEvery = inputSampleRate / 5000.0;
    OUT << "initialized with sample rate " << inputSampleRate << std::endl;
    OUT << "processing every " << mProcessEvery << " sample" << std::endl;
}

BeatDetectionPlugin::~BeatDetectionPlugin() {
}

string BeatDetectionPlugin::getIdentifier() const {
    return "vamp-beat-detection";
}

string BeatDetectionPlugin::getName() const {
    return "Beat Detection";
}

string BeatDetectionPlugin::getDescription() const {
    // Return something helpful here!
    return "";
}

string BeatDetectionPlugin::getMaker() const {
    // Your name here
    return "";
}

int BeatDetectionPlugin::getPluginVersion() const {
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 1;
}

string BeatDetectionPlugin::getCopyright() const {
    // This function is not ideally named.  It does not necessarily
    // need to say who made the plugin -- getMaker does that -- but it
    // should indicate the terms under which it is distributed.  For
    // example, "Copyright (year). All Rights Reserved", or "GPL"
    return "MIT";
}

BeatDetectionPlugin::InputDomain BeatDetectionPlugin::getInputDomain() const {
    return TimeDomain;
}

size_t BeatDetectionPlugin::getPreferredBlockSize() const {
    return 0; // 0 means "I can handle any block size"
}

size_t 
BeatDetectionPlugin::getPreferredStepSize() const {
    return 0; // 0 means "anything sensible"; in practice this
              // means the same as the block size for TimeDomain
              // plugins, or half of it for FrequencyDomain plugins
}

size_t BeatDetectionPlugin::getMinChannelCount() const {
    return 1;
}

size_t BeatDetectionPlugin::getMaxChannelCount() const {
    return 1;
}

BeatDetectionPlugin::ParameterList BeatDetectionPlugin::getParameterDescriptors() const {
    ParameterList list;

    /*
    ParameterDescriptor d;
    d.identifier = "parameter";
    d.name = "Some Parameter";
    d.description = "";
    d.unit = "";
    d.minValue = 0;
    d.maxValue = 10;
    d.defaultValue = 5;
    d.isQuantized = false;
    list.push_back(d);
    */

    return list;
}

float BeatDetectionPlugin::getParameter(string identifier) const {
    if (identifier == "parameter") {
        return 5; // return the ACTUAL current value of your parameter here!
    }
    return 0;
}

void BeatDetectionPlugin::setParameter(string identifier, float value)  {
    if (identifier == "parameter") {
        // set the actual value of your parameter
    }
}

BeatDetectionPlugin::OutputList BeatDetectionPlugin::getOutputDescriptors() const {
    OutputList list;

    // See OutputDescriptor documentation for the possibilities here.
    // Every plugin must have at least one output.

    OutputDescriptor d;
    d.identifier = "beats";
    d.name = "Beats";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = true;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.hasDuration = false;
    list.push_back(d);

    return list;
}

bool BeatDetectionPlugin::initialise(size_t channels, size_t stepSize, size_t blockSize) {
    if (channels < getMinChannelCount()
            || channels > getMaxChannelCount()) return false;

    // Real initialisation work goes here!
    mStepSize = std::min(stepSize, blockSize);

    return true;
}

void BeatDetectionPlugin::reset() {
    // Clear buffers, reset stored values, etc
    mDownSampleCounter = 0.0;
    mProcessEvery = 0.0;
}

// Based on:
// Arduino Beat Detector By Damian Peckett 2015
// http://dpeckett.com/beat-detection-on-the-arduino
// License: Public Domain.

// 20 - 200hz Single Pole Bandpass IIR Filter
float bassFilter(float sample) {
    static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
    xv[0] = xv[1]; xv[1] = xv[2];
    xv[2] = sample / 9.1f;
    yv[0] = yv[1]; yv[1] = yv[2];
    yv[2] = (xv[2] - xv[0])
        + (-0.7960060012f * yv[0]) + (1.7903124146f * yv[1]);
    return yv[2];
}

// 10hz Single Pole Lowpass IIR Filter
float envelopeFilter(float sample) { //10hz low pass
    static float xv[2] = {0,0}, yv[2] = {0,0};
    xv[0] = xv[1];
    xv[1] = sample / 160.f;
    yv[0] = yv[1];
    yv[1] = (xv[0] + xv[1]) + (0.9875119299f * yv[0]);
    return yv[1];
}

// 1.7 - 3.0hz Single Pole Bandpass IIR Filter
float beatFilter(float sample) {
    static float xv[3] = {0,0,0}, yv[3] = {0,0,0};
    xv[0] = xv[1]; xv[1] = xv[2];
    xv[2] = sample / 7.015f;
    yv[0] = yv[1]; yv[1] = yv[2];
    yv[2] = (xv[2] - xv[0])
        + (-0.7169861741f * yv[0]) + (1.4453653501f * yv[1]);
    return yv[2];
}

BeatDetectionPlugin::FeatureSet BeatDetectionPlugin::process(const float *const *inputBuffers, Vamp::RealTime timestamp) {
    FeatureSet features;
    for (size_t i = 0; i < mStepSize; i++) {
        float sample = inputBuffers[0][i];

        // do the whole filtering on about 5000Hz sample rate
        if (mDownSampleCounter + 0.0001 < mProcessEvery) {
            mDownSampleCounter += 1.0;
            continue;
        }
        mDownSampleCounter -= mProcessEvery;
        mBeatCounter++;

        // Filter only bass component
        float value = bassFilter(sample);

        // Take signal amplitude and filter
        if(value < 0)
            value = -value;
        float envelope = envelopeFilter(value);

        // Every 200 samples (25hz) filter the envelope·
        if (mBeatCounter == 200) {
            mBeatCounter = 0;

            // Filter out repeating bass sounds 100 - 180bpm
            float beat = beatFilter(envelope);

            // Threshold it based on potentiometer on AN1
            // beatThreshold = 0.02f * (float)analogRead(1);
            //beatThreshold = 9.0;
            //beatThreshold = 0.02 * beatThreshold.getValue();
            
            float beatThreshold = 9.0 / 512.0 * 2.0;
            if (!mBeatStatus && beat > beatThreshold) {
                mBeatStatus = true;
                mBeatStart = timestamp + Vamp::RealTime::frame2RealTime(i, (size_t) mInputSampleRate);
                /*
                Feature feature;
                feature.label = "B";
                feature.hasTimestamp = true;
                feature.timestamp = timestamp + Vamp::RealTime::frame2RealTime(i, (size_t) mInputSampleRate);
                features[1].push_back(feature);
                */
            } else if (mBeatStatus && beat <= beatThreshold) {
                mBeatStatus = false;
                /*
                Feature feature;
                feature.label = ".";
                feature.hasTimestamp = true;
                feature.timestamp = timestamp + Vamp::RealTime::frame2RealTime(i, (size_t) mInputSampleRate);
                features[1].push_back(feature);
                */
                Vamp::RealTime time = timestamp + Vamp::RealTime::frame2RealTime(i, (size_t) mInputSampleRate);

                Feature feature;
                feature.label = "B";
                feature.hasTimestamp = true;
                feature.timestamp = mBeatStart;
                feature.hasDuration = true;
                feature.duration = time - mBeatStart;
                features[0].push_back(feature);
            }
        }
    }
    return features;
}

BeatDetectionPlugin::FeatureSet BeatDetectionPlugin::getRemainingFeatures() {
    return FeatureSet();
}

