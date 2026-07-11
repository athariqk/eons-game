#pragma once

#include <ncore/kernel/collection.h>
#include <ncore/modules/resource/resource.h>

namespace nc {

class AudioClip : public IResource {
    NCLASS( AudioClip, IResource )

public:
    AudioClip( const void* p_data, int p_length, int p_channels, int p_frequency, int p_bits_per_sample );

    std::span<std::byte> get_data();
    std::span<const std::byte> get_data() const;

    int get_length() const;
    int get_channels() const;
    int get_frequency() const;
    int get_bits_per_sample() const;

    size_t get_size_bytes() const override;

private:
    BytesBuffer data;
    int length = 0;
    int channels;
    int frequency;
    int bits_per_sample;
};

} // namespace nc
