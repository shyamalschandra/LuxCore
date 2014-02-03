#include <string>
namespace slg { namespace ocl {
std::string KernelSource_texture_blender_funcs = 
"#line 2 \"texture_blender_funcs.cl\"\n"
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
"#ifndef TEXTURE_STACK_SIZE\n"
"#define TEXTURE_STACK_SIZE 16\n"
"#endif\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Wood texture\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined (PARAM_ENABLE_WOOD)\n"
"\n"
"float WoodTexture_Evaluate(__global Texture *texture, __global HitPoint *hitPoint){\n"
"	const float3 P = TextureMapping3D_Map(&texture->wood.mapping, hitPoint);\n"
"	float scale = 1.f;\n"
"	if(fabs(texture->wood.noisesize) > 0.00001f) scale = (1.f/texture->wood.noisesize);\n"
"\n"
"	const NoiseBase noise = texture->wood.noisebasis2;\n"
"	float wood = 0.0f;\n"
"\n"
"	switch(texture->wood.type) {\n"
"		default:\n"
"		case BANDS:\n"
"			if(noise == TEX_SIN) {\n"
"				wood = tex_sin((P.x + P.y + P.z) * 10.f);\n"
"			} else if(noise == TEX_SAW) {\n"
"				wood = tex_saw((P.x + P.y + P.z) * 10.f);\n"
"			} else {\n"
"				wood = tex_tri((P.x + P.y + P.z) * 10.f);\n"
"			}\n"
"			break;\n"
"		case RINGS:\n"
"			if(noise == TEX_SIN) {\n"
"				wood = tex_sin(sqrt(P.x*P.x + P.y*P.y + P.z*P.z) * 20.f);\n"
"			} else if(noise == TEX_SAW) {\n"
"				wood = tex_saw(sqrt(P.x*P.x + P.y*P.y + P.z*P.z) * 20.f);\n"
"			} else {\n"
"				wood = tex_tri(sqrt(P.x*P.x + P.y*P.y + P.z*P.z) * 20.f);\n"
"			}\n"
"			break;\n"
"		case BANDNOISE:			\n"
"			if(texture->wood.hard)	\n"
"				wood = texture->wood.turbulence * fabs(2.f * Noise3(scale*P) - 1.f);\n"
"			else\n"
"				wood = texture->wood.turbulence * Noise3(scale*P);\n"
"\n"
"			if(noise == TEX_SIN) {\n"
"				wood = tex_sin((P.x + P.y + P.z) * 10.f + wood);\n"
"			} else if(noise == TEX_SAW) {\n"
"				wood = tex_saw((P.x + P.y + P.z) * 10.f + wood);\n"
"			} else {\n"
"				wood = tex_tri((P.x + P.y + P.z) * 10.f + wood);\n"
"			}\n"
"			break;\n"
"		case RINGNOISE:\n"
"			if(texture->wood.hard)	\n"
"				wood = texture->wood.turbulence * fabs(2.f * Noise3(scale*P) - 1.f);\n"
"			else\n"
"				wood = texture->wood.turbulence * Noise3(scale*P);\n"
"\n"
"			if(noise == TEX_SIN) {\n"
"				wood = tex_sin(sqrt(P.x*P.x + P.y*P.y + P.z*P.z) * 20.f + wood);\n"
"			} else if(noise == TEX_SAW) {\n"
"				wood = tex_saw(sqrt(P.x*P.x + P.y*P.y + P.z*P.z) * 20.f + wood);\n"
"			} else {\n"
"				wood = tex_tri(sqrt(P.x*P.x + P.y*P.y + P.z*P.z) * 20.f + wood);\n"
"			}\n"
"			break;\n"
"	}\n"
"	wood = (wood - 0.5f) * texture->wood.contrast + texture->wood.bright - 0.5f;\n"
"	if(wood < 0.f) wood = 0.f;\n"
"	else if(wood > 1.f) wood = 1.f;\n"
"\n"
"	return wood;\n"
"}\n"
"\n"
"void WoodTexture_EvaluateFloat(__global Texture *texture, __global HitPoint *hitPoint,\n"
"		float texValues[TEXTURE_STACK_SIZE], uint *texValuesSize) {\n"
"	texValues[(*texValuesSize)++] = WoodTexture_Evaluate(texture, hitPoint);\n"
"}\n"
"\n"
"void WoodTexture_EvaluateSpectrum(__global Texture *texture, __global HitPoint *hitPoint,\n"
"	float3 texValues[TEXTURE_STACK_SIZE], uint *texValuesSize) {\n"
"    float wood = WoodTexture_Evaluate(texture, hitPoint);\n"
"\n"
"    texValues[(*texValuesSize)++] = (float3)(wood, wood, wood);\n"
"}\n"
"\n"
"void WoodTexture_EvaluateDuDv(__global Texture *texture, __global HitPoint *hitPoint,\n"
"		float2 texValues[TEXTURE_STACK_SIZE], uint *texValuesSize) {\n"
"	texValues[(*texValuesSize)++] = (float2)(DUDV_VALUE, DUDV_VALUE);\n"
"}\n"
"\n"
"#endif\n"
; } }
