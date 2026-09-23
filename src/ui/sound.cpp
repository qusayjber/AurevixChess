#include "ui/sound.hpp"
#include <cmath>
#include <cstdint>
#include <vector>

namespace ui {

namespace {
constexpr unsigned SAMPLE_RATE = 44100;
}

sf::SoundBuffer SoundManager::make_tone(float freq, float duration_ms, float attack_s, float decay_s, float amp) {
    const unsigned n = static_cast<unsigned>(SAMPLE_RATE * (duration_ms / 1000.f));
    std::vector<int16_t> samples(n);
    const float T = duration_ms / 1000.f;
    const float invA = attack_s > 0 ? 1.f/attack_s : 1.f;
    const float invD = decay_s  > 0 ? 1.f/decay_s  : 1.f;
    for (unsigned i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / SAMPLE_RATE;
        float env;
        if (t < attack_s) env = t * invA;
        else              env = std::exp(-(t - attack_s) * invD);
        float s = std::sin(2.f * 3.14159265358979f * freq * t);
        // Add a bit of harmonic for body
        s += 0.25f * std::sin(2.f * 3.14159265358979f * freq * 2.f * t);
        samples[i] = static_cast<int16_t>(std::clamp(s * env * amp * 32000.f, -32000.f, 32000.f));
        (void)T;
    }
    sf::SoundBuffer b;
    b.loadFromSamples(samples.data(), samples.size(), 1, SAMPLE_RATE);
    return b;
}

SoundManager::SoundManager() {
    buffers_[int(Sfx::Move)]      = make_tone(620.f, 90.f,  0.003f, 30.f, 0.35f);
    buffers_[int(Sfx::Capture)]   = make_tone(340.f, 130.f, 0.002f, 22.f, 0.42f);
    buffers_[int(Sfx::Check)]     = make_tone(880.f, 180.f, 0.003f, 16.f, 0.40f);
    buffers_[int(Sfx::Checkmate)] = make_tone(520.f, 420.f, 0.005f,  8.f, 0.45f);
    buffers_[int(Sfx::Illegal)]   = make_tone(150.f,  90.f, 0.002f, 26.f, 0.32f);
    buffers_[int(Sfx::Promote)]   = make_tone(1040.f, 200.f, 0.003f, 14.f, 0.42f);
    buffers_[int(Sfx::UI)]        = make_tone(760.f,  60.f, 0.002f, 40.f, 0.28f);
    for (auto& v : voices_) v.setBuffer(buffers_[0]);
}

void SoundManager::play(Sfx s) {
    if (!enabled_ || volume_ <= 0.f) return;
    sf::Sound& v = voices_[next_voice_];
    next_voice_ = (next_voice_ + 1) % voices_.size();
    v.setBuffer(buffers_[int(s)]);
    v.setVolume(volume_ * 100.f);
    v.play();
}

} // namespace ui