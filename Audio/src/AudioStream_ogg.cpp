#include "stdafx.h"
#include "AudioStreamBase.hpp"
#include <vorbis/vorbisfile.h>

class AudioStreamOGG_Impl : public AudioStreamBase
{
	OggVorbis_File m_ovf = { 0 };
	ov_callbacks callbacks =
	{
		(decltype(ov_callbacks::read_func))&AudioStreamOGG_Impl::m_Read,
		(decltype(ov_callbacks::seek_func))&AudioStreamOGG_Impl::m_Seek,
		nullptr, // Close
		(decltype(ov_callbacks::tell_func))&AudioStreamOGG_Impl::m_Tell,
	};

	vorbis_info m_info;
	Vector<float> m_pcm;
	int64 m_playPos;

public:
	~AudioStreamOGG_Impl()
	{
		Deregister();
	}
	bool Init(Audio* audio, const String& path, bool preload)
	{
		if(!AudioStreamBase::Init(audio, path, preload))
			return false;

		int32 r = ov_open_callbacks(this, &m_ovf, 0, 0, callbacks);
		if(r != 0)
		{
			Logf("ov_open_callbacks failed with code %d", Logger::Error, r);
			return false;
		}

		vorbis_comment* comments = ov_comment(&m_ovf, 0);
		vorbis_info* infoPtr = ov_info(&m_ovf, 0);
		if(!infoPtr)
			return false;
		m_info = *infoPtr;
		m_samplesTotal = ov_pcm_total(&m_ovf, 0);

		if (preload)
		{
			float** readBuffer;
			int r;
			int totalRead = 0;
			while (true)
			{
				r = ov_read_float(&m_ovf, &readBuffer, 1024, 0);
				if (r < 0)
					continue;
				if (r == 0)
					break;
				else
				{
					if (m_info.channels == 2)
					{
						for (size_t i = 0; i < r; i++)
						{
							m_pcm.Add(readBuffer[0][i]);
							m_pcm.Add(readBuffer[1][i]);
						}
					}
					else if (m_info.channels == 1)
					{
						for (size_t i = 0; i < r; i++)
						{
							m_pcm.Add(readBuffer[0][i]);
							m_pcm.Add(readBuffer[0][i]);
						}
					}
					totalRead += r;
				}
			}
			m_samplesTotal = totalRead;
			ov_clear(&m_ovf);
			m_playPos = 0;
		}

		InitSampling(m_info.rate);

		return true;
	}

	virtual void SetPosition_Internal(int32 pos)
	{
		if (m_preloaded)
		{
			if(pos < 0)
				m_playPos = 0;
			else
				m_playPos = pos;

			return;
		}
		ov_pcm_seek(&m_ovf, pos);
	}
	virtual int32 GetStreamPosition_Internal()
	{
		if (m_preloaded)
			return m_playPos;

		return (int32)ov_pcm_tell(&m_ovf);
	}
	virtual int32 GetStreamRate_Internal()
	{
		return (int32)m_info.rate;
	}

	float* GetPCM_Internal()
	{
		if (m_preloaded)
			return &m_pcm.front();

		return nullptr;
	}
	uint32 GetSampleRate_Internal() const
	{
		return m_info.rate;
	}

	virtual int32 DecodeData_Internal()
	{
		if (m_preloaded)
		{
			uint32 samplesPerRead = 128;
			for (size_t i = 0; i < samplesPerRead; i++)
			{
				if (m_playPos < 0)
				{
					m_readBuffer[0][i] = 0;
					m_readBuffer[1][i] = 0;
					m_playPos++;
					continue;
				}
				else if (m_playPos >= m_samplesTotal)
				{
					m_currentBufferSize = m_bufferSize;
					m_remainingBufferData = m_bufferSize;
					return i;
				}
				m_readBuffer[0][i] = m_pcm[m_playPos * 2];
				m_readBuffer[1][i] = m_pcm[m_playPos * 2 + 1];
				m_playPos++;
			}
			m_currentBufferSize = samplesPerRead;
			m_remainingBufferData = samplesPerRead;
			return samplesPerRead;
		}


		float** readBuffer;
		int32 r = ov_read_float(&m_ovf, &readBuffer, m_bufferSize, 0);
		if(r > 0)
		{
			if(m_info.channels == 1)
			{
				// Copy mono to read buffer
				for(int32 i = 0; i < r; i++)
				{
					m_readBuffer[0][i] = readBuffer[0][i];
					m_readBuffer[1][i] = readBuffer[0][i];
				}
			}
			else
			{
				// Copy data to read buffer
				for(int32 i = 0; i < r; i++)
				{
					m_readBuffer[0][i] = readBuffer[0][i];
					m_readBuffer[1][i] = readBuffer[1][i];
				}
			}
			m_currentBufferSize = r;
			m_remainingBufferData = r;
			return r;
		}
		else if(r == 0)
		{
			// EOF
			m_playing = false;
			return -1;
		}
		else
		{
			// Error
			m_playing = false;
			Logf("Ogg Stream error %d", Logger::Warning, r);
			return -1;
		}
	}

private:
	static size_t m_Read(void* ptr, size_t size, size_t nmemb, AudioStreamOGG_Impl* self)
	{
		return self->Reader().Serialize(ptr, nmemb*size);
	}
	static int m_Seek(AudioStreamOGG_Impl* self, int64 offset, int whence)
	{
		if(whence == SEEK_SET)
			self->Reader().Seek((size_t)offset);
		else if(whence == SEEK_CUR)
			self->Reader().Skip((size_t)offset);
		else if(whence == SEEK_END)
			self->Reader().SeekReverse((size_t)offset);
		else
			assert(false);
		return 0;
	}
	static long m_Tell(AudioStreamOGG_Impl* self)
	{
		return (long)self->Reader().Tell();
	}
};

class AudioStreamRes* CreateAudioStream_ogg(class Audio* audio, const String& path, bool preload)
{
	AudioStreamOGG_Impl* impl = new AudioStreamOGG_Impl();
	if(!impl->Init(audio, path, preload))
	{
		delete impl;
		impl = nullptr;
	}
	return impl;
}
