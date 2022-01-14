#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/Program.hh>
#include <optional>

namespace EmuEx
{

using namespace IG;

enum class ImageEffectId : uint8_t
{
	DIRECT = 0,
	HQ2X = 1,
	SCALE2X = 2,
	PRESCALE2X = 3,
};

static constexpr int lastImageEffectIdValue = (int)ImageEffectId::PRESCALE2X;

class VideoImageEffect
{
public:
	using Id = ImageEffectId;

	struct EffectDesc
	{
		const char *vShaderFilename;
		const char *fShaderFilename;
		IG::WP scale;
	};

	constexpr	VideoImageEffect() = default;
	VideoImageEffect(Gfx::Renderer &r, Id effect, IG::PixelFormat, Gfx::ColorSpace,
		const Gfx::TextureSampler &compatTexSampler, IG::WP size);
	void setImageSize(Gfx::Renderer &r, IG::WP size, const Gfx::TextureSampler &compatTexSampler);
	void setFormat(Gfx::Renderer &r, IG::PixelFormat, Gfx::ColorSpace, const Gfx::TextureSampler &compatTexSampler);
	void setCompatTextureSampler(const Gfx::TextureSampler &);
	Gfx::Program &program();
	Gfx::Texture &renderTarget();
	void drawRenderTarget(Gfx::RendererCommands &, const Gfx::TextureSpan);
	constexpr IG::PixelFormat imageFormat() const { return format; }
	operator bool() const { return (bool)prog; }

private:
	Gfx::Texture renderTarget_{};
	Gfx::Program prog{};
	int srcTexelDeltaU{};
	int srcTexelHalfDeltaU{};
	int srcPixelsU{};
	IG::WP renderTargetScale{};
	IG::WP renderTargetImgSize{};
	IG::WP inputImgSize{1, 1};
	IG::PixelFormat format{};
	Gfx::ColorSpace colorSpace{};

	void initRenderTargetTexture(Gfx::Renderer &r, const Gfx::TextureSampler &compatTexSampler);
	void updateProgramUniforms(Gfx::Renderer &r);
	void compile(Gfx::Renderer &r, EffectDesc desc, const Gfx::TextureSampler &compatTexSampler);
	void compileEffect(Gfx::Renderer &r, EffectDesc desc, bool useFallback);
};

}
