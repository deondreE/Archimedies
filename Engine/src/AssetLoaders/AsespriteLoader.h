#pragma once
#include "archpch.h"

// Context:
// @See: aseprite/docs/ase-file-specs.md at main · aseprite/aseprite

namespace Engine {
	namespace Loaders {

		enum class AseColorMode : uint16_t 
		{
			Indexed = 0,
			Grayscale = 16,
			RGBA = 32
		};

		struct AsePalleteEntry
		{
			uint8_t R = 0;
			uint8_t G = 0;
			uint8_t B = 0;
			uint8_t A = 255;
		};

		struct AseLayer
		{
			uint16_t Flags = 0;
			uint16_t Type = 0;
			uint16_t ChildLevel = 0;
			uint16_t BlendMode = 0;
			uint8_t Opacity = 255;

			std::string Name;
		};

		struct AseCel
		{
			uint16_t LayerIndex = 0;

			uint16_t X = 0;
			uint16_t Y = 0;

			uint8_t Opacity = 255;

			uint16_t Width = 0;
			uint16_t Height = 0;

			std::vector<uint8_t> Pixels; // RGBA8
		};

		struct AseFrame
		{
			uint16_t DurationMS = 0;

			std::vector<AseCel> Cels;

			// Final composited image for this frame.
			std::vector<uint8_t> CompositePixels;
		};

		struct AseSprite 
		{
			uint32_t Width = 0;
			uint32_t Height = 0;

			AseColorMode ColorMode = AseColorMode::RGBA;

			std::vector<AsePalleteEntry> Palette;
			std::vector<AseLayer> Layers;
			std::vector<AseFrame> Frames;
		};

		class AsepriteLoader
		{
		public:
			// @TODO: should be filepath maybe ?
			static std::shared_ptr<AseSprite> Load(const std::string& path);

		private:

			template<typename T>
			static T Read(std::istream& stream);

			static std::string ReadString(std::istream& stream);

			static void ReadFrame(
				std::istream& stream,
				AseSprite& sprite,
				AseFrame& frame);

			static void ReadLayerChunk(
				std::istream& stream,
				uint32_t chunkSize,
				AseSprite& sprite);

			static void ReadCelChunk(
				std::istream& stream,
				uint32_t chunkSize,
				AseSprite& sprite,
				AseFrame& frame);

			static std::vector<uint8_t> Inflate(
				const uint8_t* compressedData,
				size_t compressedSize,
				size_t expectedSize
			);

			static void ReadPaletteChunk(std::istream& stream,
				uint32_t chunkSize,
				AseSprite& sprite);

			static void CompositeFrame(const AseSprite& sprite, AseFrame& frame);
		};

	}
}