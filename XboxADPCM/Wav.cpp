#include "stdafx.h"
#include "Wav.h"

std::tuple<RIFFHeader, FMTHeader, ExtraADPCM, DATAHeader> GenerateWAVHeader(eWavFormat format, uint32_t samplerate, uint16_t numChannels, uint32_t size)
{
	ExtraADPCM extra(2, ENCODED_SAMPLES_IN_ADPCM_BLOCK);
	DATAHeader datah(size);
	FMTHeader fmth(format, numChannels, samplerate);
	RIFFHeader riffh(size + sizeof(FMTHeader) + sizeof(DATAHeader) + sizeof(RIFFHeader::WaveId));
	if (format == FMT_XBOX_ADPCM)
		riffh.Size += sizeof(ExtraADPCM);

	return std::make_tuple(riffh, fmth, extra, datah);
}
