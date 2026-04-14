#pragma once

namespace Time 
{	
	extern float CurrentTime;
	extern float DeltaTime;
	extern float FixedDeltaTime;

	extern float lastFrame;
	extern float currentFrame;
	extern float fixedTimeAccumulator;

	void Update();
	void ResetFixedTime();
	unsigned int ConsumeFixedSteps(float frameDeltaTime);
	const float FrameRate();
}
