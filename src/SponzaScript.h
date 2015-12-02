//#include "SceneScriptBase.h"
//
//class SponzaScript : public SceneScript
//{
//public:
//	virtual void InitScene(Scene *scene)
//	{
//		std::wstring modelPath = L"C:\\Users\\wqxho_000\\Downloads\\SponzaPBR_Textures\\SponzaPBR_Textures\\Converted\\sponza_new.obj";
//		// std::wstring proxyModelPath = L"..\\Content\\Models\\CornellBox\\UVUnwrapped\\cbox_unwrapped.FBX";
//
//		Model *model = scene->addModel(modelPath);
//		scene->addStaticOpaqueObject(model, 0.01f, Float3(0, 0, 0), Quaternion());
//		// scene->setProxySceneObject(proxyModelPath, 0.1f, Float3(0, 0, 0), Quaternion());
//
//		//// scene->fillPointLightsUniformGrid(50.0f, 100.0f);
//		//// scene->fillPointLightsUniformGrid(1.3f, 3.0f, Float3(0, 0, 0));
//		scene->getGlobalCameraPtr()->SetLookAt(Float3(0.0f, 2.5f, -10.0f), Float3(0.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f));
//
//		// set specular probes
//		/*scene->getProbeManagerPtr()->AddProbe(Float3(0, 0, 0), Float3(2, 2, 2));
//		scene->getProbeManagerPtr()->AddProbe(Float3(-1, 0, 0), Float3(2, 2, 2));*/
//	}
//
//	virtual void Update(Scene *scene, const SampleFramework11::Timer *timer)
//	{
//
//	}
//};