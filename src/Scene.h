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
	Scene() {};

	void Initialize(ID3D11Device *device);

	Model *addModel(const std::wstring &modelPath);

	// TODO: note that this approach fails to account for 
	// cases that a model can have semi-static semi-dynamic 
	// OR semi-opaque, semi-transparent structure. 

	// TODO: now static object is not optimized for draw calls (no batching)
	SceneObject *addStaticOpaqueObject(Model *model, float scale=1.0f, const Float3 &pos=Float3(), const Quaternion &rot=Quaternion());
	SceneObject *addDynamicOpaqueObject(Model *model, float scale= 1.0f, const Float3 &pos = Float3(), const Quaternion &rot = Quaternion());

	void sortSceneObjects(const Float4x4 &viewMatrix);

	std::vector<SceneObject> _staticOpaqueObjects;
	std::vector<SceneObject> _dynamicOpaqueObjects;
	/*std::vector<SceneObject> _staticTransparentObjects;
	std::vector<SceneObject> _dynamicTransparentObjects;*/

	std::vector<Model> _models;

private:
	Float4x4 createBase(float scale, const Float3 &pos, const Quaternion &rot);

	ID3D11Device *_device;

	std::vector<Float4x4> _objectBases;
	std::vector<Float4x4> _prevWVPs;

	static int _highest_id;
};