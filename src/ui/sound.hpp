#pragma once
#include <SFML/Audio.hpp>
#include <array>
#include <string>

namespace ui {

enum class Sfx { Move, Capture, Check, Checkmate, Illegal, Promote, UI };

class SoundManager {
public:
    SoundManager();
    void set_enabled(bool on) { enabled_ = on; }
    bool enabled() const { return enabled_; }
    void set_volume(float v01) { volume_ = v01; }
    float volume() const { return volume_; }
    void play(Sfx s);

private:
    sf::SoundBuffer buffers_[7];
    std::array<sf::Sound, 8> voices_;
    size_t next_voice_ = 0;
    bool enabled_ = true;
    float volume_ = 0.5f;

    static sf::SoundBuffer make_tone(float freq_hz, float duration_ms, float attack_s, float decay_s, float amplitude);
};

} // namespace ui