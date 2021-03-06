#pragma once
#include "PCH.h"

#include <Graphics\\Model.h>
#include <Graphics\\Camera.h>
#include <SF11_Math.h>
#include <Timer.h>

#include "Light.h"
#include "BoundUtils.h"

#include "ProbeManager.h"
//#include "CreateCubemap.h"

using namespace SampleFramework11;

struct ModelPartsBound
{
	std::vector<BSphere> BoundingSpheres;
	std::vector<BBox> BoundingBoxes;
	std::vector<uint32> FrustumTests;
	uint32 NumSuccessfulTests;

	ModelPartsBound() : NumSuccessfulTests(0) {}
};

struct SceneObjectBound
{
	ModelPartsBound *modelPartsBound;
	ModelPartsBound *originalModelPartsBound;
	BBox *bbox;
	BSphere *bsphere;
	bool32 frustumTest;
};

struct SceneObject
{
	Float4x4 *base;
	Float4x4 *prevWVP; // this is hack... 
	Model *model;
	SceneObjectBound *bound;
	int id;
};


struct OpaqueObjectDepthCompare
{
	OpaqueObjectDepthCompare(const Float4x4 &viewMat) : ViewMatrix(viewMat) {}
    Float4x4 ViewMatrix;
    bool operator()(const SceneObject &a, const SceneObject &b)
    {
        float depthA = Float3::Transform(a.base->Translation(), ViewMatrix).z;
        float depthB = Float3::Transform(b.base->Translation(), ViewMatrix).z;

		//bool stateSort = a.model == b.model; // TODO: compromise here. Need to find better way
		//return (depthA < depthB) && stateSort;

		return depthA < depthB;
    }
};

struct TransparentObjectDepthCompare
{
	TransparentObjectDepthCompare(const Float4x4 &viewMat) : ViewMatrix(viewMat) {}
	Float4x4 ViewMatrix;
	bool operator()(const SceneObject &a, const SceneObject &b)
	{
		float depthA = Float3::Transform(a.base->Translation(), ViewMatrix).z;
		float depthB = Float3::Transform(b.base->Translation(), ViewMatrix).z;
		return depthA > depthB;
	}
};

// TODO: implement scene graph - currently flat structure
// TODO: support instancing
class SceneScript;
class Scene
{
public:
	Scene();
	~Scene();

	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context, SceneScript *sceneScript, FirstPersonCamera *globalCamera);
	void Update(const Timer& timer);

	void OnSceneChange();

	Model *addModel(const std::wstring &modelPath);
	Model *addBoxModel();
	Model *addPlaneModel();

	inline void setProbeLength(float units) { _unitProbeLength = units; }
	inline float getProbeLength() { return _unitProbeLength; }

	// TODO: note that this approach fails to account for 
	// cases that a model can have semi-static semi-dynamic 
	// OR semi-opaque, semi-transparent structure. 

	// TODO: now static object is not optimized for draw calls (no batching)
	SceneObject *addStaticOpaqueObject(Model *model, float scale=1.0f, const Float3 &pos=Float3(), const Quaternion &rot=Quaternion());
	SceneObject *addDynamicOpaqueObject(Model *model, float scale= 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());

	SceneObject *addDynamicOpaqueBoxObject(float scale = 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());
	SceneObject *addDynamicOpaquePlaneObject(float scale = 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());
	SceneObject *addStaticOpaquePlaneObject(float scale = 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());

	void setProxySceneObject(const std::wstring &modelPath, float scale = 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());
	inline SceneObject *getProxySceneObjectPtr() { return &_proxySceneObject; }
	inline bool32 hasProxySceneObject() { return _hasProxySceneObject; }

	void sortSceneObjects(const Float4x4 &viewMatrix);

	inline int getNumStaticOpaqueObjects() { return _numStaticOpaqueObjects; }
	inline int getNumDynamicOpaueObjects() { return _numDynamicOpaqueObjects; }
	inline int getNumModels() { return (int)_modelIndices.size(); }
	inline Model *getModel(uint64 index) { return &_models[_modelIndices[index]]; }
	// inline MeshData *getModelData(uint64 index) { return &_modelsData[_modelIndices[index]]; }

	inline Float3 getSceneTranslation() { return _sceneTranslation; }
	inline float getSceneScale() { return _sceneScale; }
	inline Quaternion getSceneOrientation() { return _sceneOrientation; }

	inline SceneObject *getStaticOpaqueObjectsPtr() { return _staticOpaqueObjects; }
	inline SceneObject *getDynamicOpaqueObjectsPtr() { return _dynamicOpaqueObjects; }

	inline BBox *getDynamicObjectBBoxPtr() { return _dynamicOpaqueObjectsBBoxes; }
	inline BBox *getStaticObjectBBoxPtr() { return _staticOpaqueObjectsBBoxes; }

	inline FirstPersonCamera *getSceneCameraSavedPtr() { return &_sceneCamSaved; }
	inline FirstPersonCamera *getGlobalCameraPtr() { return _globalCam; }

	// Lights
	PointLight *addPointLight();
	uint32 fillPointLightsUniformGrid(float unitGridSize, float radius, Float3 offset=Float3());
	inline PointLight *getPointLightPtr() { return _pointLights; }
	inline int getNumPointLights() { return _numPointLights; }
	BBox getSceneBoundingBox();

	inline ProbeManager *Scene::getProbeManagerPtr() { return &_probeManager; }

	// Caution: too large will stack overflow
	static const int MAX_STATIC_OBJECTS = 32;
	static const int MAX_DYNAMIC_OBJECTS = 128;
	static const int MAX_MODELS = 64;
	static const int MAX_OBJECT_MATRICES = MAX_STATIC_OBJECTS + MAX_DYNAMIC_OBJECTS;
	static const int MAX_SCENE_LIGHTS = 1024;

	enum SceneObjectFlag
	{
		STATIC_OBJ = 1,
		DYNAMIC_OBJ = 2, 
		OPAQUE_OBJ = 4,
		TRANSPARAENT_OBJ = 8
	};

	
private:
	void genSceneObjectBounds(uint64 objTypeflag, uint64 sceneObjIndex, uint64 modelIndex);
	void genStaticSceneWSAABB();
	void updateDynamicSceneObjectBounds();
	void transformSceneObjectModelPartsBounds(SceneObject *obj);

	Float4x4 createBase(float scale, const Float3 &pos, const Quaternion &rot);
	uint64 getModelIndex(Model *model);

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;

	std::vector<int> _modelIndices;

	Quaternion _sceneOrientation;
	Float3 _sceneTranslation;
	float _sceneScale;

	int _numStaticOpaqueObjects;
	int _numDynamicOpaqueObjects;
	int _numObjectBases;
	int _numPrevWVPs;
	int _numLights;
	int _numPointLights;

	bool _sceneBoundGenerated;

	// TODO: refactor so that a controlled set of scene api is exposed
	// void(*_updateFunc)(Scene *scene, const Timer &timer);

	bool32 _hasProxySceneObject;
	SceneObject _proxySceneObject;
	SceneObject _staticOpaqueObjects[MAX_STATIC_OBJECTS];
	SceneObject _dynamicOpaqueObjects[MAX_DYNAMIC_OBJECTS];

	BBox _staticOpaqueObjectsBBoxes[MAX_STATIC_OBJECTS];
	BBox _dynamicOpaqueObjectsBBoxes[MAX_DYNAMIC_OBJECTS];
	
	// Mainly used for frustum culling
	BSphere _staticOpaqueObjectsBSpheres[MAX_STATIC_OBJECTS];
	BSphere _dynamicOpaqueObjectsBSpheres[MAX_DYNAMIC_OBJECTS];

	ModelPartsBound _sceneStaticOpaqueObjectModelPartsBounds[MAX_STATIC_OBJECTS];
	ModelPartsBound _sceneDynamicOpaqueObjectModelPartsBounds[MAX_DYNAMIC_OBJECTS];
	SceneObjectBound _sceneStaticOpaqueObjectBounds[MAX_STATIC_OBJECTS];
	SceneObjectBound _sceneDynamicOpaqueObjectBounds[MAX_DYNAMIC_OBJECTS];
	

	BBox _sceneWSAABB_staticObj;

	Float4x4 _objectBases[MAX_OBJECT_MATRICES];
	Float4x4 _prevWVPs[MAX_OBJECT_MATRICES];

	PointLight _pointLights[MAX_SCENE_LIGHTS];

	SceneScript *_sceneScript;

	FirstPersonCamera _sceneCamSaved;
	FirstPersonCamera *_globalCam;

	ProbeManager _probeManager;
	
	float _unitProbeLength;

private:

	// TODO: justify the usefulness of object id
	static int _highestSceneObjId;
	static int _numTotalModelsShared;

	static Model _models[MAX_MODELS];
	static ModelPartsBound _modelsData[MAX_MODELS];

	static Model *_boxModel;
	static Model *_planeModel;
	static std::unordered_map<std::wstring, Model *> _modelCache; // share model across scenes
};