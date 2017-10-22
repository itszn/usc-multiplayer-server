#include "stdafx.h"
#include "AudioStreamBase.hpp"

// Fixed point format for resampling
static uint64 fp_sampleStep = 1ull << 48;

struct WavHeader
{
	char id[4];
	uint32 nLength;

	bool operator ==(const char* rhs) const
	{
		return strncmp(id, rhs, 4) == 0;
	}
	bool operator !=(const char* rhs) const
	{
		return !(*this == rhs);
	}
};

struct WavFormat
{
	uint16 nFormat;
	uint16 nChannels;
	uint32 nSampleRate;
	uint32 nByteRate;
	uint16 nBlockAlign;
	uint16 nBitsPerSample;
};

class AudioStreamWAV_Impl : public AudioStreamBase
{
private:
	Buffer m_pcm;
	WavFormat m_format = { 0 };


	// Resampling values
	uint64 m_sampleStep = 0;
	uint64 m_sampleStepIncrement = 0;

	uint64 m_playbackPointer = 0;

public:
	~AudioStreamWAV_Impl()
	{
		Deregister();
	}

	bool Init(Audio* audio, const String& path, bool preload)
	{
		if (!AudioStreamBase::Init(audio, path, true)) // Always preload for now
			return false;

		WavHeader riff;
		m_memoryReader << riff;
		if (riff != "RIFF")
			return false;

		char riffType[4];
		m_memoryReader.SerializeObject(riffType);
		if (strncmp(riffType, "WAVE", 4) != 0)
			return false;

		while (m_memoryReader.Tell() < m_memoryReader.GetSize())
		{
			WavHeader chunkHdr;
			m_memoryReader << chunkHdr;
			if (chunkHdr == "fmt ")
			{
				m_memoryReader << m_format;
				//Logf("Sample format: %s", Logger::Info, (m_format.nFormat == 1) ? "PCM" : "Unknown");
				//Logf("Channels: %d", Logger::Info, m_format.nChannels);
				//Logf("Sample rate: %d", Logger::Info, m_format.nSampleRate);
				//Logf("Bps: %d", Logger::Info, m_format.nBitsPerSample);
			}
			else if (chunkHdr == "data") // data Chunk
			{
				// validate header
				if (m_format.nFormat != 1)
					return false;
				if (m_format.nChannels > 2 || m_format.nChannels == 0)
					return false;
				if (m_format.nBitsPerSample != 16)
					return false;

				// Read data
				m_samplesTotal = chunkHdr.nLength / sizeof(short);
				m_pcm.resize(chunkHdr.nLength);
				m_memoryReader.Serialize(m_pcm.data(), chunkHdr.nLength);
			}
			else
			{
				m_memoryReader.Skip(chunkHdr.nLength);
			}
		}
		InitSampling(m_audio->GetSampleRate());

		// Calculate the sample step if the rate is not the same as the output rate
		double sampleStep = (double)m_format.nSampleRate / (double)m_audio->GetSampleRate();
		m_sampleStepIncrement = (uint64)(sampleStep * (double)fp_sampleStep);
		m_playbackPointer = 0;
		return true;
	}
	virtual int32 GetStreamPosition_Internal()
	{
		return m_playbackPointer;
	}
	virtual int32 GetStreamRate_Internal()
	{
		return m_format.nSampleRate * m_format.nChannels;
	}
	virtual void SetPosition_Internal(int32 pos)
	{
		m_playbackPointer = pos;
	}

	virtual int32 DecodeData_Internal()
	{
		uint32 samplesPerRead = 128;

		if (m_format.nChannels == 2)
		{
			for (uint32 i = 0; i < samplesPerRead; i++)
			{
				if (m_playbackPointer >= m_samplesTotal)
				{
					return i;
				}

				int16* src = ((int16*)m_pcm.data()) + m_playbackPointer;
				m_readBuffer[0][i] = (float)src[0] / (float)0x7FFF;
				m_readBuffer[1][i] = (float)src[1] / (float)0x7FFF;

				// Increment source sample with resampling
				m_sampleStep += m_sampleStepIncrement;
				while (m_sampleStep >= fp_sampleStep)
				{
					m_playbackPointer += 2;
					m_sampleStep -= fp_sampleStep;
				}
			}
		}
		else
		{
			// Mix mono sample
			for (uint32 i = 0; i < samplesPerRead; i++)
			{
				if (m_playbackPointer >= m_samplesTotal)
				{
					return i;
				}

				int16* src = ((int16*)m_pcm.data()) + m_playbackPointer;
				m_readBuffer[0][i] = (float)src[0] / (float)0x7FFF;
				m_readBuffer[1][i] = (float)src[0] / (float)0x7FFF;

				// Increment source sample with resampling
				m_sampleStep += m_sampleStepIncrement;
				while (m_sampleStep >= fp_sampleStep)
				{
					m_playbackPointer += 1;
					m_sampleStep -= fp_sampleStep;
				}
			}
		}
		m_currentBufferSize = samplesPerRead;
		m_remainingBufferData = samplesPerRead;
		return samplesPerRead;
	}
};

class AudioStreamRes* CreateAudioStream_wav(class Audio* audio, const String& path, bool preload)
{
	AudioStreamWAV_Impl* impl = new AudioStreamWAV_Impl();
	if (!impl->Init(audio, path, preload))
	{
		delete impl;
		impl = nullptr;
	}
	return impl;
}
