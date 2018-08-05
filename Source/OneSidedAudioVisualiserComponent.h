/*
  ==============================================================================

    OneSidedAudioVisualiserComponent.h
    Created: 26 Aug 2017 1:19:44pm
    Author:  Daniel Rudrich

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
*/
class OneSidedAudioVisualiserComponent    : public AudioVisualiserComponent
{
public:
    OneSidedAudioVisualiserComponent(const int initialNumChannels) : AudioVisualiserComponent(initialNumChannels)
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

    }

    ~OneSidedAudioVisualiserComponent()
    {
    }


    float getChannelAsPathAndReturnMaxValue (Path& path, const Range<float>* levels, int numLevels, int nextSample)
    {
        path.preallocateSpace (4 * numLevels + 8);
        float maxValue = -1.0f;
        int i;
        for (i = 0; i < numLevels; ++i)
        {
            const float level = -(levels[(nextSample + i) % numLevels].getEnd()); //level is negative to point up
            
            if (level < maxValue) maxValue = level;
            if (i == 0)
                path.startNewSubPath (0.0f, level);
            else
                path.lineTo ((float) i, level);
            
        }
        path.lineTo ((float) i, 0.0f);
        path.lineTo (0.0f, 0.0f);
        
        path.closeSubPath();
        return maxValue;
    }
    
    void paintChannel (Graphics& g, Rectangle<float> area,
                                                 const Range<float>* levels, int numLevels, int nextSample) override
    {
        Path p;
        float maxValue = getChannelAsPathAndReturnMaxValue (p, levels, numLevels, nextSample);
        
        g.fillPath (p, AffineTransform::fromTargetPoints (0.0f, maxValue,               area.getX(), area.getY(),
                                                          0.0f, 0.0f,                area.getX(), area.getBottom(),
                                                          (float) numLevels, maxValue,  area.getRight(), area.getY()));
        g.drawText(String(-maxValue,2), area.getX(), area.getY(), 50, 20, Justification::left);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneSidedAudioVisualiserComponent)
};
