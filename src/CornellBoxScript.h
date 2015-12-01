#include "SceneScriptBase.h"

float dt = 1.0f / 60.0f;
q3Scene scenegb(dt);

std::vector<q3Body *> bodylist;
std::vector<SceneObject *> meshlist;
class CornellBoxScript : public SceneScript
{
public:
	virtual void InitScene(Scene *scene)
	{
		std::wstring modelPath = L"..\\Content\\Models\\CornellBox\\CornellBox_fbx.FBX";
		std::wstring proxyModelPath = L"..\\Content\\Models\\CornellBox\\UVUnwrapped\\cbox_unwrapped.FBX";

		Model *model = scene->addModel(modelPath);
		scene->addStaticOpaqueObject(model, 0.1f, Float3(0, 0, 0), Quaternion());
		scene->setProxySceneObject(proxyModelPath, 0.1f, Float3(0, 0, 0), Quaternion());

		//// scene->fillPointLightsUniformGrid(50.0f, 100.0f);
		//// scene->fillPointLightsUniformGrid(1.3f, 3.0f, Float3(0, 0, 0));
		scene->getGlobalCameraPtr()->SetLookAt(Float3(0.0f, 2.5f, -10.0f), Float3(0.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f));

		// set specular probes
		scene->getProbeManagerPtr()->AddProbe(Float3(0, 0, 0), Float3(2, 2, 2));
		scene->getProbeManagerPtr()->AddProbe(Float3(-1, 0, 0), Float3(2, 2, 2));

		//drop boxes
		acc = 0;

		// Create the floor
		q3BodyDef bodyDef;
		q3Body* body = scenegb.CreateBody(bodyDef);
		//bodylist.push_back(body);

		Model *model1 = scene->addModel(modelPath);
		//SceneObject* doo = scene->addStaticOpaqueObject(model1, 0.1f, Float3(0, 0, 0), Quaternion());
		SceneObject* doo = scene->addStaticOpaquePlaneObject( 50.0f, Float3(0, 0, 0), Quaternion());
		//meshlist.push_back(doo);
		//scene->getSceneBoundingBox();
		q3BoxDef boxDef;
		boxDef.SetRestitution(0);
		q3Transform tx;
		q3Identity(tx);

		boxDef.Set(tx, q3Vec3(50.0f, 1.0f, 50.0f));
		body->AddBox(boxDef);
	}

	virtual void Update(Scene *scene, const SampleFramework11::Timer *timer)
	{
		std::wstring modelPath = L"..\\Content\\Models\\CornellBox\\CornellBox_fbx.FBX";

		//Float4x4 camMatrix = scene->getGlobalCameraPtr()->WorldMatrix();
		//Float4x4 rotation = Quaternion::FromAxisAngle(Float3(0, 1, 0), timer->DeltaSecondsF()).ToFloat4x4();
		//camMatrix = camMatrix * rotation;
		//scene->getGlobalCameraPtr()->SetWorldMatrix(camMatrix);

		acc += dt;
		if (acc > 1.0f)
		{
			acc = 0;

			q3BodyDef bodyDef;
			bodyDef.position.Set(0.0f, 20.0f, 0.0f);
			bodyDef.axis.Set(q3RandomFloat(-1.0f, 1.0f), q3RandomFloat(-1.0f, 1.0f), q3RandomFloat(-1.0f, 1.0f));
			bodyDef.angle = q3PI * q3RandomFloat(-1.0f, 1.0f);
			bodyDef.bodyType = eDynamicBody;
			bodyDef.angularVelocity.Set(q3RandomFloat(1.0f, 3.0f), q3RandomFloat(1.0f, 3.0f), q3RandomFloat(1.0f, 3.0f));
			bodyDef.angularVelocity *= q3Sign(q3RandomFloat(-1.0f, 1.0f));
			bodyDef.linearVelocity.Set(q3RandomFloat(1.0f, 3.0f), q3RandomFloat(1.0f, 3.0f), q3RandomFloat(1.0f, 3.0f));
			bodyDef.linearVelocity *= q3Sign(q3RandomFloat(-1.0f, 1.0f));
			q3Body* body = scenegb.CreateBody(bodyDef);

			bodylist.push_back(body);

			Model *model1 = scene->addModel(modelPath);
			SceneObject* doo = scene->addDynamicOpaqueObject(model1, 0.1f, Float3(0, 20, 0), Quaternion());
			meshlist.push_back(doo);
			float radius = doo->bound->bsphere->Radius;
			float lengthbox = sqrt(2.0f)*radius;
			q3Transform tx;

			q3Identity(tx);
			q3BoxDef boxDef;
			boxDef.Set(tx, q3Vec3(lengthbox, lengthbox, lengthbox));
			body->AddBox(boxDef);

		}

		//drop boxes
		scenegb.Step();

		for (int i = 0; i < bodylist.size(); i++)
		{
			q3Vec3 temp = bodylist.at(i)->GetTransform().position;
			
			q3Quaternion temp2 = bodylist.at(i)->GetQuaternion();

			Float3 position = Float3(temp.x, temp.y, temp.z);
			Quaternion rot = Quaternion(temp2.x, temp2.y, temp2.z, temp2.w);

			Quaternion rotNorm = Quaternion::Normalize(rot);
			Float4x4 ret = rotNorm.ToFloat4x4();
			ret.Scale(Float3(1.0f));
			ret.SetTranslation(position);

			meshlist.at(i)->base = &ret;
		}
		
	}

	float acc;
};