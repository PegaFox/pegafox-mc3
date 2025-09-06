#include <chrono>

class Clock {
  public:
    float fps = 0.0f;
    uint32_t frameCount = 0;
    float deltaTime = 0.0f;

    // if newFrame is false, this function only updates deltaTime
    float get_fps(bool newFrame = true) {
      auto frameTime = std::chrono::system_clock::now();
      auto time = frameTime - lastFrameTime;
      if (newFrame)
      {
        lastFrameTime = frameTime;
        frameCount++;
        fps = 1000000000.0f/std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
      }

      deltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(time).count()/1000000000.0f;

      return fps;
    }

    // NOTE: must be run once every loop cycle with get_fps(true) alongside it
    void cap_fps(double maxFps) {
      std::chrono::system_clock::time_point step;
      std::chrono::milliseconds currentTime;
      do {
        step = std::chrono::system_clock::now();
        currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(step - lastFrameTime);
      } while (currentTime.count() <= 1000.0 / maxFps);
    }
  private:
    std::chrono::system_clock::time_point lastFrameTime = std::chrono::system_clock::now();
};
