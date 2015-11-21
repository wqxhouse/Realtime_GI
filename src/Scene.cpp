#include "Scene.h"
#include "FileIO.h"

int Scene::_highestSceneObjId = 0;
int Scene::_numTotalModelsShared = 0;
Model *Scene::_boxModel = nullptr;
Model *Scene::_planeModel = nullptr;
Model Scene::_models[Scene::MAX_MODELS];
MeshData Scene::_modelsData[Scene::MAX_MODELS];
//SceneObject Scene::_staticOpaqueObjects[Scene::MAX_STATIC_OBJECTS];
//SceneObject Scene::_dynamicOpaqueObjects[Scene::MAX_DYNAMIC_OBJECTS];
//Float4x4 Scene::_objectBases[Scene::MAX_OBJECT_MATRICES];
//Float4x4 Scene::_prevWVPs[Scene::MAX_OBJECT_MATRICES];
std::unordered_map<std::wstring, Model *> Scene::_modelCache;

Scene::Scene()
{
	_device = NULL;
	
	_numStaticOpaqueObjects = 0;
	_numDynamicOpaqueObjects = 0;
	_numObjectBases = 0;
	_numPrevWVPs = 0;

	_sceneScale = 1;
	_numLights = 0;
	_numPointLights = 0;
	_sceneBoundGenerated = false;

	_updateFunc = NULL;
}

Scene::~Scene()
{
}

void Scene::Initialize(ID3D11Device *device, ID3D11DeviceContext *context)
{
	_device = device;
	_context = context;
}

void Scene::Update(const Timer& timer)
{
	if (_sceneBoundGenerated == false)
	{
		genStaticSceneWSAABB();
		_sceneBoundGenerated = true;
	}

	if (_updateFunc)
	{
		_updateFunc(this, timer);
	}

	updateDynamicSceneObjectBounds();
}

void Scene::SetUpdateFunction(void(*update)(Scene *scene, const Timer &timer))
{
	_updateFunc = update;
}

Model *Scene::addBoxModel()
{
	if (!_boxModel)
	{
		_models[_numTotalModelsShared++].GenerateBoxScene(_device);
		MeshData & data = (_modelsData[_numTotalModelsShared - 1] = MeshData());
		ComputeModelBounds(_device, _context, &_models[_numTotalModelsShared - 1], data.BoundingSpheres, data.BoundingBoxes);
		_boxModel = &_models[_numTotalModelsShared - 1];
	}
	return _boxModel;
}

Model *Scene::addPlaneModel()
{
	if (!_planeModel)
	{
		_models[_numTotalModelsShared++].GeneratePlaneScene(_device, 1.0, Float3(), Quaternion(), L"", L"Bricks_NML.dds");
		MeshData & data = (_modelsData[_numTotalModelsShared - 1] = MeshData());
		ComputeModelBounds(_device, _context, &_models[_numTotalModelsShared - 1], data.BoundingSpheres, data.BoundingBoxes);
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
		MeshData & data = (_modelsData[_numTotalModelsShared] = MeshData());
		ComputeModelBounds(_device, _context, &_models[_numTotalModelsShared], data.BoundingSpheres, data.BoundingBoxes);
		_numTotalModelsShared++;
	}
	else if (ext == L"sdkmesh")
	{
		_models[_numTotalModelsShared].CreateFromSDKMeshFile(_device, fullPath.c_str());
		MeshData & data = (_modelsData[_numTotalModelsShared] = MeshData());
		ComputeModelBounds(_device, _context, &_models[_numTotalModelsShared], data.BoundingSpheres, data.BoundingBoxes);
		_numTotalModelsShared++;
	}
	else
	{
		_models[_numTotalModelsShared].CreateWithAssimp(_device, fullPath.c_str());
		MeshData & data = (_modelsData[_numTotalModelsShared] = MeshData());
		ComputeModelBounds(_device, _context, &_models[_numTotalModelsShared], data.BoundingSpheres, data.BoundingBoxes);
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

	uint64 modelIndex = getModelIndex(_boxModel);
	Assert_(modelIndex != -1);

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicOpaqueObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = _boxModel;
	obj.modelData = &_modelsData[modelIndex];
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	// TODO: cache merge bounds of the same model
	BBox bbox = MergeBoundingBoxes(obj.modelData->BoundingBoxes);
	BSphere bsphere = MergeBoundingSpheres(obj.modelData->BoundingSpheres);
	_dynamicOpaqueObjectsBBoxes[_numDynamicOpaqueObjects] = GetTransformedBBox(bbox, *obj.base);
	_dynamicOpaqueObjectsBSpheres[_numDynamicOpaqueObjects] = GetTransformedBSphere(bsphere, *obj.base);

	_numObjectBases++;
	_numPrevWVPs++;
	_numDynamicOpaqueObjects++;

	return &obj;
}

SceneObject *Scene::addDynamicOpaquePlaneObject(float scale, const Float3 &pos, const Quaternion &rot)
{
	if (!_planeModel)
	{
		addPlaneModel();
		_modelIndices.push_back(_numTotalModelsShared - 1);
	}

	uint64 modelIndex = getModelIndex(_planeModel);
	Assert_(modelIndex != -1);

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicOpaqueObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = _planeModel;
	obj.modelData = &_modelsData[modelIndex];
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	// TODO: cache merge bounds of the same model
	BBox bbox = MergeBoundingBoxes(obj.modelData->BoundingBoxes);
	BSphere bsphere = MergeBoundingSpheres(obj.modelData->BoundingSpheres);
	_dynamicOpaqueObjectsBBoxes[_numDynamicOpaqueObjects] = GetTransformedBBox(bbox, *obj.base);
	_dynamicOpaqueObjectsBSpheres[_numDynamicOpaqueObjects] = GetTransformedBSphere(bsphere, *obj.base);

	_numObjectBases++;
	_numPrevWVPs++;
	_numDynamicOpaqueObjects++;
	
	return &obj;
}

SceneObject *Scene::addStaticOpaqueObject(Model *model, float scale, const Float3 &pos, const Quaternion &rot)
{
	Assert_(model != nullptr);
	Assert_(_numObjectBases < MAX_STATIC_OBJECTS);
	Assert_(_numTotalModelsShared < MAX_MODELS);
	Assert_(_numObjectBases < MAX_OBJECT_MATRICES);
	Assert_(_numPrevWVPs < MAX_OBJECT_MATRICES);

	uint64 modelIndex = getModelIndex(model);
	Assert_(modelIndex != -1);

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _staticOpaqueObjects[_numStaticOpaqueObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = model;
	obj.modelData = &_modelsData[modelIndex];
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	// TODO: cache merge bounds of the same model
	BBox bbox = MergeBoundingBoxes(obj.modelData->BoundingBoxes);
	BSphere bsphere = MergeBoundingSpheres(obj.modelData->BoundingSpheres);
	_staticOpaqueObjectsBBoxes[_numStaticOpaqueObjects] = GetTransformedBBox(bbox, *obj.base);
	_staticOpaqueObjectsBSpheres[_numStaticOpaqueObjects] = GetTransformedBSphere(bsphere, *obj.base);

	_numObjectBases++;
	_numPrevWVPs++;
	_numStaticOpaqueObjects++;

	return &obj;
}

SceneObject *Scene::addDynamicOpaqueObject(Model *model, float scale, const Float3 &pos, const Quaternion &rot)
{
	Assert_(model != nullptr);
	Assert_(_numObjectBases < MAX_STATIC_OBJECTS);
	Assert_(_numTotalModelsShared < MAX_MODELS);
	Assert_(_numObjectBases < MAX_OBJECT_MATRICES);
	Assert_(_numPrevWVPs < MAX_OBJECT_MATRICES);

	uint64 modelIndex = getModelIndex(model);
	Assert_(modelIndex != -1);

	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicOpaqueObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = model;
	obj.modelData = &_modelsData[modelIndex];
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	// TODO: cache merge bounds of the same model
	BBox bbox = MergeBoundingBoxes(obj.modelData->BoundingBoxes);
	BSphere bsphere = MergeBoundingSpheres(obj.modelData->BoundingSpheres);
	_dynamicOpaqueObjectsBBoxes[_numDynamicOpaqueObjects] = GetTransformedBBox(bbox, *obj.base);
	_dynamicOpaqueObjectsBSpheres[_numDynamicOpaqueObjects] = GetTransformedBSphere(bsphere, *obj.base);

	_numObjectBases++;
	_numPrevWVPs++;
	_numDynamicOpaqueObjects++;

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
	std::sort(_staticOpaqueObjects, _staticOpaqueObjects + _numStaticOpaqueObjects, opqCmp);
	std::sort(_dynamicOpaqueObjects, _dynamicOpaqueObjects + _numDynamicOpaqueObjects, opqCmp);
	
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

void Scene::genStaticSceneWSAABB()
{
	if (_numStaticOpaqueObjects == 0) return;

	std::vector<BBox> temp(_numStaticOpaqueObjects);
	memcpy(&temp[0], _staticOpaqueObjectsBBoxes, sizeof(BBox) * _numStaticOpaqueObjects);
	_sceneWSAABB_staticObj = MergeBoundingBoxes(temp);
}

void Scene::updateDynamicSceneObjectBounds()
{
	for (uint64 i = 0; i < _numDynamicOpaqueObjects; i++)
	{
		SceneObject *obj = &_dynamicOpaqueObjects[i];
		BBox *bbox = &_dynamicOpaqueObjectsBBoxes[i];
		BSphere *bsphere = &_dynamicOpaqueObjectsBSpheres[i];

		_dynamicOpaqueObjectsBBoxes[_numDynamicOpaqueObjects] = GetTransformedBBox(*bbox, *obj->base);
		_dynamicOpaqueObjectsBSpheres[_numDynamicOpaqueObjects] = GetTransformedBSphere(*bsphere, *obj->base);
	}
}

uint64 Scene::getModelIndex(Model *model)
{
	for (uint64 i = 0; i < _numTotalModelsShared; i++)
	{
		if (&_models[i] == model)
		{
			return i;
		}
	}

	return -1;
}

BBox Scene::getSceneBoundingBox()
{
	return _sceneWSAABB_staticObj;
}

