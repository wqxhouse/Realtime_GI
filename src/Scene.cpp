#include "Scene.h"
#include "FileIO.h"

int Scene::_highest_id = 0;

Scene::Scene()
{
	_device = NULL;
	_numModels = 0;
	_numStaticObjects = 0;
	_numDynamicObjects = 0;
	_numObjectBases = 0;
	_numPrevWVPs = 0;
}

Scene::~Scene()
{
}

void Scene::Initialize(ID3D11Device *device)
{
	_device = device;
}

Model *Scene::addModel(const std::wstring &modelPath)
{
	std::wstring ext = GetFileExtension(modelPath.c_str());

	// TODO: error handling
	if (ext == L"meshdata")
	{
		_models[_numModels].CreateFromMeshData(_device, modelPath.c_str());
		_numModels++;
	}
	else if (ext == L"sdkmesh")
	{
		_models[_numModels].CreateFromSDKMeshFile(_device, modelPath.c_str());
		_numModels++;
	}
	else
	{
		_models[_numModels].CreateWithAssimp(_device, modelPath.c_str());
		_numModels++;
	}

	return &_models[_numModels - 1];
}

SceneObject *Scene::addStaticOpaqueObject(Model *model, float scale, const Float3 &pos, const Quaternion &rot)
{
	Assert_(model != nullptr);
	Assert_(_numObjectBases < MAX_STATIC_OBJECTS);
	Assert_(_numModels < MAX_MODELS);
	Assert_(_numObjectBases < MAX_OBJECT_MATRICES);
	Assert_(_numPrevWVPs < MAX_OBJECT_MATRICES);

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _staticOpaqueObjects[_numStaticObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = model;
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highest_id++;

	_numObjectBases++;
	_numPrevWVPs++;
	_numStaticObjects++;

	return &obj;
}

SceneObject *Scene::addDynamicOpaqueObject(Model *model, float scale, const Float3 &pos, const Quaternion &rot)
{
	Assert_(model != nullptr);
	Assert_(_numObjectBases < MAX_STATIC_OBJECTS);
	Assert_(_numModels < MAX_MODELS);
	Assert_(_numObjectBases < MAX_OBJECT_MATRICES);
	Assert_(_numPrevWVPs < MAX_OBJECT_MATRICES);

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = model;
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highest_id++;

	_numObjectBases++;
	_numPrevWVPs++;
	_numDynamicObjects++;

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
	std::sort(_staticOpaqueObjects, _staticOpaqueObjects + _numStaticObjects, opqCmp);
	std::sort(_dynamicOpaqueObjects, _dynamicOpaqueObjects + _numDynamicObjects, opqCmp);
	
	// TODO: transparent
}