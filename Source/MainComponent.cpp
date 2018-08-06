/*
 ==============================================================================
 
 This file was auto-generated!
 
 ==============================================================================
 */

#include "../JuceLibraryCode/JuceHeader.h"
#include "Eigen/Eigen"
#include <fftw3.h>
#include "OdfSpectralFluxLogFiltered.h"

#define fs 48000

#define maxLoopTimeInSec 20
#define prePlayTimeInSec 1
#define overheadTimeInSec 0.1
#define fftSize 2048
#define fftPowerOf2 11
#define fftHopsize 480
#define odfFs 100
#define theta 1800
#define maxTauDiff 0.03f
#define fadeLength 200

#define nTaus 160
#define tauMin 0.06f
#define tauMax 0.48f

#ifdef JUCE_IOS
    #define listen2inp false
    #define advBufferSize 64
#else
    #define listen2inp false
    #define advBufferSize 32
#endif


#ifndef M_PI
	#define M_PI 3.141592654
#endif
#include "odfThread.h"
#include "tempogramThread.h"
#include "OneSidedAudioVisualiserComponent.h"
#include "SliderLaF.h"
#include "TitleBar.h"
#include "simpleLabel.h"
#include "parula.h"
#include "logos.h"
#include "settingsComponent.h"

//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
 your controls and content.
 */
class MainContentComponent   : public AudioAppComponent, public MidiInputCallback, public Timer, public Button::Listener, public ComboBox::Listener, public KeyListener
{
public:
    //==============================================================================
    MainContentComponent()
    : tgData(nTaus,odfFs*maxLoopTimeInSec), avcIn(2), avcOdf(1),
    imgTempogram(Image::PixelFormat::ARGB, odfFs * maxLoopTimeInSec, nTaus, true)
    {
        setSize (800, 600);
        setLookAndFeel(&globalLaF);
        
        addKeyListener(this);
        
        addAndMakeVisible(&title);
        title.setTitle(String("beat-aligning"),String("guitar looper"));
        title.setFont(globalLaF.robotoBold, globalLaF.robotoLight);
        
        addAndMakeVisible(&kugLogo);
        addAndMakeVisible(&iemLogo);

        addAndMakeVisible(&tbListen);
        tbListen.setButtonText("ON/OFF");
        tbListen.addListener(this);
        tbListen.setColour(ToggleButton::tickColourId, globalLaF.ClWidgetColours[0]);
        tbListen.setComponentID("listen");
        tbListen.setToggleState(listen2inp, NotificationType::dontSendNotification);
        listenToInput = listen2inp;
        
        addAndMakeVisible(&tbDrums);
        tbDrums.setButtonText("ON/OFF");
        tbDrums.addListener(this);
        tbDrums.setColour(ToggleButton::tickColourId, globalLaF.ClWidgetColours[0]);
        tbDrums.setComponentID("drums");
        tbDrums.setToggleState(true, NotificationType::dontSendNotification);
        listenToDrums = true;
        
        addAndMakeVisible(&lbListen);
        lbListen.setText("Monitor input");
        lbListen.setJustification(Justification::left);
        
        addAndMakeVisible(&lbDrums);
        lbDrums.setText("Play drums");
        lbDrums.setJustification(Justification::left);
        
        addAndMakeVisible(&settingsButton);
        settingsButton.setComponentID("settings");
        settingsButton.setButtonText("Audio/MIDI settings");
        settingsButton.addListener(this);
        
        addAndMakeVisible(&lbCbBeatAlignmentGrid);
        lbCbBeatAlignmentGrid.setText("grid size (beats)");
        lbCbBeatAlignmentGrid.setJustification(Justification::left);
        
        addAndMakeVisible(&cbBeatAlignmentGrid);
        cbBeatAlignmentGrid.setComponentID("gridSize");
        cbBeatAlignmentGrid.addListener(this);
        cbBeatAlignmentGrid.addItem("1", 1);
        cbBeatAlignmentGrid.addItem("4", 4);
        cbBeatAlignmentGrid.addItem("8", 8);
        //cbBeatAlignmentGrid.addItem("16", 16);
        cbBeatAlignmentGrid.setSelectedId(1);
        cbBeatAlignmentGrid.setJustificationType(Justification::centred);
        
        addAndMakeVisible(&midiButton);
        midiButton.setComponentID("record");
        midiButton.setButtonText("Record/Overdub");
        midiButton.addListener(this);
        
        addAndMakeVisible(&resetButton);
        resetButton.setComponentID("reset");
        resetButton.setButtonText("Reset");
        resetButton.addListener(this);
        
        addAndMakeVisible(&avcIn);
        avcIn.setBufferSize(200);
        avcIn.setSamplesPerBlock(4800);
        avcIn.setRepaintRate(20);
        
        addAndMakeVisible(&avcOdf);
        avcOdf.setBufferSize(400);
        avcOdf.setSamplesPerBlock(2);
        avcOdf.setRepaintRate(20);
        
        addAndMakeVisible(&imgCmp);
        imgCmp.setImage(imgTempogram);
        imgCmp.setImagePlacement(RectanglePlacement::stretchToFit);
        
        
        // load drumparts
        bd.setSize(1, 32768);
        sn.setSize(1, 32768);
        hh.setSize(1, 32768);
        
        AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        WavAudioFormat wavFormat;
        
        MemoryInputStream *mis = new MemoryInputStream (BinaryData::bd_32768_wav, BinaryData::bd_32768_wavSize, false);
        ScopedPointer<AudioFormatReader> reader = wavFormat.createReaderFor(mis, true);
        reader->read(&bd, 0, 32768, 0, true, false);
        
        mis = new MemoryInputStream (BinaryData::sn_32768_wav, BinaryData::sn_32768_wavSize, false);
        reader = wavFormat.createReaderFor(mis, true);
        reader->read(&sn, 0, 32768, 0, true, false);
        
        mis = new MemoryInputStream (BinaryData::hh_32768_wav, BinaryData::hh_32768_wavSize, false);
        reader = wavFormat.createReaderFor(mis, true);
        reader->read(&hh, 0, 32768, 0, true, false);
        
        
        setSize (800, 600);
        
        
        
        // specify the number of input and output channels that we want to open
        setAudioChannels (1, 2);
        
        midiCcRecord = 16;
        midiCcReset = 18;
        
        
        deviceManager.addMidiInputCallback("", this);
        StringArray midiDevices = MidiInput::getDevices();
        for (int i = 0; i< midiDevices.size(); ++i)
        {
            deviceManager.setMidiInputEnabled(midiDevices.getReference(i),true);
        }
    
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        deviceManager.getAudioDeviceSetup(setup);
        setup.sampleRate = fs;
        setup.bufferSize = advBufferSize;
        String ret = deviceManager.setAudioDeviceSetup(setup, true);
        DBG(ret);
        DBG(deviceManager.getCurrentAudioDevice()->getName());
        


        
        
        startTimer(50);
    }
    
    ~MainContentComponent()
    {
        shutdownAudio();
        setLookAndFeel(nullptr);
//        WavAudioFormat* test = new WavAudioFormat();
//        File outputFile = File("/Users/rudrich/output.wav");
//        FileOutputStream* outputTo = outputFile.createOutputStream();
//        AudioFormatWriter* writer = test->createWriterFor(outputTo, 48000, 1, 16,NULL, 0);
//        writer->writeFromFloatArrays(loopData.getArrayOfReadPointers(), 1, 48000*10);
//        delete writer;
//        delete test;
    }
    
    enum systemState {
        startIdle,
        recordStart,
        recording,
        recordOverhead,
        looping,
        overdub
    };
    
    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double newSampleRate) override
    {
        // This function will be called when the audio device is started, or when
        // its settings (i.e. sample rate, block size, etc) are changed.
        
        // You can use this function to initialise any resources you might need,
        // but be careful - it will be called on the audio thread, not the GUI thread.
        
        // For more details, see the help for AudioProcessor::prepareToPlay()
        
        blockSize = samplesPerBlockExpected;
        sampleRate = newSampleRate;
        
        
        DBG("Samples per Block: " << samplesPerBlockExpected);

        odfData.setSize(1, sampleRate/odfFs*maxLoopTimeInSec);
        odfTm.resize(sampleRate/odfFs*maxLoopTimeInSec);
        drums.setSize(1, sampleRate * (maxLoopTimeInSec + 1)); //make it longer for drum samples' decay
        
        
        loopData.setSize(1, sampleRate * maxLoopTimeInSec);
        loopDataMaxOffset = sampleRate * (maxLoopTimeInSec - overheadTimeInSec) - 2*samplesPerBlockExpected; //just to be safe
        prePlayData.setSize(1, sampleRate * prePlayTimeInSec);
        
        
        reset();
    }
    void reset()
    {
        blockFinished = true;
        doReset = false;
        
        state = startIdle;
        stateHasChanged = true;
        
        fadesNotAddedYet = true;
        
        thrOdf.stopThread(500);
        thrTg.stopThread(500);
        
        avcIn.clear();
        avcOdf.clear();
        
        prePlayWriteOffset = 0;
        
        messageCollector.reset(sampleRate);

        odfData.clear();
        drums.clear();
        loopDataWriteOffset = sampleRate * prePlayTimeInSec;
        prePlayMaxOffset = sampleRate * prePlayTimeInSec;
        
        state = startIdle;
        overHeadCount = sampleRate * overheadTimeInSec / blockSize + 1; // make it one more.. why not?
        validLoopData = -1;
        loopDataFinished = false;
        validOdfData = -1;
        odfDataFinished = false;
        validTgData = -1;
        tgDataFinished = false;
        currOdfSample = -1;
        pathIndexValid = false;
        optimalPathPainted = false;
        drumsReady = false;
        
        
        imgTempogram.clear(imgTempogram.getBounds(),Colours::black);
        imgCmp.setImage(imgTempogram);
        imgCmp.repaint();
        processedTgData = -1;
        
        
        thrOdf.initialise(loopData.getReadPointer(0), &validLoopData, &loopDataFinished, &odfData, &validOdfData, &odfTm, &odfDataFinished);
        thrOdf.startThread(5);
        thrTg.initialise(&odfData, &validOdfData, &odfTm, &odfDataFinished, &tgData, &validTgData, &tgDataFinished,
                         pathIndex, &pathIndexValid, pathPhase, pathMag,
                         &drums, &drumsReady, &bd, &sn, &hh,
                         &startCue, &stopCue,
                         &beatAlignmentGridSize);
        thrTg.startThread(5);
    }
    
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        //AudioIODevice* device = deviceManager.getCurrentAudioDevice();
        //const BigInteger activeInputChannels = device->getActiveInputChannels();
        //const BigInteger activeOutputChannels = device->getActiveOutputChannels();
        //const int maxInputChannels = activeInputChannels.getHighestBit() + 1;
        //const int maxOutputChannels = activeOutputChannels.getHighestBit() + 1;
        
        AudioBuffer<float> inpCopy(1, blockSize);
        inpCopy.copyFrom(0, 0, *bufferToFill.buffer, 0, 0, blockSize);
        
        incomingMidi.clear();
        messageCollector.removeNextBlockOfMessages(incomingMidi, blockSize);
        
        MidiBuffer::Iterator iterator(incomingMidi);
        MidiMessage message;
        
        bool footSwitchPressed = false;
        bool resetPressed = false;
        int footSwitchSamplePosition;
        if (!incomingMidi.isEmpty())
        {
            while (!footSwitchPressed && iterator.getNextEvent(message, footSwitchSamplePosition))
            {
                if (message.isController() && message.isControllerOfType(midiCcRecord.get()) && message.getControllerValue() == 127)
                {
                    //DBG("footSwitch detected at position: " << footSwitchSamplePosition);
                    footSwitchPressed = true;
                }
                else if (message.isController() && message.isControllerOfType(midiCcReset.get()) && message.getControllerValue() == 127){
                    resetPressed = true;
                    DBG("resetButton");
                }
                
            }
        }
        
        if (footSwitchPressed)
        {
            if (state == recording && (loopDataWriteOffset + footSwitchSamplePosition) < 96000)
            {
                DBG("too few samples recorded!");
                doReset = true;
            }
            else
                switch (state) {
                    case startIdle:
                        int firstNumCopy;
                        firstNumCopy = prePlayMaxOffset-prePlayWriteOffset;
                        loopData.copyFrom(0, 0, prePlayData, 0, prePlayWriteOffset, firstNumCopy);
                        loopData.copyFrom(0, firstNumCopy, prePlayData, 0, 0, prePlayWriteOffset);
                        state = recording;
                        startCue = loopDataWriteOffset + footSwitchSamplePosition;
                        loopDataReadOffset = startCue.get();
                        DBG("start Cue: " << startCue.get() << "! start recording");
                        break;
                        
                    case recording:
                        stopCue = loopDataWriteOffset + footSwitchSamplePosition;
                        state = recordOverhead;
                        DBG("stop Cue: " << stopCue.get() << "! recording overhead now");
                        break;
                        
                    case recordOverhead:
                        break;
                    case looping:
                        state = overdub;
                        break;
                    case overdub:
                        state = looping;
                        break;
                    default:
                        break;
                }
            stateHasChanged = true;
        }
        
        
        
        // ====================== RECORD INPUT SIGNAL ============
        switch (state) {
            case startIdle:

                if (prePlayWriteOffset + blockSize >= prePlayMaxOffset)
                {
                    int firstNumCopy;
                    firstNumCopy = prePlayMaxOffset-prePlayWriteOffset;
                    prePlayData.copyFrom(0, prePlayWriteOffset, *bufferToFill.buffer, 0, 0, firstNumCopy);
                    prePlayData.copyFrom(0, 0, *bufferToFill.buffer, 0, firstNumCopy, blockSize-firstNumCopy);
 
                    prePlayWriteOffset = blockSize - firstNumCopy;
                }
                else
                {
                    prePlayData.copyFrom(0, prePlayWriteOffset, *bufferToFill.buffer, 0, 0, blockSize);
                    prePlayWriteOffset += blockSize;
                }
                break;
                
//                if (prePlayWriteOffset >= prePlayMaxOffset) prePlayWriteOffset = 0;
//                prePlayData.copyFrom(0, prePlayWriteOffset, *bufferToFill.buffer, 0, 0, blockSize);
//                prePlayWriteOffset += blockSize;
//                break;
                
            case recording:
                loopData.copyFrom(0, loopDataWriteOffset, *bufferToFill.buffer, 0, 0, blockSize);
                loopDataWriteOffset += blockSize;
                validLoopData = loopDataWriteOffset-1;
                if (loopDataWriteOffset >= loopDataMaxOffset) { // AUTOMATIC STOP
                    DBG("AUTOMATIC STOP");
                    stopCue = loopDataWriteOffset - 1; // (-1) <- does it really matter?
                    state = recordOverhead;
                }
                break;
                
            case recordOverhead:
                if (overHeadCount>0)
                {
                    loopData.copyFrom(0, loopDataWriteOffset, *bufferToFill.buffer, 0, 0, blockSize);
                    loopDataWriteOffset += blockSize;
                    validLoopData = loopDataWriteOffset-1;
                    --overHeadCount;
                }
                else {
                    loopDataFinished = true;
                    state = looping;
                    DBG("overhead recorded!  validLoopData: " << validLoopData.get() << "normal looping mode");
                }
                break;
                
            default:
                break;
        }
        
        
        int startC = startCue.get();
        int stopC = stopCue.get();
        
        bool copyDrums = drumsReady.get() && listenToDrums.get();
        
        if (copyDrums && fadesNotAddedYet)
        {
            DBG("startC: " << startC);
            DBG("fadeLength: " << fadeLength);
            DBG("stopC: " << stopC);
            DBG("stopC-fadeLength: " << stopC-fadeLength);
            DBG("startC-fadeLength: " << startC-fadeLength);
            
            
            loopData.addFromWithRamp(0, startC, loopData.getReadPointer(0,stopC), fadeLength, 1.0f, 0.0f);
            loopData.addFromWithRamp(0, stopC-fadeLength, loopData.getReadPointer(0,startC-fadeLength), fadeLength, 0.0f, 1.0f);
            
            
            fadesNotAddedYet = false;
        }
        
        
        // ====================== OUTPUT =====================
        const float* inBuffer = bufferToFill.buffer->getReadPointer (0, bufferToFill.startSample);
        float* outBufferLeft = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
        float* outBufferRight = bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample);
        
        // visualize new input
        avcIn.pushBuffer(bufferToFill);
        
        
        if (!listenToInput.get()) bufferToFill.clearActiveBufferRegion();
        
        
        
        if (state == recordOverhead || state == looping || state == overdub)
        {
            const float* loopDataPtr = loopData.getReadPointer(0);
            const float* drumDataPtr = drums.getReadPointer(0);
            if (loopDataReadOffset + blockSize >= stopC)
            {
                int firstNumCopy;
                firstNumCopy = stopC-loopDataReadOffset;
                FloatVectorOperations::add(outBufferLeft, loopDataPtr+loopDataReadOffset, firstNumCopy);
                FloatVectorOperations::add(outBufferLeft+firstNumCopy, loopDataPtr+startC, blockSize - firstNumCopy);
                if (copyDrums)
                {
                    FloatVectorOperations::add(outBufferLeft, drumDataPtr+loopDataReadOffset, firstNumCopy);
                    FloatVectorOperations::add(outBufferLeft+firstNumCopy, drumDataPtr+startC, blockSize - firstNumCopy);
                }
                if (state == overdub) {
                    loopData.addFrom(0, loopDataReadOffset, inpCopy, 0, 0, firstNumCopy);
                    loopData.addFrom(0, startC, inpCopy, 0, firstNumCopy, blockSize - firstNumCopy);
                }
                    
                loopDataReadOffset = startC + blockSize - firstNumCopy;
            }
            else
            {
                FloatVectorOperations::add(outBufferLeft, loopDataPtr+loopDataReadOffset, blockSize);
                if (copyDrums)
                {
                    FloatVectorOperations::add(outBufferLeft, drumDataPtr+loopDataReadOffset, blockSize);
                }
                if (state == overdub)
                {
                    loopData.addFrom(0, loopDataReadOffset, inpCopy, 0, 0, blockSize);
                }
                
                loopDataReadOffset += blockSize;
            }
            
        }
        
        
        //make that mono signal stereo!
        FloatVectorOperations::copy (outBufferRight, outBufferLeft, blockSize);

        // visualize new input
        avcIn.pushBuffer(bufferToFill);
        
        if (currOdfSample < validOdfData.get())
            avcOdf.pushSample(odfData.getReadPointer(0, ++currOdfSample), 1);
        
        if (resetPressed) doReset = true;
    }
    
    void releaseResources() override
    {
        // This will be called when the audio device stops, or when it is being
        // restarted due to a setting change.
        
        // For more details, see the help for AudioProcessor::releaseResources()
    }
    
    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (globalLaF.ClBackground);
        g.setFont(16.0f);
        g.setColour(Colours::white);
        g.drawText(ProjectInfo::versionString, getLocalBounds(), Justification::bottomRight);
        // You can add your drawing code here!
    }
    
    void resized() override
    {
        // This is called when the MainContentComponent is resized.
        // If you add any child components, this is where you should
        // update their positions.
        Rectangle<int> area = getBounds();
        area.removeFromLeft(30);
        area.removeFromRight(30);
        area.removeFromBottom(20);

        
        Rectangle<int> titleArea = area.removeFromTop(100);
        title.setBounds (titleArea);
        titleArea.reduce(10, 10);
        kugLogo.setBounds(titleArea.removeFromLeft(80));
        iemLogo.setBounds(titleArea.removeFromRight(80));
        
        area.removeFromTop(10);
        
        Rectangle<int> controlArea = area.removeFromRight(200);
        settingsButton.setBounds(controlArea.removeFromTop(30).removeFromLeft(200));
        controlArea.removeFromTop(20);
        
        Rectangle<int> row = controlArea.removeFromTop(25);
        tbListen.setBounds(row.removeFromLeft(50));
        row.removeFromLeft(5);
        lbListen.setBounds(row);
        
        controlArea.removeFromTop(10); //spacing
        
        row = controlArea.removeFromTop(25);
        tbDrums.setBounds(row.removeFromLeft(50));
        row.removeFromLeft(5);
        lbDrums.setBounds(row);
        
        controlArea.removeFromTop(10); //spacing
        row = controlArea.removeFromTop(25);
        cbBeatAlignmentGrid.setBounds(row.removeFromLeft(60));
        lbCbBeatAlignmentGrid.setBounds(row);
        
        
        resetButton.setBounds(controlArea.removeFromBottom(100));
        controlArea.removeFromBottom(20);
        midiButton.setBounds(controlArea.removeFromBottom(100));
 

 
        area.removeFromRight(20);
        
        avcIn.setBounds(area.removeFromTop(125));
        area.removeFromTop(20);
        avcOdf.setBounds(area.removeFromTop(125));
        area.removeFromTop(20);
        imgCmp.setBounds(area);

        
    }
    
    void handleIncomingMidiMessage (MidiInput *source, const MidiMessage &message) override {
        messageCollector.addMessageToQueue (message);
    }
    
    void buttonClicked(Button* button) override
    {
        
        if (button->getComponentID() == "reset")
        {
            reset();
        }
        else if (button->getComponentID() == "listen")
        {
            listenToInput = button->getToggleState();
        }
        else if (button->getComponentID() == "drums")
        {
            listenToDrums = button->getToggleState();
        }
        else if (button->getComponentID() == "settings")
        {
            showAudioSettingsDialog();
        }
    }
    void buttonStateChanged(Button* button) override
    {
        if (button->isDown()) {
            if (button->getComponentID() == "record" ){
                const double time = Time::getMillisecondCounterHiRes() * 0.001;
                MidiMessage m = MidiMessage::controllerEvent(1,midiCcRecord.get(),127);
                m.setTimeStamp(time);
                messageCollector.addMessageToQueue(m);
            }
        }
        //else if (button->isOver()) DBG("drüber");
        //else DBG("oben");
    }
    
    bool keyPressed(const KeyPress& key, Component* originatingComponent) override
    {
        //DBG(key.getKeyCode());
        
        if (key.getKeyCode() == 68) { // d
            tbDrums.triggerClick();
        }
        else if (key.getKeyCode() == 83) {
            showAudioSettingsDialog();
        }
        else if (key.getKeyCode() == 77) { // m
            tbListen.triggerClick();
        }
        return true;
    }
    
    void comboBoxChanged (ComboBox* comboBox) override
    {
        if (comboBox->getComponentID() == "bufferSize")
        {
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup(setup);
            
            setup.sampleRate = fs;
            setup.bufferSize = comboBox->getSelectedId();
            String ret = deviceManager.setAudioDeviceSetup(setup, true);
            if (ret.isEmpty()) {}
            else comboBox->setSelectedId(deviceManager.getCurrentAudioDevice()->getCurrentBufferSizeSamples());
        }
        else if (comboBox->getComponentID() == "ccRecord")
        {
            midiCcRecord = comboBox->getSelectedId()-1;
        }
        else if (comboBox->getComponentID() == "ccReset")
        {
            midiCcReset = comboBox->getSelectedId()-1;
        }
        else if (comboBox->getComponentID() == "gridSize")
        {
            beatAlignmentGridSize = comboBox->getSelectedId();
        }
    }
    
    void showAudioSettingsDialog()
    {
        DialogWindow::LaunchOptions o;
        
        auto totalInChannels  = 1;
        auto totalOutChannels = 2;
        
        
        
        o.content.setOwned (new SettingsComponent (*this, this, deviceManager,
                                                   totalInChannels,
                                                   totalInChannels,
                                                   totalOutChannels,
                                                   totalOutChannels,
                                                   midiCcRecord.get(),
                                                   midiCcReset.get()));
        o.content->setSize (500, 650);
        
        o.dialogTitle                   = TRANS("Audio/MIDI Settings");
        o.dialogBackgroundColour        = globalLaF.ClBackground;
        
        o.escapeKeyTriggersCloseButton  = true;
        o.useNativeTitleBar             = false;
        o.resizable                     = false;
        
        o.launchAsync();
    }
    
    void timerCallback() override
    {
        if (doReset) reset();
        if (stateHasChanged.get())
        {
            if (state == recording)
            {
                midiButton.setColour(TextButton::textColourOffId, Colours::red);
                midiButton.setColour(TextButton::textColourOnId, Colours::red);
            }
            else if (state == overdub)
            {
                midiButton.setColour(TextButton::textColourOffId, Colours::orange);
                midiButton.setColour(TextButton::textColourOnId, Colours::orange);
            }
            else {
                midiButton.setColour(TextButton::textColourOffId, Colours::white);
                midiButton.setColour(TextButton::textColourOnId, Colours::white);
            }
            stateHasChanged = false;
        }
        
        
        if (!optimalPathPainted)
        {
            bool wasLastSample = tgDataFinished.get();
            bool optimalPathReady = pathIndexValid.get();
            while (processedTgData < validTgData.get())
            {
                ++processedTgData;
                
                for (int i=0; i<nTaus; ++i)
                {
                    float data = abs(tgData(i,processedTgData));
                    int cidx;
                    if (isnan(data)) cidx = 0;
                    else
                    {
                    data *= 0.02f*63;
                     cidx = roundToInt(data);
                    if (cidx > 63) cidx = 63;
                    }
                    //DBG(tempogram(i,processedTgData).real());
                    imgTempogram.setPixelAt(processedTgData, i, Colour::fromFloatRGBA(parula[cidx][0], parula[cidx][1], parula[cidx][2], 1.0f)); //nicer with colourGradiant and lookupTable
                    
                    //imgTempogram.setPixelAt(processedTgData, i, Colours::white); //nicer with colourGradiant and lookupTable
                    //imgTempogram.multiplyAlphaAt(processedTgData, i, jmin(1.0f,data));
                }
                
                if (wasLastSample)
                {
                    //imgTempogram = imgTempogram.getClippedImage(Rectangle<int>(0,0,processedTgData,nTaus));
                    //imgCmp.setImage(imgTempogram);
                }
                
                imgCmp.repaint();
            }
            if (optimalPathReady)
            {
                for (int i = 0; i<= processedTgData; ++i)
                {
                    imgTempogram.setPixelAt(i, pathIndex[i], isnan(pathPhase[i+tgWinL_2]) ?  Colours::red : Colours::skyblue );
                }
                
                imgCmp.repaint();
                optimalPathPainted = true;
            }
        }
    }
public:
    Atomic<int> midiCcRecord;
    Atomic<int> midiCcReset;
    
private:
    //==============================================================================
    
    // system check
    bool blockFinished;
    
    // everything else
    systemState state;
    Atomic<bool> stateHasChanged;
    odfThread thrOdf;
    tempogramThread thrTg;
    Atomic<int> beatAlignmentGridSize;
    
    bool doReset;
    
    // drums
    AudioSampleBuffer bd, sn, hh;
    AudioSampleBuffer drums;
    Atomic<bool> drumsReady;
    
    // Loop Data
    AudioBuffer<float> loopData;
    int loopDataReadOffset;
    int loopDataWriteOffset;
    int loopDataMaxOffset;
    Atomic<int> startCue, stopCue;
    int overHeadCount;
    Atomic<int> validLoopData;
    Atomic<bool> loopDataFinished;
    Atomic<bool> listenToInput;
    Atomic<bool> listenToDrums;
    
    AudioBuffer<float> prePlayData;
    int prePlayWriteOffset;
    int prePlayMaxOffset;
    
    bool fadesNotAddedYet;
    
    // ODF Data
    AudioBuffer<float> odfData; // maybe change it to juce array?
    Array<int> odfTm;
    Atomic<int> validOdfData;
    Atomic<bool> odfDataFinished;
    int currOdfSample;
    
    // Tempogram Data
    Eigen::MatrixXcf tgData;
    Atomic<int> validTgData;
    Atomic<bool> tgDataFinished;
    int pathIndex[odfFs*maxLoopTimeInSec];
    float pathPhase[odfFs*maxLoopTimeInSec];
    float pathMag[odfFs*maxLoopTimeInSec];
    Atomic<bool> pathIndexValid;
    
    // MIDI
    MidiBuffer incomingMidi;
    MidiMessageCollector messageCollector;
    
    // Audio param
    int blockSize;
    double sampleRate;
    
    // GUI stuff
    LaF globalLaF;
    
    TitleBar title;
    KugLogo kugLogo;
    IemLogo iemLogo;
    
    AudioVisualiserComponent avcIn;
    OneSidedAudioVisualiserComponent avcOdf;
    
    ComboBox cbBeatAlignmentGrid;
    simpleLabel lbCbBeatAlignmentGrid;
    
    ToggleButton tbListen;
    ToggleButton tbDrums;
    simpleLabel lbListen;
    simpleLabel lbDrums;
    
    TextButton midiButton;
    TextButton resetButton;
    TextButton settingsButton;
    ImageComponent imgCmp;
    Image imgTempogram;
    int processedTgData;
    bool optimalPathPainted;
    Rectangle<int> tgBounds;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }
