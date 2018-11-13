#include "stdafx.h"
#include "OpenGL.hpp"
#include "Texture.hpp"
#include "Image.hpp"
#include <Graphics/ResourceManagers.hpp>

#ifdef __APPLE__
#include "libpng16/png.h"
#else
#include "png.h"
#endif

namespace Graphics
{
	class Texture_Impl : public TextureRes
	{
		uint32 m_texture = 0;
		TextureWrap m_wmode[2] = { TextureWrap::Repeat };
		TextureFormat m_format = TextureFormat::Invalid;
		Vector2i m_size;
		bool m_filter = true;
		bool m_mipFilter = true;
		bool m_mipmaps = false;
		float m_anisotropic = 1.0f;
		void* m_data = nullptr;
		OpenGL* m_gl = nullptr;

	public:
		Texture_Impl(OpenGL* gl)
		{
			m_gl = gl;
		}
		~Texture_Impl()
		{
			if(m_texture)
				glDeleteTextures(1, &m_texture);
		}
		bool Init()
		{
			glGenTextures(1, &m_texture);
			if(m_texture == 0)
				return false;
			return true;
		}
		virtual void Init(Vector2i size, TextureFormat format)
		{
			m_format = format;
			m_size = size;

			uint32 ifmt = -1;
			uint32 fmt = -1;
			uint32 type = -1;
			if(format == TextureFormat::D32)
			{
				ifmt = GL_DEPTH_COMPONENT32;
				fmt = GL_DEPTH_COMPONENT;
				type = GL_FLOAT;
			}
			else if(format == TextureFormat::RGBA8)
			{
				ifmt = GL_RGBA8;
				fmt = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			else
			{
				assert(false);
			}

			glBindTexture(GL_TEXTURE_2D, m_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, ifmt, size.x, size.y, 0, fmt, type, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);

			UpdateFilterState();
			UpdateWrap();
		}
		virtual void SetFromFrameBuffer(Vector2i pos = { 0, 0 })
		{
			m_gl->BlitFramebuffer();
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glReadBuffer(GL_BACK);
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pos.x, pos.y, m_size.x, m_size.y);
			GLenum err;
			bool errored = false;
			while ((err = glGetError()) != GL_NO_ERROR)
			{			
				Logf("OpenGL Error: 0x%p", Logger::Severity::Error, err);
				errored = true;
			}
			//assert(!errored);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		virtual void DumpToFile(String file)
		{
			Buffer data;
			png_image image;
			data.resize(m_size.x * m_size.y * 4);
			png_byte* dataPtr = data.data();
			int pitch = m_size.x * 4;
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataPtr);
			FILE* f = std::fopen(*file, "wb");
			///TODO: Move elsewhere
			png_structp png_ptr = NULL;
			png_infop info_ptr = NULL;
			png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			info_ptr = png_create_info_struct(png_ptr);
			if (setjmp(png_jmpbuf(png_ptr)))
			{
				/* If we get here, we had a problem writing the file */
				fclose(f);
				png_destroy_write_struct(&png_ptr, &info_ptr);
				assert(false);
			}
			png_set_IHDR(png_ptr, info_ptr, m_size.x, m_size.y,
				8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

			png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * m_size.y);
			for (size_t i = 0; i < m_size.y; ++i) {
				row_pointers[m_size.y - i - 1] = dataPtr + i*pitch;
			}

			png_init_io(png_ptr, f);
			png_set_rows(png_ptr, info_ptr, row_pointers);
			png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
			std::fclose(f);
			free(row_pointers);
		}

		virtual void SetData(Vector2i size, void* pData)
		{
			m_format = TextureFormat::RGBA8;
			m_size = size;
			m_data = pData;
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_data);
			glBindTexture(GL_TEXTURE_2D, 0);

			UpdateFilterState();
			UpdateWrap();
		}
		void UpdateFilterState()
		{
			glBindTexture(GL_TEXTURE_2D, m_texture);
			if(!m_mipmaps)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_filter ? GL_LINEAR : GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_filter ? GL_LINEAR : GL_NEAREST);
			}
			else
			{
				if(m_mipFilter)
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_filter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR);
				else
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_filter ? GL_LINEAR : GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_filter ? GL_LINEAR : GL_NEAREST);
			}
			if(GL_TEXTURE_MAX_ANISOTROPY_EXT)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_anisotropic);
			}
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		virtual void SetFilter(bool enabled, bool mipFiltering, float anisotropic)
		{
			m_mipFilter = mipFiltering;
			m_filter = enabled;
			m_anisotropic = anisotropic;
			assert(m_anisotropic >= 1.0f && m_anisotropic <= 16.0f);
			UpdateFilterState();
		}
		virtual void SetMipmaps(bool enabled)
		{
			if(enabled)
			{
				glBindTexture(GL_TEXTURE_2D, m_texture);
				glGenerateMipmap(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			m_mipmaps = enabled;
			UpdateFilterState();
		}
		virtual const Vector2i& GetSize() const
		{
			return m_size;
		}
		virtual void Bind(uint32 index)
		{
			glActiveTexture(GL_TEXTURE0 + index);
			glBindTexture(GL_TEXTURE_2D, m_texture);

		}
		virtual uint32 Handle()
		{
			return m_texture;
		}

		virtual void SetWrap(TextureWrap u, TextureWrap v) override
		{
			m_wmode[0] = u;
			m_wmode[1] = v;
			UpdateWrap();
		}

		void UpdateWrap()
		{
			uint32 wmode[] = {
				GL_REPEAT,
				GL_MIRRORED_REPEAT,
				GL_CLAMP_TO_EDGE,
			};

			glBindTexture(GL_TEXTURE_2D, m_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wmode[(size_t)m_wmode[0]]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wmode[(size_t)m_wmode[1]]);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		TextureFormat GetFormat() const
		{
			return m_format;
		}
	};

	Texture TextureRes::Create(OpenGL* gl)
	{
		Texture_Impl* pImpl = new Texture_Impl(gl);
		if(pImpl->Init())
		{
			return GetResourceManager<ResourceType::Texture>().Register(pImpl);
		}
		else
		{
			delete pImpl; pImpl = nullptr;
		}
		return Texture();
	}
	Texture TextureRes::Create(OpenGL* gl, Image image)
	{
		if(!image)
			return Texture();
		Texture_Impl* pImpl = new Texture_Impl(gl);
		if(pImpl->Init())
		{
			pImpl->SetData(image->GetSize(), image->GetBits());
			return GetResourceManager<ResourceType::Texture>().Register(pImpl);
		}
		else
		{
			delete pImpl;
			return Texture();
		}
	}

	Texture TextureRes::CreateFromFrameBuffer(class OpenGL* gl, const Vector2i& resolution)
	{
		Texture_Impl* pImpl = new Texture_Impl(gl);
		if (pImpl->Init())
		{
			pImpl->Init(resolution, TextureFormat::RGBA8);
			pImpl->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
			pImpl->SetFromFrameBuffer();
			return GetResourceManager<ResourceType::Texture>().Register(pImpl);
		}
		else
		{
			delete pImpl; pImpl = nullptr;
		}
		return Texture();
	}

	float TextureRes::CalculateHeight(float width)
	{
		Vector2 size = GetSize();
		float aspect = size.y / size.x;
		return aspect * width;
	}

	float TextureRes::CalculateWidth(float height)
	{
		Vector2 size = GetSize();
		float aspect = size.x / size.y;
		return aspect * height;
	}



}
