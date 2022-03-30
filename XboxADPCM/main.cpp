#include "stdafx.h"
#include "ImaADPCM.h"
#include "Wav.h"

enum
{
	ADPCM_INTERLEAVE_BYTES = 4,
	ADPCM_INTERLEAVE_SAMPLES = ADPCM_INTERLEAVE_BYTES * 2,

	ADPCM_BLOCK_BYTES = ENCODED_SAMPLES_IN_ADPCM_BLOCK / 2 + sizeof(CImaADPCM),
};

// convert to PCM
size_t decode(uint8_t* adpcm, int16_t* pcm, size_t num_channels, size_t num_samples_per_channel)
{
	std::vector<CImaADPCM> Converters(num_channels);
	size_t i = 0;

	for (i = 0; i < num_samples_per_channel; i += SAMPLES_IN_ADPCM_BLOCK)
	{
		for (size_t c = 0; c < num_channels; c++)
		{
			Converters[c] = *(CImaADPCM*)adpcm;
			adpcm += sizeof(CImaADPCM);
			*(pcm++) = Converters[c].GetSample();
		}

		for (size_t j = 0; j < ENCODED_SAMPLES_IN_ADPCM_BLOCK / ADPCM_INTERLEAVE_SAMPLES; j++)
		{
			for (size_t c = 0; c < num_channels; c++)
			{
				for (size_t k = 0; k < ADPCM_INTERLEAVE_SAMPLES; k++)
					pcm[k * num_channels + c] = Converters[c].DecodeSample((k & 1) ? (adpcm[k / 2] >> 4) : (adpcm[k / 2] & 0xF));
				adpcm += ADPCM_INTERLEAVE_BYTES;
			}
			pcm += ADPCM_INTERLEAVE_SAMPLES * num_channels;
		}
	}
	return i;
}

// convert to ADPCM
size_t encode(int16_t* pcm, uint8_t* adpcm, size_t num_channels, size_t num_samples_per_channel)
{
	std::vector<CImaADPCM> Converters(num_channels);
	size_t i = 0;

	for (i = 0; i < num_samples_per_channel; i += SAMPLES_IN_ADPCM_BLOCK)
	{
		for (size_t c = 0; c < num_channels; c++)
		{
			Converters[c].SetSample(*(pcm++));
			*(CImaADPCM*)adpcm = Converters[c];
			adpcm += sizeof(CImaADPCM);
		}
		
		for (size_t j = 0; j < ENCODED_SAMPLES_IN_ADPCM_BLOCK / ADPCM_INTERLEAVE_SAMPLES; j++)
		{
			for (size_t c = 0; c < num_channels; c++)
			{
				for (size_t k = 0; k < ADPCM_INTERLEAVE_SAMPLES; k++)
				{
					uint8_t encodedSample = Converters[c].EncodeSample(pcm[k * num_channels + c]);
					assert((encodedSample & 0xF0) == 0);
					if (k & 1)
						adpcm[k / 2] |= (encodedSample << 4);
					else
						adpcm[k / 2] = encodedSample;
				}

				adpcm += ADPCM_INTERLEAVE_BYTES;
			}
			pcm += ADPCM_INTERLEAVE_SAMPLES * num_channels;
		}
	}
	return i;
}

void convert(const std::filesystem::path& infile, std::filesystem::path& outfile)
{
	FILE* f = nullptr;
	_wfopen_s(&f, infile.c_str(), L"rb");
	printf("Input file: %S\n", infile.c_str());
	if (!f) throw("Could not open input file!");

	RIFFHeader riffh;
	FMTHeader fmth;
	DATAHeader datah;

	fread(&riffh, sizeof(RIFFHeader), 1, f);
	if (riffh.RiffId != RIFF_ID) throw("Incorrect RIFF header!");
	if (riffh.WaveId != WAVE_ID) throw("Incorrect WAVE header!");

	fread(&fmth, sizeof(FMTHeader), 1, f);
	if (fmth.FmtId != FMT_ID) throw("Incorrect fmt header!");

	if (outfile.empty())
	{
		if (fmth.Format == FMT_PCM)
			outfile.replace_filename(infile.stem().wstring() + L"_adpcm.wav");
		else
			outfile.replace_filename(infile.stem().wstring() + L"_pcm.wav");
	}
	printf("Output file: %S\n\n", outfile.c_str());

	if (fmth.Format == FMT_XBOX_ADPCM)
	{
		printf("Converting Xbox ADPCM -> PCM\n");
		//ExtraADPCM e;
		//fread(&e, sizeof(ExtraADPCM), 1, f);
		fseek(f, sizeof(ExtraADPCM), SEEK_CUR);
	}
	else if (fmth.Format == FMT_PCM)
	{
		printf("Converting PCM -> Xbox ADPCM\n");
		if (fmth.BitsPerSample != 16) throw("Only 16 bit PCM is supported!");
	}
	else throw("Incorrect format!");

	fread(&datah, sizeof(DATAHeader), 1, f);

	if (datah.DataId != DATA_ID)
	{
		if (datah.DataId == 'tcaf')
		{
			fseek(f, datah.ChunkSize, SEEK_CUR);
			fread(&datah, sizeof(DATAHeader), 1, f);
			if (datah.DataId != DATA_ID)
				throw("Incorrect data header!");
		}
		else throw("Incorrect data or fact header!");
	}

	std::vector<uint8_t> buf_adpcm;
	std::vector<int16_t> buf_pcm;

	if (fmth.Format == FMT_PCM) // converting PCM to Xbox ADPCM
	{
		size_t SamplesCount = datah.ChunkSize / (sizeof(int16_t) * fmth.NumChannels);
		printf("Number of samples per channel: %i\n", SamplesCount);

		size_t extraSamples = SamplesCount % SAMPLES_IN_ADPCM_BLOCK;
		if (extraSamples > 0)
		{
			extraSamples = SAMPLES_IN_ADPCM_BLOCK - extraSamples;
			SamplesCount += extraSamples;
			printf("Allocating extra samples: %i\n", extraSamples);
		}

		size_t AdpcmSize = SamplesCount / SAMPLES_IN_ADPCM_BLOCK * ADPCM_BLOCK_BYTES * fmth.NumChannels;

		buf_adpcm.resize(AdpcmSize, 0);
		buf_pcm.resize(SamplesCount * fmth.NumChannels, 0);

		fread(buf_pcm.data(), 1, datah.ChunkSize, f);
		fclose(f);

		encode(buf_pcm.data(), buf_adpcm.data(), fmth.NumChannels, SamplesCount);

		_wfopen_s(&f, outfile.c_str(), L"wb");

		if (!f) throw("Could not create an output file!");

		auto wavHeader = GenerateWAVHeader(FMT_XBOX_ADPCM, fmth.SamplesPerSec, fmth.NumChannels, AdpcmSize);

#define write_header_to_file(i) fwrite(&std::get<i>(wavHeader), sizeof(std::get<i>(wavHeader)), 1, f)

		write_header_to_file(0); // riff
		write_header_to_file(1); // fmt
		write_header_to_file(2); // extra adpcm
		write_header_to_file(3); // data

		fwrite(buf_adpcm.data(), 1, AdpcmSize, f);
		fclose(f);

		printf("Converting to Xbox ADPCM - DONE!\n");
	}
	else if (fmth.Format == FMT_XBOX_ADPCM) // converting Xbox ADPCM to PCM
	{
		size_t SamplesCount = fmth.NumChannels * SAMPLES_IN_ADPCM_BLOCK * (datah.ChunkSize / (ADPCM_BLOCK_BYTES * fmth.NumChannels));

		buf_adpcm.resize(datah.ChunkSize);
		buf_pcm.resize(SamplesCount);

		fread(buf_adpcm.data(), 1, datah.ChunkSize, f);
		fclose(f);

		decode(buf_adpcm.data(), buf_pcm.data(), fmth.NumChannels, SamplesCount / fmth.NumChannels);

		auto wavHeader = GenerateWAVHeader(FMT_PCM, fmth.SamplesPerSec, fmth.NumChannels, buf_pcm.size() * sizeof(int16_t));

		_wfopen_s(&f, outfile.c_str(), L"wb");

		if (!f) throw("Could not create an output file!");

		write_header_to_file(0); // riff
		write_header_to_file(1); // fmt
		write_header_to_file(3); // data

#undef write_header_to_file

		fwrite(buf_pcm.data(), sizeof(int16_t), buf_pcm.size(), f);
		fclose(f);

		printf("Converting to PCM - DONE!\n");
	}
}

int wmain(int argc, wchar_t* argv[])
{
	printf("Xbox ADPCM Decoder/Encoder\nCoded by Sergeanur\n\n");
	if (argc < 2)
	{
		printf("Usage: input_file [output_file]\nNote: The program will automaticly detect format type (PCM or Xbox ADPCM).\n");
		return 0;
	}

	std::filesystem::path infile = argv[1], outfile;
	if (argc > 2) outfile = argv[2];
	try
	{
		convert(infile, outfile);
	}
	catch (const char* error)
	{
		printf("ERROR: %s\n", error);
	}

	//system("pause");
	return 0;
}
