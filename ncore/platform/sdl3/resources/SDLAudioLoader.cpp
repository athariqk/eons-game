#include "SDLAudioLoader.h"

#include <SDL3/SDL_audio.h>
#include <utils/Structures.h>

namespace ncore {

AudioClip SDLAudioLoader::load_from_disk(std::string location) {
    uint8_t *raw_buf = new uint8_t[0];
    uint32_t wav_len = 0;

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = 44100;

    if (!SDL_LoadWAV(location.c_str(), &spec, &raw_buf, &wav_len)) {
        NC_LOG_ERROR_C(log::AUDIO, "WAV load FAIL: {}", SDL_GetError());
        return {};
    }

    AudioClip result;
    result.data = BytesBuffer(raw_buf, raw_buf + wav_len);
    result.length = wav_len;
    result.channels = spec.channels;
    result.frequency = spec.freq;
    result.bits_per_sample = SDL_AUDIO_BITSIZE(spec.format);
    return result;
}

} // namespace ncore
