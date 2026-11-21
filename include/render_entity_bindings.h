#pragma once

#include "shader_uniform_variable.h"

namespace renderer
{
	struct alignas(16) EntityBindings
	{
		const ShaderUniformVariable* transformUniform		{ nullptr };
		const ShaderUniformVariable* uvUniform				{ nullptr };
		const ShaderUniformVariable* materialColorUniform	{ nullptr };
		const ShaderUniformVariable* tileSizeUniform		{ nullptr };
		const ShaderUniformVariable* layerUniform			{ nullptr };
		const ShaderUniformVariable* placeholder			{ nullptr };
	};

	inline constexpr bool isValid(const EntityBindings* bindings)
	{
		return
			bindings->transformUniform &&
			bindings->uvUniform &&
			bindings->materialColorUniform &&
			bindings->tileSizeUniform &&
			bindings->layerUniform;
	}
}
