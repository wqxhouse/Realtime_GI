#pragma once
#include "PCH.h"

#include <Graphics\\Model.h>
#include <SF11_Math.h>

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

	// TODO: note that this approach fails to account for 
	// cases that a model can have semi-static semi-dynamic 
	// OR semi-opaque, semi-transparent structure. 

	// TODO: now static object is not optimized for draw calls (no batching)
	SceneObject *addStaticOpaqueObject(Model *model, float scale=1.0f, const Float3 &pos=Float3(), const Quaternion &rot=Quaternion());
	SceneObject *addDynamicOpaqueObject(Model *model, float scale= 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());

	void sortSceneObjects(const Float4x4 &viewMatrix);

	inline int getNumStaticOpaqueObjects() { return _numStaticObjects;  }
	inline int getNumDynmamicOpaueObjects() { return _numDynamicObjects; }
	inline int getNumModels() { return _numModels; }

	inline SceneObject *getStaticOpaqueObjectsPtr() { return _staticOpaqueObjects; }
	inline SceneObject *getDynamicOpaqueObjectsPtr() { return _dynamicOpaqueObjects; }
	inline Model *getModelsPtr() { return _models; }
	
private:
	Float4x4 createBase(float scale, const Float3 &pos, const Quaternion &rot);

	static const int MAX_STATIC_OBJECTS = 256;
	static const int MAX_DYNAMIC_OBJECTS = 1024;
	static const int MAX_MODELS = 64;
	static const int MAX_OBJECT_MATRICES = MAX_STATIC_OBJECTS + MAX_DYNAMIC_OBJECTS;

	ID3D11Device *_device;

	int _numModels;
	int _numStaticObjects;
	int _numDynamicObjects;
	int _numObjectBases;
	int _numPrevWVPs;

	SceneObject _staticOpaqueObjects[MAX_STATIC_OBJECTS];
	SceneObject _dynamicOpaqueObjects[MAX_DYNAMIC_OBJECTS];
	Model _models[MAX_MODELS];
	Float4x4 _objectBases[MAX_OBJECT_MATRICES];
	Float4x4 _prevWVPs[MAX_OBJECT_MATRICES];

	//std::vector<SceneObject> _staticOpaqueObjects;
	//std::vector<SceneObject> _dynamicOpaqueObjects;
	/*std::vector<SceneObject> _staticTransparentObjects;
	std::vector<SceneObject> _dynamicTransparentObjects;*/
	//std::vector<Model> _models;
	//std::vector<Float4x4> _objectBases;
	//std::vector<Float4x4> _prevWVPs;

	static int _highest_id;
};