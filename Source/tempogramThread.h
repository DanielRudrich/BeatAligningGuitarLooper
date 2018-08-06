/*
 ==============================================================================
 
 tempogramThread.h
 Created: 23 Aug 2017 6:00:57pm
 Author:  Daniel Rudrich
 
 ==============================================================================
 */

#pragma once


#define tgWinL 150 //is 1.5s
#define tgWinL_2 75
#define discardLevel 0.1f
#define negInf -INFINITY

#include <complex>
#include <cmath>

class tempogramThread    : public Thread
{
public:
    tempogramThread()
    : Thread ("tempogramThread"), taus(nTaus), phaseStep(nTaus), prevData(nTaus), currData(nTaus),
                        prevAngle(nTaus), currAngle(nTaus), dftWeights(nTaus,tgWinL),
                        D(nTaus,odfFs*maxLoopTimeInSec),idxMat(nTaus,odfFs*maxLoopTimeInSec),
                        tempoDiff(nTaus), tauDiff(nTaus), candidates(nTaus)
    {
        dsp::WindowingFunction<float>::fillWindowingTables(hannWindow, tgWinL, dsp::WindowingFunction<float>::WindowingMethod::hann);

        { // TAU CALCULATION
            float nOct = log2f(tauMax/tauMin);
            float factor = exp2f(nOct/nTaus);
            
            taus[0] = tauMin;
            phaseStep(0) = 2.0f * M_PI / odfFs / taus[0];
            //DBG(phaseStep[0]);
            for (int i=1; i<nTaus; ++i)
            {
                taus[i] = taus[i-1]*factor;
                phaseStep(i) = 2.0f * M_PI / odfFs / taus[i];
                //DBG(phaseStep[i]);
            }
        }

        using namespace std::complex_literals;
        for (int i = 0; i<nTaus; ++i)
        {
            for (int j = 0; j<tgWinL; ++j)
            {
                dftWeights(i,j) = std::exp(-1i* std::complex<double>(j*phaseStep(i),0.0f)) * std::complex<double>(hannWindow[j],0.0);
                
            }
        }
    }
    
    ~tempogramThread()
    {
        // allow the thread 2 seconds to stop cleanly - should be plenty of time.
        stopThread (5000);
    }
    
    
    void initialise(AudioBuffer<float>* newOdfDataPtr, Atomic<int>* newValidOdfDataPtr, Array<int>* newOdfTmPtr, Atomic<bool>* newOdfDataFinishedPtr,
                    Eigen::MatrixXcf* newTgDataPtr, Atomic<int>* newValidTgDataPtr, Atomic<bool>* newTgDataFinishedPtr,
                    int* newPathIndexPtr, Atomic<bool>* newPathIndexValidPtr, float* newPathPhasePtr, float* newPathMagPtr,
                    AudioBuffer<float>* newDrumsPtr, Atomic<bool>* newDrumsReadyPtr,
                    AudioSampleBuffer* newBdPtr,AudioSampleBuffer* newSnPtr,AudioSampleBuffer* newHhPtr,
                    Atomic<int>* newStartCuePtr, Atomic<int>* newStopCuePtr,
                    Atomic<int>* newGridSizePtr)
    {
        
        odfDataPtr = newOdfDataPtr;
        validOdfDataPtr = newValidOdfDataPtr;
        odfTmPtr = newOdfTmPtr;
        odfDataFinishedPtr = newOdfDataFinishedPtr;
        
        tgDataPtr = newTgDataPtr;
        validTgDataPtr = newValidTgDataPtr;
        tgDataFinishedPtr = newTgDataFinishedPtr;
        
        pathIndexPtr = newPathIndexPtr;
        pathIndexValidPtr = newPathIndexValidPtr;
        pathPhasePtr = newPathPhasePtr;
        pathMagPtr = newPathMagPtr;
        
        drumsPtr = newDrumsPtr;
        drumsReadyPtr = newDrumsReadyPtr;
        bdPtr = newBdPtr;
        snPtr = newSnPtr;
        hhPtr = newHhPtr;
        
        startCuePtr = newStartCuePtr;
        stopCuePtr = newStopCuePtr;
        
        gridSizePtr = newGridSizePtr;
        
        startSample = 0;
        beatPos.resize(500); //should be plenty.. otherwise there might be some reallocations...
        beatPos.clearQuick();
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
                bool wasLastSample = odfDataFinishedPtr->get();
                int validOdfDataOffset = validOdfDataPtr->get();
                while (validOdfDataOffset - startSample >= tgWinL)
                {
                    // ========== TEMPOGRAM ===========
                    const float* readPtr = odfDataPtr->getReadPointer(0,startSample);
                    Eigen::VectorXf inpVec(tgWinL);
                    FloatVectorOperations::copy(inpVec.data(), readPtr, tgWinL);
                    currData = dftWeights * inpVec;

                    currAngle = Eigen::arg(currData);
                    prevAngle = currAngle - prevAngle; //as from now prevAngle is a temporary working variable
                    prevAngle -= phaseStep;
                    prevAngle = prevAngle.unaryExpr([](const float x) { return powf((1.0f - fabs(fmod(x/M_PI + 5.0f,2.0f) - 1.0f)),100.0f); });
                    
                    //DBG("max: " << FloatVectorOperations::findMaximum(prevAngle.data(),nTaus) << "min: " << FloatVectorOperations::findMinimum(prevAngle.data(),nTaus));
  
                    currData = currData * prevAngle; // apply enhancement

                    tgDataPtr->block<nTaus,1>(0,startSample) = currData;
                    *validTgDataPtr = startSample;
                    
                    prevAngle = currAngle;
                    
                    
                    
                    // ============ TEMPOPATH ============
                    if (startSample < 1)
                    {
                        D.block<nTaus,1>(0,0) = abs(currData); // initialise D matrix
                    }
                    else //start dynamic programming
                    {
                        // thats the basis gain
                        D.block<nTaus,1>(0,startSample) = abs(currData);
                        //DBG("max value in M: " << abs(currData).maxCoeff());
                        
                        // here comes the penalty
                        for (int i=0; i<nTaus; ++i)
                        {
                            tauDiff = taus(i)-taus;
                            tauDiff = abs(tauDiff);
//                            tauDiff = taus*(1/taus(i));
//                            tauDiff = log(tauDiff);
//                            tauDiff = abs(tauDiff);
//                            tauDiff = exp(tauDiff);
//                            tauDiff = tauDiff-1;
                            
                            tempoDiff = 1/taus;
                            tempoDiff = 1/taus(i) - tempoDiff;
                            tempoDiff = abs(tempoDiff);
                            tempoDiff *= theta;
                            
                            
                            Eigen::Index maxIdx;
                            candidates = D.block<nTaus,1>(0,startSample-1);
                            candidates = candidates - tempoDiff.matrix();
                            
                            //tempoMask: tauDiff should be smaller maxTauDiff
                            for (int tauIdx = 0; tauIdx<nTaus; ++tauIdx)
                            {
                                if (tauDiff(tauIdx) > maxTauDiff) candidates(tauIdx) = negInf;
                            }
                            
                            D(i,startSample) += candidates.maxCoeff(&maxIdx);
                            idxMat(i,startSample) = (int) maxIdx;
                        }
                    }
                    
                    // ==== updating startSample ======
                    ++startSample; //so many samples have been processed
                }
                if (wasLastSample)
                {
                    *tgDataFinishedPtr = true;
                    DBG("DP finished. Now backtracing..");
                    
                    Eigen::Index maxIdx;
                    D.block<nTaus,1>(0,startSample-1).maxCoeff(&maxIdx);
                    *(pathIndexPtr+startSample-1) = (int) maxIdx;
                    *(pathPhasePtr + startSample-1 + tgWinL_2) = std::arg(tgDataPtr->operator()(maxIdx, startSample-1))
                                                                + tgWinL_2 * phaseStep(maxIdx);
                    *(pathMagPtr + startSample-1 + tgWinL_2) = std::abs(tgDataPtr->operator()(maxIdx, startSample-1));
                    
                    for (int i = startSample-2; i>=0; --i) //todo: optimize with decrement temporary pointers!
                    {
                        int idx = idxMat(*(pathIndexPtr+i+1),i+1);
                        *(pathIndexPtr + i) = idx;
                        *(pathPhasePtr + i + tgWinL_2) = std::arg(tgDataPtr->operator()(idx, i)) + tgWinL_2 * phaseStep(idx);
                        *(pathMagPtr + i + tgWinL_2) = std::abs(tgDataPtr->operator()(idx, i));
                    }
                    
                    // magnitude normalisation
                    float maxMag = FloatVectorOperations::findMaximum(pathMagPtr+tgWinL_2, startSample);
                    FloatVectorOperations::multiply(pathMagPtr+tgWinL_2, 1/maxMag, startSample);
                    
                    //discarding unreliable phase information and calculating mean tau
                    int count = 0;
                    float tauAccum = 0.0f;
                    float* phPtr = pathPhasePtr + tgWinL_2;
                    float* magPtr = pathMagPtr + tgWinL_2;
                    
                    int firstValidIndex = -1;
                    
                    for (int i=0; i<startSample; ++i) {
                        if (*magPtr++ < discardLevel)
                        {
                            *phPtr++ = NAN;
                        }
                        else
                        {
                            if (firstValidIndex == -1) firstValidIndex = i;
                            tauAccum += taus(*(pathIndexPtr+i));
                            //DBG(taus(*(pathIndexPtr+i)));
                            ++count;
                            ++phPtr;
                        }
                    }
                    
                    *pathIndexValidPtr = true; // from now on path can be painted in gui
                    meanTau = tauAccum / count;
                    DBG("meanTau: " << meanTau*1000 << "ms");
                    
                    // ================ PHASE EXTRAPOLATION
//                    float step = phaseStep(*pathIndexPtr);
//                    for (int i = 0; i<tgWinL_2; ++i)
//                    {
//                        *(pathPhasePtr + tgWinL_2 - i - 1) = *(pathPhasePtr + tgWinL_2 - i) - step;
//                    }
//                    step = phaseStep(*(pathIndexPtr+startSample-1));
//                    for (int i = startSample-1+tgWinL_2; i<validOdfDataOffset; ++i)
//                    {
//                        *(pathPhasePtr + i) = *(pathPhasePtr + i - 1) + step;
//                    }
                    
                    
                    float step = phaseStep(pathIndexPtr[firstValidIndex]);
                    for (int i = -firstValidIndex; i<tgWinL_2; ++i)
                    {
                        *(pathPhasePtr + tgWinL_2 - i - 1) = *(pathPhasePtr + tgWinL_2 - i) - step;
                    }
                    step = phaseStep(*(pathIndexPtr+startSample-1));
                    for (int i = startSample-1+tgWinL_2; i<validOdfDataOffset; ++i)
                    {
                        *(pathPhasePtr + i) = *(pathPhasePtr + i - 1) + step;
                    }
                    
                    
                    
                    
                    
                    //================= ZERO CROSSING AND BEAT PLACEMENT
                    float prevV = pathPhasePtr[0];
                    bool prevIsNeg = false;
                    
                    if (!isnan(prevV)) {
                        prevV = sinf(prevV);
                        prevIsNeg = signbit(prevV);
                    }

                    for (int i=1; i<validOdfDataOffset; ++i) {
                        
                        float currV = pathPhasePtr[i];
                        if (!isnan(currV))
                        {
                            bool currIsNeg;
                            currV = sinf(currV);
                            currIsNeg = signbit(currV);
                            
                            if (!isnan(prevV) && prevIsNeg && !currIsNeg) //interpolate
                            {
                                float delta = - prevV / (currV-prevV);
                                int V = roundToInt(odfTmPtr->getUnchecked(i-1) + delta * fftHopsize);
                                beatPos.add(V);
                            }
                            
                            prevV = currV;
                            prevIsNeg = currIsNeg;
                        }
                        else {
                            prevV = NAN;
                            //++i;
                        }
                    }
                    
                    // ================ INTERPOLATE BEATS IN GAPS ===
                    int meanDiff = roundToInt(fs*meanTau);
                    int maxDiff = roundToInt(meanDiff*1.8f);
                    DBG("letztes: " << beatPos.getLast());
                    int initialSizeOfBeats = beatPos.size();
                    for (int i=1; i<initialSizeOfBeats; ++i)
                    {
                        int diff = beatPos.getUnchecked(i) - beatPos.getUnchecked(i-1);
                        if (diff>maxDiff) // do some interpolation
                        {
                            int nBeatsToAddPlusOne = roundToInt(((float)diff)/meanDiff);
                            DBG("flat: " <<((float)diff)/meanDiff << "beats to insert+1:" << nBeatsToAddPlusOne);
                            float step = diff/nBeatsToAddPlusOne;
                            for (int nBeat=1; nBeat<nBeatsToAddPlusOne; ++nBeat)
                            {
                                beatPos.add(roundToInt(beatPos.getUnchecked(i-1) + nBeat*step));
                            }
                        }
                    }
                    
                    for (int i=0; i<beatPos.size(); ++i)
                    {
                        DBG(beatPos.getUnchecked(i));
                    }
                    beatPos.sort();
                    
                    // ====== ALIGN START AND STOP CUES  ===
                    int startC = startCuePtr->get();
                    int stopC = stopCuePtr->get();
                    int firstBeat = 0;
                    int lastBeat = beatPos.size()-1;
                    
                    int prevDist = 1234567; //dummy value
                    for (int i=0; i<=lastBeat; ++i)
                    {
                        int curDist = abs(beatPos.getUnchecked(i) - startC);
                        if (curDist > prevDist) {
                            *startCuePtr = beatPos.getUnchecked(i-1);
                            firstBeat = i-1;
                            DBG("NEW START-CUE: " << startCuePtr->get());
                            break;
                        }
                        prevDist = curDist;
                    }
                    
                    int gridSize = gridSizePtr->get();
                    lastBeat = lastBeat - (lastBeat-firstBeat)%gridSize;
                    DBG("grid size: " << gridSize << " - firstBeatInd: " << firstBeat << " - lastBeat init: " << lastBeat);
                    prevDist = 1234567; //dummy value
                    for (int i=beatPos.size()-1; i>=0; --i)
                    {
                        if ((i-firstBeat)%gridSize == 0) {
                            DBG("i: " << i << " - i-firstbeat: " << i-firstBeat);
                            
                            int curDist = abs(beatPos.getUnchecked(i) - stopC);
                            if (curDist > prevDist) {
                                lastBeat = i+gridSize;
                                *stopCuePtr = beatPos.getUnchecked(lastBeat);
                                DBG("stop cue index: " << lastBeat);
                                DBG("NEW STOP-CUE: " << stopCuePtr->get());
                                break;
                            }
                            prevDist = curDist;
                        }
                    }
                    
                    DBG("adding drums");
                    //generate Drums
                    int drumCount = 1;
                    for (int i=firstBeat; i<lastBeat; ++i)
                    {
                        if (drumCount > 8) drumCount = 1;
                        
                        switch (drumCount) {
                            case 1:
                                drumsPtr->addFrom(0, beatPos.getUnchecked(i), *bdPtr, 0, 0, 32768);
                                drumsPtr->addFrom(0, beatPos.getUnchecked(i), *hhPtr, 0, 0, 32768);
                                break;
                            case 3:
                                drumsPtr->addFrom(0, beatPos.getUnchecked(i), *hhPtr, 0, 0, 32768);
                                break;
                            case 5:
                                drumsPtr->addFrom(0, beatPos.getUnchecked(i), *snPtr, 0, 0, 32768);
                                drumsPtr->addFrom(0, beatPos.getUnchecked(i), *hhPtr, 0, 0, 32768);
                                break;
                            case 7:
                                drumsPtr->addFrom(0, beatPos.getUnchecked(i), *hhPtr, 0, 0, 32768);
                                break;
                        }
                        //DBG(beatPos.getUnchecked(i));
                        
                        
                        ++drumCount;
                    }
                    
                    
                    *drumsReadyPtr = true;
                    
                    DBG("TG/TP finished! Stopping thread.");
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
    
    // taus
    Eigen::ArrayXf taus;
    Eigen::ArrayXf phaseStep;

    
    float hannWindow[tgWinL];
    
    Atomic<int> *gridSizePtr = nullptr;
    
    Eigen::VectorXcf prevData;
    Eigen::ArrayXcf currData;
    Eigen::ArrayXf prevAngle, currAngle;
    Eigen::MatrixXcf dftWeights;
    
    Atomic<int>* startCuePtr = nullptr;
    Atomic<int>* stopCuePtr = nullptr;
    
    // Drums
    AudioBuffer<float>* drumsPtr = nullptr;
    Atomic<bool>* drumsReadyPtr = nullptr;
    AudioSampleBuffer* bdPtr = nullptr;
    AudioSampleBuffer* snPtr = nullptr;
    AudioSampleBuffer* hhPtr = nullptr;
    
    // ODF Data
    AudioBuffer<float> *odfDataPtr = nullptr;
    Atomic<int> *validOdfDataPtr = nullptr;
    Array<int> *odfTmPtr = nullptr;
    Atomic<bool> *odfDataFinishedPtr = nullptr;
    int startSample = 0;
    
    // Tempogram Data
    Eigen::MatrixXcf* tgDataPtr = nullptr;
    Atomic<int> *validTgDataPtr = nullptr;
    Atomic<bool> *tgDataFinishedPtr = nullptr;
    
    //tempoPath Data
    Eigen::MatrixXf D;
    Eigen::MatrixXi idxMat;
    Eigen::ArrayXf tempoDiff;
    Eigen::ArrayXf tauDiff;
    Eigen::VectorXf candidates;
    int *pathIndexPtr = nullptr;
    Atomic<bool> *pathIndexValidPtr = nullptr;
    float *pathPhasePtr = nullptr;
    float *pathMagPtr = nullptr;
    float meanTau;
    
    Array<int> beatPos;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (tempogramThread)
};
