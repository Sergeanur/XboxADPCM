#include "stdafx.h"
#include "IMA_ADPCM.h"

enum
{
	RIFF_ID	= 'FFIR',//'RIFF',
	WAVE_ID	= 'EVAW',//'WAVE',
	FMT_ID	= ' tmf',//'fmt ',
	DATA_ID	= 'atad'//'data'
};

#define FMT_PCM 1
#define FMT_XBOX_ADPCM 0x69

struct RIFFHeader
{
	uint32_t	RiffId;
	uint32_t	Size;
	uint32_t	WaveId;

	RIFFHeader() { memset(this, 0, sizeof(RIFFHeader)); };

	RIFFHeader(uint32_t _size) : Size(_size), RiffId(RIFF_ID), WaveId(WAVE_ID)
	{ };
};

struct FMTHeader
{
	uint32_t	FmtId;
	uint32_t	ChunkSize;
	uint16_t	Format;
	uint16_t	NumChannels;
	uint32_t	SamplesPerSec;
	uint32_t	AvgBytesPerSec;
	uint16_t	BlockAlign;
	uint16_t	BitsPerSample;

	FMTHeader () { memset(this, 0, sizeof(FMTHeader)); };
	FMTHeader (uint16_t _Format, uint16_t _NumChannels, uint32_t _SamplesPerSec)
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

	FMTHeader (uint32_t _ChunkSize, uint16_t _Format, uint16_t _NumChannels, uint32_t _SamplesPerSec, uint32_t _AvgBytesPerSec, uint16_t _BlockAlign, uint16_t _BitsPerSample)
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
	uint16_t	Extra2;
	uint16_t	SamplesInBlock;

	ExtraADPCM () { memset(this, 0, sizeof(ExtraADPCM)); };
	ExtraADPCM (uint16_t a, uint16_t b) : Extra2(a), SamplesInBlock(b)
	{ };
};

struct DATAHeader
{
	uint32_t	DataId;
	uint32_t	ChunkSize;

	DATAHeader() { memset(this, 0, sizeof(DATAHeader)); };

	DATAHeader(uint32_t _size) : ChunkSize(_size), DataId(DATA_ID)
	{ };
};

void decode(BYTE* adpcm, short* pcm, int num_channels, int num_samples_per_channel)
{
	IMA_ADPCM* Converters = new IMA_ADPCM[num_channels];
	
	for (int i = 0; i < num_samples_per_channel; i += 65)
	{
		for (int c = 0; c < num_channels; c++)
		{
			Converters[c].PredictedValue = ((WORD*)adpcm)[0];
			Converters[c].StepIndex = ((WORD*)adpcm)[1];
			adpcm += 4;
			*pcm = Converters[c].PredictedValue;
			pcm++;
		}
	
		// каналы чередуются после каждых 4-х байт
		// 4 bytes per channel
		for (int j = 0; j < 8; j++)
		{
			for (int c = 0; c < num_channels; c++)
			{
				short temp_buf[8];
				Converters[c].Decode(temp_buf, adpcm, 0, 32);
				adpcm += 4;
				for (int k = 0; k < 8; k++)
					pcm[k * num_channels + c] = temp_buf[k];
			}
			pcm += 8 * num_channels;
		}
	}

	delete []Converters;
}

void encode(short* buf, BYTE* _buf_ad, int num_channels, int num_samples_per_channel)
{	
	IMA_ADPCM* Converters = new IMA_ADPCM[num_channels];
	
	for (int c = 0; c < num_channels; c++)
		Converters[c].StepIndex = 0;

	for (int i = 0; i < num_samples_per_channel; i += 65)
	{
		for (int c = 0; c < num_channels; c++)
		{
			Converters[c].PredictedValue = *(buf++);
			((WORD*)_buf_ad)[0] = Converters[c].PredictedValue;
			((WORD*)_buf_ad)[1] = Converters[c].StepIndex;
			_buf_ad += 4;
		}
		
		// каналы чередуются после каждых 4-х байт
		// 4 bytes per channel
		for (int j = 0; j < 8; j++)
		{
			for (int c = 0; c < num_channels; c++)
			{
				short temp_buf[8];
				for (int k = 0; k < 8; k++)
					temp_buf[k] = buf[k * num_channels + c];

				Converters[c].Encode(_buf_ad, 0, (int16_t*)temp_buf, 8 * 2);
				_buf_ad += 4;
			}
			buf += 8 * num_channels;
		}
	}

	delete []Converters;
}

void CritError(const char* errstr)
{
	printf("ERROR: %s\n", errstr);
	//system("pause");
	exit(-1);
}

int main(int argc, char* argv[])
{
	printf("Xbox ADPCM Decoder/Encoder\nCoded by Sergeanur\nADPCM code by J.D.Medhurst (a.k.a. Tixy)\n\n");
	if (argc < 2)
	{
		printf("Usage: input_file [output_file]\nNote: The program will automaticly detect format type (PCM or Xbox ADPCM).\n");
		return 0;
	}
	FILE* f = fopen(argv[1], "rb");
	printf("Input file: %s\n", argv[1]);
	if (!f) CritError("Could not open input file!");

	RIFFHeader riffh;
	FMTHeader fmth;
	DATAHeader datah;

	fread(&riffh, sizeof(RIFFHeader), 1, f);
	if (riffh.RiffId != RIFF_ID) CritError("Incorrect RIFF header!");
	if (riffh.WaveId != WAVE_ID) CritError("Incorrect WAVE header!");

	fread(&fmth, sizeof(FMTHeader), 1, f);
	if (fmth.FmtId != FMT_ID) CritError("Incorrect fmt header!");

	std::string output_name;
	if (argc > 2) output_name = argv[2];
	else
	{
		char *dot = strrchr(argv[1], '.');
		if (dot)
			*dot = 0;
		output_name = argv[1];
		if (fmth.Format == 1) output_name += "_adpcm.wav";
		else output_name += "_pcm.wav";
		if (dot)
			*dot = '.';
	}
	printf("Output file: %s\n\n", output_name.c_str());

	if (fmth.Format == FMT_XBOX_ADPCM)
	{
		printf("Converting Xbox ADPCM -> PCM\n");
		ExtraADPCM e;
		fread(&e, sizeof(ExtraADPCM), 1, f);
	}
	else if (fmth.Format == FMT_PCM)
	{
		printf("Converting PCM -> Xbox ADPCM\n");
		if (fmth.BitsPerSample != 16) CritError("Only 16 bps is supported!");
	}
	else CritError("Incorrect format!"); 

	fread(&datah, sizeof(DATAHeader), 1, f);

	if (datah.DataId != DATA_ID)
	{
		if (datah.DataId == 'tcaf')
		{
			fseek(f, datah.ChunkSize, SEEK_CUR);
			fread(&datah, sizeof(DATAHeader), 1, f);
			if (datah.DataId != DATA_ID)
				CritError("Incorrect data header!");
		}
		else CritError("Incorrect data or fact header!");
	}
	
	//printf("%x\n", datah.ChunkSize);
	
	int iNumOfSamples = datah.ChunkSize / (sizeof(short) * fmth.NumChannels);
	printf("Number of samples per channel: %i\n", iNumOfSamples);

	if (fmth.Format == FMT_PCM) // converting PCM to XBOX ADPCM
	{
		int iExtraSize = iNumOfSamples;
		while (iExtraSize >= 65) iExtraSize -= 65;
		if (iExtraSize)
		{
			iExtraSize = 65 - iExtraSize;
			iNumOfSamples += iExtraSize;
			printf("Allocating extra size: %i\n", iExtraSize);
		}

		short* buf = new short[iNumOfSamples * fmth.NumChannels];
		memset(buf, 0, (iNumOfSamples * fmth.NumChannels) * 2);
	
		int AssumeSize = (iNumOfSamples * fmth.NumChannels) / (0x41 * fmth.NumChannels) * (0x24 * fmth.NumChannels);
		BYTE* buf_ad = new BYTE[AssumeSize + 0x30];
		fread(buf, 1, datah.ChunkSize, f);
		fclose(f);

		encode(buf, buf_ad, fmth.NumChannels, iNumOfSamples);

		f = fopen(output_name.c_str(), "wb");
		
		if (!f) CritError("Could not create output file!");

		
		RIFFHeader riffh_adpcm(AssumeSize + 0x28);
		FMTHeader fmth_adpcm(FMT_XBOX_ADPCM, fmth.NumChannels, fmth.SamplesPerSec);
		ExtraADPCM extra_adpcm(2, 0x40);
		DATAHeader datah_adpcm(AssumeSize);
		
		fwrite(&riffh_adpcm, sizeof(RIFFHeader), 1, f);
		fwrite(&fmth_adpcm, sizeof(FMTHeader), 1, f);
		fwrite(&extra_adpcm, sizeof(ExtraADPCM), 1, f);
		fwrite(&datah_adpcm, sizeof(DATAHeader), 1, f);
		fwrite(buf_ad, AssumeSize, 1, f);

		fclose(f);

		delete []buf;
		delete []buf_ad;

		printf("Converting to Xbox ADPCM - DONE!\n");
	}
	else if (fmth.Format == FMT_XBOX_ADPCM) // converting XBOX ADPCM to PCM
	{
		BYTE* buf_adpcm = new BYTE[datah.ChunkSize];
		int AssumeSize = 2 * fmth.NumChannels * 0x41 * (datah.ChunkSize / (0x24 * fmth.NumChannels));//(iNumOfSamples * fmth.NumChannels) / (0x41 * fmth.NumChannels) * (0x24 * fmth.NumChannels);
		BYTE* buf_pcm = new BYTE[AssumeSize];
		fread(buf_adpcm, 1, datah.ChunkSize, f);
		fclose(f);
		
		decode(buf_adpcm, (short*)buf_pcm, fmth.NumChannels, 0x41 * (datah.ChunkSize / (0x24 * fmth.NumChannels)));

		RIFFHeader riffh_pcm(AssumeSize + 0x24);
		FMTHeader fmth_pcm(FMT_PCM, fmth.NumChannels, fmth.SamplesPerSec);
		DATAHeader datah_pcm(AssumeSize);

		f = fopen(output_name.c_str(), "wb");

		if (!f) CritError("Could not create output file!");

		fwrite(&riffh_pcm, sizeof(RIFFHeader), 1, f);
		fwrite(&fmth_pcm, sizeof(FMTHeader), 1, f);
		fwrite(&datah_pcm, sizeof(DATAHeader), 1, f);
		fwrite(buf_pcm, AssumeSize, 1, f);
		fclose(f);

		delete []buf_adpcm;
		delete []buf_pcm;

		printf("Converting to PCM - DONE!\n");
	}

	//system("pause");
	return 0;
}

