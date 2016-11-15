
// This is a skeleton file for use in creating your own plugin
// libraries.  Replace BeatDetectionPlugin and myPlugin throughout with the name
// of your first plugin class, and fill in the gaps as appropriate.


#include "BeatDetectionPlugin.h"

#define OUT (std::cout << "[BeatDetection] ")

BeatDetectionPlugin::BeatDetectionPlugin(float inputSampleRate) :
    Plugin(inputSampleRate), mInputSampleRate(inputSampleRate), mStepSize(0), mCounter(0.0), mBeatCounter(0), mBeatStatus(false)
    // Also be sure to set your plugin parameters (presumably stored
    // in member variables) to their default values here -- the host
    // will not do that for you
{
    OUT << "initialized with sample rate " << inputSampleRate << std::endl;
    mProcessEvery = inputSampleRate / 5000.0;
    OUT << "processing every " << mProcessEvery << " sample" << std::endl;
}

BeatDetectionPlugin::~BeatDetectionPlugin()
{
}

string
BeatDetectionPlugin::getIdentifier() const
{
    return "beat-detection";
}

string
BeatDetectionPlugin::getName() const
{
    return "Beat Detection";
}

string
BeatDetectionPlugin::getDescription() const
{
    // Return something helpful here!
    return "";
}

string
BeatDetectionPlugin::getMaker() const
{
    // Your name here
    return "";
}

int
BeatDetectionPlugin::getPluginVersion() const
{
    // Increment this each time you release a version that behaves
    // differently from the previous one
    return 1;
}

string
BeatDetectionPlugin::getCopyright() const
{
    // This function is not ideally named.  It does not necessarily
    // need to say who made the plugin -- getMaker does that -- but it
    // should indicate the terms under which it is distributed.  For
    // example, "Copyright (year). All Rights Reserved", or "GPL"
    return "";
}

BeatDetectionPlugin::InputDomain
BeatDetectionPlugin::getInputDomain() const
{
    return TimeDomain;
}

size_t
BeatDetectionPlugin::getPreferredBlockSize() const
{
    return 0; // 0 means "I can handle any block size"
}

size_t 
BeatDetectionPlugin::getPreferredStepSize() const
{
    return 0; // 0 means "anything sensible"; in practice this
              // means the same as the block size for TimeDomain
              // plugins, or half of it for FrequencyDomain plugins
}

size_t
BeatDetectionPlugin::getMinChannelCount() const
{
    return 1;
}

size_t
BeatDetectionPlugin::getMaxChannelCount() const
{
    return 1;
}

BeatDetectionPlugin::ParameterList
BeatDetectionPlugin::getParameterDescriptors() const
{
    ParameterList list;

    // If the plugin has no adjustable parameters, return an empty
    // list here (and there's no need to provide implementations of
    // getParameter and setParameter in that case either).

    // Note that it is your responsibility to make sure the parameters
    // start off having their default values (e.g. in the constructor
    // above).  The host needs to know the default value so it can do
    // things like provide a "reset to default" function, but it will
    // not explicitly set your parameters to their defaults for you if
    // they have not changed in the mean time.

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

    return list;
}

float
BeatDetectionPlugin::getParameter(string identifier) const
{
    if (identifier == "parameter") {
        return 5; // return the ACTUAL current value of your parameter here!
    }
    return 0;
}

void
BeatDetectionPlugin::setParameter(string identifier, float value) 
{
    if (identifier == "parameter") {
        // set the actual value of your parameter
    }
}

BeatDetectionPlugin::ProgramList
BeatDetectionPlugin::getPrograms() const
{
    ProgramList list;

    // If you have no programs, return an empty list (or simply don't
    // implement this function or getCurrentProgram/selectProgram)

    return list;
}

string
BeatDetectionPlugin::getCurrentProgram() const
{
    return ""; // no programs
}

void
BeatDetectionPlugin::selectProgram(string name)
{
}

BeatDetectionPlugin::OutputList
BeatDetectionPlugin::getOutputDescriptors() const
{
    OutputList list;

    // See OutputDescriptor documentation for the possibilities here.
    // Every plugin must have at least one output.

    OutputDescriptor d;
    d.identifier = "output";
    d.name = "My Output";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::OneSamplePerStep;
    d.hasDuration = false;
    list.push_back(d);

    d.identifier = "beats";
    d.name = "Beats";
    d.description = "";
    d.unit = "";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.sampleRate = mInputSampleRate;
    d.hasDuration = false;
    list.push_back(d);

    return list;
}

bool
BeatDetectionPlugin::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) return false;

    // Real initialisation work goes here!
    mStepSize = std::min(stepSize, blockSize);

    return true;
}

void
BeatDetectionPlugin::reset()
{
    // Clear buffers, reset stored values, etc
    mCounter = 0.0;
    mProcessEvery = 0.0;
}

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

BeatDetectionPlugin::FeatureSet
BeatDetectionPlugin::process(const float *const *inputBuffers, Vamp::RealTime timestamp) {
    FeatureSet features;
    for (size_t i = 0; i < mStepSize; i++) {
        float sample = inputBuffers[0][i];
        /*
        if (mCounter == mInputSampleRate) {
            Feature feature;
            feature.label = "Beat!";
            feature.hasTimestamp = true;
            feature.timestamp = timestamp + Vamp::RealTime::frame2RealTime(i, (size_t) mInputSampleRate);
            features[1].push_back(feature);
            mCounter = 0;
        }
        */
        if (mCounter + 0.0001 >= mProcessEvery) {
            // Filter only bass component
            float value = bassFilter(sample);

            // Take signal amplitude and filter
            if(value < 0)
                value =- value;
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
                    features[1].push_back(feature);
                }

                // If we are above threshold, light up LED
                // return beat > beatThreshold ? BEAT_ON : BEAT_OFF;
            }
            mCounter -= mProcessEvery;
            mBeatCounter++;
        }
        mCounter += 1.0;
    }
    return features;
}

BeatDetectionPlugin::FeatureSet
BeatDetectionPlugin::getRemainingFeatures()
{
    return FeatureSet();
}

