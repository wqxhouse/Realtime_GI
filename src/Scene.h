#pragma once
#include "PCH.h"

#include <Graphics\\Model.h>
#include <SF11_Math.h>

#include "Light.h"

using namespace SampleFramework11;

struct SceneObject
{
	Float4x4 *base;
	Float4x4 *prevWVP; // this is hack... 
	Model *model;
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

class Scene
{
public:
	Scene();
	~Scene();

	void Initialize(ID3D11Device *device);

	Model *addModel(const std::wstring &modelPath);
	Model *addBoxModel();
	Model *addPlaneModel();

	// TODO: note that this approach fails to account for 
	// cases that a model can have semi-static semi-dynamic 
	// OR semi-opaque, semi-transparent structure. 

	// TODO: now static object is not optimized for draw calls (no batching)
	SceneObject *addStaticOpaqueObject(Model *model, float scale=1.0f, const Float3 &pos=Float3(), const Quaternion &rot=Quaternion());
	SceneObject *addDynamicOpaqueObject(Model *model, float scale= 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());

	SceneObject *addDynamicOpaqueBoxObject(float scale = 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());
	SceneObject *addDynamicOpaquePlaneObject(float scale = 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());

	void sortSceneObjects(const Float4x4 &viewMatrix);

	inline int getNumStaticOpaqueObjects() { return _numStaticObjects;  }
	inline int getNumDynmamicOpaueObjects() { return _numDynamicObjects; }
	inline int getNumModels() { return (int)_modelIndices.size(); }
	inline Model *getModel(uint64 index) { return &_models[_modelIndices[index]]; }

	inline Float3 getSceneTranslation() { return _sceneTranslation; }
	inline float getSceneScale() { return _sceneScale;  }

	inline SceneObject *getStaticOpaqueObjectsPtr() { return _staticOpaqueObjects; }
	inline SceneObject *getDynamicOpaqueObjectsPtr() { return _dynamicOpaqueObjects; }

	inline Quaternion getSceneOrientation()
	{
		return _sceneOrientation;
	}

	// Lights
	PointLight *addPointLight();
	inline PointLight *getPointLightPtr() { return _pointLights; }
	inline int getNumPointLights() { return _numPointLights; }

	// Caution: too large will stack overflow
	static const int MAX_STATIC_OBJECTS = 32;
	static const int MAX_DYNAMIC_OBJECTS = 128;
	static const int MAX_MODELS = 64;
	static const int MAX_OBJECT_MATRICES = MAX_STATIC_OBJECTS + MAX_DYNAMIC_OBJECTS;
	static const int MAX_SCENE_LIGHTS = 1024;
	
private:

	ID3D11Device *_device;

	std::vector<int> _modelIndices;
	Quaternion _sceneOrientation;
	Float3 _sceneTranslation;

	float _sceneScale;
	int _numStaticObjects;
	int _numDynamicObjects;
	int _numObjectBases;
	int _numPrevWVPs;
	int _numLights;
	int _numPointLights;

	Float4x4 createBase(float scale, const Float3 &pos, const Quaternion &rot);

	SceneObject _staticOpaqueObjects[MAX_STATIC_OBJECTS];
	SceneObject _dynamicOpaqueObjects[MAX_DYNAMIC_OBJECTS];
	Float4x4 _objectBases[MAX_OBJECT_MATRICES];
	Float4x4 _prevWVPs[MAX_OBJECT_MATRICES];

	PointLight _pointLights[MAX_SCENE_LIGHTS];

	// TODO: justify the usefulness of object id
	static int _highestSceneObjId;
	static int _numTotalModelsShared;
	static Model _models[MAX_MODELS];
	static Model *_boxModel;
	static Model *_planeModel;
	static std::unordered_map<std::wstring, Model *> _modelCache; // share model across scenes
};