/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "slg/film/film.h"

using namespace luxrays;
using namespace slg;

//------------------------------------------------------------------------------
// FilmOutput
//------------------------------------------------------------------------------

void FilmOutputs::Add(const FilmOutputType type, const std::string &fileName) {
	types.push_back(type);
	fileNames.push_back(fileName);
}

//------------------------------------------------------------------------------
// Film
//------------------------------------------------------------------------------

Film::Film(const u_int w, const u_int h) {
	initialized = false;

	width = w;
	height = h;

	channel_RADIANCE_PER_PIXEL_NORMALIZED = NULL;
	channel_RADIANCE_PER_SCREEN_NORMALIZED = NULL;
	channel_ALPHA = NULL;
	channel_RGB_TONEMAPPED = NULL;

	convTest = NULL;

	enabledOverlappedScreenBufferUpdate = true;

	filter = NULL;
	filterLUTs = NULL;
	SetFilter(new GaussianFilter(1.5f, 1.5f, 2.f));

	toneMapParams = new LinearToneMapParams();

	SetGamma();
}

Film::~Film() {
	delete toneMapParams;

	delete convTest;

	delete channel_RADIANCE_PER_PIXEL_NORMALIZED;
	delete channel_RADIANCE_PER_SCREEN_NORMALIZED;
	delete channel_ALPHA;
	delete channel_RGB_TONEMAPPED;

	delete filterLUTs;
	delete filter;
}

void Film::AddChannel(const FilmChannelType type) {
	if (initialized)
		throw std::runtime_error("it is possible to add a channel to a Film only before the initialization");

	channels.insert(type);
}

void Film::RemoveChannel(const FilmChannelType type) {
	if (initialized)
		throw std::runtime_error("it is possible to remove a channel of a Film only before the initialization");

	channels.erase(type);
}

void Film::Init(const u_int w, const u_int h) {
	if (initialized)
		throw std::runtime_error("A Film can not be initialized multiple times");

	initialized = true;
	width = w;
	height = h;
	pixelCount = w * h;

	delete convTest;
	convTest = NULL;

	// Delete all already allocated channels
	delete channel_RADIANCE_PER_PIXEL_NORMALIZED;
	delete channel_RADIANCE_PER_SCREEN_NORMALIZED;
	delete channel_ALPHA;
	delete channel_RGB_TONEMAPPED;
	
	// Allocate all required channels
	if (channels.count(RADIANCE_PER_PIXEL_NORMALIZED) > 0) {
		channel_RADIANCE_PER_PIXEL_NORMALIZED = new GenericFrameBuffer<4, float>(width, height);
		channel_RADIANCE_PER_PIXEL_NORMALIZED->Clear();
	}
	if (channels.count(RADIANCE_PER_SCREEN_NORMALIZED) > 0) {
		channel_RADIANCE_PER_SCREEN_NORMALIZED = new GenericFrameBuffer<4, float>(width, height);
		channel_RADIANCE_PER_SCREEN_NORMALIZED->Clear();
	}
	if (channels.count(ALPHA) > 0) {
		channel_ALPHA = new GenericFrameBuffer<2, float>(width, height);
		channel_ALPHA->Clear();
	}
	if (channels.count(TONEMAPPED_FRAMEBUFFER) > 0) {
		channel_RGB_TONEMAPPED = new GenericFrameBuffer<3, float>(width, height);
		channel_RGB_TONEMAPPED->Clear();

		convTest = new ConvergenceTest(width, height);
	}

	// Initialize the stats
	statsTotalSampleCount = 0.0;
	statsAvgSampleSec = 0.0;
	statsStartSampleTime = WallClockTime();
}

void Film::SetGamma(const float g) {
	gamma = g;

	float x = 0.f;
	const float dx = 1.f / GAMMA_TABLE_SIZE;
	for (u_int i = 0; i < GAMMA_TABLE_SIZE; ++i, x += dx)
		gammaTable[i] = powf(Clamp(x, 0.f, 1.f), 1.f / g);
}

void Film::SetFilter(Filter *flt) {
	delete filterLUTs;
	filterLUTs = NULL;
	delete filter;
	filter = flt;

	if (filter) {
		const u_int size = Max<u_int>(4, Max(filter->xWidth, filter->yWidth) + 1);
		filterLUTs = new FilterLUTs(*filter, size);
	}
}

void Film::Reset() {
	if (HasChannel(RADIANCE_PER_PIXEL_NORMALIZED))
		channel_RADIANCE_PER_PIXEL_NORMALIZED->Clear();
	if (HasChannel(RADIANCE_PER_SCREEN_NORMALIZED))
		channel_RADIANCE_PER_SCREEN_NORMALIZED->Clear();
	if (HasChannel(ALPHA))
		channel_ALPHA->Clear();

	// convTest has to be reseted explicitely

	statsTotalSampleCount = 0.0;
	statsAvgSampleSec = 0.0;
	statsStartSampleTime = WallClockTime();
}

void Film::AddFilm(const Film &film,
		const u_int srcOffsetX, const u_int srcOffsetY,
		const u_int srcWidth, const u_int srcHeight,
		const u_int dstOffsetX, const u_int dstOffsetY) {
	statsTotalSampleCount += film.statsTotalSampleCount;

	if (HasChannel(RADIANCE_PER_PIXEL_NORMALIZED) && film.HasChannel(RADIANCE_PER_PIXEL_NORMALIZED)) {
		for (u_int y = 0; y < srcHeight; ++y) {
			for (u_int x = 0; x < srcWidth; ++x) {
				const float *srcPixel = film.channel_RADIANCE_PER_PIXEL_NORMALIZED->GetPixel(srcOffsetX + x, srcOffsetY + y);
				channel_RADIANCE_PER_PIXEL_NORMALIZED->AddPixel(dstOffsetX + x, dstOffsetY + y, srcPixel);
			}
		}
	}

	if (HasChannel(RADIANCE_PER_SCREEN_NORMALIZED) && film.HasChannel(RADIANCE_PER_SCREEN_NORMALIZED)) {
		for (u_int y = 0; y < srcHeight; ++y) {
			for (u_int x = 0; x < srcWidth; ++x) {
				const float *srcPixel = film.channel_RADIANCE_PER_SCREEN_NORMALIZED->GetPixel(srcOffsetX + x, srcOffsetY + y);
				channel_RADIANCE_PER_SCREEN_NORMALIZED->AddPixel(dstOffsetX + x, dstOffsetY + y, srcPixel);
			}
		}
	}

	if (HasChannel(ALPHA) && film.HasChannel(ALPHA)) {
		for (u_int y = 0; y < srcHeight; ++y) {
			for (u_int x = 0; x < srcWidth; ++x) {
				const float *srcPixel = film.channel_ALPHA->GetPixel(srcOffsetX + x, srcOffsetY + y);
				channel_ALPHA->AddPixel(dstOffsetX + x, dstOffsetY + y, srcPixel);
			}
		}
	}
}

void Film::Output(const FilmOutputs &filmOutputs) {
	for (u_int i = 0; i < filmOutputs.GetCount(); ++i)
		Output(filmOutputs.GetType(i), filmOutputs.GetFileName(i));
}

void Film::Output(const FilmOutputs::FilmOutputType type, const std::string &fileName) {
	// Image format
	FREE_IMAGE_FORMAT fif = FREEIMAGE_GETFIFFROMFILENAME(FREEIMAGE_CONVFILENAME(fileName).c_str());
	if (fif == FIF_UNKNOWN)
		throw std::runtime_error("Image type unknown");

	// HDR image or not
	const bool hdrImage = ((fif == FIF_HDR) || (fif == FIF_EXR));

	// Image type and bit count
	FREE_IMAGE_TYPE imageType;
	u_int bitCount;
	switch (type) {
		case FilmOutputs::RGB:
			if (!hdrImage)
				return;
			imageType = FIT_RGBF;
			bitCount = 96;
			break;
		case FilmOutputs::RGB_TONEMAPPED:
			imageType = hdrImage ? FIT_RGBF : FIT_BITMAP;
			bitCount = hdrImage ? 96 : 24;
			break;
		case FilmOutputs::RGBA:
			if (!hdrImage)
				return;
			imageType = FIT_RGBAF;
			bitCount = 128;
			break;
		case FilmOutputs::RGBA_TONEMAPPED:
			imageType = hdrImage ? FIT_RGBAF : FIT_BITMAP;
			bitCount = hdrImage ? 128 : 32;
			break;
		case FilmOutputs::ALPHA:
			if (HasChannel(ALPHA)) {
				imageType = hdrImage ? FIT_FLOAT : FIT_BITMAP;
				bitCount = hdrImage ? 32 : 8;
			} else
				return;
			break;
		default:
			throw std::runtime_error("Unknown film output type");
	}

	if ((type == FilmOutputs::RGB) || (type == FilmOutputs::RGBA)) {
		// In order to merge the 2 sample buffers
		UpdateScreenBufferImpl(TONEMAP_NONE);
	}

	// Allocate the image
	FIBITMAP *dib = FreeImage_AllocateT(imageType, width, height, bitCount);
	if (!dib)
		throw std::runtime_error("Unable to allocate FreeImage image");

	// Build the image
	u_int pitch = FreeImage_GetPitch(dib);
	BYTE *bits = (BYTE *)FreeImage_GetBits(dib);
	for (u_int y = 0; y < height; ++y) {
		for (u_int x = 0; x < width; ++x) {
			switch (type) {
				case FilmOutputs::RGB: {
					FIRGBF *dst = (FIRGBF *)bits;
					// channel_RGB_TONEMAPPED has the _untonemapped_ version of
					// the image in the case of an HDR because of the call
					// to UpdateScreenBufferImpl(TONEMAP_NONE)
					const float *src = channel_RGB_TONEMAPPED->GetPixel(x, y);
					dst[x].red = src[0];
					dst[x].green = src[1];
					dst[x].blue = src[2];
					break;
				}
				case FilmOutputs::RGB_TONEMAPPED: {
					if (hdrImage) {
						FIRGBF *dst = (FIRGBF *)bits;
						const float *src = channel_RGB_TONEMAPPED->GetPixel(x, y);
						dst[x].red = src[0];
						dst[x].green = src[1];
						dst[x].blue = src[2];
					} else {
						BYTE *dst = &bits[x * 3];
						const float *src = channel_RGB_TONEMAPPED->GetPixel(x, y);
						dst[FI_RGBA_RED] = (BYTE)(src[0] * 255.f + .5f);
						dst[FI_RGBA_GREEN] = (BYTE)(src[1] * 255.f + .5f);
						dst[FI_RGBA_BLUE] = (BYTE)(src[2] * 255.f + .5f);
					}
					break;
				}
				case FilmOutputs::RGBA: {
					FIRGBAF *dst = (FIRGBAF *)bits;
					// channel_RGB_TONEMAPPED has the _untonemapped_ version of
					// the image in the case of an HDR because of the call
					// to UpdateScreenBufferImpl(TONEMAP_NONE)
					const float *src = channel_RGB_TONEMAPPED->GetPixel(x, y);
					dst[x].red = src[0];
					dst[x].green = src[1];
					dst[x].blue = src[2];

					const float *alphaData = channel_ALPHA->GetPixel(x, y);
					if (alphaData[1] == 0.f)
						dst[x].alpha = 0.f;
					else
						dst[x].alpha = alphaData[0] / alphaData[1];
					break;
				}
				case FilmOutputs::RGBA_TONEMAPPED: {
					if (hdrImage) {
						FIRGBAF *dst = (FIRGBAF *)bits;
						const float *src = channel_RGB_TONEMAPPED->GetPixel(x, y);
						dst[x].red = src[0];
						dst[x].green = src[1];
						dst[x].blue = src[2];

						const float *alphaData = channel_ALPHA->GetPixel(x, y);
						if (alphaData[1] == 0.f)
							dst[x].alpha = 0.f;
						else
							dst[x].alpha = alphaData[0] / alphaData[1];
					} else {
						BYTE *dst = &bits[x * 3];
						const float *src = channel_RGB_TONEMAPPED->GetPixel(x, y);
						dst[FI_RGBA_RED] = (BYTE)(src[0] * 255.f + .5f);
						dst[FI_RGBA_GREEN] = (BYTE)(src[1] * 255.f + .5f);
						dst[FI_RGBA_BLUE] = (BYTE)(src[2] * 255.f + .5f);

						const float *alphaData = channel_ALPHA->GetPixel(x, y);
						if (alphaData[1] == 0.f)
							dst[FI_RGBA_ALPHA] = 0;
						else
							dst[FI_RGBA_ALPHA] = (BYTE)((alphaData[0] / alphaData[1]) * 255.f + .5f);
					}
					break;
				}
				case FilmOutputs::ALPHA: {
					if (hdrImage) {
						float *dst = (float *)bits;

						const float *alphaData = channel_ALPHA->GetPixel(x, y);
						if (alphaData[1] == 0.f)
							dst[x] = 0.f;
						else
							dst[x] = alphaData[0] / alphaData[1];
					} else {
						BYTE *dst = &bits[x];

						const float *alphaData = channel_ALPHA->GetPixel(x, y);
						if (alphaData[1] == 0.f)
							*dst = 0;
						else
							*dst = (BYTE)((alphaData[0] / alphaData[1]) * 255.f + .5f);
					}
					break;
				}
				default:
					throw std::runtime_error("Unknown film output type");
			}
		}

		// Next line
		bits += pitch;
	}

	if (!FREEIMAGE_SAVE(fif, dib, FREEIMAGE_CONVFILENAME(fileName).c_str(), 0))
		throw std::runtime_error("Failed image save");

	FreeImage_Unload(dib);

	if ((type == FilmOutputs::RGB) || (type == FilmOutputs::RGBA)) {
		// To restore the used tonemapping instead of NONE
		UpdateScreenBuffer();
	}
}

void Film::UpdateScreenBuffer() {
	UpdateScreenBufferImpl(toneMapParams->GetType());
}

void Film::MergeSampleBuffers(Spectrum *p, std::vector<bool> &frameBufferMask) const {
	const u_int pixelCount = width * height;

	// Merge RADIANCE_PER_PIXEL_NORMALIZED and RADIANCE_PER_SCREEN_NORMALIZED buffers

	if (HasChannel(RADIANCE_PER_PIXEL_NORMALIZED)) {
		for (u_int i = 0; i < pixelCount; ++i) {
			const float *sp = channel_RADIANCE_PER_PIXEL_NORMALIZED->GetPixel(i);

			const float weight = sp[3];
			if (weight > 0.f) {
				p[i] = Spectrum(sp) / weight;
				frameBufferMask[i] = true;
			}
		}
	}

	if (HasChannel(RADIANCE_PER_SCREEN_NORMALIZED)) {
		const float factor = pixelCount / statsTotalSampleCount;

		for (u_int i = 0; i < pixelCount; ++i) {
			const float *sp = channel_RADIANCE_PER_SCREEN_NORMALIZED->GetPixel(i);

			if (sp[3] > 0.f) {
				if (frameBufferMask[i])
					p[i] += Spectrum(sp) * factor;
				else
					p[i] = Spectrum(sp) * factor;
				frameBufferMask[i] = true;
			}
		}
	}

	if (!enabledOverlappedScreenBufferUpdate) {
		for (u_int i = 0; i < pixelCount; ++i) {
			if (!frameBufferMask[i]) {
				p[i].r = 0.f;
				p[i].g = 0.f;
				p[i].b = 0.f;
			}
		}
	}
}

void Film::UpdateScreenBufferImpl(const ToneMapType type) {
	if ((!HasChannel(RADIANCE_PER_PIXEL_NORMALIZED) && !HasChannel(RADIANCE_PER_SCREEN_NORMALIZED)) ||
			!HasChannel(TONEMAPPED_FRAMEBUFFER)) {
		// Nothing to do
		return;
	}

	switch (type) {
		case TONEMAP_NONE: {
			Spectrum *p = (Spectrum *)channel_RGB_TONEMAPPED->GetPixels();
			std::vector<bool> frameBufferMask(pixelCount, false);

			MergeSampleBuffers(p, frameBufferMask);
			break;
		}
		case TONEMAP_LINEAR: {
			const LinearToneMapParams &tm = (LinearToneMapParams &)(*toneMapParams);
			Spectrum *p = (Spectrum *)channel_RGB_TONEMAPPED->GetPixels();
			const u_int pixelCount = width * height;
			std::vector<bool> frameBufferMask(pixelCount, false);

			MergeSampleBuffers(p, frameBufferMask);

			// Gamma correction
			for (u_int i = 0; i < pixelCount; ++i) {
				if (frameBufferMask[i])
					p[i] = Radiance2Pixel(tm.scale * p[i]);
			}
			break;
		}
		case TONEMAP_REINHARD02: {
			const Reinhard02ToneMapParams &tm = (Reinhard02ToneMapParams &)(*toneMapParams);

			const float alpha = .1f;
			const float preScale = tm.preScale;
			const float postScale = tm.postScale;
			const float burn = tm.burn;

			Spectrum *p = (Spectrum *)channel_RGB_TONEMAPPED->GetPixels();
			const u_int pixelCount = width * height;

			std::vector<bool> frameBufferMask(pixelCount, false);
			MergeSampleBuffers(p, frameBufferMask);

			// Use the frame buffer as temporary storage and calculate the average luminance
			float Ywa = 0.f;

			for (u_int i = 0; i < pixelCount; ++i) {
				if (frameBufferMask[i]) {
					// Convert to XYZ color space
					Spectrum xyz;
					xyz.r = 0.412453f * p[i].r + 0.357580f * p[i].g + 0.180423f * p[i].b;
					xyz.g = 0.212671f * p[i].r + 0.715160f * p[i].g + 0.072169f * p[i].b;
					xyz.b = 0.019334f * p[i].r + 0.119193f * p[i].g + 0.950227f * p[i].b;
					p[i] = xyz;

					Ywa += p[i].g;
				}
			}
			Ywa /= pixelCount;

			// Avoid division by zero
			if (Ywa == 0.f)
				Ywa = 1.f;

			const float Yw = preScale * alpha * burn;
			const float invY2 = 1.f / (Yw * Yw);
			const float pScale = postScale * preScale * alpha / Ywa;

			for (u_int i = 0; i < pixelCount; ++i) {
				if (frameBufferMask[i]) {
					Spectrum xyz = p[i];

					const float ys = xyz.g;
					xyz *= pScale * (1.f + ys * invY2) / (1.f + ys);

					// Convert back to RGB color space
					p[i].r =  3.240479f * xyz.r - 1.537150f * xyz.g - 0.498535f * xyz.b;
					p[i].g = -0.969256f * xyz.r + 1.875991f * xyz.g + 0.041556f * xyz.b;
					p[i].b =  0.055648f * xyz.r - 0.204043f * xyz.g + 1.057311f * xyz.b;

					// Gamma correction
					p[i].r = Radiance2PixelFloat(p[i].r);
					p[i].g = Radiance2PixelFloat(p[i].g);
					p[i].b = Radiance2PixelFloat(p[i].b);
				}
			}
			break;
		}
		default:
			assert (false);
			break;
	}
}

void Film::AddSampleResult(const u_int x, const u_int y,
		const SampleResult &sampleResult, const float weight)  {
	if (channel_RADIANCE_PER_PIXEL_NORMALIZED && sampleResult.HasChannel(RADIANCE_PER_PIXEL_NORMALIZED) &&
			!sampleResult.radiancePerPixelNormalized.IsNaN() && !sampleResult.radiancePerPixelNormalized.IsInf()) {
		float pixel[4];
		pixel[0] = sampleResult.radiancePerPixelNormalized.r * weight;
		pixel[1] = sampleResult.radiancePerPixelNormalized.g * weight;
		pixel[2] = sampleResult.radiancePerPixelNormalized.b * weight;
		pixel[3] = weight;
		channel_RADIANCE_PER_PIXEL_NORMALIZED->AddPixel(x, y, pixel);
	}

	if (channel_RADIANCE_PER_SCREEN_NORMALIZED && sampleResult.HasChannel(RADIANCE_PER_SCREEN_NORMALIZED) &&
			!sampleResult.radiancePerScreenNormalized.IsNaN() && !sampleResult.radiancePerScreenNormalized.IsInf()) {
		float pixel[4];
		pixel[0] = sampleResult.radiancePerScreenNormalized.r * weight;
		pixel[1] = sampleResult.radiancePerScreenNormalized.g * weight;
		pixel[2] = sampleResult.radiancePerScreenNormalized.b * weight;
		pixel[3] = weight;
		channel_RADIANCE_PER_SCREEN_NORMALIZED->AddPixel(x, y, pixel);
	}

	// Faster than HasChannel(ALPHA)
	if (channel_ALPHA && sampleResult.HasChannel(ALPHA) && !isnan(sampleResult.alpha) && !isinf(sampleResult.alpha)) {
		float pixel[2];
		pixel[0] = weight * sampleResult.alpha;
		pixel[1] = weight;
		channel_ALPHA->AddPixel(x, y, pixel);
	}
}

void Film::AddSample(const u_int x, const u_int y,
		const SampleResult &sampleResult, const float weight) {
	if ((x < 0) || (x >= width) || (y < 0) || (y >= height))
		return;

	AddSampleResult(x, y, sampleResult, weight);
}

void Film::SplatSample(const SampleResult &sampleResult, const float weight) {
	if (!filter) {
		const int x = Ceil2Int(sampleResult.filmX - 0.5f);
		const int y = Ceil2Int(sampleResult.filmY - 0.5f);

		if ((x >= 0.f) && (x < (int)width) && (y >= 0.f) && (y < (int)height))
			AddSampleResult(x, y, sampleResult, weight);
	} else {
		// Compute sample's raster extent
		const float dImageX = sampleResult.filmX - .5f;
		const float dImageY = sampleResult.filmY - .5f;
		const FilterLUT *filterLUT = filterLUTs->GetLUT(dImageX - floorf(sampleResult.filmX), dImageY - floorf(sampleResult.filmY));
		const float *lut = filterLUT->GetLUT();

		const int x0 = Ceil2Int(dImageX - filter->xWidth);
		const int x1 = x0 + filterLUT->GetWidth();
		const int y0 = Ceil2Int(dImageY - filter->yWidth);
		const int y1 = y0 + filterLUT->GetHeight();

		for (int iy = y0; iy < y1; ++iy) {
			if (iy < 0) {
				lut += filterLUT->GetWidth();
				continue;
			} else if(iy >= (int)height)
				break;

			for (int ix = x0; ix < x1; ++ix) {
				const float filterWeight = *lut++;

				if ((ix < 0) || (ix >= (int)width))
					continue;

				const float filteredWeight = weight * filterWeight;
				AddSampleResult(ix, iy, sampleResult, filteredWeight);
			}
		}
	}
}

void Film::ResetConvergenceTest() {
	if (convTest)
		convTest->Reset();
}

u_int Film::RunConvergenceTest() {
	return convTest->Test((const float *)channel_RGB_TONEMAPPED->GetPixels());
}
