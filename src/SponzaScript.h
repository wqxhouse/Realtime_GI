#include "SceneScriptBase.h"

class SponzaScript : public SceneScript
{
public:
	virtual void InitScene(Scene *scene)
	{
		//std::wstring modelPath = L"C:\\Users\\wqxho_000\\Downloads\\SponzaPBR_Textures\\SponzaPBR_Textures\\Converted\\sponza_new.obj";
		//// std::wstring proxyModelPath = L"C:\\Users\\wqxho_000\\Downloads\\SponzaPBR_Textures\\SponzaPBR_Textures\\Converted\\sponza_unwrapped.fbx";
		//std::wstring proxyModelPath = L"C:\\Users\\wqxho_000\\Downloads\\SponzaPBR_Textures\\SponzaPBR_Textures\\Converted\\sponza_high_res_unwrapped.fbx";

		/*std::wstring modelPath = L"C:\\MyUnity513WS\\Converted\\sponza_new.obj";
		std::wstring proxyModelPath = L"C:\\MyUnity513WS\\Converted\\sponza_high_res_unwrapped.fbx";*/
		
		std::wstring modelPath = L"..\\Content\\Models\\sphere\\sphere_new.FBX";
		std::wstring proxyModelPath = L"..\\Content\\Models\\CornellBox\\UVUnwrapped\\cbox_unwrapped.FBX";
		Model *model = scene->addModel(modelPath);
		scene->addStaticOpaqueObject(model, 0.03f, Float3(0, 0, 0), Quaternion());
		scene->setProxySceneObject(proxyModelPath, 0.03f, Float3(0, 0, 0), Quaternion());

		//// scene->fillPointLightsUniformGrid(50.0f, 100.0f);
		//// scene->fillPointLightsUniformGrid(1.3f, 3.0f, Float3(0, 0, 0));
		scene->getGlobalCameraPtr()->SetLookAt(Float3(0.0f, 2.5f, -10.0f), Float3(0.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f));

		// set specular probes
		/*scene->getProbeManagerPtr()->AddProbe(Float3(0, 0, 0), Float3(2, 2, 2));
		scene->getProbeManagerPtr()->AddProbe(Float3(-1, 0, 0), Float3(2, 2, 2));*/
		scene->setProbeLength(6.0f);
		PointLight *l = scene->addPointLight();
		l->cRadius = 100.0f;
		l->cPos = Float3(0, 1, 0);
		l->cPos = Float3(1, 0, 0) * 40;



	//C:\MyUnity513WS\Converted
	}

	virtual void Update(Scene *scene, const SampleFramework11::Timer *timer)
	{

	}
};
