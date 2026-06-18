#include "sdl_audio_loader.h"

#include <SDL3/SDL_audio.h>

#include <utils/macro.h>

namespace ncore {

std::unique_ptr<AudioClip> SDLAudioLoader::load(const std::string_view path) {
    uint8_t *raw_buf = new uint8_t[0];
    uint32_t wav_len = 0;

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = 44100;

    if (!SDL_LoadWAV(path.data(), &spec, &raw_buf, &wav_len)) {
        NC_LOG_ERROR_C(log::AUDIO, "WAV load FAIL: {}", SDL_GetError());
        return {};
    }

    auto result = std::make_unique<AudioClip>();
    result->data = BytesBuffer(raw_buf, raw_buf + wav_len);
    result->length = wav_len;
    result->channels = spec.channels;
    result->frequency = spec.freq;
    result->bits_per_sample = SDL_AUDIO_BITSIZE(spec.format);
    return result;
}

} // namespace ncore
