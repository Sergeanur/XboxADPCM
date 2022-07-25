#include "stdafx.h"
#include "Riff.h"

bool Riff::HasChunk(uint32_t chunkid) const
{
	for (auto& chunk : chunks)
	{
		if (chunk.id == chunkid)
			return true;
	}
	return false;
}

Riff::tChunkData& Riff::GetChunkData(uint32_t chunkid)
{
	for (auto& chunk : chunks)
	{
		if (chunk.id == chunkid)
			return chunk.chunkData;
	}

	throw("Didn't find chunk");
}

const Riff::tChunkData& Riff::GetChunkData(uint32_t chunkid) const
{
	for (const auto& chunk : chunks)
	{
		if (chunk.id == chunkid)
			return chunk.chunkData;
	}

	throw("Didn't find chunk");
}

Riff::tFileChunkHeader Riff::ReadChunkHeaderFromFile(FILE* f)
{
	tFileChunkHeader header;
	if (fread(&header, sizeof(header), 1, f) < 1)
		throw("Couldn't read chunk header from file");
	return header;
}

Riff::tChunkData& Riff::AddChunk(uint32_t chunkid)
{
	chunks.emplace_back();
	chunks.back().id = chunkid;
	return chunks.back().chunkData;
}

uint32_t Riff::GetDataSizeForRiffHeader() const
{
	uint32_t size = sizeof(dataId);
	for (const tChunk& chunk : chunks)
		size += ((chunk.chunkData.size() + 1) & ~1) + sizeof(tFileChunkHeader);
	return size;
}

void Riff::DeleteEmptyChunks()
{
	auto it = chunks.begin();
	while (it != chunks.end())
	{
		auto thisIt = it++;
		if (thisIt->chunkData.size() == 0)
			chunks.erase(thisIt);
	}
}

void Riff::ReadFromFile(const std::filesystem::path& path)
{
	chunks.clear();

	FILE* f = nullptr;
	_wfopen_s(&f, path.c_str(), L"rb");

	if (!f) throw ("Could not open input file!");

	fseek(f, 0, SEEK_END);
	long filesize = ftell(f);
	fseek(f, 0, SEEK_SET);

	tFileChunkHeader mainHeader = ReadChunkHeaderFromFile(f);
	if (mainHeader.id != 'FFIR') throw("Incorrect RIFF header!");

	uint32_t file_dataId;
	if (fread(&file_dataId, sizeof(file_dataId), 1, f) != 1)
		throw ("Couldn't read RIFF data type");

	if (file_dataId != dataId)
		throw ("RIFF data type is different from expected one");

	while (!feof(f) && ftell(f) < filesize)
	{
		if (ftell(f) & 1)
			fseek(f, 1, SEEK_CUR);

		if (feof(f) || ftell(f) >= filesize)
			break;

		tFileChunkHeader header = ReadChunkHeaderFromFile(f);
		tChunkData& chunkData = AddChunk(header.id);
		chunkData.resize(header.size);
		fread(chunkData.data(), 1, header.size, f);
	}

	if (GetDataSizeForRiffHeader() != mainHeader.size)
		printf("WARNING: RIFF header data size didn't match an actual data size\n");

	fclose(f);

}
void Riff::WriteToFile(const std::filesystem::path& path) const
{
	FILE* f = nullptr;
	_wfopen_s(&f, path.c_str(), L"wb");

	if (!f) throw ("Could not create an output file");

	tFileChunkHeader mainHeader;
	mainHeader.id = 'FFIR';
	mainHeader.size = GetDataSizeForRiffHeader();

	fwrite(&mainHeader, sizeof(mainHeader), 1, f);
	fwrite(&dataId, sizeof(dataId), 1, f);

	for (const tChunk& chunk : chunks)
	{
		tFileChunkHeader header = chunk.GenerateChunkHeader();
		fwrite(&header, sizeof(header), 1, f);
		fwrite(chunk.chunkData.data(), 1, header.size, f);
		if (ftell(f) & 1)
		{
			uint8_t zero = 0;
			fwrite(&zero, 1, 1, f);
		}
	}

	fclose(f);
}