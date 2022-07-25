#include "stdafx.h"
#include "Wav.h"

void CWavFile::ReadFromFile(const std::filesystem::path& path)
{
	RiffFile.ReadFromFile(path);

	if (!RiffFile.HasChunk(FMT_ID))
		throw("fmt chunk missing in wav file!");
	if (!RiffFile.HasChunk(DATA_ID))
		throw("data chunk missing in wav file!");

	Riff::tChunkData& fmt_data = RiffFile.GetChunkData(FMT_ID);

	if (fmt_data.size() < sizeof(FMTHeaderBase))
		throw("Size of fmt section is less than supported one!");

	FMTHeaderBase* fmth = (FMTHeaderBase*)fmt_data.data();

	Format = fmth->Format;
	NumChannels = fmth->NumChannels;
	SamplesPerSec = fmth->SamplesPerSec;

	if (Format == FMT_PCM)
	{
		if (fmth->BitsPerSample != 16)
			throw("Only 16 bit PCM is supported!");
	}
	else if (Format != FMT_XBOX_ADPCM)
		throw("Unsupported format!");
}

void CWavFile::SaveToFile(const std::filesystem::path& path)
{
	Riff::tChunkData& fmt_data = RiffFile.GetChunkData(FMT_ID);

	switch (Format)
	{
	case FMT_PCM:
	{
		fmt_data.resize(sizeof(FMTHeaderBase));
		new (fmt_data.data())FMTHeaderBase(Format, NumChannels, SamplesPerSec);
		break;
	}
	case FMT_XBOX_ADPCM:
	{
		fmt_data.resize(sizeof(FMTHeaderADPCM));
		new (fmt_data.data())FMTHeaderADPCM(Format, NumChannels, SamplesPerSec);
		break;
	}
	default:
		assert(0);
		break;
	}

	RiffFile.DeleteEmptyChunks();
	RiffFile.WriteToFile(path);
}