// has three write-only bytes used for communication, the first two are data bytes, the third is a mode byte
class Speaker: public ExternalDevice<uint16_t>
{
  public:
    virtual uint8_t read(uint16_t position)
    {
      return 0;
    }

    virtual void write(uint16_t position, uint8_t value)
    {
      if (position == 2)
      {
        mode.operation = Mode::Operation(value >> 4);
        mode.selectedSound = value & 0xF;
      }

      switch (mode.operation)
      {
        case Mode::Listen:
          if (position == 0)
          {
            int16_t samples[sounds[mode.selectedSound].getSampleCount()+1];

            std::copy(sounds[mode.selectedSound].getSamples(), sounds[mode.selectedSound].getSamples() + sounds[mode.selectedSound].getSampleCount(), samples);

            samples[sounds[mode.selectedSound].getSampleCount()] = value;

            if (!sounds[mode.selectedSound].loadFromSamples(samples, sounds[mode.selectedSound].getSampleCount()+1, 1, 256, std::vector<sf::SoundChannel>{sf::SoundChannel::Mono}))
            {
              std::cerr << "Failed to load sound\n";
            }
          } else if (position == 1)
          {
            int16_t samples[sounds[mode.selectedSound].getSampleCount()];

            std::copy(sounds[mode.selectedSound].getSamples(), sounds[mode.selectedSound].getSamples() + sounds[mode.selectedSound].getSampleCount(), samples);

            samples[sounds[mode.selectedSound].getSampleCount()-1] |= uint16_t(value) << 8;

            if (!sounds[mode.selectedSound].loadFromSamples(samples, sounds[mode.selectedSound].getSampleCount(), 1, 256, std::vector<sf::SoundChannel>{sf::SoundChannel::Mono}))
            {
              std::cerr << "Failed to load sound\n";
            }
          }
          break;
        case Mode::Play:
          if (position == 2)
          {
            speaker.setBuffer(sounds[mode.selectedSound]);
            speaker.play();
          }
          break;
        case Mode::Volume:
          if (position == 0)
          {
            speaker.setVolume(float(value)/255.0f * 100.0f);
          }
          break;
        case Mode::Stop:
          speaker.stop();
          break;
        case Mode::New:
          if (!sounds[mode.selectedSound].loadFromSamples(nullptr, 0, 1, 256, std::vector<sf::SoundChannel>{sf::SoundChannel::Mono}))
          {
            std::cerr << "Failed to load sound\n";
          }
          break;
      }
    }
  private:
    sf::SoundBuffer sounds[16];
    sf::Sound speaker = sf::Sound(sounds[0]);

    struct Mode {
      enum Operation
      {
        Listen,
        Play,
        Volume,
        Stop,
        New
      } operation;

      uint8_t selectedSound;
    } mode;
};