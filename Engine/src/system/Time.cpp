#include "system/Time.h"

#include <glfw3.h>

namespace Time
{
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

		fixedTimeAccumulator += frameDeltaTime;

		unsigned int fixedSteps = 0;
		while (fixedTimeAccumulator >= FixedDeltaTime)
		{
			fixedTimeAccumulator -= FixedDeltaTime;
			fixedSteps++;
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
