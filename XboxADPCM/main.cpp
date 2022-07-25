#include "stdafx.h"
#include "ImaADPCM.h"
#include "Wav.h"

// convert to PCM
size_t decode(const uint8_t* adpcm, int16_t* pcm, size_t num_channels, size_t num_samples_per_channel)
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

size_t decode(const std::vector<uint8_t>& adpcm, std::vector<uint8_t>& pcm, size_t num_channels, size_t num_samples_per_channel)
{
	if (pcm.size() < num_samples_per_channel * num_channels * sizeof(int16_t))
		pcm.resize(num_samples_per_channel * num_channels * sizeof(int16_t), 0);
	return decode(adpcm.data(), (int16_t*)pcm.data(), num_channels, num_samples_per_channel);
}

// convert to ADPCM
size_t encode(const int16_t* pcm, uint8_t* adpcm, size_t num_channels, size_t num_samples_per_channel)
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

size_t encode(const std::vector<uint8_t>& pcm, std::vector<uint8_t>& adpcm, size_t num_channels, size_t num_samples_per_channel)
{
	size_t AdpcmSize = num_samples_per_channel / SAMPLES_IN_ADPCM_BLOCK * ADPCM_BLOCK_BYTES * num_channels;
	if (adpcm.size() < AdpcmSize)
		adpcm.resize(AdpcmSize, 0);
	return encode((const int16_t*)pcm.data(), adpcm.data(), num_channels, num_samples_per_channel);
}

void convert(const std::filesystem::path& infile, std::filesystem::path& outfile)
{
	CWavFile inwav, outwav;
	printf("Input file: %S\n", infile.c_str());
	inwav.ReadFromFile(infile);

	switch (inwav.Format)
	{
	case FMT_PCM:
		outwav.Format = FMT_XBOX_ADPCM;
		printf("Converting PCM -> Xbox ADPCM\n");
		break;
	case FMT_XBOX_ADPCM:
		outwav.Format = FMT_PCM;
		printf("Converting Xbox ADPCM -> PCM\n");
		break;
	default:
		break;
	}

	outwav.NumChannels = inwav.NumChannels;
	outwav.SamplesPerSec = inwav.SamplesPerSec;

	if (outfile.empty())
		outfile.replace_filename(infile.stem().wstring() + (inwav.Format == FMT_PCM ? L"_adpcm.wav" : L"_pcm.wav"));
	printf("Output file: %S\n\n", outfile.c_str());

	if (inwav.Format == FMT_PCM) // converting PCM to Xbox ADPCM
	{
		size_t SamplesCount = inwav.GetSampleData().size() / (sizeof(int16_t) * inwav.NumChannels);
		printf("Number of samples per channel: %i\n", SamplesCount);

		size_t extraSamples = SamplesCount % SAMPLES_IN_ADPCM_BLOCK;
		if (extraSamples > 0)
		{
			extraSamples = SAMPLES_IN_ADPCM_BLOCK - extraSamples;
			SamplesCount += extraSamples;
			printf("Allocating extra samples: %i\n", extraSamples);
			inwav.GetSampleData().resize(SamplesCount * sizeof(int16_t), 0);
		}

		encode(inwav.GetSampleData(), outwav.GetSampleData(), inwav.NumChannels, SamplesCount);
		outwav.SaveToFile(outfile);

		printf("Converting to Xbox ADPCM - DONE!\n");
	}
	else if (inwav.Format == FMT_XBOX_ADPCM) // converting Xbox ADPCM to PCM
	{
		size_t SamplesCount = SAMPLES_IN_ADPCM_BLOCK * (inwav.GetSampleData().size() / (ADPCM_BLOCK_BYTES * inwav.NumChannels));
		printf("Number of samples per channel: %i\n", SamplesCount);

		decode(inwav.GetSampleData(), outwav.GetSampleData(), inwav.NumChannels, SamplesCount);
		outwav.SaveToFile(outfile);

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
