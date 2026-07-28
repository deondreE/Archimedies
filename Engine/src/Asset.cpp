#include "Asset.h"
#include <cstring>

namespace Engine {
	namespace {

		template<typename T>
		void WriteRaw(std::ofstream& out, const T& value)
		{
			out.write(reinterpret_cast<const char*>(&value), sizeof(T));
		}

		template<typename T>
		void ReadRaw(std::ifstream& in, T& value)
		{
			in.read(reinterpret_cast<char*>(&value), sizeof(T));
		}

		void WriteString(std::ofstream& out, const std::string& s)
		{
			u32 len = static_cast<u32>(s.size());
			WriteRaw(out, len);
			out.write(s.data(), len);
		}

		bool ReadString(std::ifstream& in, std::string& out)
		{
			u32 len = 0;
			ReadRaw(in, len);
			if (!in.good()) {
				return false;
			}

			out.resize(len);
			if (len > 0)
				in.read(out.data(), len);
			return in.good();
		}
	}

	void AssetPackWriter::AddAsset(const std::string& name, AssetType type, const fs::path& sourceFile)
	{
		_Pending.push_back({ name, type, sourceFile });
	}

	bool AssetPackWriter::Write(const fs::path& outFile)
	{
		std::vector<u64> sizes;
		sizes.reserve(_Pending.size());

		for (const auto& asset : _Pending) {
			std::error_code ec;
			u64 size = fs::file_size(asset.SourceFile, ec);
			if (ec)
				return false;
			sizes.push_back(size);
		}

		std::ofstream out(outFile, std::ios::binary);
		if (!out.is_open())
			return false;

		AssetPackHeader header;
		header.EntryCount = static_cast<u32>(_Pending.size());

		out.write(header.Magic, sizeof(header.Magic));
		WriteRaw(out, header.Version);
		WriteRaw(out, header.EntryCount);

		// Entry table, with offsets computed relative to the data section
		u64 runningOffset = 0;
		for (size_t i = 0; i < _Pending.size(); ++i) {
			const auto& asset = _Pending[i];

			WriteString(out, asset.Name);

			u32 typeValue = static_cast<u32>(asset.Type);
			WriteRaw(out, typeValue);
			WriteRaw(out, runningOffset);
			WriteRaw(out, sizes[i]);

			u32 flags = 0;
			WriteRaw(out, flags);

			runningOffset += sizes[i];
		}

		// Data Section: raw bytes of each source file, back to back.
		std::vector<char> buffer;
		for (size_t i = 0; i < _Pending.size(); ++i) {
			std::ifstream in(_Pending[i].SourceFile, std::ios::binary);
			if (!in.is_open())
				return false;

			buffer.resize(sizes[i]);
			if (sizes[i] > 0)
				in.read(buffer.data(), sizes[i]);
			if (!in.good() && !in.eof())
				return false;

			out.write(buffer.data(), sizes[i]);
		}

		return out.good();
	}

	AssetPackReader::~AssetPackReader()
	{
		Close();
	}

	bool AssetPackReader::Open(const fs::path& packFile)
	{
		Close();

		_Stream.open(packFile, std::ios::binary);
		if (!_Stream.is_open())
			return false;

		AssetPackHeader header;
		_Stream.read(header.Magic, sizeof(header.Magic));
		ReadRaw(_Stream, header.Version);
		ReadRaw(_Stream, header.EntryCount);

		if (!_Stream.good()) {
			Close();
			return false;
		}

		if (std::memcmp(header.Magic, AssetPackHeader::kMagic, sizeof(header.Magic)) != 0 ||
			header.Version != AssetPackHeader::kVersion)
		{
			Close();
			return false;
		}

		_Entries.clear();
		_Entries.reserve(header.EntryCount);

		for (u32 i = 0; i < header.EntryCount; ++i) {
			AssetPackEntry entry;

			if (!ReadString(_Stream, entry.Name)) {
				Close();
				return false;
			}

			u32 typeValue = 0;
			ReadRaw(_Stream, typeValue);
			entry.Type = static_cast<AssetType>(typeValue);

			ReadRaw(_Stream, entry.Offset);
			ReadRaw(_Stream, entry.Size);
			ReadRaw(_Stream, entry.Flags);

			if (!_Stream.good()) {
				Close();
				return false;
			}

			_Entries.emplace(entry.Name, std::move(entry));
		}

		_DataSectionStart = static_cast<u64>(_Stream.tellg());
		return true;
	}

	void AssetPackReader::Close()
	{
		if (_Stream.is_open())
			_Stream.close();
		_Entries.clear();
		_DataSectionStart = 0;
	}

	std::optional<AssetBuffer> AssetPackReader::Load(const std::string& name)
	{
		if (!IsOpen())
			return std::nullopt;

		auto it = _Entries.find(name);
		if (it == _Entries.end())
			return std::nullopt;

		const AssetPackEntry& entry = it->second;

		_Stream.clear();
		_Stream.seekg(static_cast<std::streamoff>(_DataSectionStart + entry.Offset));
		if (!_Stream.good())
			return std::nullopt;

		AssetBuffer buffer;
		buffer.name = entry.Name;
		buffer.Type = entry.Type;
		buffer.Data.resize(entry.Size);

		if (entry.Size > 0) {
			_Stream.read(reinterpret_cast<char*>(buffer.Data.data()), entry.Size);
		}

		if (!_Stream.good() && !_Stream.eof())
			return std::nullopt;

		return buffer;
	}

	std::vector<std::string> AssetPackReader::ListAssets() const
	{
		std::vector<std::string> names;
		names.reserve(_Entries.size());

		for (const auto& [name, entry] : _Entries)
			names.push_back(name);

		return names;
	}
}