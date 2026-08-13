#ifndef PIPEFRAME_TIME_H
#define PIPEFRAME_TIME_H

class Time {
  public:
    static constexpr float FixedDeltaTime = 1.0f / 60.0f;
    static constexpr float MaximumFrameDeltaTime = 0.25f;
};

#endif // PIPEFRAME_TIME_H
