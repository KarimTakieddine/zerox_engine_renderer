#pragma once

#include <cstdint>

namespace renderer
{
	constexpr size_t MAX_UNIFORM_NAME_LENGTH = 1 << 4;

	enum class GPUDataType : int16_t
	{
		UNKNOWN				= 0,
		INT					= 1,
		UINT				= 2,
		FLOAT				= 3,
		UVEC_2				= 4,
		VEC_2				= 5,
		VEC_3				= 6,
		VEC_4				= 7,
		MAT_4				= 8,
		SAMPLER_2D			= 9,
		SAMPLER_2D_ARRAY	= 10
	};

	struct alignas(16) ShaderUniformVariable
	{
		const char* name	{ nullptr };
		unsigned int count	{ 1 };
		GPUDataType type	{ GPUDataType::UNKNOWN };
		int16_t location	{ -1 };
	};
}
