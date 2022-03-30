#include "stdafx.h"
#include "ImaADPCM.h"

const uint16_t CImaADPCM::StepTable[] =
{
	7, 8, 9, 10, 11, 12, 13, 14,
	16, 17, 19, 21, 23, 25, 28, 31,
	34, 37, 41, 45, 50, 55, 60, 66,
	73, 80, 88, 97, 107, 118, 130, 143,
	157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411,
	1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
	3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767
};

int16_t CImaADPCM::DecodeSample(uint8_t adpcm)
{
	int32_t step = StepTable[index];
	index = GetNewIndex(adpcm);

	int32_t diff = step >> 3;
	for (uint8_t i = 0; i < 3; i++)
	{
		if (adpcm & (4 >> i))
			diff += step >> i;
	}
	if (adpcm & 8) diff = -diff;

	sample = static_cast<int16_t>(std::clamp(sample + diff, (int32_t)INT16_MIN, (int32_t)INT16_MAX));
	return sample;
}

uint8_t CImaADPCM::EncodeSample(int16_t pcm)
{
	int32_t delta = pcm;
	delta -= sample;
	uint8_t adpcm = 0;
	if (delta < 0)
	{
		delta = -delta;
		adpcm |= 8;
	}

	uint16_t step = StepTable[index];
	int32_t diff = step >> 3;
	for (uint8_t i = 0; i < 3; i++)
	{
		if (delta > step)
		{
			delta -= step;
			diff += step;
			adpcm |= (4 >> i);
		}
		step >>= 1;
	}
	if (adpcm & 8) diff = -diff;

	sample = static_cast<int16_t>(std::clamp(sample + diff, (int32_t)INT16_MIN, (int32_t)INT16_MAX));
	index = GetNewIndex(adpcm);
	return adpcm;
}

uint8_t CImaADPCM::GetNewIndex(uint8_t adpcm)
{
	adpcm &= 7;
	uint8_t curIndex = index;
	if (adpcm & 4)
	{
		curIndex += (adpcm - 3) * 2;
		if (curIndex > ARRAY_COUNT(StepTable)-1)
			curIndex = ARRAY_COUNT(StepTable)-1;
	}
	else if (curIndex > 0)
		curIndex--;
	return curIndex;
}
