#define _USE_MATH_DEFINES
#include <sstream>
#include <math.h>

#include <omp.h>

#include <tiny_obj_loader.h>

#include "pathtracer.h"

PathTracer::PathTracer() : mRng(std::random_device()())
{
	mOutImg = 0;
	mTotalImg = 0;
	mMaxDepth = 3;

	mCamDir = glm::vec3(0.0f, 0.0f, 1.0f);
	mCamUp = glm::vec3(0.0f, 1.0f, 0.0f);
	mCamFocal = 0.1f;
	mCamFovy = 90;

	mSamples = 0;
	mNeedReset = false;
	mExit = false;

	mOutSpectrumResult = 0;
	mTotalSpectrumResult = 0;
}

PathTracer::~PathTracer()
{
	if (mTotalImg)
		delete[] mTotalImg;

	if (mBvh)
		delete mBvh;

	for (auto texture : mLoadedTextures)
		delete texture;

	if (mTotalSpectrumResult)
		delete[] mTotalSpectrumResult;
}

void PathTracer::LoadObject(std::string file, glm::mat4 model)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file.c_str()))
	{
		int nameStartIndex = file.find_last_of('/') + 1;
		if (nameStartIndex > file.size() - 1)
			nameStartIndex = 0;
		int nameEndIndex = file.find_last_of(".");
		if (nameEndIndex == std::string::npos)
			nameEndIndex = file.size() - 1;
		std::string objName = file.substr(nameStartIndex, nameEndIndex - nameStartIndex);
		PathTracerLoader::Object obj(objName);

		for (int i = 0; i < shapes.size(); i++)
		{
			std::string elementName = shapes[i].name;
			PathTracerLoader::Element element(elementName);
			obj.elements.push_back(element);

			for (int j = 0; j < shapes[i].mesh.num_face_vertices.size(); j++)
			{
				if (shapes[i].mesh.num_face_vertices[j] != 3)
					continue;

				Triangle t;

				int vi = shapes[i].mesh.indices[j * 3].vertex_index;
				int ti = shapes[i].mesh.indices[j * 3].texcoord_index;
				int ni = shapes[i].mesh.indices[j * 3].normal_index;
				t.v1 = glm::vec3(-attrib.vertices[3 * vi],
					attrib.vertices[3 * vi + 1],
					attrib.vertices[3 * vi + 2]);
				t.v1 = glm::vec3(model * glm::vec4(t.v1, 1.0f));
				if (attrib.normals.size() != 0)
				{
					t.n1 = glm::vec3(-attrib.normals[3 * ni],
						attrib.normals[3 * ni + 1],
						attrib.normals[3 * ni + 2]);
					t.n1 = glm::vec3(model * glm::vec4(t.n1, 0.0f));
				}
				if (attrib.texcoords.size() != 0)
				{
					t.uv1 = glm::vec2(attrib.texcoords[2 * ti],
						1.0f - attrib.texcoords[2 * ti + 1]);
				}

				vi = shapes[i].mesh.indices[j * 3 + 1].vertex_index;
				ti = shapes[i].mesh.indices[j * 3 + 1].texcoord_index;
				ni = shapes[i].mesh.indices[j * 3 + 1].normal_index;
				t.v2 = glm::vec3(-attrib.vertices[3 * vi],
					attrib.vertices[3 * vi + 1],
					attrib.vertices[3 * vi + 2]);
				t.v2 = glm::vec3(model * glm::vec4(t.v2, 1.0f));
				if (attrib.normals.size() != 0)
				{
					t.n2 = glm::vec3(-attrib.normals[3 * ni],
						attrib.normals[3 * ni + 1],
						attrib.normals[3 * ni + 2]);
					t.n2 = glm::vec3(model * glm::vec4(t.n2, 0.0f));
				}
				if (attrib.texcoords.size() != 0)
					t.uv2 = glm::vec2(attrib.texcoords[2 * ti],
						1.0f - attrib.texcoords[2 * ti + 1]);

				vi = shapes[i].mesh.indices[j * 3 + 2].vertex_index;
				ti = shapes[i].mesh.indices[j * 3 + 2].texcoord_index;
				ni = shapes[i].mesh.indices[j * 3 + 2].normal_index;
				t.v3 = glm::vec3(-attrib.vertices[3 * vi],
					attrib.vertices[3 * vi + 1],
					attrib.vertices[3 * vi + 2]);
				t.v3 = glm::vec3(model * glm::vec4(t.v3, 1.0f));
				if (attrib.normals.size() != 0)
				{
					t.n3 = glm::vec3(-attrib.normals[3 * ni],
						attrib.normals[3 * ni + 1],
						attrib.normals[3 * ni + 2]);
					t.n3 = glm::vec3(model * glm::vec4(t.n3, 0.0f));
				}
				if (attrib.texcoords.size() != 0)
				{
					t.uv3 = glm::vec2(attrib.texcoords[2 * ti],
						1.0f - attrib.texcoords[2 * ti + 1]);
				}

				t.Init();

				if (shapes[i].mesh.smoothing_group_ids.size() != 0)
				{
					if (shapes[i].mesh.smoothing_group_ids[j] != 0)
						t.smoothing = true;
				}

				t.objectId = mLoadedObjects.size();
				t.elementId = i;

				mTriangles.push_back(t);
			}
		}
		mLoadedObjects.push_back(obj);
	}
}

void PathTracer::SetNormalTextureForElement(int objId, int elementId, std::string file)
{
	Material& mat = mLoadedObjects[objId].elements[elementId].material;
	if (mat.normalTexId != -1)
		mLoadedTextures[mat.normalTexId]->Load(file);
	else
	{
		Image* texture = new Image(file);
		mat.normalTexId = mLoadedTextures.size();
		mLoadedTextures.push_back(texture);
	}
}

void PathTracer::SetTemperatureTextureForElement(int objId, int elementId, std::string file)
{
	Material& mat = mLoadedObjects[objId].elements[elementId].material;
	if (mat.temperatureTexId != -1)
		mLoadedTextures[mat.temperatureTexId]->Load(file);
	else
	{
		Image* texture = new Image(file);
		mat.temperatureTexId = mLoadedTextures.size();
		mLoadedTextures.push_back(texture);
	}
}

void PathTracer::SetMaterial(int objId, int elementId, Material& material)
{
	if (objId >= mLoadedObjects.size())
		return;
	if (elementId >= mLoadedObjects[objId].elements.size())
		return;

	material.normalTexId = mLoadedObjects[objId].elements[elementId].material.normalTexId;

	mLoadedObjects[objId].elements[elementId].material = material;
}

void PathTracer::BuildBVH()
{
	if (mBvh)
		delete mBvh;
	mBvh = new BVHNode();
	mBvh->Construct(mTriangles);
}

void PathTracer::ResetImage()
{
	mNeedReset = true;
}

void PathTracer::ClearScene()
{
	mTriangles.swap(std::vector<Triangle>());
	mLoadedObjects.swap(std::vector<PathTracerLoader::Object>());
	if (mBvh)
		delete mBvh;
	mBvh = 0;
	for (auto texture : mLoadedTextures)
		delete texture;
	mLoadedTextures.swap(std::vector<Image*>());

	if (mTotalImg)
		delete[] mTotalImg;
	mTotalImg = 0;
	if (mTotalSpectrumResult)
		delete[] mTotalSpectrumResult;
	mTotalSpectrumResult = 0;
}

void PathTracer::SetOutImage(GLubyte* out)
{
	mOutImg = out;
}

void PathTracer::SetWaveLengths(std::vector<float>& waves)
{
	mWaveLengths = waves;
}

void PathTracer::SetResolution(glm::ivec2 res)
{
	mResolution = res;
	mTotalImg = new float[res.x * res.y * 3];

	mTotalSpectrumResult = new Wave[res.x * res.y];
	for (int i = 0; i < res.x * res.y; i++)
		mTotalSpectrumResult[i].Initialize(mWaveLengths.size());
}

std::vector<PathTracerLoader::Object> PathTracer::GetLoadedObjects()
{
	return mLoadedObjects;
}

void PathTracer::SetSpectrumMaterials(std::vector<SpectrumMaterial>& materials)
{
	mSpectrumMaterials = materials;
}

void PathTracer::InitializeSpectrumMaterials()
{
	for (auto& obj : mLoadedObjects)
	{
		for (auto& elements : obj.elements)
		{
			if (elements.material.spectrumMatId == -1)
			{
				elements.material.reflectivity.Initialize(mWaveLengths.size());
				elements.material.emissivity.Initialize(mWaveLengths.size());
			}
			else
			{
				elements.material.reflectivity = GetReflectivity(elements.material.spectrumMatId);
				elements.material.emissivity = GetEmissivity(elements.material.spectrumMatId,
					elements.material.temperature);
			}
		}
	}
}

void PathTracer::SetSky(int materialId, float temperature)
{
	if (materialId == -1 || materialId >= mSpectrumMaterials.size())
	{
		mWaveSky.Initialize(mWaveLengths.size());
		return;
	}

	Wave waveSky(mWaveLengths.size());
	for (int i = 0; i < waveSky.size(); i++)
		waveSky[i] = BBP(temperature + 273.15f, i) * mSpectrumMaterials[materialId].emissivity[i];
	mWaveSky = waveSky;
}

glm::ivec2 PathTracer::GetResolution()
{
	return mResolution;
}

int PathTracer::GetTriangleCount()
{
	return mTriangles.size();
}

int PathTracer::GetTraceDepth()
{
	return mMaxDepth;
}

void PathTracer::SetTraceDepth(int depth)
{
	mMaxDepth = depth;
}

void PathTracer::SetOutSpectrumResult(Wave* result)
{
	mOutSpectrumResult = result;
}

void PathTracer::SetCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up)
{
	mCamPos = pos;
	mCamDir = glm::normalize(dir);
	mCamUp = glm::normalize(up);
}

void PathTracer::SetProjection(float f, float fovy)
{
	mCamFocal = f;
	if (mCamFocal <= 0.0f)
		mCamFocal = 0.1f;
	mCamFovy = fovy;
	if (mCamFovy <= 0.0f)
		mCamFovy = 0.1f;
	else if (mCamFovy >= 180.0f)
		mCamFovy = 179.5;
}

int PathTracer::GetSamples()
{
	return mSamples;
}

float PathTracer::BBP(float temperature, int waveId)
{
	float c = 299792458.0f;
	float k = 138064852e-31;
	float h = 2.0f * M_PI * 105457180e-42;

	float v = mWaveLengths[waveId];
	float T = temperature;
	return 2e8 * (h * c * c * v * v * v) / (exp(100.0f * h * c * v / k / T) - 1.0f);
}

Wave PathTracer::GetReflectivity(int materialId)
{
	Wave refl(mWaveLengths.size());
	for (int i = 0; i < mWaveLengths.size(); i++)
		refl[i] = 1.0f - mSpectrumMaterials[materialId].emissivity[i];
	return refl;
}

Wave PathTracer::GetEmissivity(int materialId, float temperature)
{
	Wave emis(mWaveLengths.size());
	for (int i = 0; i < mWaveLengths.size(); i++)
		emis[i] = BBP(temperature + 273.15f, i) * mSpectrumMaterials[materialId].emissivity[i];
	return emis;
}

float PathTracer::Rand()
{
	std::uniform_real_distribution<float> dis(0.0f, 1.0f);
	return dis(mRng);
}

glm::vec2 PathTracer::GetUV(glm::vec3& p, Triangle& t)
{
	glm::vec3 v2 = p - t.v1;
	float d20 = glm::dot(v2, t.barycentricInfo.v0);
	float d21 = glm::dot(v2, t.barycentricInfo.v1);
	
	float alpha = (t.barycentricInfo.d11 * d20 - t.barycentricInfo.d01 * d21) *
		t.barycentricInfo.invDenom;
	float beta = (t.barycentricInfo.d00 * d21 - t.barycentricInfo.d01 * d20) *
		t.barycentricInfo.invDenom;

	return (1.0f - alpha - beta) * t.uv1 + alpha * t.uv2 + beta * t.uv3;
}

glm::vec3 PathTracer::GetSmoothNormal(glm::vec3& p, Triangle& t)
{
	glm::vec3 v2 = p - t.v1;
	float d20 = glm::dot(v2, t.barycentricInfo.v0);
	float d21 = glm::dot(v2, t.barycentricInfo.v1);

	float alpha = (t.barycentricInfo.d11 * d20 - t.barycentricInfo.d01 * d21) *
		t.barycentricInfo.invDenom;
	float beta = (t.barycentricInfo.d00 * d21 - t.barycentricInfo.d01 * d20) *
		t.barycentricInfo.invDenom;

	glm::vec3 n = (1.0f - alpha - beta) * t.n1 + alpha * t.n2 + beta * t.n3;
	glm::vec3 res = glm::normalize(glm::vec3(n.x, -n.y, n.z));
	return glm::normalize(n);
}

Wave PathTracer::Trace(glm::vec3 ro, glm::vec3 rd, int depth, bool inside)
{
	Wave waveZero(mWaveLengths.size());

	float d = 0.0f;
	Triangle t;
	if (mBvh->Hit(ro, rd, t, d))
	{
		Material& mat = mLoadedObjects[t.objectId].elements[t.elementId].material;
		glm::vec3 p = ro + rd * d;
		glm::vec2 uv = GetUV(p, t);
		glm::vec3 n = t.normal;
		if (t.smoothing)
			n = GetSmoothNormal(p, t);
		if (glm::dot(n, rd) > 0.0f)
			n = -n;
		if (mat.normalTexId != -1)
		{
			glm::mat3 TBN = glm::mat3(t.tangent, t.bitangent, n);
			glm::vec3 nt = glm::vec3(mLoadedTextures[mat.normalTexId]->tex2D(uv)) * 2.0f - 1.0f;
			if (nt.z < 0.0f)
				nt = glm::vec3(nt.x, nt.y, 0.0f);
			nt = glm::normalize(nt);
			n = glm::normalize(TBN * nt);
		}
		p += n * EPS;

		if (depth < mMaxDepth * 2)
		{
			depth++;
			// Russian Roulette Path Termination
			float prob = glm::min(0.95f, glm::max(glm::max(mat.baseColor.x, mat.baseColor.y), mat.baseColor.z));
			if (depth >= mMaxDepth)
			{
				if (glm::abs(Rand()) > prob)
					return mat.emissivity;
			}

			glm::vec3 r = glm::reflect(rd, n);
			glm::vec3 reflectDir;
			if (mat.type == MaterialType::SPECULAR)
				reflectDir = r;
			else if (mat.type == MaterialType::DIFFUSE)
			{
				// Monte Carlo Integration
				glm::vec3 u = glm::abs(n.x) < 1.0f - EPS ? glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), n) : glm::cross(glm::vec3(1.0f), n);
				u = glm::normalize(u);
				glm::vec3 v = glm::normalize(glm::cross(u, n));
				float w = Rand(), theta = Rand();
				// uniformly sampling on hemisphere
				reflectDir = w * cosf(2.0f * M_PI * theta) * u + w * sinf(2.0f * M_PI * theta) * v + glm::sqrt(1.0f - w * w) * n;
				reflectDir = glm::normalize(reflectDir);
			}
			else if (mat.type == MaterialType::GLOSSY)
			{
				// Monte Carlo Integration
				glm::vec3 u = glm::abs(n.x) < 1 - FLT_EPSILON ? glm::cross(glm::vec3(1, 0, 0), r) : glm::cross(glm::vec3(1), r);
				u = glm::normalize(u);
				glm::vec3 v = glm::cross(u, r);
				float w = Rand() * mat.roughness, theta = Rand();
				// wighted sampling on hemisphere
				reflectDir = w * cosf(2 * M_PI * theta) * u + w * sinf(2 * M_PI * theta) * v + glm::sqrt(1 - w * w) * r;
			}
			else if (mat.type == MaterialType::GLASS)
			{
				float nc = 1.0f, ng = 1.5f;
				// Snells law
				float eta = inside ? ng / nc : nc / ng;
				float r0 = glm::pow((nc - ng) / (nc + ng), 2.0f);
				float c = glm::abs(glm::dot(rd, n));
				float k = 1.0f - eta * eta * (1.0f - c * c);
				if (k < 0.0f)
					reflectDir = r;
				else
				{
					// Shilick's approximation of Fresnel's equation
					float re = r0 + (1.0f - r0) * glm::pow(1.0f - c, 2.0f);
					if (glm::abs(Rand()) < re)
						reflectDir = r;
					else
					{
						reflectDir = glm::normalize(eta * rd - (eta * glm::dot(n, rd) + glm::sqrt(k)) * n);
						p -= n * EPS * 2.0f;
						inside = !inside;
					}
				}
			}

			Wave emissivity = mat.emissivity;
			Wave reflectivity = mat.reflectivity;
			if (mat.temperatureTexId != -1)
			{
				// TODO map temperature value here
				float temperature = mLoadedTextures[mat.normalTexId]->tex2D(uv).r;
				emissivity = GetEmissivity(mat.spectrumMatId, temperature);
			}
			if (emissivity.size() < mWaveLengths.size())
				emissivity = waveZero;
			if (reflectivity.size() < mWaveLengths.size())
				reflectivity = waveZero;

			return mat.emissivity + Trace(p, reflectDir, depth, inside) * mat.reflectivity;
		}
	}

	if (mWaveSky.size() < mWaveLengths.size())
		return waveZero;
	return mWaveSky;
}

void PathTracer::RenderFrame()
{
	mExit = false;

	if (mNeedReset)
	{
		for (int i = 0; i < mResolution.x * mResolution.y; i++)
			mTotalSpectrumResult[i].Initialize(mWaveLengths.size());

		for (int i = 0; i < mResolution.x * mResolution.y * 3; i++)
			mTotalImg[i] = 0.0f;
		mNeedReset = false;
        mSamples = 0;
	}

	mSamples++;

	// Position world space image plane
	glm::vec3 imgCenter = mCamPos + mCamDir * mCamFocal;
	float imgHeight = 2.0f * mCamFocal * tan((mCamFovy / 2.0f) * M_PI / 180.0f);
	float aspect = (float)mResolution.x / (float)mResolution.y;
	float imgWidth = imgHeight * aspect;
	float deltaX = imgWidth / (float)mResolution.x;
	float deltaY = imgHeight / (float)mResolution.y;
	glm::vec3 camRight = glm::normalize(glm::cross(mCamUp, mCamDir));

	// Starting at top left
	glm::vec3 topLeft = imgCenter - camRight * (imgWidth * 0.5f);
	topLeft += mCamUp * (imgHeight * 0.5f);

	int numThreads = omp_get_max_threads();
	if (numThreads > 2)
		numThreads -= 3;
	else if (numThreads > 1)
		numThreads -= 2;
	else if (numThreads > 0)
		numThreads--;
	// Loop through each pixel
	#pragma omp parallel for num_threads(numThreads)
	for (int i = 0; i < mResolution.y; i++)
	{
		if (mExit)
			break;

		glm::vec3 pixel = topLeft - mCamUp * ((float)i * deltaY);
		for (int j = 0; j < mResolution.x; j++)
		{
			glm::vec3 rayDir = glm::normalize(pixel - mCamPos);
			float seed = 0.0f;

			Wave wave = Trace(mCamPos, rayDir);

			int imgPixel = ((mResolution.y - 1 - i) * mResolution.x + j);

			mTotalSpectrumResult[imgPixel] += wave;
			mOutSpectrumResult[imgPixel] = mTotalSpectrumResult[imgPixel] / (float)mSamples;

			/*if (mOutputChannel >= 0 && mOutputChannel < mNumWaves)
			{
				mOutImg[imgPixel * 3] = mOutSpectrumResult[imgPixel][mOutputChannel] * 255;
				mOutImg[imgPixel * 3 + 1] = mOutSpectrumResult[imgPixel][mOutputChannel] * 255;
				mOutImg[imgPixel * 3 + 2] = mOutSpectrumResult[imgPixel][mOutputChannel] * 255;
			}*/

			//glm::vec3 color = Trace(mCamPos, rayDir);

			//// Draw
			//int imgPixel = ((mResolution.y - 1 - i) * mResolution.x + j) * 3;

			//mTotalImg[imgPixel] += color.r;
			//mTotalImg[imgPixel + 1] += color.g;
			//mTotalImg[imgPixel + 2] += color.b;

			//glm::vec3 res = glm::vec3
			//(
			//	mTotalImg[imgPixel] / (float)mSamples,
			//	mTotalImg[imgPixel + 1] / (float)mSamples,
			//	mTotalImg[imgPixel + 2] / (float)mSamples
			//);

			//res = glm::clamp(res, glm::vec3(0.0f), glm::vec3(1.0f));

			//mOutImg[imgPixel] = res.r * 255;
			//mOutImg[imgPixel + 1] = res.g * 255;
			//mOutImg[imgPixel + 2] = res.b * 255;

			pixel += camRight * deltaX;
		}
	}
}

void PathTracer::Exit()
{
	mExit = true;
}
