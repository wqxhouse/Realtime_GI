#include "Scene.h"
#include "FileIO.h"

int Scene::_highest_id = 0;

void Scene::Initialize(ID3D11Device *device)
{
	_device = device;
}

Model *Scene::addModel(const std::wstring &modelPath)
{
	std::wstring ext = GetFileExtension(modelPath.c_str());
	_models.resize(_models.size() + 1);

	if (ext == L"meshdata")
	{
		_models.back().CreateFromMeshData(_device, modelPath.c_str());
	}
	else if (ext == L"sdkmesh")
	{
		_models.back().CreateFromSDKMeshFile(_device, modelPath.c_str());
	}
	else if (ext == L"")
	{
		_models.back().CreateWithAssimp(_device, modelPath.c_str());
	}

	return &_models.back();
}

SceneObject *Scene::addStaticOpaqueObject(Model *model, float scale, const Float3 &pos, const Quaternion &rot)
{
	Assert_(model != nullptr);
	_staticOpaqueObjects.resize(_staticOpaqueObjects.size() + 1);
	_objectBases.resize(_objectBases.size() + 1);
	_objectBases.back() = createBase(scale, pos, rot);
	_prevWVPs.resize(_prevWVPs.size() + 1);

	SceneObject &obj = _staticOpaqueObjects.back();
	obj.base = &_objectBases.back();
	obj.model = model;
	obj.prevWVP = &_prevWVPs.back();
	obj.id = _highest_id++;

	return &obj;
}

SceneObject *Scene::addDynamicOpaqueObject(Model *model, float scale, const Float3 &pos, const Quaternion &rot)
{
	Assert_(model != nullptr);
	_dynamicOpaqueObjects.resize(_dynamicOpaqueObjects.size() + 1);
	_objectBases.resize(_objectBases.size() + 1);
	_objectBases.back() = createBase(scale, pos, rot);
	_prevWVPs.resize(_prevWVPs.size() + 1);
	
	SceneObject &obj = _dynamicOpaqueObjects.back();
	obj.base = &_objectBases.back();
	obj.model = model;
	obj.prevWVP = &_prevWVPs.back();
	obj.id = _highest_id++;;

	return &obj;
}

Float4x4 Scene::createBase(float scale, const Float3 &pos, const Quaternion &rot)
{
	// Assume uniform scaling
	Quaternion rotNorm = Quaternion::Normalize(rot);
	Float4x4 ret = rotNorm.ToFloat4x4();
	ret.Scale(Float3(scale));
	ret.SetTranslation(pos);
	return ret;
}

void Scene::sortSceneObjects(const Float4x4 &viewMatrix)
{
	// opaque
	OpaqueObjectDepthCompare opqCmp(viewMatrix);
	std::sort(_staticOpaqueObjects.begin(), _staticOpaqueObjects.end(), opqCmp);
	std::sort(_dynamicOpaqueObjects.begin(), _dynamicOpaqueObjects.end(), opqCmp);
	
	// TODO: transparent
}