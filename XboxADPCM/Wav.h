#pragma once

#include "ImaADPCM.h"
#include "Riff.h"

enum
{
	ENCODED_SAMPLES_IN_ADPCM_BLOCK = 64,
	SAMPLES_IN_ADPCM_BLOCK, // plus one

	ADPCM_INTERLEAVE_BYTES = 4,
	ADPCM_INTERLEAVE_SAMPLES = ADPCM_INTERLEAVE_BYTES * 2,

	ADPCM_BLOCK_BYTES = ENCODED_SAMPLES_IN_ADPCM_BLOCK / 2 + sizeof(CImaADPCM),
};

enum eWavFormat : uint16_t
{
	FMT_NONE = 0,
	FMT_PCM = 1,
	FMT_XBOX_ADPCM = 0x69
};

class CWavFile
{
	enum eChunkId : uint32_t
	{
		WAVE_ID = 'EVAW', //'WAVE',
		FMT_ID  = ' tmf', //'fmt ',
		DATA_ID = 'atad', //'data'
		NULL_ID = 0,
	};

	Riff RiffFile = Riff(WAVE_ID);

	struct FMTHeaderBase
	{
		eWavFormat	Format = FMT_NONE;
		uint16_t	NumChannels = 0;
		uint32_t	SamplesPerSec = 0;
		uint32_t	AvgBytesPerSec = 0;
		uint16_t	BlockAlign = 0;
		uint16_t	BitsPerSample = 0;

		FMTHeaderBase() = default;
		FMTHeaderBase(eWavFormat _Format, uint16_t _NumChannels, uint32_t _SamplesPerSec) :
			Format(_Format),
			NumChannels(_NumChannels),
			SamplesPerSec(_SamplesPerSec)
		{
			if (Format == FMT_PCM)
			{
				BitsPerSample = 16;
				BlockAlign = sizeof(int16_t) * NumChannels;
				AvgBytesPerSec = NumChannels * SamplesPerSec * (BitsPerSample / 8);
			}
			else if (Format == FMT_XBOX_ADPCM)
			{
				BitsPerSample = 4;
				BlockAlign = ADPCM_BLOCK_BYTES * NumChannels;
				AvgBytesPerSec = SamplesPerSec * BlockAlign >> 6; // 4 bytes Byterate ( (Samplerate * Block Alignment) >> 6 ) 
			}
		};

		FMTHeaderBase(eWavFormat _Format, uint16_t _NumChannels, uint32_t _SamplesPerSec, uint32_t _AvgBytesPerSec, uint16_t _BlockAlign, uint16_t _BitsPerSample)
			: Format(_Format),
			NumChannels(_NumChannels),
			SamplesPerSec(_SamplesPerSec),
			AvgBytesPerSec(_AvgBytesPerSec),
			BlockAlign(_BlockAlign),
			BitsPerSample(_BitsPerSample)
		{ };
	};

	struct FMTHeaderADPCM : public FMTHeaderBase
	{
		uint16_t Extra = 2;
		uint16_t SamplesInBlock = ENCODED_SAMPLES_IN_ADPCM_BLOCK;

		FMTHeaderADPCM() = default;
		FMTHeaderADPCM(eWavFormat _Format, uint16_t _NumChannels, uint32_t _SamplesPerSec) : FMTHeaderBase(_Format, _NumChannels, _SamplesPerSec) {};
		FMTHeaderADPCM(eWavFormat _Format, uint16_t _NumChannels, uint32_t _SamplesPerSec, uint32_t _AvgBytesPerSec, uint16_t _BlockAlign, uint16_t _BitsPerSample)
			: FMTHeaderBase(_Format, _NumChannels, _SamplesPerSec, _AvgBytesPerSec, _BlockAlign, _BitsPerSample) {}
	};

public:
	eWavFormat	Format = FMT_NONE;
	uint16_t	NumChannels = 0;
	uint32_t	SamplesPerSec = 0;

	CWavFile()
	{
		RiffFile.AddChunk(FMT_ID);
		RiffFile.AddChunk(DATA_ID);
	}

	std::vector<uint8_t>& GetSampleData() { return RiffFile.GetChunkData(DATA_ID); }
	const std::vector<uint8_t>& GetSampleData() const { return RiffFile.GetChunkData(DATA_ID); }

	void ReadFromFile(const std::filesystem::path& path);
	void SaveToFile(const std::filesystem::path& path);
};