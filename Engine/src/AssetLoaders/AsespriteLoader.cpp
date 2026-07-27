#include "archpch.h"
#include "AsespriteLoader.h"

namespace Engine {
	namespace Loaders {

		namespace
		{
			constexpr uint16_t ASE_MAGIC = 0xA5E0;

			constexpr uint16_t CHUNK_LAYER = 0x2004;
			constexpr uint16_t CHUNK_CEL = 0x2005;
			constexpr uint16_t CHUNK_PALLETE = 0x2019;
		}

		template<typename T>
		T AsepriteLoader::Read(std::istream& stream) {
			T value{};
			stream.read(reinterpret_cast<char*>(&value), sizeof(T));
			return value;
		}

		std::string AsepriteLoader::ReadString(std::istream& stream) {
			uint16_t len = Read<uint16_t>(stream);

			std::string str;
			str.resize(len);

			stream.read(str.data(), len);
			return str;
		}

		std::vector<uint8_t> AsepriteLoader::Inflate(
			const uint8_t* compressedData,
			size_t compressedSize,
			size_t expectedSize) {
			std::vector<uint8_t> output(expectedSize);

			uLongf destLen = static_cast<uLongf>(expectedSize);

			int result = uncompress(
				output.data(),
				&destLen,
				compressedData,
				static_cast<uLongf>(compressedSize));
			assert(result == Z_OK);

			output.resize(destLen);
			return output;
		}

		static void BlendPixel(uint8_t* dst, const uint8_t* src) {
			float sourceAlpha = src[3] / 255.0f;
			float destAlpha = dst[3] / 255.0f;

			float outA = sourceAlpha + destAlpha * (1.0 - sourceAlpha);
			if (outA <= 0.0f)
				return;

			for (int i = 0; i < 3; i++) {
				float s = src[i] / 255.0f;
				float d = dst[i] / 255.0f;

float out = (s * sourceAlpha + d * destAlpha * (1.0 - sourceAlpha)) / outA;

dst[i] = static_cast<uint8_t>(out * 255.0f);
			}
			dst[3] = static_cast<uint8_t>(outA * 255.0f);
		}

		void AsepriteLoader::CompositeFrame(
			const AseSprite& sprite,
			AseFrame& frame
		) {
			frame.CompositePixels.resize(
				sprite.Width *
				sprite.Height *
				4);
			std::fill(frame.CompositePixels.begin(), frame.CompositePixels.end(), 0);

			for (const auto& cel : frame.Cels) {
				for (uint32_t y = 0; y < cel.Height; y++) {
					for (uint32_t x = 0; x < cel.Width; x++) {
						int32_t dstX = cel.X + x;
						int32_t dstY = cel.Y + y;

						if (dstX < 0 || dstY < 0 || dstX >= (int32_t)sprite.Width || dstY >= (int32_t)sprite.Height) {
							continue;
						}

						uint32_t srcIndex = (y * cel.Width + x) * 4;
						uint32_t dstIndex = (dstY * sprite.Width + dstX) * 4;

						BlendPixel(&frame.CompositePixels[dstIndex], &cel.Pixels[srcIndex]);
					}
				}
			}
		}

		void AsepriteLoader::ReadLayerChunk(
			std::istream& stream,
			uint32_t chunkSize,
			AseSprite& sprite
		) {
			AseLayer layer{};

			layer.Flags = Read<uint16_t>(stream);
			layer.Type = Read<uint16_t>(stream);
			layer.ChildLevel = Read<uint16_t>(stream);

			stream.seekg(2, std::ios::cur); // width
			stream.seekg(2, std::ios::cur); // height

			layer.BlendMode = Read<uint16_t>(stream);
			layer.Opacity = Read<uint8_t>(stream);

			stream.seekg(3, std::ios::cur);

			layer.Name = ReadString(stream);
			sprite.Layers.push_back(std::move(layer));
		}

		void AsepriteLoader::ReadCelChunk(
			std::istream& stream,
			uint32_t chunkSize,
			AseSprite& sprite,
			AseFrame& frame)
		{
			AseCel cel{};

			cel.LayerIndex = Read<uint16_t>(stream);

			cel.X = Read<uint16_t>(stream);
			cel.Y = Read<uint16_t>(stream);

			cel.Opacity = Read<uint16_t>(stream);

			uint16_t celType = Read<uint16_t>(stream);

			if (celType != 2) {
				stream.seekg(chunkSize - 26, std::ios::cur);
				return;
			}
			cel.Width = Read<uint16_t>(stream);
			cel.Height = Read<uint16_t>(stream);

			const size_t pixelBytes = cel.Width * cel.Height * 4;
			const size_t headerBytes = 16 + 4;
			const size_t compressedSize = chunkSize - 6 - headerBytes;
			std::vector<uint8_t> compressed(compressedSize);

			stream.read(reinterpret_cast<char*>(compressed.data()), compressedSize);
			cel.Pixels = Inflate(
				compressed.data(),
				compressed.size(),
				pixelBytes);

			frame.Cels.push_back(
				std::move(cel));
		}

		void AsepriteLoader::ReadPaletteChunk(std::istream& stream,
			uint32_t chunkSize,
			AseSprite& sprite) {
			uint32_t paletteSize = Read<uint32_t>(stream);

			uint32_t first = Read<uint32_t>(stream);
			uint32_t last = Read<uint32_t>(stream);

			stream.seekg(8, std::ios::cur);

			sprite.Palette.resize(paletteSize);

			for (uint32_t i = first; i <= last; i++)
			{
				uint16_t flags = Read<uint16_t>(stream);

				auto& color = sprite.Palette[i];

				color.R = Read<uint8_t>(stream);
				color.G = Read<uint8_t>(stream);
				color.B = Read<uint8_t>(stream);
				color.A = Read<uint8_t>(stream);

				if (flags & 1)
					ReadString(stream);
			}
		}

		// Load
		std::shared_ptr<AseSprite> AsepriteLoader::Load(const std::string& path) {
			auto sprite = std::make_shared<AseSprite>();

			std::ifstream file(path, std::ios::binary);
			assert(file);

			uint32_t fileSize = Read<uint32_t>(file);
			uint16_t magic = Read<uint16_t>(file);
			assert(magic == ASE_MAGIC);
			
			uint16_t frameCount = Read<uint16_t>(file);

			sprite->Width = Read<uint16_t>(file);
			sprite->Height = Read<uint16_t>(file);

			sprite->ColorMode = static_cast<AseColorMode>(
				Read<uint16_t>(file));

			file.seekg(128, std::ios::beg);

			sprite->Frames.resize(frameCount);

			for (uint32_t frameIndex = 0; frameIndex < frameCount; frameIndex++) {
				auto& frame = sprite->Frames[frameIndex];
				
				uint32_t frameBytes = Read<uint32_t>(file);
				uint16_t frameMagic = Read<uint16_t>(file);
				assert(frameMagic == 0xF1FA);

				uint16_t oldChunkCount = Read<uint16_t>(file);
				frame.DurationMS = Read<uint16_t>(file);

				file.seekg(2, std::ios::cur);

				uint32_t chunkCount = Read<uint32_t>(file);

				if (chunkCount == 0)
					chunkCount = oldChunkCount;

				for (int32_t c = 0; c < chunkCount; c++) {
					uint32_t chunkSize = Read<uint32_t>(file);
					uint16_t chunkType = Read<uint16_t>(file);

					auto start = file.tellg();

					switch (chunkType) {
					case CHUNK_LAYER:
						ReadLayerChunk(
							file,
							chunkSize,
							*sprite);
						break;
					case CHUNK_CEL:
						ReadCelChunk(
							file,
							chunkSize,
							*sprite,
							frame);
						break;
					case  CHUNK_PALLETE:
						ReadPaletteChunk(
							file,
							chunkSize,
							*sprite);
						break;
					default:
						break;
					}

					auto end = start + static_cast<std::streamoff>(
						chunkSize - 6);
					file.seekg(end);
				}

				CompositeFrame(*sprite, frame);
			}
			return sprite;
		}
	}
}