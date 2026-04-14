#include "system/Time.h"

#include <glfw3.h>

namespace Time
{
	constexpr float MaxFixedFrameDeltaTime = 0.25f;
	constexpr unsigned int MaxFixedStepsPerFrame = 8;

	float CurrentTime = 0.0f;
	float DeltaTime = 0.0f;
	float FixedDeltaTime = 1.0f / 60.0f;

	float lastFrame = 0.0f;
	float currentFrame = 0.0f;
	float fixedTimeAccumulator = 0.0f;

	void Update()
	{
		CurrentTime = static_cast<float>(glfwGetTime());
		currentFrame = CurrentTime;
		DeltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
	}

	void ResetFixedTime()
	{
		fixedTimeAccumulator = 0.0f;
	}

	unsigned int ConsumeFixedSteps(float frameDeltaTime)
	{
		if (FixedDeltaTime <= 0.0f)
		{
			ResetFixedTime();
			return 0;
		}

		if (frameDeltaTime < 0.0f)
		{
			frameDeltaTime = 0.0f;
		}
		else if (frameDeltaTime > MaxFixedFrameDeltaTime)
		{
			frameDeltaTime = MaxFixedFrameDeltaTime;
		}

		fixedTimeAccumulator += frameDeltaTime;

		unsigned int fixedSteps = 0;
		while (fixedTimeAccumulator >= FixedDeltaTime && fixedSteps < MaxFixedStepsPerFrame)
		{
			fixedTimeAccumulator -= FixedDeltaTime;
			fixedSteps++;
		}

		if (fixedSteps == MaxFixedStepsPerFrame && fixedTimeAccumulator >= FixedDeltaTime)
		{
			fixedTimeAccumulator = 0.0f;
		}

		return fixedSteps;
	}
	
	const float FrameRate()
	{
		if (DeltaTime == 0.0f)
			return 0.0f;
		return 1.0f / DeltaTime;
	}
}
