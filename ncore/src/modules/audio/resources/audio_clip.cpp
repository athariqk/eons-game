#include <ncore/modules/audio/resources/audio_clip.h>

namespace nc {

AudioClip::AudioClip( const void* p_data, int p_length, int p_channels, int p_frequency, int p_bits_per_sample ) :
    length( p_length ), channels( p_channels ), frequency( p_frequency ), bits_per_sample( p_bits_per_sample )
{
    auto* p = static_cast<const std::byte*>( p_data );
    data.assign( p, p + static_cast<size_t>( p_length ) );
}

std::span<std::byte> AudioClip::get_data()
{
    return data;
}

std::span<const std::byte> AudioClip::get_data() const
{
    return data;
}

int AudioClip::get_length() const
{
    return length;
}

int AudioClip::get_channels() const
{
    return channels;
}

int AudioClip::get_frequency() const
{
    return frequency;
}

int AudioClip::get_bits_per_sample() const
{
    return bits_per_sample;
}

size_t AudioClip::get_size_bytes()
{
    return data.size();
}

} // namespace nc
