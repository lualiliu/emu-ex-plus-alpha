#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/base/Error.hh>
#include <imagine/util/Point2D.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/concepts.hh>
#include <optional>
#include <stdexcept>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/gfx-globals.hh>
#endif

namespace IG::Gfx
{

class RendererTask;
class RendererCommands;
class SyncFence;
class Texture;

using GP = FP;
using GCRect = IG::CoordinateRect<float, true, true>;

static GCRect makeGCRectRel(GP p, GP size)
{
	return GCRect::makeRel(p, size);
}

static auto pixelToTexC(IG::integral auto pixel, IG::integral auto total)
{
	return pixel / (float)total;
}

enum WrapMode
{
	WRAP_REPEAT,
	WRAP_CLAMP
};

enum MipFilterMode
{
	MIP_FILTER_NONE,
	MIP_FILTER_NEAREST,
	MIP_FILTER_LINEAR,
};

enum { BLEND_MODE_OFF = 0, BLEND_MODE_ALPHA, BLEND_MODE_INTENSITY };

enum { IMG_MODE_MODULATE = 0, IMG_MODE_BLEND, IMG_MODE_REPLACE, IMG_MODE_ADD };

enum { BLEND_EQ_ADD, BLEND_EQ_SUB, BLEND_EQ_RSUB };

enum { BOTH_FACES, FRONT_FACES, BACK_FACES };

enum class ColorName
{
	RED,
	GREEN,
	BLUE,
	CYAN,
	YELLOW,
	MAGENTA,
	WHITE,
	BLACK
};

enum TransformTargetEnum { TARGET_WORLD, TARGET_TEXTURE };

enum class CommonProgram
{
	// color replacement shaders
	TEX_REPLACE,
	TEX_ALPHA_REPLACE,
	TEX_EXTERNAL_REPLACE,

	// color modulation shaders
	TEX,
	TEX_ALPHA,
	TEX_EXTERNAL,
	NO_TEX
};

enum class CommonTextureSampler
{
	CLAMP,
	NEAREST_MIP_CLAMP,
	NO_MIP_CLAMP,
	NO_LINEAR_NO_MIP_CLAMP,
	REPEAT,
	NEAREST_MIP_REPEAT
};

class TextureSpan
{
public:
	constexpr TextureSpan(const Texture *tex = {}, FRect uv = {{}, {1., 1.}}):
		tex{tex}, uv{uv}
	{}
	constexpr const Texture *texture() const { return tex; }
	constexpr auto uvBounds() const { return uv; }
	explicit operator bool() const;

protected:
	const Texture *tex;
	FRect uv;
};

enum class TextureBufferMode : uint8_t
{
	DEFAULT,
	SYSTEM_MEMORY,
	ANDROID_HARDWARE_BUFFER,
	ANDROID_SURFACE_TEXTURE,
	PBO,
};

enum class DrawAsyncMode : uint8_t
{
	AUTO, NONE, PRESENT, FULL
};

class DrawParams
{
public:
	constexpr DrawParams() = default;
	constexpr DrawParams(DrawAsyncMode asyncMode):
		asyncMode_{asyncMode}
	{}

	void setAsyncMode(DrawAsyncMode mode)
	{
		asyncMode_ = mode;
	}

	DrawAsyncMode asyncMode() const { return asyncMode_; }

private:
	DrawAsyncMode asyncMode_ = DrawAsyncMode::AUTO;
};

static constexpr Color color(float r, float g, float b, float a = 1.f)
{
	if constexpr(std::is_floating_point_v<ColorComp>)
	{
		return {(ColorComp)r, (ColorComp)g, (ColorComp)b, (ColorComp)a};
	}
	else
	{
		return {255.f * r, 255.f * g, 255.f * b, 255.f * a};
	}
}

static constexpr Color color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	if constexpr(std::is_floating_point_v<ColorComp>)
	{
		return {r / 255.f, g / 255.f, b / 255.f, a / 255.f};
	}
	else
	{
		return {(ColorComp)r, (ColorComp)g, (ColorComp)b, (ColorComp)a};
	}
}

static constexpr Color color(ColorName c)
{
	switch(c)
	{
		case ColorName::RED: return color(1.f, 0.f, 0.f);
		case ColorName::GREEN: return color(0.f, 1.f, 0.f);
		case ColorName::BLUE: return color(0.f, 0.f, 1.f);
		case ColorName::CYAN: return color(0.f, 1.f, 1.f);
		case ColorName::YELLOW: return color(1.f, 1.f, 0.f);
		case ColorName::MAGENTA: return color(1.f, 0.f, 1.f);
		case ColorName::WHITE: return color(1.f, 1.f, 1.f);
		case ColorName::BLACK: return color(0.f, 0.f, 0.f);
		default: return color(0.f, 0.f, 0.f, 0.f);
	}
}

}
