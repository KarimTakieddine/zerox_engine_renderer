#pragma once

#include "shader_uniform_variable.h"

namespace renderer
{
	struct alignas(16) ShaderProgram
	{
		enum class LinkStatus : uint64_t
		{
			UNLINKED	= 0,
			LINKED		= 1
		};

		LinkStatus linkStatus			{ LinkStatus::UNLINKED };
		const size_t* shaderIndices		{ nullptr };
		size_t shaderCount				{ 0 };
		ShaderUniformVariable* uniforms	{ nullptr };
		size_t uniformCount				{ 0 };
		unsigned int id					{ 0 };
		unsigned int refCount			{ 0 };
	};

	inline constexpr bool isActive(const ShaderProgram* shaderProgram)
	{
		return shaderProgram->id != 0;
	}
}
