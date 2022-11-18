#ifndef __PATHTRACER_H__
#define __PATHTRACER_H__

#include <string>
#include <vector>
#include <random>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "mesh.h"
#include "wave.h"

enum class MaterialType
{
	DIFFUSE,
	SPECULAR,
	GLOSSY,
	GLASS
};

struct Material
{
	MaterialType type = MaterialType::DIFFUSE;
	glm::vec3 baseColor = glm::vec3(1.0f);
	float roughness = 0.0f;
	glm::vec3 emissive = glm::vec3(0.0f);

	int normalTexId = -1;

	/* ---- SPECTRUM PARAMS ---- */
	int spectrumMatId = -1;
	float temperature = 0.0f;
	int temperatureTexId = -1;
	Wave reflectivity;
	Wave emissivity;
};

namespace PathTracerLoader
{
	struct Element
	{
		std::string name;
		Material material;

		Element()
		{
			name = "";
		}

		Element(const std::string& name)
		{
			this->name = name;
		}
	};

	struct Object
	{
		std::string name;
		std::vector<Element> elements;

		Object()
		{
			name = "";
		}

		Object(const std::string& name)
		{
			this->name = name;
		}
	};
}

struct SpectrumMaterial
{
	std::string name = "";
	std::vector<float> emissivity{};
	
	// GUI-related stuff
	bool isSelected = false;
	int editingWaveId = -1;
};

class PathTracer
{
private:
	std::vector<Triangle> mTriangles;
	BVHNode* mBvh;

	std::vector<PathTracerLoader::Object> mLoadedObjects;
	std::vector<Image*> mLoadedTextures;

	glm::ivec2 mResolution;
	GLubyte* mOutImg;
	float* mTotalImg;
	int mMaxDepth;

	glm::vec3 mCamPos;
	glm::vec3 mCamDir;
	glm::vec3 mCamUp;
	float mCamFocal;
	float mCamFovy;

	int mSamples;
	bool mNeedReset;
	bool mExit;

	std::mt19937 mRng;

	/* ----- SPECTRUM PARAMS ----- */
	std::vector<float> mWaveLengths;
	std::vector<SpectrumMaterial> mSpectrumMaterials;

	Wave* mOutSpectrumResult;
	Wave* mTotalSpectrumResult;

	Wave mWaveSky;
	/* ----- SPECTRUM PARAMS ----- */

public:
	PathTracer();
	~PathTracer();

private:
	/* ----- SPECTRUM FUNCTIONS ----- */
	const float BBP(float temperature, int waveId) const;
	const Wave GetReflectivity(int materialId) const;
	const Wave GetEmissivity(int materialId, float temperature) const;
	/* ----- SPECTRUM FUNCTIONS ----- */

	const float Rand();
	const glm::vec2 GetUV(const glm::vec3& p, const Triangle& t) const;
	const glm::vec3 GetSmoothNormal(const glm::vec3& p, const Triangle& t) const;
	const Wave Trace(const glm::vec3& ro, const glm::vec3& rd, int depth = 0, bool inside = false);

public:
	void LoadObject(const std::string& file, const glm::mat4& model);
	void SetNormalTextureForElement(int objId, int elementId, const std::string& file);
	void SetMaterial(int objId, int elementId, Material& material);
	void BuildBVH();
	void ResetImage();
	void ClearScene();

	const int GetSamples() const;
	const int GetTriangleCount() const;
	const int GetTraceDepth() const;
	void SetTraceDepth(int depth);
	void SetOutImage(GLubyte* out);
	void SetResolution(const glm::ivec2& res);
	const glm::ivec2 GetResolution() const;
	std::vector<PathTracerLoader::Object> GetLoadedObjects() const;

	/* ----- SPECTRUM FUNCTIONS ----- */
	void InitializeSpectrumMaterials();
	void SetOutSpectrumResult(Wave* res);
	void SetWaveLengths(const std::vector<float>& waves);
	void SetSpectrumMaterials(const std::vector<SpectrumMaterial>& materials);
	void SetSky(int materialId, float temperature);
	void SetTemperatureTextureForElement(int objId, int elementId, const std::string& file);
	/* ----- SPECTRUM FUNCTIONS ----- */

	void SetCamera(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& up);
	void SetProjection(float f, float fovy);
	void RenderFrame();
	void Exit();
};

#endif
