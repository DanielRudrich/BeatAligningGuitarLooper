/*
  ==============================================================================

    settingsComponent.h
    Created: 2 Sep 2017 1:55:34pm
    Author:  Daniel Rudrich

  ==============================================================================
*/

#pragma once

class SettingsComponent : public Component
{
public:
    SettingsComponent (AudioAppComponent& pluginHolder,
                       juce::ComboBox::Listener* comboBoxListener,
                       AudioDeviceManager& deviceManagerToUse,
                       int minAudioInputChannels,
                       int maxAudioInputChannels,
                       int minAudioOutputChannels,
                       int maxAudioOutputChannels,
                       int ccRecord,
                       int ccReset)
    :
    cbListener(comboBoxListener),
    owner (pluginHolder),
    deviceSelector (deviceManagerToUse,
                    minAudioInputChannels, maxAudioInputChannels,
                    minAudioOutputChannels, maxAudioOutputChannels,
                    true, false,
                    true, false),
    lbCcRecord  ("Record MIDI CC#:", "Record MIDI CC#:"),
    lbCcReset  ("Reset MIDI CC#:", "Reset MIDI CC#:")
    {
        setOpaque (true);
        
        
        addAndMakeVisible (deviceSelector);
        

        for (int cc = 0; cc<= 128; ++cc)
        {
            cbCcRecord.addItem(String(cc), cc+1);
            cbCcReset.addItem(String(cc), cc+1);
        }
        
        cbCcRecord.setSelectedItemIndex(ccRecord);
        cbCcReset.setSelectedItemIndex(ccReset);
        cbCcRecord.setComponentID("ccRecord");
        cbCcReset.setComponentID("ccReset");
        cbCcRecord.addListener(cbListener);
        cbCcReset.addListener(cbListener);
        
        addAndMakeVisible (lbCcRecord);
        addAndMakeVisible (cbCcRecord);
        lbCcRecord.attachToComponent (&cbCcRecord, true);
        addAndMakeVisible (lbCcReset);
        addAndMakeVisible (cbCcReset);
        lbCcReset.attachToComponent (&cbCcReset, true);

    }
    
    void paint (Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    }
    
    void resized() override
    {
        auto r = getLocalBounds();
        

        auto itemHeight = deviceSelector.getItemHeight();
        auto extra = r.removeFromTop (itemHeight);
            
        //auto seperatorHeight = (itemHeight >> 1);


        
        deviceSelector.setBounds (r);
        r.removeFromTop(deviceSelector.getHeight());
        r.setY(r.getY()-20);
        
        r.removeFromLeft(extra.proportionOfWidth (0.35f));
        r.removeFromRight(extra.proportionOfWidth (0.30f));
        
        cbCcRecord.setBounds (r.removeFromTop(itemHeight));
        r.removeFromTop(4);
        cbCcReset.setBounds (r.removeFromTop(itemHeight));
    }
    
private:
    //==============================================================================
    juce::ComboBox::Listener* cbListener;
    AudioAppComponent& owner;
    AudioDeviceSelectorComponent deviceSelector;
    Label lbCcRecord, lbCcReset;
    ComboBox cbCcRecord, cbCcReset;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsComponent)
};


