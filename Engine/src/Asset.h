#pragma once
#include <fstream>
#include <optional>
#include <filesystem>
#include <cstdint>
#include <unordered_map>

using u64 = uint64_t;
using u32 = uint32_t;
using u8 = uint8_t;

namespace fs = std::filesystem;

namespace Engine {
	
	enum class AssetType : u32 
	{
		Unknown = 0,
		Mesh = 1, 
		Texture = 2,
		Audio = 3,
		Shader = 4
	};

	// Owns the raw bytes for a single loaded asset, plus enough
	// metadata to know what it is and know how to interpret it.
	struct AssetBuffer {
		std::string name;
		AssetType Type = AssetType::Unknown;
		std::vector<u8> Data;

		size_t Size() const { return Data.size(); }
		bool IsValid() const { return !Data.empty(); }

		const u8* RawData() { return Data.data(); }
	};

	// Asset Pack things

	// On-disk layout:
	//
	//   [Header]
	//   [Entry] * EntryCount
	//   [Raw asset data, back to back, in entry order]
	//
	// Entry offsets are relative to the start of the data section
	// (i.e. right after the last entry record), not the file start.
	struct AssetPackHeader
	{
		static constexpr char     kMagic[4] = { 'A', 'P', 'A', 'K' };
		static constexpr uint32_t kVersion = 1;

		char Magic[4] = { 'A', 'P', 'A', 'K' };
		u32 Version = kVersion;
		u32 EntryCount = 0;
	};

	struct AssetPackEntry 
	{
		std::string Name;
		AssetType Type = AssetType::Unknown;
		u64 Offset = 0;
		u64 Size = 0;
		u32 Flags = 0;
	};

	// Builds a .pak out of loose files on disk.
	class AssetPackWriter
	{
	public:
		// Queues a source file to be embedded in the pack under `name`.
		// The file is not read until Write() is called.
		void AddAsset(const std::string& name, AssetType type, const fs::path& sourceFile);

		// Writes the pack to `outFile`. Returns false on any IO error
		// (e.g. a queued source file no longer exists).
		bool Write(const fs::path& outFile);

		size_t PendingCount() const { return _Pending.size(); }
	private:
		struct PendingAsset
		{
			std::string Name;
			AssetType Type;
			fs::path SourceFile;
		};

		std::vector<PendingAsset> _Pending;
	};

	// Opens a .pak file and loads individual assets from it on demand,
	// without pulling the whole pack into memory up front.
	class AssetPackReader
	{
	public:
		AssetPackReader() = default;
		~AssetPackReader();

		AssetPackReader(const AssetPackReader&) = delete;
		AssetPackReader& operator=(const AssetPackReader&) = delete;

		// Opens the pack and reads its entry table. Returns false if the
		// file can't be opened or the header/magic/version don't match.
		bool Open(const fs::path& packFile);
		void Close();

		bool IsOpen() const { return _Stream.is_open(); }

		// Reads one asset's bytes off disk into an AssetBuffer.
		// Returns std::nullopt if `name` isn't in the pack or the pack
		// isn't open.
		std::optional<AssetBuffer> Load(const std::string& name);

		bool Contains(const std::string& name) const { return _Entries.find(name) != _Entries.end(); }

		std::vector<std::string> ListAssets() const;

	private:
		mutable std::ifstream _Stream;
		std::unordered_map<std::string, AssetPackEntry> _Entries;
		u64 _DataSectionStart = 0;
 };
}