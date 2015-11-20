#include "Scene.h"
#include "FileIO.h"

int Scene::_highestSceneObjId = 0;
int Scene::_numTotalModelsShared = 0;
Model *Scene::_boxModel = nullptr;
Model *Scene::_planeModel = nullptr;
Model Scene::_models[Scene::MAX_MODELS];
//SceneObject Scene::_staticOpaqueObjects[Scene::MAX_STATIC_OBJECTS];
//SceneObject Scene::_dynamicOpaqueObjects[Scene::MAX_DYNAMIC_OBJECTS];
//Float4x4 Scene::_objectBases[Scene::MAX_OBJECT_MATRICES];
//Float4x4 Scene::_prevWVPs[Scene::MAX_OBJECT_MATRICES];
std::unordered_map<std::wstring, Model *> Scene::_modelCache;

Scene::Scene()
{
	_device = NULL;
	
	_numStaticObjects = 0;
	_numDynamicObjects = 0;
	_numObjectBases = 0;
	_numPrevWVPs = 0;

	_sceneScale = 1;
	_numLights = 0;
	_numPointLights = 0;
}

Scene::~Scene()
{
}

void Scene::Initialize(ID3D11Device *device)
{
	_device = device;
}

Model *Scene::addBoxModel()
{
	if (!_boxModel)
	{
		_models[_numTotalModelsShared++].GenerateBoxScene(_device);
		_boxModel = &_models[_numTotalModelsShared - 1];
	}
	return _boxModel;
}

Model *Scene::addPlaneModel()
{
	if (!_planeModel)
	{
		_models[_numTotalModelsShared++].GeneratePlaneScene(_device, 1.0, Float3(), Quaternion(), L"", L"Bricks_NML.dds");
		_planeModel = &_models[_numTotalModelsShared - 1];
	}

	return _planeModel;
}

Model *Scene::addModel(const std::wstring &modelPath)
{
	std::wstring dir = GetDirectoryFromFilePath(modelPath.c_str());
	std::wstring name = GetFileName(modelPath.c_str());
	std::wstring fullPath = dir + name;

	if (_modelCache.find(fullPath) != _modelCache.end())
	{
		return _modelCache[fullPath];
	}

	std::wstring ext = GetFileExtension(modelPath.c_str());

	// TODO: error handling
	if (ext == L"meshdata")
	{
		_models[_numTotalModelsShared].CreateFromMeshData(_device, fullPath.c_str());
		_numTotalModelsShared++;
	}
	else if (ext == L"sdkmesh")
	{
		_models[_numTotalModelsShared].CreateFromSDKMeshFile(_device, fullPath.c_str());
		_numTotalModelsShared++;
	}
	else
	{
		_models[_numTotalModelsShared].CreateWithAssimp(_device, fullPath.c_str());
		_numTotalModelsShared++;
	}

	_modelIndices.push_back(_numTotalModelsShared - 1);
	_modelCache.insert(std::make_pair(fullPath, &_models[_numTotalModelsShared - 1]));
	return &_models[_numTotalModelsShared - 1];
}


SceneObject *Scene::addDynamicOpaqueBoxObject(float scale, const Float3 &pos, const Quaternion &rot)
{
	if (!_boxModel)
	{
		addBoxModel();
		_modelIndices.push_back(_numTotalModelsShared - 1);
	}

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = _boxModel;
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	_numObjectBases++;
	_numPrevWVPs++;
	_numDynamicObjects++;

	return &obj;
}

SceneObject *Scene::addDynamicOpaquePlaneObject(float scale, const Float3 &pos, const Quaternion &rot)
{
	if (!_planeModel)
	{
		addPlaneModel();
		_modelIndices.push_back(_numTotalModelsShared - 1);
	}

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = _planeModel;
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	_numObjectBases++;
	_numPrevWVPs++;
	_numDynamicObjects++;
	
	return &obj;
}

SceneObject *Scene::addStaticOpaqueObject(Model *model, float scale, const Float3 &pos, const Quaternion &rot)
{
	Assert_(model != nullptr);
	Assert_(_numObjectBases < MAX_STATIC_OBJECTS);
	Assert_(_numTotalModelsShared < MAX_MODELS);
	Assert_(_numObjectBases < MAX_OBJECT_MATRICES);
	Assert_(_numPrevWVPs < MAX_OBJECT_MATRICES);

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _staticOpaqueObjects[_numStaticObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = model;
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	_numObjectBases++;
	_numPrevWVPs++;
	_numStaticObjects++;

	return &obj;
}

SceneObject *Scene::addDynamicOpaqueObject(Model *model, float scale, const Float3 &pos, const Quaternion &rot)
{
	Assert_(model != nullptr);
	Assert_(_numObjectBases < MAX_STATIC_OBJECTS);
	Assert_(_numTotalModelsShared < MAX_MODELS);
	Assert_(_numObjectBases < MAX_OBJECT_MATRICES);
	Assert_(_numPrevWVPs < MAX_OBJECT_MATRICES);

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = model;
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

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

PointLight *Scene::addPointLight()
{
	if (_numLights > MAX_SCENE_LIGHTS) return NULL;
	_pointLights[_numPointLights] = PointLight();
	_pointLights[_numPointLights].cColor = Float3(0.1f, 0.3f, 0.7f);
	_pointLights[_numPointLights].cPos = Float3();
	_pointLights[_numPointLights].cRadius = 1.0f;

	_numPointLights++;
	_numLights++;

	return &_pointLights[_numPointLights - 1];
}