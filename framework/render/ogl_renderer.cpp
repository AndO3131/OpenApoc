#include "framework/renderer.h"
#include "framework/image.h"
#include "framework/palette.h"

#include "gl_3_0.cpp"

namespace OpenApoc {

namespace {

class Program
{
	public:
		GLuint prog;
		static GLuint CreateShader(GLenum type, const std::string source)
		{
			GLuint shader = gl::CreateShader(type);
			const GLchar *string = source.c_str();
			GLint stringLength = source.length();
			gl::ShaderSource(shader, 1, &string, &stringLength);
			gl::CompileShader(shader);
			GLint compileStatus;
			gl::GetShaderiv(shader, gl::COMPILE_STATUS, &compileStatus);
			if (compileStatus == gl::TRUE_)
				return shader;

			GLint logLength;
			gl::GetShaderiv(shader, gl::INFO_LOG_LENGTH, &logLength);

			std::unique_ptr<char[]> log(new char[logLength]);
			gl::GetShaderInfoLog(shader, logLength, NULL, log.get());

			std::cerr << "Shader compile error:\n\"" << std::string(log.get()) << "\"\n";

			gl::DeleteShader(shader);
			return 0;
		}
		Program(const std::string vertexSource, const std::string fragmentSource)
			: prog(0)
		{
			GLuint vShader = CreateShader(gl::VERTEX_SHADER, vertexSource);
			if (!vShader)
			{
				std::cerr << "Failed to compile vertex shader\n";
				return;
			}
			GLuint fShader = CreateShader(gl::FRAGMENT_SHADER, fragmentSource);
			if (!fShader)
			{
				std::cerr << "Failed to compile fragment shader\n";
				gl::DeleteShader(vShader);
				return;
			}

			prog = gl::CreateProgram();
			gl::AttachShader(prog, vShader);
			gl::AttachShader(prog, fShader);

			gl::DeleteShader(vShader);
			gl::DeleteShader(fShader);

			gl::LinkProgram(prog);

			GLint linkStatus;
			gl::GetProgramiv(prog, gl::LINK_STATUS, &linkStatus);
			if (linkStatus == gl::TRUE_)
				return;

			GLint logLength;
			gl::GetProgramiv(prog, gl::INFO_LOG_LENGTH, &logLength);

			std::unique_ptr<char[]> log(new char[logLength]);
			gl::GetProgramInfoLog(prog, logLength, NULL, log.get());

			std::cerr << "Program link error:\n\"" << std::string(log.get()) << "\"\n";

			gl::DeleteProgram(prog);
			prog = 0;
			return;

		}

		void Uniform(GLuint loc, Colour c)
		{
			gl::Uniform4f(loc, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
		}

		void Uniform(GLuint loc, Vec2<float> v)
		{
			gl::Uniform2f(loc, v.x, v.y);
		}
		void Uniform(GLuint loc, Vec2<int> v)
		{
			//FIXME: Float conversion
			gl::Uniform2f(loc, v.x, v.y);
		}
		void Uniform(GLuint loc, float v)
		{
			gl::Uniform1f(loc, v);
		}
		void Uniform(GLuint loc, int v)
		{
			gl::Uniform1i(loc, v);
		}

		void Uniform(GLuint loc, bool v)
		{
			gl::Uniform1f(loc, (v ? 1.0f : 0.0f));
		}

		virtual ~Program()
		{
			if (prog)
				gl::DeleteProgram(prog);
		};
};

class SpriteProgram : public Program
{
	protected:
		SpriteProgram(const std::string vertexSource, const std::string fragmentSource)
			: Program(vertexSource, fragmentSource)
			{
			}
	public:
		GLuint posLoc;
		GLuint sizeLoc;
		GLuint offsetLoc;
		GLuint screenSizeLoc;
		GLuint texLoc;
		GLuint flipYLoc;
};

class RGBProgram : public SpriteProgram
{
	private:
		constexpr static const char* vertexSource = {
                "#version 130\n"
                "in vec2 position;\n"
                "uniform vec2 size;\n"
                "uniform vec2 offset;\n"
				"out vec2 texcoord;\n"
                "uniform vec2 screenSize;\n"
				"uniform bool flipY;\n"
                "void main() {\n"
				"  texcoord = position;\n"
                "  vec2 tmpPos = position;\n"
                "  tmpPos *= size;\n"
                "  tmpPos += offset;\n"
                "  tmpPos /= screenSize;\n"
                "  tmpPos -= vec2(0.5,0.5);\n"
				"  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
				"  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
                "}\n"
		};
		constexpr static const char* fragmentSource  = {
                "#version 130\n"
                "in vec2 texcoord;\n"
                "uniform sampler2D tex;\n"
				"out vec4 out_colour;\n"
                "void main() {\n"
				" out_colour = texture2D(tex, texcoord);\n"
                "}\n"
		};
	public:
		RGBProgram()
			: SpriteProgram(vertexSource, fragmentSource)
			{
				this->posLoc = gl::GetAttribLocation(this->prog, "position");
				this->sizeLoc = gl::GetUniformLocation(this->prog, "size");
				this->offsetLoc = gl::GetUniformLocation(this->prog, "offset");
				this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
				this->texLoc = gl::GetUniformLocation(this->prog, "tex");
				this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
			}
		void setUniforms(Vec2<float> offset, Vec2<float> size, Vec2<int> screenSize, bool flipY, GLint texUnit = 0)
		{
			this->Uniform(this->offsetLoc, offset);
			this->Uniform(this->sizeLoc, size);
			this->Uniform(this->screenSizeLoc, screenSize);
			this->Uniform(this->texLoc, texUnit);
			this->Uniform(this->flipYLoc, flipY);
		}
};

class PaletteProgram : public SpriteProgram
{
	private:
		constexpr static const char* vertexSource = {
				"#version 130\n"
				"in vec2 position;\n"
				"uniform vec2 size;\n"
				"uniform vec2 offset;\n"
				"out vec2 texcoord;\n"
				"uniform vec2 screenSize;\n"
				"uniform bool flipY;\n"
				"void main() {\n"
				"  texcoord = position;\n"
				"  vec2 tmpPos = position;\n"
				"  tmpPos *= size;\n"
				"  tmpPos += offset;\n"
				"  tmpPos /= screenSize;\n"
				"  tmpPos -= vec2(0.5,0.5);\n"
				"  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
				"  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
				"}\n"
		};
		constexpr static const char* fragmentSource  = {
				"#version 130\n"
				"in vec2 texcoord;\n"
				"uniform sampler2D tex;\n"
				"uniform sampler2D pal;\n"
				"out vec4 out_colour;\n"
				"void main() {\n"
				" out_colour = texture2D(pal, vec2(texture2D(tex,texcoord).r,0));\n"
				"}\n"
		};
	public:
		GLuint palLoc;
		PaletteProgram()
			: SpriteProgram(vertexSource, fragmentSource)
			{
				this->posLoc = gl::GetAttribLocation(this->prog, "position");
				this->sizeLoc = gl::GetUniformLocation(this->prog, "size");
				this->offsetLoc = gl::GetUniformLocation(this->prog, "offset");
				this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
				this->texLoc = gl::GetUniformLocation(this->prog, "tex");
				this->palLoc = gl::GetUniformLocation(this->prog, "pal");
				this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
			}
		void setUniforms(Vec2<float> offset, Vec2<float> size, Vec2<int> screenSize, bool flipY, GLint texUnit = 0, GLint palUnit = 1)
		{
			this->Uniform(this->offsetLoc, offset);
			this->Uniform(this->sizeLoc, size);
			this->Uniform(this->screenSizeLoc, screenSize);
			this->Uniform(this->texLoc, texUnit);
			this->Uniform(this->palLoc, palUnit);
			this->Uniform(this->flipYLoc, flipY);
		}
};
class SolidColourProgram : public Program
{
	private:
		constexpr static const char* vertexSource = {
				"#version 130\n"
				"in vec2 position;\n"
				"uniform vec2 size;\n"
				"uniform vec2 offset;\n"
				"uniform vec2 screenSize;\n"
				"uniform bool flipY;\n"
				"void main() {\n"
				"  vec2 tmpPos = position;\n"
				"  tmpPos *= size;\n"
				"  tmpPos += offset;\n"
				"  tmpPos /= screenSize;\n"
				"  tmpPos -= vec2(0.5,0.5);\n"
				"  if (flipY) gl_Position = vec4((tmpPos.x*2), -(tmpPos.y*2),0,1);\n"
				"  else gl_Position = vec4((tmpPos.x*2), (tmpPos.y*2),0,1);\n"
				"}\n"
		};
		constexpr static const char* fragmentSource  = {
				"#version 130\n"
				"uniform vec4 colour;\n"
				"out vec4 out_colour;\n"
				"void main() {\n"
				" out_colour = colour;\n"
				"}\n"
		};
	public:
		GLuint posLoc;
		GLuint sizeLoc;
		GLuint offsetLoc;
		GLuint screenSizeLoc;
		GLuint colourLoc;
		GLuint flipYLoc;
		SolidColourProgram()
			: Program(vertexSource, fragmentSource)
			{
				this->posLoc = gl::GetAttribLocation(this->prog, "position");
				this->sizeLoc = gl::GetUniformLocation(this->prog, "size");
				this->offsetLoc = gl::GetUniformLocation(this->prog, "offset");
				this->screenSizeLoc = gl::GetUniformLocation(this->prog, "screenSize");
				this->colourLoc = gl::GetUniformLocation(this->prog, "colour");
				this->flipYLoc = gl::GetUniformLocation(this->prog, "flipY");
			}
		void setUniforms(Vec2<float> offset, Vec2<float> size, Vec2<int> screenSize, bool flipY, Colour colour)
		{
			this->Uniform(this->offsetLoc, offset);
			this->Uniform(this->sizeLoc, size);
			this->Uniform(this->screenSizeLoc, screenSize);
			this->Uniform(this->colourLoc, colour);
			this->Uniform(this->flipYLoc, flipY);
		}
};

class IdentityQuad
{
public:
	static void draw(GLuint attribPos)
	{
		static const float vertices[] =
		{
			0.0f, 0.0f,
			0.0f, 1.0f,
			1.0f, 0.0f,
			1.0f, 1.0f
		};
		gl::EnableVertexAttribArray(attribPos);
		gl::VertexAttribPointer(attribPos, 2, gl::FLOAT, gl::FALSE_, 0, &vertices);
		gl::DrawArrays(gl::TRIANGLE_STRIP, 0, 4);
	}
};

class ActiveTexture
{
	ActiveTexture(const ActiveTexture &) = delete;
public:
	GLenum prevUnit;
	static GLenum getUnitEnum(int unit)
	{
		return gl::TEXTURE0 + unit;
	}

	ActiveTexture(int unit)
	{
		gl::GetIntegerv(gl::ACTIVE_TEXTURE, (GLint*)&prevUnit);
		gl::ActiveTexture(getUnitEnum(unit));
	}
	~ActiveTexture()
	{
		gl::ActiveTexture(prevUnit);
	}
};

class UnpackAlignment
{
	UnpackAlignment(const UnpackAlignment &) = delete;
public:
	GLint prevAlign;
	UnpackAlignment(int align)
	{
		gl::GetIntegerv(gl::UNPACK_ALIGNMENT, &prevAlign);
		gl::PixelStorei(gl::UNPACK_ALIGNMENT, align);
	}
	~UnpackAlignment()
	{
		gl::PixelStorei(gl::UNPACK_ALIGNMENT, prevAlign);
	}
};

class BindTexture
{
	BindTexture(const BindTexture &) = delete;
public:
	GLenum bind;
	GLuint prevID;
	int unit;
	static GLenum getBindEnum(GLenum e)
	{
		switch (e) {
			case gl::TEXTURE_1D: return gl::TEXTURE_BINDING_1D;
			case gl::TEXTURE_2D: return gl::TEXTURE_BINDING_2D;
			case gl::TEXTURE_3D: return gl::TEXTURE_BINDING_3D;
			default: assert(0);
		}
	}
	BindTexture(GLuint id, GLint unit = 0, GLenum bind = gl::TEXTURE_2D)
		: unit(unit), bind(bind)
	{
		ActiveTexture a(unit);
		gl::GetIntegerv(getBindEnum(bind), (GLint*)&prevID);
		gl::BindTexture(bind, id);
	}
	~BindTexture()
	{
		ActiveTexture a(unit);
		gl::BindTexture(bind, prevID);
	}
};

class BindFramebuffer
{
	BindFramebuffer(const BindFramebuffer &) = delete;
public:
	GLuint prevID;
	BindFramebuffer(GLuint id)
	{
		gl::GetIntegerv(gl::DRAW_FRAMEBUFFER_BINDING, (GLint*)&prevID);
		gl::BindFramebuffer(gl::DRAW_FRAMEBUFFER, id);

	}
	~BindFramebuffer()
	{
		gl::BindFramebuffer(gl::DRAW_FRAMEBUFFER, prevID);
	}
};

class FBOData : public RendererImageData
{
public:
	GLuint fbo;
	GLuint tex;
	Vec2<float> size;
	//Constructor /only/ to be used for default surface (FBO ID == 0)
	FBOData(GLuint fbo)
		//FIXME: Check FBO == 0
		//FIXME: Warn if trying to texture from FBO 0
		: fbo(fbo), tex(-1), size(0,0){}

	FBOData(Vec2<int> size)
		:size(size.x, size.y)
	{
		gl::GenTextures(1, &this->tex);
		BindTexture b(this->tex);
		gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA8, size.x, size.y, 0, gl::RGBA, gl::UNSIGNED_BYTE, NULL);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
		gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
		gl::GenFramebuffers(1, &this->fbo);
		BindFramebuffer f(this->fbo);

		gl::FramebufferTexture2D(gl::DRAW_FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D, this->tex, 0);
		assert(gl::CheckFramebufferStatus(gl::DRAW_FRAMEBUFFER) == gl::FRAMEBUFFER_COMPLETE);


	}
	virtual ~FBOData()
	{
		if (tex)
			gl::DeleteTextures(1, &tex);
		if (fbo)
			gl::DeleteFramebuffers(1, &fbo);
	}
};

class GLRGBImage : public RendererImageData
{
	public:
		GLuint texID;
		Vec2<float> size;
		std::weak_ptr<RGBImage> parent;
		GLRGBImage(std::shared_ptr<RGBImage> parent)
			: parent(parent), size(parent->size)
		{
			RGBImageLock l(parent, ImageLockUse::Read);
			gl::GenTextures(1, &this->texID);
			BindTexture b(this->texID);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
			gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, parent->size.x, parent->size.y, 0, gl::RGBA, gl::UNSIGNED_BYTE, l.getData());

		}
		virtual ~GLRGBImage()
		{
			gl::DeleteTextures(1, &this->texID);
		}
};

class GLPalette : public RendererImageData
{
	public:
		GLuint texID;
		Vec2<float> size;
		std::weak_ptr<Palette> parent;
		GLPalette(std::shared_ptr<Palette> parent)
			: parent(parent), size(Vec2<float>(parent->colours.size(), 1))
		{
			gl::GenTextures(1, &this->texID);
			BindTexture b(this->texID);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
			gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGBA, parent->colours.size(), 1, 0, gl::RGBA, gl::UNSIGNED_BYTE, parent->colours.data());

		}
		virtual ~GLPalette()
		{
			gl::DeleteTextures(1, &this->texID);
		}
};

class GLPaletteImage : public RendererImageData
{
	public:
		GLuint texID;
		Vec2<float> size;
		std::weak_ptr<PaletteImage> parent;
		GLPaletteImage(std::shared_ptr<PaletteImage> parent)
			: parent(parent), size(parent->size)
		{
			PaletteImageLock l(parent, ImageLockUse::Read);
			gl::GenTextures(1, &this->texID);
			BindTexture b(this->texID);
			UnpackAlignment align(1);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
			gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
			gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RED, parent->size.x, parent->size.y, 0, gl::RED, gl::UNSIGNED_BYTE, l.getData());

		}
		virtual ~GLPaletteImage()
		{
			gl::DeleteTextures(1, &this->texID);
		}
};
class OGL30Renderer : public Renderer
{
private:
	std::shared_ptr<RGBProgram> rgbProgram;
	std::shared_ptr<SolidColourProgram> colourProgram;
	std::shared_ptr<PaletteProgram> paletteProgram;
	GLuint currentBoundProgram;
	GLuint currentBoundFBO;

	std::shared_ptr<Surface> currentSurface;
	std::shared_ptr<Palette> currentPalette;

	friend class RendererSurfaceBinding;
	virtual void setSurface(std::shared_ptr<Surface> s)
	{
		this->flush();
		this->currentSurface = s;
		if (!s->rendererPrivateData)
			s->rendererPrivateData.reset(new FBOData(s->size));

		FBOData *fbo = static_cast<FBOData*>(s->rendererPrivateData.get());
		gl::BindFramebuffer(gl::FRAMEBUFFER, fbo->fbo);
		this->currentBoundFBO = fbo->fbo;
		gl::Viewport(0, 0, s->size.x, s->size.y);
	};
	virtual std::shared_ptr<Surface> getSurface()
	{
		return currentSurface;
	};
	std::shared_ptr<Surface> defaultSurface;
public:
	OGL30Renderer();
	virtual ~OGL30Renderer();
	virtual void clear(Colour c = Colour{0,0,0,0});
	virtual void setPalette(std::shared_ptr<Palette> p)
	{
		if (!p->rendererPrivateData)
			p->rendererPrivateData.reset(new GLPalette(p));
		this->currentPalette = p;
	}
	
	virtual void draw(std::shared_ptr<Image> i, Vec2<float> position);
	virtual void drawRotated(Image &i, Vec2<float> center, Vec2<float> position, float angle){};
	virtual void drawScaled(Image &i, Vec2<float> position, Vec2<float> size, Scaler scaler = Scaler::Linear){};
	virtual void drawTinted(Image &i, Vec2<float> position, Colour tint){};
	virtual void drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c);
	virtual void drawRect(Vec2<float> position, Vec2<float> size, Colour c, float thickness = 1.0){};
	virtual void drawLine(Vec2<float> p1, Vec2<float> p2, Colour c, float thickness = 1.0){};
	virtual void flush();
	virtual std::string getName();
	virtual std::shared_ptr<Surface>getDefaultSurface()
	{
		return this->defaultSurface;
	};

	void BindProgram(std::shared_ptr<Program> p)
	{
		if (this->currentBoundProgram == p->prog)
			return;
		gl::UseProgram(p->prog);
		this->currentBoundProgram = p->prog;
	}



	void DrawRGB(GLRGBImage &img, Vec2<float> offset)
	{
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(offset, img.size, this->currentSurface->size, flipY);
		BindTexture t(img.texID);
		IdentityQuad::draw(rgbProgram->posLoc);
	}
	
	void DrawPalette(GLPaletteImage &img, Vec2<float> offset)
	{
		BindProgram(paletteProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		paletteProgram->setUniforms(offset, img.size, this->currentSurface->size, flipY);
		BindTexture t(img.texID, 0);

		BindTexture p(static_cast<GLPalette*>(this->currentPalette->rendererPrivateData.get())->texID, 1);

		IdentityQuad::draw(paletteProgram->posLoc);
	}

	void DrawSurface(FBOData &fbo, Vec2<float> offset)
	{
		BindProgram(rgbProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		rgbProgram->setUniforms(offset, fbo.size, this->currentSurface->size, flipY);
		BindTexture t(fbo.tex);
		IdentityQuad::draw(rgbProgram->posLoc);
	}

	void DrawRect(Vec2<float> offset, Vec2<float> size, Colour c)
	{
		BindProgram(colourProgram);
		bool flipY = false;
		if (currentBoundFBO == 0)
			flipY = true;
		colourProgram->setUniforms(offset, size, this->currentSurface->size, flipY, c);
		IdentityQuad::draw(colourProgram->posLoc);
	}

};


OGL30Renderer::OGL30Renderer()
	: rgbProgram(new RGBProgram()), colourProgram(new SolidColourProgram()), paletteProgram(new PaletteProgram())
{
	GLint viewport[4];
	gl::GetIntegerv(gl::VIEWPORT, viewport);
	std::cerr << "Viewport {" << viewport[0] << "," << viewport[1] << "," << viewport[2] << "," << viewport[3] << "}\n";
	assert(viewport[0] == 0 && viewport[1] == 0);
	this->defaultSurface = std::make_shared<Surface>(Vec2<int>{viewport[2], viewport[3]});
	this->defaultSurface->rendererPrivateData.reset(new FBOData(0));
	this->currentSurface = this->defaultSurface;

	GLint maxTexArrayLayers;
	gl::GetIntegerv(gl::MAX_ARRAY_TEXTURE_LAYERS, &maxTexArrayLayers);
	std::cerr << "MAX_ARRAY_TEXTURE_LAYERS: \"" << maxTexArrayLayers << "\"\n";
	GLint maxTexUnits;
	gl::GetIntegerv(gl::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTexUnits);
	std::cerr << "MAX_COMBINED_TEXTURE_IMAGE_UNITS: \"" << maxTexUnits << "\"\n";
	gl::Enable(gl::BLEND);
	gl::BlendFunc(gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
}

OGL30Renderer::~OGL30Renderer()
{

}

void
OGL30Renderer::clear(Colour c)
{
	this->flush();
	gl::ClearColor(c.r/255.0f, c.g/255.0f, c.b/255.0f, c.a/255.0f);
	gl::Clear(gl::COLOR_BUFFER_BIT);
}

void OGL30Renderer::draw(std::shared_ptr<Image> image, Vec2<float> position)
{
	std::shared_ptr<RGBImage> rgbImage = std::dynamic_pointer_cast<RGBImage>(image);
	if (rgbImage)
	{
		GLRGBImage *img = dynamic_cast<GLRGBImage*>(rgbImage->rendererPrivateData.get());
		if (!img)
		{
			img = new GLRGBImage(rgbImage);
			image->rendererPrivateData.reset(img);
		}
		DrawRGB(*img, position);
		return;
	}
	
	std::shared_ptr<PaletteImage> paletteImage = std::dynamic_pointer_cast<PaletteImage>(image);
	if (paletteImage)
	{
		GLPaletteImage *img = dynamic_cast<GLPaletteImage*>(paletteImage->rendererPrivateData.get());
		if (!img)
		{
			img = new GLPaletteImage(paletteImage);
			image->rendererPrivateData.reset(img);
		}
		DrawPalette(*img, position);
		return;
	}

	std::shared_ptr<Surface> surface = std::dynamic_pointer_cast<Surface>(image);
	if (surface)
	{
		FBOData *fbo = dynamic_cast<FBOData*>(surface->rendererPrivateData.get());
		if (!fbo)
		{
			fbo = new FBOData(image->size);
			image->rendererPrivateData.reset(fbo);
		}
		DrawSurface(*fbo, position);
		return;
	}
}
void OGL30Renderer::drawFilledRect(Vec2<float> position, Vec2<float> size, Colour c)
{
	DrawRect(position, size, c);
}

void
OGL30Renderer::flush()
{
}

std::string
OGL30Renderer::getName()
{
	return "OGL3.0 Renderer";
}

}; //anonymouse namespace

OpenApoc::Renderer *
OpenApoc::Renderer::createRenderer()
{
	auto success = gl::sys::LoadFunctions();
	if (!success)
	{
		std::cerr << "Failed to load GL3.0\n";
		return nullptr;
	}
	if (success.GetNumMissing())
	{
		std::cerr << "Failed to load " << success.GetNumMissing() << " GL3.0 functions\n";
		return nullptr;
	}
	return new OGL30Renderer();
}
}; //namesapce OpenApoc
