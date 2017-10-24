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

	void m_decode_ms_adpcm(const Buffer& encoded)
	{
		int16* pcm = (int16*)m_pcm.data();
		for (size_t b = 0; b < (encoded.size() / m_format.nBlockAlign); b++)
		{
			int8* src = ((int8*)encoded.data()) + (b * m_format.nBlockAlign);
			int8 blockPredictors[] = { 0, 0 };
			int32 ideltas[] = { 0, 0 };
			int32 sample1[] = { 0, 0 };
			int32 sample2[] = { 0, 0 };

			blockPredictors[0] = *src++;
			blockPredictors[1] = *src++;
			assert(blockPredictors[0] >= 0 && blockPredictors[0] < 7);
			assert(blockPredictors[1] >= 0 && blockPredictors[0] < 7);

			int16* src_16 = (int16*)src;
			ideltas[0] = *src_16++;
			ideltas[1] = *src_16++;

			sample1[0] = *src_16++;
			sample1[1] = *src_16++;

			sample2[0] = *src_16++;
			sample2[1] = *src_16++;

			*pcm++ = sample2[0];
			*pcm++ = sample2[1];
			*pcm++ = sample1[0];
			*pcm++ = sample1[1];


			src = (int8*)src_16;

			int AdaptationTable[] = {
				230, 230, 230, 230, 307, 409, 512, 614,
				768, 614, 512, 409, 307, 230, 230, 230
			};
			int AdaptCoeff1[] = { 256, 512, 0, 192, 240, 460, 392 };
			int AdaptCoeff2[] = { 0, -256, 0, 64, 0, -208, -232 };

			// Decode the rest of the data in the block
			int remainingInBlock = m_format.nBlockAlign - 14;
			while (remainingInBlock > 0)
			{
				int8 nibbleData = *src++;

				int8 nibbles[] = { 0, 0 };
				nibbles[0] = nibbleData >> 4;
				nibbles[0] &= 0x0F;
				nibbles[1] = nibbleData & 0x0F;

				int16 predictors[] = { 0, 0 };
				for (size_t i = 0; i < 2; i++)
				{


					predictors[i] = ((sample1[i] * AdaptCoeff1[blockPredictors[i]]) + (sample2[i] * AdaptCoeff2[blockPredictors[i]])) / 256;
					if (nibbles[i] & 0x08)
						predictors[i] += (nibbles[i] - 0x10) * ideltas[i];
					else
						predictors[i] += nibbles[i] * ideltas[i];
					*pcm++ = predictors[i];
					sample2[i] = sample1[i];
					sample1[i] = predictors[i];
					ideltas[i] = (AdaptationTable[nibbles[i]] * ideltas[i]) / 256;
					if (ideltas[i] < 16)
						ideltas[i] = 16;
				}
				remainingInBlock--;
			}
		}
	}


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
				String format = "Unknown";
				if (m_format.nFormat == 1)
					format = "PCM";
				else if (m_format.nFormat == 2)
					format = "MS_ADPCM";

				Logf("Sample format: %s", Logger::Info, format);
				Logf("Channels: %d", Logger::Info, m_format.nChannels);
				Logf("Sample rate: %d", Logger::Info, m_format.nSampleRate);
				Logf("Bps: %d", Logger::Info, m_format.nBitsPerSample);
				if (m_format.nFormat == 2)
				{
					uint16 cbSize;
					m_memoryReader << cbSize;
					m_memoryReader.Skip(cbSize);
				}
			}
			else if (chunkHdr == "fact")
			{
				uint32 fh;
				m_memoryReader << fh;
				m_samplesTotal = fh;
			}
			else if (chunkHdr == "data") // data Chunk
			{
				// validate header
				if (m_format.nFormat != 1 && m_format.nFormat != 2)
					return false;
				if (m_format.nChannels > 2 || m_format.nChannels == 0)
					return false;
				if (m_format.nBitsPerSample != 16 && m_format.nBitsPerSample != 4)
					return false;

				// Read data
				if (m_format.nFormat == 1)
				{
					m_samplesTotal = chunkHdr.nLength / sizeof(short);
					m_pcm.resize(chunkHdr.nLength);
					m_memoryReader.Serialize(m_pcm.data(), chunkHdr.nLength);
				}
				else if (m_format.nFormat == 2)
				{
					Buffer encoded;
					encoded.resize(chunkHdr.nLength);
					m_memoryReader.Serialize(encoded.data(), chunkHdr.nLength);
					m_samplesTotal -= m_samplesTotal % m_format.nBlockAlign;
					m_samplesTotal += m_format.nBlockAlign;
					m_pcm.resize(m_samplesTotal * m_format.nChannels * sizeof(short));
					// TODO: dont decode it all on load.
					m_decode_ms_adpcm(encoded);
				}
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
		return m_playbackPointer / m_format.nChannels;
	}
	virtual int32 GetStreamRate_Internal()
	{
		return m_format.nSampleRate;
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
				if ((m_playbackPointer / m_format.nChannels) >= m_samplesTotal)
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
