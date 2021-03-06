#ifndef _BEAT_DETECTION_PLUGIN_H_
#define _BEAT_DETECTION_PLUGIN_H_

#include <vamp-sdk/Plugin.h>

using std::string;

class BeatDetectionPlugin : public Vamp::Plugin {
public:
    BeatDetectionPlugin(float inputSampleRate);
    virtual ~BeatDetectionPlugin();

    string getIdentifier() const;
    string getName() const;
    string getDescription() const;
    string getMaker() const;
    int getPluginVersion() const;
    string getCopyright() const;

    InputDomain getInputDomain() const;
    size_t getPreferredBlockSize() const;
    size_t getPreferredStepSize() const;
    size_t getMinChannelCount() const;
    size_t getMaxChannelCount() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(string identifier) const;
    void setParameter(string identifier, float value);

    /*
    ProgramList getPrograms() const;
    string getCurrentProgram() const;
    void selectProgram(string name);
    */

    OutputList getOutputDescriptors() const;

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    float mInputSampleRate;
    size_t mStepSize;

    float mDownSampleCounter;
    float mProcessEvery;
    int mBeatCounter;

    bool mBeatStatus;
    Vamp::RealTime mBeatStart;
};

#endif
