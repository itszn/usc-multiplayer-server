#include "stdafx.h"
#include "Sample.hpp"
#include "Audio_Impl.hpp"
#include "Audio.hpp"

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

BinaryStream& operator <<(BinaryStream& stream, WavHeader& hdr)
{
	stream.SerializeObject(hdr.id);
	stream << hdr.nLength;
	return stream;
}
BinaryStream& operator <<(BinaryStream& stream, WavFormat& fmt)
{
	stream.SerializeObject(fmt);
	return stream;
}

class Sample_Impl : public SampleRes
{
public:
	Audio* m_audio;
	Buffer m_pcm;
	WavFormat m_format = { 0 };
	uint32 m_samplesTotal;

	uint32 m_decode_ms_adpcm(const Buffer& encoded, int16* pcm, uint64 pos)
	{
		int8* src = ((int8*)encoded.data()) + pos;
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
		uint32 decodedCount = 2;

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
			decodedCount++;
			remainingInBlock--;
		}
		return decodedCount;
	}


	mutex m_lock;

	// Resampling values
	uint64 m_sampleStep = 0;
	uint64 m_sampleStepIncrement = 0;

	uint64 m_playbackPointer = 0;
	uint64 m_length = 0;
	bool m_playing = false;
	bool m_looping = false;

public:
	~Sample_Impl()
	{
		Deregister();
	}
	virtual void Play(bool looping) override
	{
		m_lock.lock();
		m_playing = true;
		m_playbackPointer = 0;
		m_looping = looping;
		m_lock.unlock();	
	}
	virtual void Stop() override
	{
		m_lock.lock();
		m_playing = false;
		m_looping = false;
		m_lock.unlock();
	}
	bool Init(const String& path)
	{
		File file;
		if(!file.OpenRead(path))
			return false;
		FileReader stream = FileReader(file);

		WavHeader riff;
		stream << riff;
		if(riff != "RIFF")
			return false;

		char riffType[4];
		stream.SerializeObject(riffType);
		if(strncmp(riffType, "WAVE", 4) != 0)
			return false;

		while(stream.Tell() < stream.GetSize())
		{
			WavHeader chunkHdr;
			stream << chunkHdr;
			if(chunkHdr == "fmt ")
			{
				stream << m_format;
				//Logf("Sample format: %s", Logger::Info, (m_format.nFormat == 1) ? "PCM" : "Unknown");
				//Logf("Channels: %d", Logger::Info, m_format.nChannels);
				//Logf("Sample rate: %d", Logger::Info, m_format.nSampleRate);
				//Logf("Bps: %d", Logger::Info, m_format.nBitsPerSample);

				// In case there's some extra data at the end that we dont use.
				if (m_format.nFormat == 2)
				{
					uint16 cbSize;
					stream << cbSize;
					stream.Skip(cbSize);
				}
				else
				stream.Skip(chunkHdr.nLength - sizeof(WavFormat));
			}
			else if (chunkHdr == "fact")
			{
				uint32 fh;
				stream << fh;
				m_samplesTotal = fh;
			}
			else if(chunkHdr == "data") // data Chunk
			{
				// validate header
				if(m_format.nFormat != 1 && m_format.nFormat != 2)
					return false;
				if(m_format.nChannels > 2 || m_format.nChannels == 0)
					return false;
				if(m_format.nFormat == 1 && m_format.nBitsPerSample != 16)
					return false;
				if (m_format.nFormat == 2 && m_format.nBitsPerSample != 4)
					return false;

				if (m_format.nFormat == 1)
				{
					// Read data
					m_length = chunkHdr.nLength / sizeof(short);
					m_pcm.resize(chunkHdr.nLength);
					stream.Serialize(m_pcm.data(), chunkHdr.nLength);
				}
				else if (m_format.nFormat == 2)
				{
					m_length = chunkHdr.nLength;
					Buffer encoded(m_length * m_format.nChannels * sizeof(short));
					stream.Serialize(encoded.data(), chunkHdr.nLength);
					m_pcm.resize(m_length * m_format.nChannels * sizeof(short));
					int16* pcmptr = (int16*)m_pcm.data();
					int pos = 0;
					int totalDecoded = 0;
					while (pos <= m_length)
					{
						int decoded = m_decode_ms_adpcm(encoded, pcmptr, pos);
						pcmptr += 2 * decoded;
						totalDecoded += decoded;
						pos += m_format.nBlockAlign;
					}
					m_length = totalDecoded * 2;
				}
			}
			else
			{
				stream.Skip(chunkHdr.nLength);
			}
		}

		// Calculate the sample step if the rate is not the same as the output rate
		double sampleStep = (double)m_format.nSampleRate / (double)m_audio->GetSampleRate();
		m_sampleStepIncrement = (uint64)(sampleStep * (double)fp_sampleStep);

		return true;
	}
	virtual void Process(float* out, uint32 numSamples) override
	{
		if(!m_playing)
			return;

		m_lock.lock();
		if(m_format.nChannels == 2)
		{
			// Mix stereo sample
			for(uint32 i = 0; i < numSamples; i++)
			{
				if(m_playbackPointer >= m_length)
				{
					if(m_looping)
					{
						m_playbackPointer = 0;
					}
					else
					{
						// Playback ended
						m_playing = false;
						break;
					}
				}

				int16* src = ((int16*)m_pcm.data()) + m_playbackPointer;
				out[i * 2] = (float)src[0] / (float)0x7FFF;
				out[i * 2 + 1] = (float)src[1] / (float)0x7FFF;

				// Increment source sample with resampling
				m_sampleStep += m_sampleStepIncrement;
				while(m_sampleStep >= fp_sampleStep)
				{
					m_playbackPointer += 2;
					m_sampleStep -= fp_sampleStep;
				}
			}
		}
		else 
		{
			// Mix mono sample
			for(uint32 i = 0; i < numSamples; i++)
			{
				if(m_playbackPointer >= m_length)
				{
					if(m_looping)
					{
						m_playbackPointer = 0;
					}
					else
					{
						// Playback ended
						m_playing = false;
						break;
					}
				}

				int16* src = ((int16*)m_pcm.data()) + m_playbackPointer;
				out[i * 2] = (float)src[0] / (float)0x7FFF;
				out[i * 2 + 1] = (float)src[0] / (float)0x7FFF;

				// Increment source sample with resampling
				m_sampleStep += m_sampleStepIncrement;
				while(m_sampleStep >= fp_sampleStep)
				{
					m_playbackPointer += 1;
					m_sampleStep -= fp_sampleStep;
				}
			}
		}
		m_lock.unlock();
	}
	const Buffer& GetData() const
	{
		return m_pcm;
	}
	uint32 GetBitsPerSample() const
	{
		return m_format.nBitsPerSample;
	}
	uint32 GetNumChannels() const
	{
		return m_format.nChannels;
	}
	int32 GetPosition() const
	{
		return 0;
	}
	float* GetPCM()
	{
		return nullptr;
	}
	uint32 GetSampleRate() const
	{
		return m_format.nSampleRate;
	}

};

Sample SampleRes::Create(Audio* audio, const String& path)
{
	Sample_Impl* res = new Sample_Impl();
	res->m_audio = audio;

	if(!res->Init(path))
	{
		delete res;
		return Sample();
	}

	audio->GetImpl()->Register(res);

	return Sample(res);
}