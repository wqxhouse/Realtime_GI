#include "SceneScriptBase.h"

float dt = 1.0f / 60.0f;
q3Scene scenegb(dt);

std::vector<q3Body *> bodylist;
std::vector<SceneObject *> meshlist;


class DropBoxesScript: public SceneScript
{
public:
	virtual void InitScene(Scene *scene)
	{
		std::wstring proxyModelPath = L"..\\Content\\Models\\CornellBox\\UVUnwrapped\\cbox_unwrapped.FBX";
		std::wstring cubePath = L"..\\Content\\Models\\Cube\\cube.FBX";
		modelcube = scene->addModel(cubePath);
		scene->setProxySceneObject(proxyModelPath, 0.1f, Float3(0, 0, 0), Quaternion());

		//scene->getGlobalCameraPtr()->SetLookAt(Float3(0.0f, 2.5f, -10.0f), Float3(0.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f));

		scenegb.SetGravity(q3Vec3(0, -100.0f, 0));
		//drop boxes
		acc = 0;

		// Create the floor
		q3BodyDef bodyDef;
		q3Body* body = scenegb.CreateBody(bodyDef);
		//bodylist.push_back(body);

		
		SceneObject* doo = scene->addStaticOpaquePlaneObject(40.0f, Float3(0, 0, 0), Quaternion());
		//meshlist.push_back(doo);
		//scene->getSceneBoundingBox();
		q3BoxDef boxDef;
		boxDef.SetRestitution(0);
		q3Transform tx;
		q3Identity(tx);

		boxDef.Set(tx, q3Vec3(40.0f, 1.0f, 50.0f));
		body->AddBox(boxDef);
	}

	virtual void Update(Scene *scene, const SampleFramework11::Timer *timer)
	{
		acc += dt;
		if (acc > 1.5f)
		{
			acc = 0;

			q3BodyDef bodyDef;
			bodyDef.position.Set(q3RandomFloat(-20.0f, 20.0f), 10.0f, q3RandomFloat(-20.0f, 20.0f));
			bodyDef.axis.Set(q3RandomFloat(-1.0f, 1.0f), q3RandomFloat(-1.0f, 1.0f), q3RandomFloat(-1.0f, 1.0f));
			bodyDef.angle = q3PI * q3RandomFloat(-1.0f, 1.0f);
			bodyDef.bodyType = eDynamicBody;
			bodyDef.angularVelocity.Set(q3RandomFloat(1.0f, 3.0f), q3RandomFloat(1.0f, 3.0f), q3RandomFloat(1.0f, 3.0f));
			bodyDef.angularVelocity *= q3Sign(q3RandomFloat(-1.0f, 1.0f));
			bodyDef.linearVelocity.Set(q3RandomFloat(1.0f, 3.0f), q3RandomFloat(1.0f, 3.0f), q3RandomFloat(1.0f, 3.0f));
			bodyDef.linearVelocity *= q3Sign(q3RandomFloat(-1.0f, 1.0f));
			q3Body* body = scenegb.CreateBody(bodyDef);

			bodylist.push_back(body);


			SceneObject* doo = scene->addDynamicOpaqueObject(modelcube, 0.02f, Float3(bodyDef.position.x, bodyDef.position.y, bodyDef.position.z), Quaternion());
			//SceneObject* doo = scene->addDynamicOpaqueBoxObject( 0.02f, Float3(0, 10, 0), Quaternion());
			meshlist.push_back(doo);
			float radius = doo->bound->bsphere->Radius;
			float lengthbox = 3;
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
			ret.Scale(Float3(0.02f));
			ret.SetTranslation(position);

			//meshlist.at(i)->base ->SetTranslation(position);
			*(meshlist.at(i)->base) = ret;//->SetTranslation(position);


		}
	}
	float acc;
	Model *modelcube;
};