#pragma once

#include "texture_data.h"

namespace renderer
{
	struct TextureArray
	{
		TextureData data				{ };
		const char* const* imageFiles	{ nullptr };
		size_t imageCount				{ 0 };
	};
}
