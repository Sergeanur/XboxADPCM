#pragma once

enum
{
	RIFF_ID = 'FFIR',//'RIFF',
	WAVE_ID = 'EVAW',//'WAVE',
	FMT_ID = ' tmf',//'fmt ',
	DATA_ID = 'atad'//'data'
};

enum eWavFormat : uint16_t
{
	FMT_NONE = 0,
	FMT_PCM = 1,
	FMT_XBOX_ADPCM = 0x69
};

enum
{
	ENCODED_SAMPLES_IN_ADPCM_BLOCK = 64,
	SAMPLES_IN_ADPCM_BLOCK, // plus one
};

struct RIFFHeader
{
	uint32_t RiffId = RIFF_ID;
	uint32_t Size = 0;
	uint32_t WaveId = WAVE_ID;

	RIFFHeader() = default;

	RIFFHeader(uint32_t _size) : Size(_size), RiffId(RIFF_ID), WaveId(WAVE_ID)
	{ };
};

struct FMTHeader
{
	uint32_t	FmtId = FMT_ID;
	uint32_t	ChunkSize = 0;
	eWavFormat	Format = FMT_NONE;
	uint16_t	NumChannels = 0;
	uint32_t	SamplesPerSec = 0;
	uint32_t	AvgBytesPerSec = 0;
	uint16_t	BlockAlign = 0;
	uint16_t	BitsPerSample = 0;

	FMTHeader() = default;
	FMTHeader(eWavFormat _Format, uint16_t _NumChannels, uint32_t _SamplesPerSec)
		: FmtId(FMT_ID),
		Format(_Format),
		NumChannels(_NumChannels),
		SamplesPerSec(_SamplesPerSec)
	{
		if (Format == FMT_PCM)
		{
			BitsPerSample = 16;
			AvgBytesPerSec = NumChannels * SamplesPerSec * (BitsPerSample / 8);
			ChunkSize = 0x10;
			BlockAlign = 2 * NumChannels;
		}
		else if (Format == FMT_XBOX_ADPCM)
		{
			ChunkSize = 0x14;
			BlockAlign = 0x24 * NumChannels;
			BitsPerSample = 4;
			AvgBytesPerSec = SamplesPerSec * BlockAlign >> 6; // 4 bytes Byterate ( (Samplerate * Block Alignment) >> 6 ) 
		}
	};

	FMTHeader(uint32_t _ChunkSize, eWavFormat _Format, uint16_t _NumChannels, uint32_t _SamplesPerSec, uint32_t _AvgBytesPerSec, uint16_t _BlockAlign, uint16_t _BitsPerSample)
		: FmtId(FMT_ID),
		Format(_Format),
		NumChannels(_NumChannels),
		SamplesPerSec(_SamplesPerSec),
		AvgBytesPerSec(_AvgBytesPerSec),
		BlockAlign(_BlockAlign),
		BitsPerSample(_BitsPerSample)
	{ };
};

struct ExtraADPCM
{
	uint16_t Extra = 0;
	uint16_t SamplesInBlock = 0;

	ExtraADPCM() = default;
	ExtraADPCM(uint16_t a, uint16_t b) : Extra(a), SamplesInBlock(b)
	{ };
};

struct DATAHeader
{
	uint32_t DataId = DATA_ID;
	uint32_t ChunkSize = 0;

	DATAHeader() = default;

	DATAHeader(uint32_t _size) : ChunkSize(_size), DataId(DATA_ID)
	{ };
};

std::tuple<RIFFHeader, FMTHeader, ExtraADPCM, DATAHeader> GenerateWAVHeader(eWavFormat format, uint32_t samplerate, uint16_t numChannels, uint32_t size);
