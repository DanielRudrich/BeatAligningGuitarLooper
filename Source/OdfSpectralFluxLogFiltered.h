/*
 ==============================================================================
 
 OdfSpectralFluxLogFiltered.h
 Created: 17 Oct 2017 10:17:07am
 Author:  Daniel Rudrich
 
 ==============================================================================
 */


#pragma once

#define preWm 0.997f
#define preWnf 0.6f
#define lambda 2
#define fftSize 2048
#define fftPowerOf2 11
#define hopSize 480

#include "semiFilter.h"

typedef Eigen::Triplet<double> T;

class OdfSpectralFluxLogFiltered
{
public:
    OdfSpectralFluxLogFiltered()
    : fft(fftPowerOf2), currentFFTData(nFFTBins), currentFilteredData(nSemitones), previousFilteredData(nSemitones), semitoneFilterMatrix(nSemitones,nFFTBins)
    {
        // create hann window
        dsp::WindowingFunction<float>::fillWindowingTables(hannWindow, fftSize, dsp::WindowingFunction<float>::WindowingMethod::hann);
        
        // create sparse semitome filter matrix
        std::vector<T> coefficients;
        coefficients.reserve(nSemitoneFilterValues);
        for (int i = 0; i < nSemitoneFilterValues; ++i)
            coefficients.push_back(T(semiFilterI[i]-1,semiFilterJ[i]-1,semiFilterV[i]));
        
        semitoneFilterMatrix.setFromTriplets(coefficients.begin(), coefficients.end());
        
        // reset state
        reset();
    }
    
    ~OdfSpectralFluxLogFiltered() {}
    
    
    void reset()
    {
        FloatVectorOperations::fill(currentFFTData.data(), 0.0f, nFFTBins);
        FloatVectorOperations::fill(preWP, 0.0f, nFFTBins);
        FloatVectorOperations::fill(currentFilteredData.data(), 1.0f, nSemitones);
        
        notYetUsedAudioDataCount = 0;
        processedFrames = 0;
        //startSample = 0;
        //odfSample = 0;
    }
    
    int processData(float* destination,  const float* source, int nSamples)
    {
        const int initialNotYetUsedAudioDataCount = notYetUsedAudioDataCount;
        int nowProcessedFrames = 0;
        int notYetUsedAudioDataOffset = 0;
        int usedSamples = 0;
        
        
        while (notYetUsedAudioDataCount > 0 && notYetUsedAudioDataCount + nSamples >= fftSize)
        {
            // copy not yet used data into fftInOut buffer (with hann windowing)
            FloatVectorOperations::multiply(fftInOut,
                                            notYetUsedAudioData + notYetUsedAudioDataOffset, hannWindow, notYetUsedAudioDataCount);
            
            // fill up fftInOut buffer with new data (with hann windowing)
            FloatVectorOperations::multiply(fftInOut + notYetUsedAudioDataCount,
                                            source, hannWindow + notYetUsedAudioDataCount, fftSize - notYetUsedAudioDataCount);
            
            // process frame in buffer, store ODF value in destination and increase count
            *(destination + nowProcessedFrames++) = processFrameInBuffer();
            
            notYetUsedAudioDataOffset += hopSize;
            notYetUsedAudioDataCount -= hopSize;
        }
        
        if (notYetUsedAudioDataCount > 0) // not enough new input samples to use all of the previous data
        {
            FloatVectorOperations::copy(notYetUsedAudioData, notYetUsedAudioData + initialNotYetUsedAudioDataCount - notYetUsedAudioDataCount, notYetUsedAudioDataCount);
            FloatVectorOperations::copy(notYetUsedAudioData + notYetUsedAudioDataCount, source + usedSamples, nSamples);
            notYetUsedAudioDataCount += nSamples;
        }
        else  // all of the previous data used
        {
            int dataOffset = - notYetUsedAudioDataCount;
            
            while (nSamples - dataOffset >= fftSize)
            {
                FloatVectorOperations::multiply(fftInOut, source + dataOffset, hannWindow, fftSize);
                *(destination + nowProcessedFrames++) = processFrameInBuffer();
                
                dataOffset += hopSize;
            }
            
            int remainingSamples = nSamples - dataOffset;
            if (remainingSamples > 0)
            {
                FloatVectorOperations::copy(notYetUsedAudioData, source + dataOffset, nSamples-dataOffset);
            }
            notYetUsedAudioDataCount = remainingSamples;
        }
        
        processedFrames += nowProcessedFrames;
        return nowProcessedFrames;
    }
    
private:
    float processFrameInBuffer() {
        //store previous data
        previousFilteredData = currentFilteredData;
        
        // perform fast Fourier transform
        fft.performFrequencyOnlyForwardTransform(fftInOut);
        
        FloatVectorOperations::copy(currentFFTData.data(), fftInOut + firstFFTBin, nFFTBins);
        
        // perform spectral whitening
        for (int i=0; i<nFFTBins; ++i)
        {
            preWP[i] = jmax(currentFFTData[i], preWm * preWP[i],preWnf);
            currentFFTData[i] /= preWP[i];
        }
        
        // perform filtering in frequency domain
        currentFilteredData = semitoneFilterMatrix*currentFFTData;
        
        // log(lambda * X_filt + 1)
        currentFilteredData *= lambda;
        FloatVectorOperations::add(currentFilteredData.data(), 1.0f, nSemitones);
        for (int i=0; i<nSemitones; ++i)
        {
            currentFilteredData[i] = log(currentFilteredData[i]);
        }
        
        // calculate spectral difference
        float spectralDifference[nSemitones];
        FloatVectorOperations::subtract(spectralDifference, currentFilteredData.data(), previousFilteredData.data(), nSemitones);
        FloatVectorOperations::clip(spectralDifference, spectralDifference, 0.0f, 10000.0f, nSemitones); //heaviside
        
        // sum all clipped spectral differences
        float value = 0.0f;
        
        for (int i=0; i<nSemitones; ++i)
            value += spectralDifference[i];
        
        return 0.1f * value; //TODO remove scaling
    }
    
    
private:
    dsp::FFT fft;
    
    float hannWindow[fftSize];
    float fftInOut[fftSize*2];
    float preWP[nFFTBins];
    
    float notYetUsedAudioData[fftSize-1];
    int notYetUsedAudioDataCount;
    
    int processedFrames;
    
    Eigen::VectorXf currentFFTData;
    Eigen::VectorXf currentFilteredData, previousFilteredData;
    Eigen::SparseMatrix<float> semitoneFilterMatrix;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OdfSpectralFluxLogFiltered)
};
