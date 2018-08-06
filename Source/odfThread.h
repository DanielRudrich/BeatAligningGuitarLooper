/*
 ==============================================================================
 
 odfThread.h
 Created: 23 Aug 2017 9:58:11am
 Author:  Daniel Rudrich
 
 ==============================================================================
 */
#pragma once


typedef Eigen::Triplet<double> T;

class odfThread    : public Thread
{
public:
    odfThread()
    : Thread ("odfThread")
    {
    }
    
    ~odfThread()
    {
        // allow the thread 2 seconds to stop cleanly - should be plenty of time.
        stopThread (2000);
    }
    
    
    void initialise(const float* newLoopDataPtr, Atomic<int>* newValidLoopDataPtr, Atomic<bool>* newLoopDataFinishedPtr,
                    AudioBuffer<float>* newOdfDataPtr, Atomic<int>* newValidOdfDataPtr, Array<int>* newOdfTmPtr, Atomic<bool>* newOdfDataFinishedPtr)
    {
        loopDataPtr = newLoopDataPtr;
        validLoopDataPtr = newValidLoopDataPtr;
        loopDataFinishedPtr = newLoopDataFinishedPtr;
        
        odfDataPtr = newOdfDataPtr;
        validOdfDataPtr = newValidOdfDataPtr;
        odfTmPtr = newOdfTmPtr;
        odfDataFinishedPtr = newOdfDataFinishedPtr;
        
        startSample = 0;
        odfSample = 0;
        odf.reset();
        initialised = true;
    }
    
    void run() override
    {
        // threadShouldExit() returns true when the stopThread() method has been
        // called, so we should check it often, and exit as soon as it gets flagged.
        while (! threadShouldExit())
        {
            if (initialised)
            {
                bool wasLastSample = loopDataFinishedPtr->get();
                int validDataOffset = validLoopDataPtr->get();
                while (validDataOffset - startSample >= fftSize)
                {
                    float* wptr = odfDataPtr->getWritePointer(0);
                    //wptr[odfSample] = 0.0f;
                    int nNewOdfSamples = odf.processData(wptr+odfSample, loopDataPtr+startSample, validDataOffset-startSample);
                    DBG(nNewOdfSamples);
                    for (int i = 0; i < nNewOdfSamples; ++i) {
                        odfTmPtr->setUnchecked(odfSample, fftSize / 2 + odfSample * fftHopsize);
                        ++odfSample;
                    }
                    
                    //odfSample += nNewOdfSamples;
                    *validOdfDataPtr = odfSample - 1;
                    
                   // ++odfSample;
                    //startSample += fftHopsize;
                    startSample = validDataOffset;
                }
                if (wasLastSample) {
                    *odfDataFinishedPtr = true;
                    DBG("ODF finished! Stopping thread.");
                    signalThreadShouldExit();
                }
                else
                {
                    wait(8);
                }
                //DBG(validDataPtr->get());
            }
            else wait (50);
        }
    }
    
private:
    bool initialised = false;
    
    
    OdfSpectralFluxLogFiltered odf;
    
    
    // Loop Data
    const float *loopDataPtr = nullptr;
    Atomic<int> *validLoopDataPtr = nullptr;
    Atomic<bool> *loopDataFinishedPtr = nullptr;
    int startSample = 0;
    
    
    // ODF Data
    AudioBuffer<float> *odfDataPtr = nullptr;
    Atomic<int> *validOdfDataPtr = nullptr;
    Array<int> *odfTmPtr = nullptr;
    Atomic<bool> *odfDataFinishedPtr = nullptr;
    int odfSample = 0;
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (odfThread)
};
