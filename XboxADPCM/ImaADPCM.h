#pragma once

class CImaADPCM
{
	static const uint16_t StepTable[];

	int16_t sample = 0;
	uint8_t index = 0;
	uint8_t _padding = 0;

public:
	CImaADPCM() = default;
	int16_t DecodeSample(uint8_t adpcm);
	uint8_t EncodeSample(int16_t pcm);

	inline int16_t GetSample() const { return sample; }
	inline void SetSample(int16_t pcm) { sample = pcm; }
private:
	uint8_t GetNewIndex(uint8_t adpcm);
};

static_assert(sizeof(CImaADPCM) == 4, "size of CImaADPCM must be 4");
