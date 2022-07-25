#pragma once

class Riff
{
public:
	typedef std::vector<uint8_t> tChunkData;
private:
	struct tFileChunkHeader
	{
		uint32_t id;
		uint32_t size;
	};

	struct tChunk
	{
		uint32_t id;
		tChunkData chunkData;

		tFileChunkHeader GenerateChunkHeader() const
		{
			tFileChunkHeader h;
			h.id = id;
			h.size = chunkData.size();
			return h;
		}
	};

	uint32_t dataId;
	std::list<tChunk> chunks;

	static tFileChunkHeader ReadChunkHeaderFromFile(FILE* f);

public:

	Riff(uint32_t _dataId) : dataId(_dataId) {}

	bool HasChunk(uint32_t chunkid) const;
	tChunkData& GetChunkData(uint32_t chunkid);
	const tChunkData& GetChunkData(uint32_t chunkid) const;
	tChunkData& AddChunk(uint32_t chunkid);

	uint32_t GetDataSizeForRiffHeader() const;
	void DeleteEmptyChunks();

	void ReadFromFile(const std::filesystem::path& path);
	void WriteToFile(const std::filesystem::path& path) const;

};