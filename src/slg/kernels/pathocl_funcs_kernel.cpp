#include <string>
namespace slg { namespace ocl {
std::string KernelSource_pathocl_funcs = 
"#line 2 \"patchocl_funcs.cl\"\n"
"\n"
"/***************************************************************************\n"
" * Copyright 1998-2013 by authors (see AUTHORS.txt)                        *\n"
" *                                                                         *\n"
" *   This file is part of LuxRender.                                       *\n"
" *                                                                         *\n"
" * Licensed under the Apache License, Version 2.0 (the \"License\");         *\n"
" * you may not use this file except in compliance with the License.        *\n"
" * You may obtain a copy of the License at                                 *\n"
" *                                                                         *\n"
" *     http://www.apache.org/licenses/LICENSE-2.0                          *\n"
" *                                                                         *\n"
" * Unless required by applicable law or agreed to in writing, software     *\n"
" * distributed under the License is distributed on an \"AS IS\" BASIS,       *\n"
" * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*\n"
" * See the License for the specific language governing permissions and     *\n"
" * limitations under the License.                                          *\n"
" ***************************************************************************/\n"
"\n"
"// List of symbols defined at compile time:\n"
"//  PARAM_MAX_PATH_DEPTH\n"
"//  PARAM_RR_DEPTH\n"
"//  PARAM_RR_CAP\n"
"\n"
"// (optional)\n"
"//  PARAM_IMAGE_FILTER_TYPE (0 = No filter, 1 = Box, 2 = Gaussian, 3 = Mitchell, 4 = Blackman-Harris)\n"
"//  PARAM_IMAGE_FILTER_WIDTH_X\n"
"//  PARAM_IMAGE_FILTER_WIDTH_Y\n"
"//  PARAM_IMAGE_FILTER_PIXEL_WIDTH_X\n"
"//  PARAM_IMAGE_FILTER_PIXEL_WIDTH_Y\n"
"// (Box filter)\n"
"// (Gaussian filter)\n"
"//  PARAM_IMAGE_FILTER_GAUSSIAN_ALPHA\n"
"// (Mitchell filter)\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_B\n"
"//  PARAM_IMAGE_FILTER_MITCHELL_C\n"
"\n"
"// (optional)\n"
"//  PARAM_SAMPLER_TYPE (0 = Inlined Random, 1 = Metropolis, 2 = Sobol)\n"
"// (Metropolis)\n"
"//  PARAM_SAMPLER_METROPOLIS_LARGE_STEP_RATE\n"
"//  PARAM_SAMPLER_METROPOLIS_MAX_CONSECUTIVE_REJECT\n"
"//  PARAM_SAMPLER_METROPOLIS_IMAGE_MUTATION_RANGE\n"
"// (Sobol)\n"
"//  PARAM_SAMPLER_SOBOL_STARTOFFSET\n"
"//  PARAM_SAMPLER_SOBOL_MAXDEPTH\n"
"\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Init Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"void GenerateCameraPath(\n"
"		__global GPUTaskDirectLight *taskDirectLight,\n"
"		__global GPUTaskState *taskState,\n"
"		__global Sample *sample,\n"
"		__global float *sampleData,\n"
"		__global Camera *camera,\n"
"		const uint filmWidth,\n"
"		const uint filmHeight,\n"
"		__global Ray *ray,\n"
"		Seed *seed) {\n"
"#if (PARAM_SAMPLER_TYPE == 0)\n"
"	const float time = Rnd_FloatValue(seed);\n"
"\n"
"#if defined(PARAM_CAMERA_HAS_DOF)\n"
"	const float dofSampleX = Rnd_FloatValue(seed);\n"
"	const float dofSampleY = Rnd_FloatValue(seed);\n"
"#endif\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"	const float eyePassThrough = Rnd_FloatValue(seed);\n"
"#endif\n"
"#endif\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 1)\n"
"	__global float *sampleDataPathBase = Sampler_GetSampleDataPathBase(sample, sampleData);\n"
"	\n"
"	const float time = Sampler_GetSamplePath(IDX_EYE_TIME);\n"
"\n"
"#if defined(PARAM_CAMERA_HAS_DOF)\n"
"	const float dofSampleX = Sampler_GetSamplePath(IDX_DOF_X);\n"
"	const float dofSampleY = Sampler_GetSamplePath(IDX_DOF_Y);\n"
"#endif\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"	const float eyePassThrough = Sampler_GetSamplePath(IDX_EYE_PASSTHROUGH);\n"
"#endif\n"
"#endif\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 2)\n"
"	const float time = Sampler_GetSamplePath(IDX_EYE_TIME);\n"
"\n"
"#if defined(PARAM_CAMERA_HAS_DOF)\n"
"	const float dofSampleX = Sampler_GetSamplePath(IDX_DOF_X);\n"
"	const float dofSampleY = Sampler_GetSamplePath(IDX_DOF_Y);\n"
"#endif\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"	const float eyePassThrough = Sampler_GetSamplePath(IDX_EYE_PASSTHROUGH);\n"
"#endif\n"
"#endif\n"
"\n"
"	Camera_GenerateRay(camera, filmWidth, filmHeight, ray,\n"
"			sample->result.filmX, sample->result.filmY, time\n"
"#if defined(PARAM_CAMERA_HAS_DOF)\n"
"			, dofSampleX, dofSampleY\n"
"#endif\n"
"			);\n"
"\n"
"	// Initialize the path state\n"
"	taskState->state = RT_NEXT_VERTEX; // Or MK_RT_NEXT_VERTEX (they have the same value)\n"
"	taskState->depth = 1;\n"
"	VSTORE3F(WHITE, taskState->throughput.c);\n"
"	taskDirectLight->lastBSDFEvent = SPECULAR; // SPECULAR is required to avoid MIS\n"
"	taskDirectLight->lastPdfW = 1.f;\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"	// This is a bit tricky. I store the passThroughEvent in the BSDF\n"
"	// before of the initialization because it can be used during the\n"
"	// tracing of next path vertex ray.\n"
"\n"
"	taskState->bsdf.hitPoint.passThroughEvent = eyePassThrough;\n"
"#endif\n"
"\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"	sample->result.directShadowMask = 1.f;\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"	sample->result.indirectShadowMask = 1.f;\n"
"#endif\n"
"\n"
"	sample->result.lastPathVertex = (PARAM_MAX_PATH_DEPTH == 1);\n"
"}\n"
"\n"
"__kernel __attribute__((work_group_size_hint(64, 1, 1))) void Init(\n"
"		uint seedBase,\n"
"		__global GPUTask *tasks,\n"
"		__global GPUTaskDirectLight *tasksDirectLight,\n"
"		__global GPUTaskState *tasksState,\n"
"		__global GPUTaskStats *taskStats,\n"
"		__global Sample *samples,\n"
"		__global float *samplesData,\n"
"#if defined(PARAM_HAS_VOLUMES)\n"
"		__global PathVolumeInfo *pathVolInfos,\n"
"#endif\n"
"		__global Ray *rays,\n"
"		__global Camera *camera,\n"
"		const uint filmWidth,\n"
"		const uint filmHeight\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= PARAM_TASK_COUNT)\n"
"		return;\n"
"\n"
"	// Initialize the task\n"
"	__global GPUTask *task = &tasks[gid];\n"
"	__global GPUTaskDirectLight *taskDirectLight = &tasksDirectLight[gid];\n"
"	__global GPUTaskState *taskState = &tasksState[gid];\n"
"\n"
"	// Initialize random number generator\n"
"	Seed seed;\n"
"	Rnd_Init(seedBase + gid, &seed);\n"
"\n"
"	// Initialize the sample and path\n"
"	__global Sample *sample = &samples[gid];\n"
"	__global float *sampleData = Sampler_GetSampleData(sample, samplesData);\n"
"	Sampler_Init(&seed, sample, sampleData, filmWidth, filmHeight);\n"
"	GenerateCameraPath(taskDirectLight, taskState, sample, sampleData, camera, filmWidth, filmHeight, &rays[gid], &seed);\n"
"\n"
"	// Save the seed\n"
"	task->seed.s1 = seed.s1;\n"
"	task->seed.s2 = seed.s2;\n"
"	task->seed.s3 = seed.s3;\n"
"\n"
"	__global GPUTaskStats *taskStat = &taskStats[gid];\n"
"	taskStat->sampleCount = 0;\n"
"\n"
"#if defined(PARAM_HAS_VOLUMES)\n"
"	PathVolumeInfo_Init(&pathVolInfos[gid]);\n"
"#endif\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Utility functions\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined(PARAM_HAS_ENVLIGHTS)\n"
"void DirectHitInfiniteLight(\n"
"		const BSDFEvent lastBSDFEvent,\n"
"		__global const Spectrum *pathThroughput,\n"
"		const float3 eyeDir, const float lastPdfW,\n"
"		__global SampleResult *sampleResult\n"
"		LIGHTS_PARAM_DECL) {\n"
"	const float3 throughput = VLOAD3F(pathThroughput->c);\n"
"\n"
"	for (uint i = 0; i < envLightCount; ++i) {\n"
"		__global LightSource *light = &lights[envLightIndices[i]];\n"
"\n"
"		float directPdfW;\n"
"		const float3 lightRadiance = EnvLight_GetRadiance(light, -eyeDir, &directPdfW\n"
"				LIGHTS_PARAM);\n"
"\n"
"		if (!Spectrum_IsBlack(lightRadiance)) {\n"
"			// MIS between BSDF sampling and direct light sampling\n"
"			const float weight = ((lastBSDFEvent & SPECULAR) ? 1.f : PowerHeuristic(lastPdfW, directPdfW));\n"
"			const float3 radiance = weight * throughput * lightRadiance;\n"
"\n"
"			SampleResult_AddEmission(sampleResult, light->lightID, radiance);\n"
"		}\n"
"	}\n"
"}\n"
"#endif\n"
"\n"
"#if (PARAM_TRIANGLE_LIGHT_COUNT > 0)\n"
"void DirectHitFiniteLight(\n"
"		const BSDFEvent lastBSDFEvent,\n"
"		__global const Spectrum *pathThroughput, const float distance, __global BSDF *bsdf,\n"
"		const float lastPdfW, __global SampleResult *sampleResult\n"
"		LIGHTS_PARAM_DECL) {\n"
"	float directPdfA;\n"
"	const float3 emittedRadiance = BSDF_GetEmittedRadiance(bsdf, &directPdfA\n"
"			LIGHTS_PARAM);\n"
"\n"
"	if (!Spectrum_IsBlack(emittedRadiance)) {\n"
"		// Add emitted radiance\n"
"		float weight = 1.f;\n"
"		if (!(lastBSDFEvent & SPECULAR)) {\n"
"			const float lightPickProb = Scene_SampleAllLightPdf(lightsDistribution,\n"
"					lights[bsdf->triangleLightSourceIndex].lightSceneIndex);\n"
"			const float directPdfW = PdfAtoW(directPdfA, distance,\n"
"				fabs(dot(VLOAD3F(&bsdf->hitPoint.fixedDir.x), VLOAD3F(&bsdf->hitPoint.shadeN.x))));\n"
"\n"
"			// MIS between BSDF sampling and direct light sampling\n"
"			weight = PowerHeuristic(lastPdfW, directPdfW * lightPickProb);\n"
"		}\n"
"		const float3 lightRadiance = weight * VLOAD3F(pathThroughput->c) * emittedRadiance;\n"
"\n"
"		SampleResult_AddEmission(sampleResult, BSDF_GetLightID(bsdf\n"
"				MATERIALS_PARAM), lightRadiance);\n"
"	}\n"
"}\n"
"#endif\n"
"\n"
"float RussianRouletteProb(const float3 color) {\n"
"	return clamp(Spectrum_Filter(color), PARAM_RR_CAP, 1.f);\n"
"}\n"
"\n"
"__global LightSource * DirectLight_Illuminate(\n"
"#if defined(PARAM_HAS_INFINITELIGHTS)\n"
"		const float worldCenterX,\n"
"		const float worldCenterY,\n"
"		const float worldCenterZ,\n"
"		const float worldRadius,\n"
"#endif\n"
"#if (PARAM_TRIANGLE_LIGHT_COUNT > 0)\n"
"		__global HitPoint *tmpHitPoint,\n"
"#endif\n"
"		const float u0, const float u1, const float u2,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"		const float lightPassThroughEvent,\n"
"#endif\n"
"		const float3 point,\n"
"		float *lightPickPdf, float3 *lightRayDir,\n"
"		float *distance, float *directPdfW,\n"
"		float3 *lightRadiance\n"
"		LIGHTS_PARAM_DECL) {\n"
"	// Pick a light source to sample\n"
"	const uint lightIndex = Scene_SampleAllLights(lightsDistribution, u0, lightPickPdf);\n"
"	__global LightSource *light = &lights[lightIndex];\n"
"\n"
"	\n"
"	// Illuminate the point\n"
"	*lightRadiance = Light_Illuminate(\n"
"			&lights[lightIndex],\n"
"			point,\n"
"			u1, u2,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"			lightPassThroughEvent,\n"
"#endif\n"
"#if defined(PARAM_HAS_INFINITELIGHTS)\n"
"			worldCenterX, worldCenterY, worldCenterZ, worldRadius,\n"
"#endif\n"
"#if (PARAM_TRIANGLE_LIGHT_COUNT > 0)\n"
"			tmpHitPoint,\n"
"#endif		\n"
"			lightRayDir, distance, directPdfW\n"
"			LIGHTS_PARAM);\n"
"	\n"
"	return light;\n"
"}\n"
"\n"
"bool DirectLight_BSDFSampling(\n"
"		__global LightSource *light,\n"
"		const float3 lightRadiance, const float lightPickPdf, const float3 lightRayDir,\n"
"		const float distance, const float directPdfW,\n"
"		const float time,\n"
"		const bool lastPathVertex, const uint depth,\n"
"		__global const Spectrum *pathThroughput, __global BSDF *bsdf,\n"
"		__global Ray *shadowRay, __global Spectrum *radiance, __global uint *ID\n"
"		MATERIALS_PARAM_DECL) {\n"
"	// Setup the shadow ray\n"
"	BSDFEvent event;\n"
"	float bsdfPdfW;\n"
"	const float3 bsdfEval = BSDF_Evaluate(bsdf,\n"
"			lightRayDir, &event, &bsdfPdfW\n"
"			MATERIALS_PARAM);\n"
"\n"
"	if (Spectrum_IsBlack(bsdfEval))\n"
"		return false;\n"
"\n"
"	const float cosThetaToLight = fabs(dot(lightRayDir, VLOAD3F(&bsdf->hitPoint.shadeN.x)));\n"
"	const float directLightSamplingPdfW = directPdfW * lightPickPdf;\n"
"	const float factor = 1.f / directLightSamplingPdfW;\n"
"\n"
"	// Russian Roulette\n"
"	bsdfPdfW *= (depth >= PARAM_RR_DEPTH) ? RussianRouletteProb(bsdfEval) : 1.f;\n"
"\n"
"	// MIS between direct light sampling and BSDF sampling\n"
"	//\n"
"	// Note: I have to avoiding MIS on the last path vertex\n"
"	const float weight = (!lastPathVertex && Light_IsEnvOrIntersectable(light)) ?\n"
"		PowerHeuristic(directLightSamplingPdfW, bsdfPdfW) : 1.f;\n"
"\n"
"	VSTORE3F((weight * factor) * VLOAD3F(pathThroughput->c) * bsdfEval * lightRadiance, radiance->c);\n"
"	*ID = light->lightID;\n"
"\n"
"	// Setup the shadow ray\n"
"	const float3 hitPoint = VLOAD3F(&bsdf->hitPoint.p.x);\n"
"	const float epsilon = fmax(MachineEpsilon_E_Float3(hitPoint), MachineEpsilon_E(distance));\n"
"\n"
"	Ray_Init4(shadowRay, hitPoint, lightRayDir,\n"
"		epsilon,\n"
"		distance - epsilon, time);\n"
"\n"
"	return true;\n"
"}\n"
"\n"
"bool DirectLightSampling_ONE(\n"
"#if defined(PARAM_HAS_INFINITELIGHTS)\n"
"		const float worldCenterX,\n"
"		const float worldCenterY,\n"
"		const float worldCenterZ,\n"
"		const float worldRadius,\n"
"#endif\n"
"#if (PARAM_TRIANGLE_LIGHT_COUNT > 0)\n"
"		__global HitPoint *tmpHitPoint,\n"
"#endif\n"
"		const float time, const float u0, const float u1, const float u2,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"		const float lightPassThroughEvent,\n"
"#endif\n"
"		const bool lastPathVertex, const uint depth,\n"
"		__global const Spectrum *pathThroughput, __global BSDF *bsdf,\n"
"		__global Ray *shadowRay, __global Spectrum *radiance, __global uint *ID\n"
"		LIGHTS_PARAM_DECL) {\n"
"	float lightPickPdf, distance, directPdfW;\n"
"	float3 lightRayDir, lightRadiance;\n"
"\n"
"	__global LightSource *light = DirectLight_Illuminate(\n"
"#if defined(PARAM_HAS_INFINITELIGHTS)\n"
"		worldCenterX, worldCenterY, worldCenterZ, worldRadius,\n"
"#endif\n"
"#if (PARAM_TRIANGLE_LIGHT_COUNT > 0)\n"
"		tmpHitPoint,\n"
"#endif\n"
"		u0, u1, u2,\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"		lightPassThroughEvent,\n"
"#endif\n"
"		VLOAD3F(&bsdf->hitPoint.p.x),\n"
"		&lightPickPdf, &lightRayDir,\n"
"		&distance, &directPdfW,\n"
"		&lightRadiance\n"
"		LIGHTS_PARAM);\n"
"\n"
"	if (Spectrum_IsBlack(lightRadiance))\n"
"		return false;\n"
"\n"
"	return DirectLight_BSDFSampling(\n"
"			light, lightRadiance, lightPickPdf, lightRayDir,\n"
"			distance, directPdfW,\n"
"			time, lastPathVertex, depth,\n"
"			pathThroughput, bsdf,\n"
"			shadowRay, radiance, ID\n"
"			MATERIALS_PARAM);\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Kernel parameters\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined(PARAM_HAS_VOLUMES)\n"
"#define KERNEL_ARGS_VOLUMES \\\n"
"		, __global PathVolumeInfo *pathVolInfos \\\n"
"		, __global PathVolumeInfo *directLightVolInfos\n"
"#else\n"
"#define KERNEL_ARGS_VOLUMES\n"
"#endif\n"
"\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_0)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_0 \\\n"
"		, __global float *filmRadianceGroup0\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_0\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_1)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_1 \\\n"
"		, __global float *filmRadianceGroup1\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_1\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_2)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_2 \\\n"
"		, __global float *filmRadianceGroup2\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_2\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_3)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_3 \\\n"
"		, __global float *filmRadianceGroup3\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_3\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_4)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_4 \\\n"
"		, __global float *filmRadianceGroup4\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_4\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_5)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_5 \\\n"
"		, __global float *filmRadianceGroup5\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_5\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_6)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_6 \\\n"
"		, __global float *filmRadianceGroup6\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_6\n"
"#endif\n"
"#if defined(PARAM_FILM_RADIANCE_GROUP_7)\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_7 \\\n"
"		, __global float *filmRadianceGroup7\n"
"#else\n"
"#define KERNEL_ARGS_FILM_RADIANCE_GROUP_7\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_ALPHA)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_ALPHA \\\n"
"		, __global float *filmAlpha\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_ALPHA\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DEPTH)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DEPTH \\\n"
"		, __global float *filmDepth\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DEPTH\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_POSITION)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_POSITION \\\n"
"		, __global float *filmPosition\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_POSITION\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_GEOMETRY_NORMAL)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_GEOMETRY_NORMAL \\\n"
"		, __global float *filmGeometryNormal\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_GEOMETRY_NORMAL\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_SHADING_NORMAL)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_SHADING_NORMAL \\\n"
"		, __global float *filmShadingNormal\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_SHADING_NORMAL\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID \\\n"
"		, __global uint *filmMaterialID\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_DIFFUSE)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_DIFFUSE \\\n"
"		, __global float *filmDirectDiffuse\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_DIFFUSE\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_GLOSSY)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_GLOSSY \\\n"
"		, __global float *filmDirectGlossy\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_GLOSSY\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_EMISSION)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_EMISSION \\\n"
"		, __global float *filmEmission\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_EMISSION\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_DIFFUSE)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_DIFFUSE \\\n"
"		, __global float *filmIndirectDiffuse\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_DIFFUSE\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_GLOSSY)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_GLOSSY \\\n"
"		, __global float *filmIndirectGlossy\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_GLOSSY\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SPECULAR)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SPECULAR \\\n"
"		, __global float *filmIndirectSpecular\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SPECULAR\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_MATERIAL_ID_MASK)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_ID_MASK \\\n"
"		, __global float *filmMaterialIDMask\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_ID_MASK\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_DIRECT_SHADOW_MASK)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_SHADOW_MASK \\\n"
"		, __global float *filmDirectShadowMask\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_DIRECT_SHADOW_MASK\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_INDIRECT_SHADOW_MASK)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SHADOW_MASK \\\n"
"		, __global float *filmIndirectShadowMask\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SHADOW_MASK\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_UV)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_UV \\\n"
"		, __global float *filmUV\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_UV\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_RAYCOUNT)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_RAYCOUNT \\\n"
"		, __global float *filmRayCount\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_RAYCOUNT\n"
"#endif\n"
"#if defined(PARAM_FILM_CHANNELS_HAS_BY_MATERIAL_ID)\n"
"#define KERNEL_ARGS_FILM_CHANNELS_BY_MATERIAL_ID \\\n"
"		, __global float *filmByMaterialID\n"
"#else\n"
"#define KERNEL_ARGS_FILM_CHANNELS_BY_MATERIAL_ID\n"
"#endif\n"
"\n"
"#define KERNEL_ARGS_FILM \\\n"
"		, const uint filmWidth, const uint filmHeight \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_0 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_1 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_2 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_3 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_4 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_5 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_6 \\\n"
"		KERNEL_ARGS_FILM_RADIANCE_GROUP_7 \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_ALPHA \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_DEPTH \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_POSITION \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_GEOMETRY_NORMAL \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_SHADING_NORMAL \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_MATERIAL_ID \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_DIRECT_DIFFUSE \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_DIRECT_GLOSSY \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_EMISSION \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_INDIRECT_DIFFUSE \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_INDIRECT_GLOSSY \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SPECULAR \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_ID_MASK \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_DIRECT_SHADOW_MASK \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_INDIRECT_SHADOW_MASK \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_UV \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_RAYCOUNT \\\n"
"		KERNEL_ARGS_FILM_CHANNELS_BY_MATERIAL_ID\n"
"\n"
"#if defined(PARAM_HAS_INFINITELIGHTS)\n"
"#define KERNEL_ARGS_INFINITELIGHTS \\\n"
"		, const float worldCenterX \\\n"
"		, const float worldCenterY \\\n"
"		, const float worldCenterZ \\\n"
"		, const float worldRadius\n"
"#else\n"
"#define KERNEL_ARGS_INFINITELIGHTS\n"
"#endif\n"
"\n"
"#if defined(PARAM_HAS_NORMALS_BUFFER)\n"
"#define KERNEL_ARGS_NORMALS_BUFFER \\\n"
"		, __global Vector *vertNormals\n"
"#else\n"
"#define KERNEL_ARGS_NORMALS_BUFFER\n"
"#endif\n"
"#if defined(PARAM_HAS_UVS_BUFFER)\n"
"#define KERNEL_ARGS_UVS_BUFFER \\\n"
"		, __global UV *vertUVs\n"
"#else\n"
"#define KERNEL_ARGS_UVS_BUFFER\n"
"#endif\n"
"#if defined(PARAM_HAS_COLS_BUFFER)\n"
"#define KERNEL_ARGS_COLS_BUFFER \\\n"
"		, __global Spectrum *vertCols\n"
"#else\n"
"#define KERNEL_ARGS_COLS_BUFFER\n"
"#endif\n"
"#if defined(PARAM_HAS_ALPHAS_BUFFER)\n"
"#define KERNEL_ARGS_ALPHAS_BUFFER \\\n"
"		__global float *vertAlphas,\n"
"#else\n"
"#define KERNEL_ARGS_ALPHAS_BUFFER\n"
"#endif\n"
"\n"
"#if defined(PARAM_HAS_ENVLIGHTS)\n"
"#define KERNEL_ARGS_ENVLIGHTS \\\n"
"		, __global uint *envLightIndices \\\n"
"		, const uint envLightCount\n"
"#else\n"
"#define KERNEL_ARGS_ENVLIGHTS\n"
"#endif\n"
"#if defined(PARAM_HAS_INFINITELIGHT)\n"
"#define KERNEL_ARGS_INFINITELIGHT \\\n"
"		, __global float *infiniteLightDistribution\n"
"#else\n"
"#define KERNEL_ARGS_INFINITELIGHT\n"
"#endif\n"
"\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_0 \\\n"
"		, __global ImageMap *imageMapDescs, __global float *imageMapBuff0\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_0\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_1 \\\n"
"		, __global float *imageMapBuff1\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_1\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_2 \\\n"
"		, __global float *imageMapBuff2\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_2\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_3 \\\n"
"		, __global float *imageMapBuff3\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_3\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_4 \\\n"
"		, __global float *imageMapBuff4\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_4\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_5)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_5 \\\n"
"		, __global float *imageMapBuff5\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_5\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_6)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_6 \\\n"
"		, __global float *imageMapBuff6\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_6\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_7)\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_7 \\\n"
"		, __global float *imageMapBuff7\n"
"#else\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGE_7\n"
"#endif\n"
"#define KERNEL_ARGS_IMAGEMAPS_PAGES \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_0 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_1 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_2 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_3 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_4 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_5 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_6 \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGE_7\n"
"\n"
"#define KERNEL_ARGS \\\n"
"		__global GPUTask *tasks \\\n"
"		, __global GPUTaskDirectLight *tasksDirectLight \\\n"
"		, __global GPUTaskState *tasksState \\\n"
"		, __global GPUTaskStats *taskStats \\\n"
"		, __global Sample *samples \\\n"
"		, __global float *samplesData \\\n"
"		KERNEL_ARGS_VOLUMES \\\n"
"		, __global Ray *rays \\\n"
"		, __global RayHit *rayHits \\\n"
"		/* Film parameters */ \\\n"
"		KERNEL_ARGS_FILM \\\n"
"		/* Scene parameters */ \\\n"
"		KERNEL_ARGS_INFINITELIGHTS \\\n"
"		, __global Material *mats \\\n"
"		, __global Texture *texs \\\n"
"		, __global uint *meshMats \\\n"
"		, __global Mesh *meshDescs \\\n"
"		, __global Point *vertices \\\n"
"		KERNEL_ARGS_NORMALS_BUFFER \\\n"
"		KERNEL_ARGS_UVS_BUFFER \\\n"
"		KERNEL_ARGS_COLS_BUFFER \\\n"
"		KERNEL_ARGS_ALPHAS_BUFFER \\\n"
"		, __global Triangle *triangles \\\n"
"		, __global Camera *camera \\\n"
"		/* Lights */ \\\n"
"		, __global LightSource *lights \\\n"
"		KERNEL_ARGS_ENVLIGHTS \\\n"
"		, __global uint *meshTriLightDefsOffset \\\n"
"		KERNEL_ARGS_INFINITELIGHT \\\n"
"		, __global float *lightsDistribution \\\n"
"		/* Images */ \\\n"
"		KERNEL_ARGS_IMAGEMAPS_PAGES\n"
"\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// To initialize image maps page pointer table\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_0)\n"
"#define INIT_IMAGEMAPS_PAGE_0 imageMapBuff[0] = imageMapBuff0;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_0\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_1)\n"
"#define INIT_IMAGEMAPS_PAGE_1 imageMapBuff[1] = imageMapBuff1;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_1\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_2)\n"
"#define INIT_IMAGEMAPS_PAGE_2 imageMapBuff[2] = imageMapBuff2;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_3\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_3)\n"
"#define INIT_IMAGEMAPS_PAGE_3 imageMapBuff[3] = imageMapBuff3;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_3\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_4)\n"
"#define INIT_IMAGEMAPS_PAGE_4 imageMapBuff[4] = imageMapBuff4;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_4\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_5)\n"
"#define INIT_IMAGEMAPS_PAGE_5 imageMapBuff[5] = imageMapBuff5;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_5\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_6)\n"
"#define INIT_IMAGEMAPS_PAGE_6 imageMapBuff[6] = imageMapBuff6;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_6\n"
"#endif\n"
"#if defined(PARAM_IMAGEMAPS_PAGE_7)\n"
"#define INIT_IMAGEMAPS_PAGE_7 imageMapBuff[7] = imageMapBuff7;\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGE_7\n"
"#endif\n"
"\n"
"#if defined(PARAM_HAS_IMAGEMAPS)\n"
"#define INIT_IMAGEMAPS_PAGES \\\n"
"	__global float *imageMapBuff[PARAM_IMAGEMAPS_COUNT]; \\\n"
"	INIT_IMAGEMAPS_PAGE_0 \\\n"
"	INIT_IMAGEMAPS_PAGE_1 \\\n"
"	INIT_IMAGEMAPS_PAGE_2 \\\n"
"	INIT_IMAGEMAPS_PAGE_3 \\\n"
"	INIT_IMAGEMAPS_PAGE_4 \\\n"
"	INIT_IMAGEMAPS_PAGE_5 \\\n"
"	INIT_IMAGEMAPS_PAGE_6 \\\n"
"	INIT_IMAGEMAPS_PAGE_7\n"
"#else\n"
"#define INIT_IMAGEMAPS_PAGES\n"
"#endif\n"
"\n"
; } }