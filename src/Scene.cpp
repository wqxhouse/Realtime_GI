#include "Scene.h"
#include "FileIO.h"

int Scene::_highestSceneObjId = 0;
int Scene::_numTotalModelsShared = 0;
Model *Scene::_boxModel = nullptr;
Model *Scene::_planeModel = nullptr;
Model Scene::_models[Scene::MAX_MODELS];
ModelPartsBound Scene::_modelsData[Scene::MAX_MODELS];
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

	_sceneWSAABB_staticObj.Max = XMFLOAT3(0, 0, 0);
	_sceneWSAABB_staticObj.Min = XMFLOAT3(0, 0, 0);

	_updateFunc = NULL;

	_hasProxySceneObject = false;
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
		ModelPartsBound & data = (_modelsData[_numTotalModelsShared - 1] = ModelPartsBound());
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
		ModelPartsBound & data = (_modelsData[_numTotalModelsShared - 1] = ModelPartsBound());
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
		ModelPartsBound & data = (_modelsData[_numTotalModelsShared] = ModelPartsBound());
		ComputeModelBounds(_device, _context, &_models[_numTotalModelsShared], data.BoundingSpheres, data.BoundingBoxes);
		_numTotalModelsShared++;
	}
	else if (ext == L"sdkmesh")
	{
		_models[_numTotalModelsShared].CreateFromSDKMeshFile(_device, fullPath.c_str());
		ModelPartsBound & data = (_modelsData[_numTotalModelsShared] = ModelPartsBound());
		ComputeModelBounds(_device, _context, &_models[_numTotalModelsShared], data.BoundingSpheres, data.BoundingBoxes);
		_numTotalModelsShared++;
	}
	else
	{
		_models[_numTotalModelsShared].CreateWithAssimp(_device, fullPath.c_str());
		ModelPartsBound & data = (_modelsData[_numTotalModelsShared] = ModelPartsBound());
		ComputeModelBounds(_device, _context, &_models[_numTotalModelsShared], data.BoundingSpheres, data.BoundingBoxes);
		_numTotalModelsShared++;
	}

	_modelIndices.push_back(_numTotalModelsShared - 1);
	_modelCache.insert(std::make_pair(fullPath, &_models[_numTotalModelsShared - 1]));

	return &_models[_numTotalModelsShared - 1];
}

void Scene::setProxySceneObject(const std::wstring &modelPath, float scale, const Float3 &pos, const Quaternion &rot)
{
	Model *m = addModel(modelPath);
	_objectBases[_numObjectBases] = createBase(scale, pos, rot);
	_prevWVPs[_numPrevWVPs] = _objectBases[_numObjectBases];

	SceneObject &obj = _proxySceneObject;
	obj.base = &_objectBases[_numObjectBases];
	obj.model = m;
	obj.bound = nullptr;
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	_numObjectBases++;
	_numPrevWVPs++;

	_hasProxySceneObject = true;
}

void Scene::genSceneObjectBounds(uint64 objTypeflag, uint64 sceneObjIndex, uint64 modelIndex)
{
	bool isstatic = (objTypeflag & STATIC_OBJ) > 0;
	bool isopaque = (objTypeflag & OPAQUE_OBJ) > 0;
	SceneObject *sceneObj = nullptr;
	SceneObjectBound *sceneObjBound = nullptr;
	ModelPartsBound *sceneModelPartsBound = nullptr;
	BBox *sceneObjBBox = nullptr;
	BSphere *sceneObjBSphere = nullptr;

	if (isstatic)
	{
		if (isopaque)
		{
			sceneObj = &_staticOpaqueObjects[sceneObjIndex];
			sceneObjBound = &_sceneStaticOpaqueObjectBounds[sceneObjIndex];
			sceneObjBBox = &_staticOpaqueObjectsBBoxes[sceneObjIndex];
			sceneObjBSphere = &_staticOpaqueObjectsBSpheres[sceneObjIndex];
			sceneModelPartsBound = &_sceneStaticOpaqueObjectModelPartsBounds[sceneObjIndex];
		}
	}
	else
	{	
		if (isopaque)
		{
			sceneObj = &_dynamicOpaqueObjects[sceneObjIndex];
			sceneObjBound = &_sceneDynamicOpaqueObjectBounds[sceneObjIndex];
			sceneObjBBox = &_dynamicOpaqueObjectsBBoxes[sceneObjIndex];
			sceneObjBSphere = &_dynamicOpaqueObjectsBSpheres[sceneObjIndex];
			sceneModelPartsBound = &_sceneDynamicOpaqueObjectModelPartsBounds[sceneObjIndex];
		}
	}

	// memcpy(sceneModelPartsBound, &_modelsData[modelIndex], sizeof(ModelPartsBound));
	sceneModelPartsBound->FrustumTests = { };
	sceneModelPartsBound->NumSuccessfulTests = 0;
	sceneModelPartsBound->BoundingBoxes.resize(_modelsData[modelIndex].BoundingBoxes.size());
	sceneModelPartsBound->BoundingSpheres.resize(_modelsData[modelIndex].BoundingSpheres.size());
	std::copy(_modelsData[modelIndex].BoundingBoxes.begin(), _modelsData[modelIndex].BoundingBoxes.end(), sceneModelPartsBound->BoundingBoxes.begin());
	std::copy(_modelsData[modelIndex].BoundingSpheres.begin(), _modelsData[modelIndex].BoundingSpheres.end(), sceneModelPartsBound->BoundingSpheres.begin());
	
	sceneObjBound->modelPartsBound = sceneModelPartsBound;
	sceneObjBound->frustumTest = false;
	sceneObjBound->originalModelPartsBound = &_modelsData[modelIndex];
	sceneObjBound->bbox = sceneObjBBox;
	sceneObjBound->bsphere = sceneObjBSphere;

	transformSceneObjectModelPartsBounds(sceneObj);

	// TODO: cache merge bounds of the same model
	BBox bbox = MergeBoundingBoxes(sceneModelPartsBound->BoundingBoxes);
	BSphere bsphere = MergeBoundingSpheres(sceneModelPartsBound->BoundingSpheres);
	*sceneObjBBox = bbox;
	*sceneObjBSphere = bsphere;
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
	_sceneStaticOpaqueObjectBounds[_numStaticOpaqueObjects] = SceneObjectBound();

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicOpaqueObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = _boxModel;
	obj.bound = &_sceneDynamicOpaqueObjectBounds[_numDynamicOpaqueObjects];
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	genSceneObjectBounds(DYNAMIC_OBJ | OPAQUE_OBJ, _numDynamicOpaqueObjects, modelIndex);

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
	_sceneStaticOpaqueObjectBounds[_numStaticOpaqueObjects] = SceneObjectBound();

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicOpaqueObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = _planeModel;
	obj.bound = &_sceneDynamicOpaqueObjectBounds[_numDynamicOpaqueObjects];
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	
	genSceneObjectBounds(DYNAMIC_OBJ | OPAQUE_OBJ, _numDynamicOpaqueObjects, modelIndex);

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
	_sceneStaticOpaqueObjectBounds[_numStaticOpaqueObjects] = SceneObjectBound();

	SceneObject &obj = _staticOpaqueObjects[_numStaticOpaqueObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = model;
	obj.bound = &_sceneStaticOpaqueObjectBounds[_numStaticOpaqueObjects];
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	genSceneObjectBounds(STATIC_OBJ | OPAQUE_OBJ, _numStaticOpaqueObjects, modelIndex);

	_numObjectBases++;
	_numPrevWVPs++;
	_numStaticOpaqueObjects++;

	genStaticSceneWSAABB();

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
	_sceneStaticOpaqueObjectBounds[_numStaticOpaqueObjects] = SceneObjectBound();

	SceneObject &obj = _dynamicOpaqueObjects[_numDynamicOpaqueObjects];
	obj.base = &_objectBases[_numObjectBases];
	obj.model = model;
	obj.bound = &_sceneDynamicOpaqueObjectBounds[_numDynamicOpaqueObjects];
	obj.prevWVP = &_prevWVPs[_numPrevWVPs];
	obj.id = _highestSceneObjId++;

	genSceneObjectBounds(DYNAMIC_OBJ | OPAQUE_OBJ, _numDynamicOpaqueObjects, modelIndex);

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

		transformSceneObjectModelPartsBounds(obj);

		BBox bbox = MergeBoundingBoxes(obj->bound->modelPartsBound->BoundingBoxes);
		BSphere bsphere = MergeBoundingSpheres(obj->bound->modelPartsBound->BoundingSpheres);

		_dynamicOpaqueObjectsBBoxes[_numDynamicOpaqueObjects] = bbox;
		_dynamicOpaqueObjectsBSpheres[_numDynamicOpaqueObjects] = bsphere;
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

void Scene::transformSceneObjectModelPartsBounds(SceneObject *obj)
{
	Float4x4 &transform = *obj->base;
	ModelPartsBound *partsBound = obj->bound->originalModelPartsBound;
	ModelPartsBound *dstPartsBound = obj->bound->modelPartsBound;

	for (size_t i = 0; i < partsBound->BoundingSpheres.size(); i++)
	{
		BBox &orig_bbox = partsBound->BoundingBoxes[i];
		BSphere &orig_bsphere = partsBound->BoundingSpheres[i];

		BBox &dst_bbox = dstPartsBound->BoundingBoxes[i];
		BSphere &dst_bsphere = dstPartsBound->BoundingSpheres[i];

		dst_bbox = GetTransformedBBox(orig_bbox, transform);
		dst_bsphere = GetTransformedBSphere(orig_bsphere, transform);
	}
	
}

