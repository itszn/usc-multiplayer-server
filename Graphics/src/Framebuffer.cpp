#include "stdafx.h"
#include "Framebuffer.hpp"
#include "Texture.hpp"
#include <Graphics/ResourceManagers.hpp>
#include "OpenGL.hpp"

namespace Graphics
{
	class Framebuffer_Impl : public FramebufferRes
	{
		OpenGL* m_gl;
		uint32 m_fb = 0;
		Vector2i m_textureSize;
		uint32 m_tex;
		bool m_isBound = false;
		bool m_depthAttachment = false;
		bool m_isMultisample = false;
		uint32 m_sampleCount = 0;

		friend class OpenGL;
	public:
		Framebuffer_Impl(OpenGL* gl) : m_gl(gl)
		{
		}
		~Framebuffer_Impl()
		{
			if(m_fb > 0)
				glDeleteFramebuffers(1, &m_fb);
			assert(!m_isBound);
		}
		bool Init()
		{
			glGenFramebuffers(1, &m_fb);
			return m_fb != 0;
		}

		bool InitMultisample(uint8 num_samples)
		{
			m_isMultisample = true;
			m_sampleCount = num_samples;
			glGenTextures(1, &m_tex);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_tex);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, num_samples, GL_RGBA8, 512, 512, false);

			glGenFramebuffers(1, &m_fb);
			glBindFramebuffer(GL_FRAMEBUFFER, m_fb);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_tex, 0);

			return IsComplete();
		}

		virtual bool AttachTexture(Texture tex)
		{
			if(!tex)
				return false;
			m_textureSize = tex->GetSize();
			uint32 texHandle = (uint32)tex->Handle();
			TextureFormat fmt = tex->GetFormat();
			if(fmt == TextureFormat::D32)
			{
				#ifdef __APPLE__
				glFramebufferTexture2D(m_fb, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texHandle, 0);
				#else
				glNamedFramebufferTexture2DEXT(m_fb, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texHandle, 0);
				#endif

				m_depthAttachment = true;
			}
			else
			{
				#ifdef __APPLE__
				glFramebufferTexture2D(m_fb, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texHandle, 0);
				#else
				glNamedFramebufferTexture2DEXT(m_fb, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texHandle, 0);
				#endif
			}

			return IsComplete();
		}
		virtual void Bind()
		{
			assert(!m_isBound);

			// Adjust viewport to frame buffer
			//glGetIntegerv(GL_VIEWPORT, &m_gl->m_lastViewport.pos.x);
			//glViewport(0, 0, m_textureSize.x, m_textureSize.y);
			glBindFramebuffer(GL_FRAMEBUFFER, m_fb);

			if(m_depthAttachment)
			{
				GLenum drawBuffers[2] =
				{
					GL_COLOR_ATTACHMENT0,
					GL_DEPTH_ATTACHMENT
				};
				glDrawBuffers(2, drawBuffers);
			}
			else
			{
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
			}

			m_isBound = true;
			//m_gl->m_boundFramebuffer = this;
		}
		virtual void Unbind()
		{
			assert(m_isBound && m_gl->m_boundFramebuffer == this);

			// Restore viewport
			//Recti& vp = m_gl->m_lastViewport;
			//glViewport(vp.pos.x, vp.pos.y, vp.size.x, vp.size.y);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDrawBuffer(GL_BACK);

			m_isBound = false;
			//m_gl->m_boundFramebuffer = nullptr;
		}
		virtual bool IsComplete() const
		{
			#ifdef __APPLE__
			int complete = glCheckFramebufferStatus(m_fb);
			#else
			int complete = glCheckNamedFramebufferStatus(m_fb, GL_DRAW_FRAMEBUFFER);
			#endif	

			return complete == GL_FRAMEBUFFER_COMPLETE;
		}
		virtual uint32 Handle() const
		{
			return m_fb;
		}

		virtual bool Resize(const Vector2i& size)
		{
			m_textureSize = size;

			if (!m_isMultisample)
				return true;
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_tex);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_sampleCount, GL_RGBA8, size.x, size.y, false);
			return true;
		}

		virtual void Display()
		{
			uint32 width = m_textureSize.x;
			uint32 height = m_textureSize.y;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fb);
			glDrawBuffer(GL_BACK);
			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fb);

		}
		virtual uint32 TextureHandle() const
		{
			return m_tex;
		}
	};

	Framebuffer FramebufferRes::Create(class OpenGL* gl)
	{
		Framebuffer_Impl* pImpl = new Framebuffer_Impl(gl);
		if(!pImpl->Init())
		{
			delete pImpl;
			return Framebuffer();
		}
		else
		{
			return GetResourceManager<ResourceType::Framebuffer>().Register(pImpl);
		}
	}

	Framebuffer FramebufferRes::CreateMultisample(class OpenGL* gl, uint8 num_samples)
	{
		Framebuffer_Impl* pImpl = new Framebuffer_Impl(gl);
		if (!pImpl->InitMultisample(num_samples))
		{
			delete pImpl;
			return Framebuffer();
		}
		else
		{
			return GetResourceManager<ResourceType::Framebuffer>().Register(pImpl);
		}
	}
}