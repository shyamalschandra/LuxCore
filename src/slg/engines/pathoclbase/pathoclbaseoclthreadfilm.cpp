/***************************************************************************
 * Copyright 1998-2018 by authors (see AUTHORS.txt)                        *
 *                                                                         *
 *   This file is part of LuxCoreRender.                                   *
 *                                                                         *
 * Licensed under the Apache License, Version 2.0 (the "License");         *
 * you may not use this file except in compliance with the License.        *
 * You may obtain a copy of the License at                                 *
 *                                                                         *
 *     http://www.apache.org/licenses/LICENSE-2.0                          *
 *                                                                         *
 * Unless required by applicable law or agreed to in writing, software     *
 * distributed under the License is distributed on an "AS IS" BASIS,       *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*
 * See the License for the specific language governing permissions and     *
 * limitations under the License.                                          *
 ***************************************************************************/

#if !defined(LUXRAYS_DISABLE_OPENCL)

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "luxrays/core/geometry/transform.h"
#include "luxrays/utils/ocl.h"
#include "luxrays/core/oclintersectiondevice.h"
#include "luxrays/kernels/kernels.h"

#include "luxcore/cfg.h"

#include "slg/slg.h"
#include "slg/kernels/kernels.h"
#include "slg/renderconfig.h"
#include "slg/engines/pathoclbase/pathoclbase.h"

#if defined(__APPLE__)
//OSX version detection
#include <sys/sysctl.h>
#endif

using namespace std;
using namespace luxrays;
using namespace slg;

//------------------------------------------------------------------------------
// ThreadFilm
//------------------------------------------------------------------------------

PathOCLBaseOCLRenderThread::ThreadFilm::ThreadFilm(PathOCLBaseOCLRenderThread *thread) {
	film = NULL;

	// Film buffers
	channel_ALPHA_Buff = NULL;
	channel_DEPTH_Buff = NULL;
	channel_POSITION_Buff = NULL;
	channel_GEOMETRY_NORMAL_Buff = NULL;
	channel_SHADING_NORMAL_Buff = NULL;
	channel_MATERIAL_ID_Buff = NULL;
	channel_DIRECT_DIFFUSE_Buff = NULL;
	channel_DIRECT_GLOSSY_Buff = NULL;
	channel_EMISSION_Buff = NULL;
	channel_INDIRECT_DIFFUSE_Buff = NULL;
	channel_INDIRECT_GLOSSY_Buff = NULL;
	channel_INDIRECT_SPECULAR_Buff = NULL;
	channel_MATERIAL_ID_MASK_Buff = NULL;
	channel_DIRECT_SHADOW_MASK_Buff = NULL;
	channel_INDIRECT_SHADOW_MASK_Buff = NULL;
	channel_UV_Buff = NULL;
	channel_RAYCOUNT_Buff = NULL;
	channel_BY_MATERIAL_ID_Buff = NULL;
	channel_IRRADIANCE_Buff = NULL;
	channel_OBJECT_ID_Buff = NULL;
	channel_OBJECT_ID_MASK_Buff = NULL;
	channel_BY_OBJECT_ID_Buff = NULL;
	channel_SAMPLECOUNT_Buff = NULL;
	channel_CONVERGENCE_Buff = NULL;

	renderThread = thread;
}

PathOCLBaseOCLRenderThread::ThreadFilm::~ThreadFilm() {
	delete film;

	FreeAllOCLBuffers();
}

void PathOCLBaseOCLRenderThread::ThreadFilm::Init(Film *engineFlm,
		const u_int threadFilmWidth, const u_int threadFilmHeight,
		const u_int *threadFilmSubRegion) {
	engineFilm = engineFlm;

	const u_int filmPixelCount = threadFilmWidth * threadFilmHeight;

	// Delete previous allocated Film
	delete film;

	// Allocate the new Film
	film = new Film(threadFilmWidth, threadFilmHeight, threadFilmSubRegion);
	film->CopyDynamicSettings(*engineFilm);
	film->Init();

	//--------------------------------------------------------------------------
	for (u_int i = 0; i < channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff.size(); ++i)
		renderThread->FreeOCLBuffer(&channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]);

	if (film->GetRadianceGroupCount() > 8)
		throw runtime_error("PathOCL supports only up to 8 Radiance Groups");

	channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff.resize(film->GetRadianceGroupCount(), NULL);
	for (u_int i = 0; i < channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff.size(); ++i) {
		renderThread->AllocOCLBufferRW(&channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i],
				sizeof(float[4]) * filmPixelCount, "RADIANCE_PER_PIXEL_NORMALIZEDs[" + ToString(i) + "]");
	}
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::ALPHA))
		renderThread->AllocOCLBufferRW(&channel_ALPHA_Buff, sizeof(float[2]) * filmPixelCount, "ALPHA");
	else
		renderThread->FreeOCLBuffer(&channel_ALPHA_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::DEPTH))
		renderThread->AllocOCLBufferRW(&channel_DEPTH_Buff, sizeof(float) * filmPixelCount, "DEPTH");
	else
		renderThread->FreeOCLBuffer(&channel_DEPTH_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::POSITION))
		renderThread->AllocOCLBufferRW(&channel_POSITION_Buff, sizeof(float[3]) * filmPixelCount, "POSITION");
	else
		renderThread->FreeOCLBuffer(&channel_POSITION_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::GEOMETRY_NORMAL))
		renderThread->AllocOCLBufferRW(&channel_GEOMETRY_NORMAL_Buff, sizeof(float[3]) * filmPixelCount, "GEOMETRY_NORMAL");
	else
		renderThread->FreeOCLBuffer(&channel_GEOMETRY_NORMAL_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::SHADING_NORMAL))
		renderThread->AllocOCLBufferRW(&channel_SHADING_NORMAL_Buff, sizeof(float[3]) * filmPixelCount, "SHADING_NORMAL");
	else
		renderThread->FreeOCLBuffer(&channel_SHADING_NORMAL_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::MATERIAL_ID))
		renderThread->AllocOCLBufferRW(&channel_MATERIAL_ID_Buff, sizeof(u_int) * filmPixelCount, "MATERIAL_ID");
	else
		renderThread->FreeOCLBuffer(&channel_MATERIAL_ID_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::DIRECT_DIFFUSE))
		renderThread->AllocOCLBufferRW(&channel_DIRECT_DIFFUSE_Buff, sizeof(float[4]) * filmPixelCount, "DIRECT_DIFFUSE");
	else
		renderThread->FreeOCLBuffer(&channel_DIRECT_DIFFUSE_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::DIRECT_GLOSSY))
		renderThread->AllocOCLBufferRW(&channel_DIRECT_GLOSSY_Buff, sizeof(float[4]) * filmPixelCount, "DIRECT_GLOSSY");
	else
		renderThread->FreeOCLBuffer(&channel_DIRECT_GLOSSY_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::EMISSION))
		renderThread->AllocOCLBufferRW(&channel_EMISSION_Buff, sizeof(float[4]) * filmPixelCount, "EMISSION");
	else
		renderThread->FreeOCLBuffer(&channel_EMISSION_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::INDIRECT_DIFFUSE))
		renderThread->AllocOCLBufferRW(&channel_INDIRECT_DIFFUSE_Buff, sizeof(float[4]) * filmPixelCount, "INDIRECT_DIFFUSE");
	else
		renderThread->FreeOCLBuffer(&channel_INDIRECT_DIFFUSE_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::INDIRECT_GLOSSY))
		renderThread->AllocOCLBufferRW(&channel_INDIRECT_GLOSSY_Buff, sizeof(float[4]) * filmPixelCount, "INDIRECT_GLOSSY");
	else
		renderThread->FreeOCLBuffer(&channel_INDIRECT_GLOSSY_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::INDIRECT_SPECULAR))
		renderThread->AllocOCLBufferRW(&channel_INDIRECT_SPECULAR_Buff, sizeof(float[4]) * filmPixelCount, "INDIRECT_SPECULAR");
	else
		renderThread->FreeOCLBuffer(&channel_INDIRECT_SPECULAR_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::MATERIAL_ID_MASK)) {
		if (film->GetChannelCount(Film::MATERIAL_ID_MASK) > 1)
			throw runtime_error("PathOCL supports only 1 MATERIAL_ID_MASK");
		else
			renderThread->AllocOCLBufferRW(&channel_MATERIAL_ID_MASK_Buff,
					sizeof(float[2]) * filmPixelCount, "MATERIAL_ID_MASK");
	} else
		renderThread->FreeOCLBuffer(&channel_MATERIAL_ID_MASK_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::DIRECT_SHADOW_MASK))
		renderThread->AllocOCLBufferRW(&channel_DIRECT_SHADOW_MASK_Buff, sizeof(float[2]) * filmPixelCount, "DIRECT_SHADOW_MASK");
	else
		renderThread->FreeOCLBuffer(&channel_DIRECT_SHADOW_MASK_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::INDIRECT_SHADOW_MASK))
		renderThread->AllocOCLBufferRW(&channel_INDIRECT_SHADOW_MASK_Buff, sizeof(float[2]) * filmPixelCount, "INDIRECT_SHADOW_MASK");
	else
		renderThread->FreeOCLBuffer(&channel_INDIRECT_SHADOW_MASK_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::UV))
		renderThread->AllocOCLBufferRW(&channel_UV_Buff, sizeof(float[2]) * filmPixelCount, "UV");
	else
		renderThread->FreeOCLBuffer(&channel_UV_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::RAYCOUNT))
		renderThread->AllocOCLBufferRW(&channel_RAYCOUNT_Buff, sizeof(float) * filmPixelCount, "RAYCOUNT");
	else
		renderThread->FreeOCLBuffer(&channel_RAYCOUNT_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::BY_MATERIAL_ID)) {
		if (film->GetChannelCount(Film::BY_MATERIAL_ID) > 1)
			throw runtime_error("PathOCL supports only 1 BY_MATERIAL_ID");
		else
			renderThread->AllocOCLBufferRW(&channel_BY_MATERIAL_ID_Buff,
					sizeof(float[4]) * filmPixelCount, "BY_MATERIAL_ID");
	} else
		renderThread->FreeOCLBuffer(&channel_BY_MATERIAL_ID_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::IRRADIANCE))
		renderThread->AllocOCLBufferRW(&channel_IRRADIANCE_Buff, sizeof(float[4]) * filmPixelCount, "IRRADIANCE");
	else
		renderThread->FreeOCLBuffer(&channel_IRRADIANCE_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::OBJECT_ID))
		renderThread->AllocOCLBufferRW(&channel_OBJECT_ID_Buff, sizeof(u_int) * filmPixelCount, "OBJECT_ID");
	else
		renderThread->FreeOCLBuffer(&channel_OBJECT_ID_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::OBJECT_ID_MASK)) {
		if (film->GetChannelCount(Film::OBJECT_ID_MASK) > 1)
			throw runtime_error("PathOCL supports only 1 OBJECT_ID_MASK");
		else
			renderThread->AllocOCLBufferRW(&channel_OBJECT_ID_MASK_Buff,
					sizeof(float[2]) * filmPixelCount, "OBJECT_ID_MASK");
	} else
		renderThread->FreeOCLBuffer(&channel_OBJECT_ID_MASK_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::BY_OBJECT_ID)) {
		if (film->GetChannelCount(Film::BY_OBJECT_ID) > 1)
			throw runtime_error("PathOCL supports only 1 BY_OBJECT_ID");
		else
			renderThread->AllocOCLBufferRW(&channel_BY_OBJECT_ID_Buff,
					sizeof(float[4]) * filmPixelCount, "BY_OBJECT_ID");
	} else
		renderThread->FreeOCLBuffer(&channel_BY_OBJECT_ID_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::SAMPLECOUNT))
		renderThread->AllocOCLBufferRW(&channel_SAMPLECOUNT_Buff, sizeof(u_int) * filmPixelCount, "SAMPLECOUNT");
	else
		renderThread->FreeOCLBuffer(&channel_SAMPLECOUNT_Buff);
	//--------------------------------------------------------------------------
	if (film->HasChannel(Film::CONVERGENCE))
		renderThread->AllocOCLBufferRW(&channel_CONVERGENCE_Buff, sizeof(float) * filmPixelCount, "CONVERGENCE");
	else
		renderThread->FreeOCLBuffer(&channel_CONVERGENCE_Buff);
}

void PathOCLBaseOCLRenderThread::ThreadFilm::FreeAllOCLBuffers() {
	// Film buffers
	for (u_int i = 0; i < channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff.size(); ++i)
		renderThread->FreeOCLBuffer(&channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]);
	channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff.clear();
	renderThread->FreeOCLBuffer(&channel_ALPHA_Buff);
	renderThread->FreeOCLBuffer(&channel_DEPTH_Buff);
	renderThread->FreeOCLBuffer(&channel_POSITION_Buff);
	renderThread->FreeOCLBuffer(&channel_GEOMETRY_NORMAL_Buff);
	renderThread->FreeOCLBuffer(&channel_SHADING_NORMAL_Buff);
	renderThread->FreeOCLBuffer(&channel_MATERIAL_ID_Buff);
	renderThread->FreeOCLBuffer(&channel_DIRECT_DIFFUSE_Buff);
	renderThread->FreeOCLBuffer(&channel_DIRECT_GLOSSY_Buff);
	renderThread->FreeOCLBuffer(&channel_EMISSION_Buff);
	renderThread->FreeOCLBuffer(&channel_INDIRECT_DIFFUSE_Buff);
	renderThread->FreeOCLBuffer(&channel_INDIRECT_GLOSSY_Buff);
	renderThread->FreeOCLBuffer(&channel_INDIRECT_SPECULAR_Buff);
	renderThread->FreeOCLBuffer(&channel_MATERIAL_ID_MASK_Buff);
	renderThread->FreeOCLBuffer(&channel_DIRECT_SHADOW_MASK_Buff);
	renderThread->FreeOCLBuffer(&channel_INDIRECT_SHADOW_MASK_Buff);
	renderThread->FreeOCLBuffer(&channel_UV_Buff);
	renderThread->FreeOCLBuffer(&channel_RAYCOUNT_Buff);
	renderThread->FreeOCLBuffer(&channel_BY_MATERIAL_ID_Buff);
	renderThread->FreeOCLBuffer(&channel_IRRADIANCE_Buff);
	renderThread->FreeOCLBuffer(&channel_OBJECT_ID_Buff);
	renderThread->FreeOCLBuffer(&channel_OBJECT_ID_MASK_Buff);
	renderThread->FreeOCLBuffer(&channel_BY_OBJECT_ID_Buff);
	renderThread->FreeOCLBuffer(&channel_SAMPLECOUNT_Buff);
	renderThread->FreeOCLBuffer(&channel_CONVERGENCE_Buff);
}

u_int PathOCLBaseOCLRenderThread::ThreadFilm::SetFilmKernelArgs(cl::Kernel &filmClearKernel,
		u_int argIndex) const {
	// Film parameters
	filmClearKernel.setArg(argIndex++, film->GetWidth());
	filmClearKernel.setArg(argIndex++, film->GetHeight());

	const u_int *filmSubRegion = film->GetSubRegion();
	filmClearKernel.setArg(argIndex++, filmSubRegion[0]);
	filmClearKernel.setArg(argIndex++, filmSubRegion[1]);
	filmClearKernel.setArg(argIndex++, filmSubRegion[2]);
	filmClearKernel.setArg(argIndex++, filmSubRegion[3]);

	for (u_int i = 0; i < channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff.size(); ++i)
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]);
	if (film->HasChannel(Film::ALPHA))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_ALPHA_Buff);
	if (film->HasChannel(Film::DEPTH))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_DEPTH_Buff);
	if (film->HasChannel(Film::POSITION))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_POSITION_Buff);
	if (film->HasChannel(Film::GEOMETRY_NORMAL))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_GEOMETRY_NORMAL_Buff);
	if (film->HasChannel(Film::SHADING_NORMAL))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_SHADING_NORMAL_Buff);
	if (film->HasChannel(Film::MATERIAL_ID))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_MATERIAL_ID_Buff);
	if (film->HasChannel(Film::DIRECT_DIFFUSE))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_DIRECT_DIFFUSE_Buff);
	if (film->HasChannel(Film::DIRECT_GLOSSY))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_DIRECT_GLOSSY_Buff);
	if (film->HasChannel(Film::EMISSION))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_EMISSION_Buff);
	if (film->HasChannel(Film::INDIRECT_DIFFUSE))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_INDIRECT_DIFFUSE_Buff);
	if (film->HasChannel(Film::INDIRECT_GLOSSY))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_INDIRECT_GLOSSY_Buff);
	if (film->HasChannel(Film::INDIRECT_SPECULAR))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_INDIRECT_SPECULAR_Buff);
	if (film->HasChannel(Film::MATERIAL_ID_MASK))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_MATERIAL_ID_MASK_Buff);
	if (film->HasChannel(Film::DIRECT_SHADOW_MASK))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_DIRECT_SHADOW_MASK_Buff);
	if (film->HasChannel(Film::INDIRECT_SHADOW_MASK))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_INDIRECT_SHADOW_MASK_Buff);
	if (film->HasChannel(Film::UV))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_UV_Buff);
	if (film->HasChannel(Film::RAYCOUNT))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_RAYCOUNT_Buff);
	if (film->HasChannel(Film::BY_MATERIAL_ID))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_BY_MATERIAL_ID_Buff);
	if (film->HasChannel(Film::IRRADIANCE))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_IRRADIANCE_Buff);
	if (film->HasChannel(Film::OBJECT_ID))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_OBJECT_ID_Buff);
	if (film->HasChannel(Film::OBJECT_ID_MASK))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_OBJECT_ID_MASK_Buff);
	if (film->HasChannel(Film::BY_OBJECT_ID))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_BY_OBJECT_ID_Buff);
	if (film->HasChannel(Film::SAMPLECOUNT))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_SAMPLECOUNT_Buff);
	if (film->HasChannel(Film::CONVERGENCE))
		filmClearKernel.setArg(argIndex++, sizeof(cl::Buffer), channel_CONVERGENCE_Buff);

	return argIndex;
}

void PathOCLBaseOCLRenderThread::ThreadFilm::RecvFilm(cl::CommandQueue &oclQueue) {
	// Async. transfer of the Film buffers

	for (u_int i = 0; i < channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff.size(); ++i) {
		if (channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]) {
			oclQueue.enqueueReadBuffer(
				*(channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]),
				CL_FALSE,
				0,
				channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]->getInfo<CL_MEM_SIZE>(),
				film->channel_RADIANCE_PER_PIXEL_NORMALIZEDs[i]->GetPixels());
		}
	}

	if (channel_ALPHA_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_ALPHA_Buff,
			CL_FALSE,
			0,
			channel_ALPHA_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_ALPHA->GetPixels());
	}
	if (channel_DEPTH_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_DEPTH_Buff,
			CL_FALSE,
			0,
			channel_DEPTH_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_DEPTH->GetPixels());
	}
	if (channel_POSITION_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_POSITION_Buff,
			CL_FALSE,
			0,
			channel_POSITION_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_POSITION->GetPixels());
	}
	if (channel_GEOMETRY_NORMAL_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_GEOMETRY_NORMAL_Buff,
			CL_FALSE,
			0,
			channel_GEOMETRY_NORMAL_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_GEOMETRY_NORMAL->GetPixels());
	}
	if (channel_SHADING_NORMAL_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_SHADING_NORMAL_Buff,
			CL_FALSE,
			0,
			channel_SHADING_NORMAL_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_SHADING_NORMAL->GetPixels());
	}
	if (channel_MATERIAL_ID_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_MATERIAL_ID_Buff,
			CL_FALSE,
			0,
			channel_MATERIAL_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_MATERIAL_ID->GetPixels());
	}
	if (channel_DIRECT_DIFFUSE_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_DIRECT_DIFFUSE_Buff,
			CL_FALSE,
			0,
			channel_DIRECT_DIFFUSE_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_DIRECT_DIFFUSE->GetPixels());
	}
	if (channel_MATERIAL_ID_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_MATERIAL_ID_Buff,
			CL_FALSE,
			0,
			channel_MATERIAL_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_MATERIAL_ID->GetPixels());
	}
	if (channel_DIRECT_GLOSSY_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_DIRECT_GLOSSY_Buff,
			CL_FALSE,
			0,
			channel_DIRECT_GLOSSY_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_DIRECT_GLOSSY->GetPixels());
	}
	if (channel_EMISSION_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_EMISSION_Buff,
			CL_FALSE,
			0,
			channel_EMISSION_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_EMISSION->GetPixels());
	}
	if (channel_INDIRECT_DIFFUSE_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_INDIRECT_DIFFUSE_Buff,
			CL_FALSE,
			0,
			channel_INDIRECT_DIFFUSE_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_INDIRECT_DIFFUSE->GetPixels());
	}
	if (channel_INDIRECT_GLOSSY_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_INDIRECT_GLOSSY_Buff,
			CL_FALSE,
			0,
			channel_INDIRECT_GLOSSY_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_INDIRECT_GLOSSY->GetPixels());
	}
	if (channel_INDIRECT_SPECULAR_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_INDIRECT_SPECULAR_Buff,
			CL_FALSE,
			0,
			channel_INDIRECT_SPECULAR_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_INDIRECT_SPECULAR->GetPixels());
	}
	if (channel_MATERIAL_ID_MASK_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_MATERIAL_ID_MASK_Buff,
			CL_FALSE,
			0,
			channel_MATERIAL_ID_MASK_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_MATERIAL_ID_MASKs[0]->GetPixels());
	}
	if (channel_DIRECT_SHADOW_MASK_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_DIRECT_SHADOW_MASK_Buff,
			CL_FALSE,
			0,
			channel_DIRECT_SHADOW_MASK_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_DIRECT_SHADOW_MASK->GetPixels());
	}
	if (channel_INDIRECT_SHADOW_MASK_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_INDIRECT_SHADOW_MASK_Buff,
			CL_FALSE,
			0,
			channel_INDIRECT_SHADOW_MASK_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_INDIRECT_SHADOW_MASK->GetPixels());
	}
	if (channel_UV_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_UV_Buff,
			CL_FALSE,
			0,
			channel_UV_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_UV->GetPixels());
	}
	if (channel_RAYCOUNT_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_RAYCOUNT_Buff,
			CL_FALSE,
			0,
			channel_RAYCOUNT_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_RAYCOUNT->GetPixels());
	}
	if (channel_BY_MATERIAL_ID_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_BY_MATERIAL_ID_Buff,
			CL_FALSE,
			0,
			channel_BY_MATERIAL_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_BY_MATERIAL_IDs[0]->GetPixels());
	}
	if (channel_IRRADIANCE_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_IRRADIANCE_Buff,
			CL_FALSE,
			0,
			channel_IRRADIANCE_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_IRRADIANCE->GetPixels());
	}
	if (channel_OBJECT_ID_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_OBJECT_ID_Buff,
			CL_FALSE,
			0,
			channel_OBJECT_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_OBJECT_ID->GetPixels());
	}
	if (channel_OBJECT_ID_MASK_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_OBJECT_ID_MASK_Buff,
			CL_FALSE,
			0,
			channel_OBJECT_ID_MASK_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_OBJECT_ID_MASKs[0]->GetPixels());
	}
	if (channel_BY_OBJECT_ID_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_BY_OBJECT_ID_Buff,
			CL_FALSE,
			0,
			channel_BY_OBJECT_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_BY_OBJECT_IDs[0]->GetPixels());
	}
	if (channel_SAMPLECOUNT_Buff) {
		oclQueue.enqueueReadBuffer(
			*channel_SAMPLECOUNT_Buff,
			CL_FALSE,
			0,
			channel_SAMPLECOUNT_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_SAMPLECOUNT->GetPixels());
	}
	if (channel_CONVERGENCE_Buff) {
		// This may look wrong but CONVERGENCE channel is compute by the engine
		// film convergence test on the CPU so I write instead of read (to
		// synchronize the content).
		oclQueue.enqueueWriteBuffer(
			*channel_CONVERGENCE_Buff,
			CL_FALSE,
			0,
			channel_CONVERGENCE_Buff->getInfo<CL_MEM_SIZE>(),
			engineFilm->channel_CONVERGENCE->GetPixels());
	}
}

void PathOCLBaseOCLRenderThread::ThreadFilm::SendFilm(cl::CommandQueue &oclQueue) {
	// Async. transfer of the Film buffers

	for (u_int i = 0; i < channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff.size(); ++i) {
		if (channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]) {
			oclQueue.enqueueWriteBuffer(
				*(channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]),
				CL_FALSE,
				0,
				channel_RADIANCE_PER_PIXEL_NORMALIZEDs_Buff[i]->getInfo<CL_MEM_SIZE>(),
				film->channel_RADIANCE_PER_PIXEL_NORMALIZEDs[i]->GetPixels());
		}
	}

	if (channel_ALPHA_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_ALPHA_Buff,
			CL_FALSE,
			0,
			channel_ALPHA_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_ALPHA->GetPixels());
	}
	if (channel_DEPTH_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_DEPTH_Buff,
			CL_FALSE,
			0,
			channel_DEPTH_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_DEPTH->GetPixels());
	}
	if (channel_POSITION_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_POSITION_Buff,
			CL_FALSE,
			0,
			channel_POSITION_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_POSITION->GetPixels());
	}
	if (channel_GEOMETRY_NORMAL_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_GEOMETRY_NORMAL_Buff,
			CL_FALSE,
			0,
			channel_GEOMETRY_NORMAL_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_GEOMETRY_NORMAL->GetPixels());
	}
	if (channel_SHADING_NORMAL_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_SHADING_NORMAL_Buff,
			CL_FALSE,
			0,
			channel_SHADING_NORMAL_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_SHADING_NORMAL->GetPixels());
	}
	if (channel_MATERIAL_ID_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_MATERIAL_ID_Buff,
			CL_FALSE,
			0,
			channel_MATERIAL_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_MATERIAL_ID->GetPixels());
	}
	if (channel_DIRECT_DIFFUSE_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_DIRECT_DIFFUSE_Buff,
			CL_FALSE,
			0,
			channel_DIRECT_DIFFUSE_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_DIRECT_DIFFUSE->GetPixels());
	}
	if (channel_MATERIAL_ID_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_MATERIAL_ID_Buff,
			CL_FALSE,
			0,
			channel_MATERIAL_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_MATERIAL_ID->GetPixels());
	}
	if (channel_DIRECT_GLOSSY_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_DIRECT_GLOSSY_Buff,
			CL_FALSE,
			0,
			channel_DIRECT_GLOSSY_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_DIRECT_GLOSSY->GetPixels());
	}
	if (channel_EMISSION_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_EMISSION_Buff,
			CL_FALSE,
			0,
			channel_EMISSION_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_EMISSION->GetPixels());
	}
	if (channel_INDIRECT_DIFFUSE_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_INDIRECT_DIFFUSE_Buff,
			CL_FALSE,
			0,
			channel_INDIRECT_DIFFUSE_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_INDIRECT_DIFFUSE->GetPixels());
	}
	if (channel_INDIRECT_GLOSSY_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_INDIRECT_GLOSSY_Buff,
			CL_FALSE,
			0,
			channel_INDIRECT_GLOSSY_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_INDIRECT_GLOSSY->GetPixels());
	}
	if (channel_INDIRECT_SPECULAR_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_INDIRECT_SPECULAR_Buff,
			CL_FALSE,
			0,
			channel_INDIRECT_SPECULAR_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_INDIRECT_SPECULAR->GetPixels());
	}
	if (channel_MATERIAL_ID_MASK_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_MATERIAL_ID_MASK_Buff,
			CL_FALSE,
			0,
			channel_MATERIAL_ID_MASK_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_MATERIAL_ID_MASKs[0]->GetPixels());
	}
	if (channel_DIRECT_SHADOW_MASK_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_DIRECT_SHADOW_MASK_Buff,
			CL_FALSE,
			0,
			channel_DIRECT_SHADOW_MASK_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_DIRECT_SHADOW_MASK->GetPixels());
	}
	if (channel_INDIRECT_SHADOW_MASK_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_INDIRECT_SHADOW_MASK_Buff,
			CL_FALSE,
			0,
			channel_INDIRECT_SHADOW_MASK_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_INDIRECT_SHADOW_MASK->GetPixels());
	}
	if (channel_UV_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_UV_Buff,
			CL_FALSE,
			0,
			channel_UV_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_UV->GetPixels());
	}
	if (channel_RAYCOUNT_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_RAYCOUNT_Buff,
			CL_FALSE,
			0,
			channel_RAYCOUNT_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_RAYCOUNT->GetPixels());
	}
	if (channel_BY_MATERIAL_ID_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_BY_MATERIAL_ID_Buff,
			CL_FALSE,
			0,
			channel_BY_MATERIAL_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_BY_MATERIAL_IDs[0]->GetPixels());
	}
	if (channel_IRRADIANCE_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_IRRADIANCE_Buff,
			CL_FALSE,
			0,
			channel_IRRADIANCE_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_IRRADIANCE->GetPixels());
	}
	if (channel_OBJECT_ID_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_OBJECT_ID_Buff,
			CL_FALSE,
			0,
			channel_OBJECT_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_OBJECT_ID->GetPixels());
	}
	if (channel_OBJECT_ID_MASK_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_OBJECT_ID_MASK_Buff,
			CL_FALSE,
			0,
			channel_OBJECT_ID_MASK_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_OBJECT_ID_MASKs[0]->GetPixels());
	}
	if (channel_BY_OBJECT_ID_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_BY_OBJECT_ID_Buff,
			CL_FALSE,
			0,
			channel_BY_OBJECT_ID_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_BY_OBJECT_IDs[0]->GetPixels());
	}
	if (channel_SAMPLECOUNT_Buff) {
		oclQueue.enqueueWriteBuffer(
			*channel_SAMPLECOUNT_Buff,
			CL_FALSE,
			0,
			channel_SAMPLECOUNT_Buff->getInfo<CL_MEM_SIZE>(),
			film->channel_SAMPLECOUNT->GetPixels());
	}
	if (channel_CONVERGENCE_Buff) {
		// The CONVERGENCE channel is compute by the engine
		// film convergence test on the CPU so I write the engine film, not the
		// thread film.
		oclQueue.enqueueWriteBuffer(
			*channel_CONVERGENCE_Buff,
			CL_FALSE,
			0,
			channel_CONVERGENCE_Buff->getInfo<CL_MEM_SIZE>(),
			engineFilm->channel_CONVERGENCE->GetPixels());
	}
}

void PathOCLBaseOCLRenderThread::ThreadFilm::ClearFilm(cl::CommandQueue &oclQueue,
		cl::Kernel &filmClearKernel, const size_t filmClearWorkGroupSize) {
	// Set kernel arguments
	SetFilmKernelArgs(filmClearKernel, 0);
	
	// Clear the film
	const u_int filmPixelCount = film->GetWidth() * film->GetHeight();
	oclQueue.enqueueNDRangeKernel(filmClearKernel, cl::NullRange,
			cl::NDRange(RoundUp<u_int>(filmPixelCount, filmClearWorkGroupSize)),
			cl::NDRange(filmClearWorkGroupSize));
}

#endif
