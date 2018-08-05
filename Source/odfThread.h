/*
 ==============================================================================
 
 odfThread.h
 Created: 23 Aug 2017 9:58:11am
 Author:  Daniel Rudrich
 
 ==============================================================================
 */
#pragma once

#define startBin (2)
#define nBins 698
#define preWm 0.997f
#define preWnf 0.6f
#define nSemi 50
#define lambda 2
#include "semiFilter.h"

typedef Eigen::Triplet<double> T;

class odfThread    : public Thread
{
public:
    odfThread()
    : Thread ("odfThread"), currData(698), currFiltered(nSemi), prevFiltered(nSemi), odfFFT(fftPowerOf2), semiFilter(50,698)
    {
        //dsp::WindowingFunction<float> winFct(fftSize,dsp::WindowingFunction<float>::WindowingMethod::hann);
        dsp::WindowingFunction<float>::fillWindowingTables(hannWindow, fftSize, dsp::WindowingFunction<float>::WindowingMethod::hann);
        
        FloatVectorOperations::fill(currData.data(), 0.0f, nBins);
        FloatVectorOperations::fill(preWP, 0.0f, nBins);
        FloatVectorOperations::fill(currFiltered.data(), 1.0f, nSemi);
        
        //create semiFilter Matrix
        
        std::vector<T> coefficients;
        coefficients.reserve(semiFilterNumValues);
        for (int i = 0; i < semiFilterNumValues; ++i)
        {
            coefficients.push_back(T(semiFilterI[i]-1,semiFilterJ[i]-1,semiFilterV[i]));
        }
        
        //DBG(semiFilter.rows() << "x" << semiFilter.cols());
        semiFilter.setFromTriplets(coefficients.begin(), coefficients.end());
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
                    FloatVectorOperations::multiply(fftInOut, loopDataPtr+startSample, hannWindow, fftSize);
                    odfFFT.performFrequencyOnlyForwardTransform(fftInOut);
                    
                    FloatVectorOperations::copy(currData.data(), fftInOut + startBin, nBins);
                    
                    for (int i=0; i<nBins; ++i)
                    {
                        preWP[i] = jmax(currData[i], preWm * preWP[i],preWnf);
                        currData[i] /= preWP[i];
                    }
                    
                    prevFiltered = currFiltered;
                    currFiltered = semiFilter*currData;
                    currFiltered *= lambda;
                    FloatVectorOperations::add(currFiltered.data(),1,nSemi);
                    
                    for (int i=0; i<nSemi; ++i)
                    {
                        currFiltered[i] = log(currFiltered[i]);
                    }
                    
                    FloatVectorOperations::subtract(temp, currFiltered.data(), prevFiltered.data(), nSemi);
                    FloatVectorOperations::clip(temp, temp, 0.0f, 10000.0f, nSemi); //heaviside
                    
                    float* wptr = odfDataPtr->getWritePointer(0);
                    wptr[odfSample] = 0.0f;
                    for (int i=0; i<nSemi; ++i)
                    {
                        wptr[odfSample] += temp[i];
                    }
                    wptr[odfSample] *= 0.1f;
                    odfTmPtr->setUnchecked(odfSample, fftSize / 2 + odfSample * fftHopsize);
                    *validOdfDataPtr = odfSample;
                    
                    ++odfSample;
                    startSample += fftHopsize;
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
    
        dsp::FFT odfFFT;
    
    
    float fftInOut[fftSize*2];
    //float currData[nBins]
    float preWP[nBins];
    float hannWindow[fftSize];
    
    float temp[nSemi];
    
    Eigen::VectorXf currData;
    Eigen::VectorXf currFiltered, prevFiltered;
    Eigen::SparseMatrix<float> semiFilter;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (odfThread)
};
