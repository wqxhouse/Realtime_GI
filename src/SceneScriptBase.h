#pragma once
class Scene;

namespace SampleFramework11
{
	class Timer;
}

class SceneScript
{
public:
	virtual void InitScene(Scene *scene) = 0;
	virtual void Update(Scene *scene, const SampleFramework11::Timer *timer) = 0;
};