#pragma once

#include "shader_variable.h"

namespace renderer
{
	struct alignas(16) EntityBindings
	{
		const ShaderVariable* positionAttribute		{ nullptr };
		const ShaderVariable* colorAttribute		{ nullptr };
		const ShaderVariable* uvAttribute			{ nullptr };
		const ShaderVariable* transformUniform		{ nullptr };
		const ShaderVariable* uvUniform				{ nullptr };
		const ShaderVariable* materialColorUniform	{ nullptr };
		const ShaderVariable* tileSizeUniform		{ nullptr };
		const ShaderVariable* layerUniform			{ nullptr };
	};

	inline constexpr bool isValid(const EntityBindings* bindings)
	{
		return
			bindings->positionAttribute		&&
			bindings->colorAttribute		&&
			bindings->uvAttribute			&&
			bindings->transformUniform		&&
			bindings->uvUniform				&&
			bindings->materialColorUniform	&&
			bindings->tileSizeUniform		&&
			bindings->layerUniform;
	}
}
