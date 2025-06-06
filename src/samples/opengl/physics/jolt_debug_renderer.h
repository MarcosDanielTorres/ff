#pragma once

class JoltDebugRenderer : public JPH::DebugRenderer {
private:
	class BatchImpl : public JPH::RefTargetVirtual {
	public:
		std::vector<Vertex> vtx;
		std::vector<JPH::uint32> idx;

		BatchImpl(const Vertex* inVert, int inVertCount, const JPH::uint32* inIdx, int inIdxCount) {
			vtx.resize(inVertCount);
			for (int i = 0; i < inVertCount; ++i) {
				vtx[i] = inVert[i];
			}

			if (inIdx) {
				idx.resize(inIdxCount);
				for (int i = 0; i < inIdxCount; ++i) {
					idx[i] = inIdx[i];
				}
			}
			else {
				idx.resize(inIdxCount);
				for (int i = 0; i < inIdxCount; ++i)
					idx[i] = i;
			}
		}

		int mRefCount = 0;
		JPH::Array<JPH::DebugRenderer::Triangle> mTriangles;

		void AddRef() override {
			mRefCount++;
		}

		void Release() override {
			if (--mRefCount == 0) {
				delete this;
			}
		}
	};

	struct RayCast {
		glm::vec3 ro;
		glm::vec3 rd;
		float t;
	};

public:
	std::vector<RayCast> ray_cast_list;
	JPH::Color lines_color = JPH::Color::sGreen;

public:
	JPH::Vec3 mCameraPos{};

	JoltDebugRenderer() {
		Initialize();
	}

	void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
	void DrawTriangle(
		JPH::RVec3Arg inV1,
		JPH::RVec3Arg inV2,
		JPH::RVec3Arg inV3,
		JPH::ColorArg inColor,
		ECastShadow inCastShadow = ECastShadow::Off) override;
	Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;
	Batch CreateTriangleBatch(
		const Vertex* inVertices,
		int inVertexCount,
		const std::uint32_t* inIndices,
		int inIndexCount) override;
	void  DrawGeometry(
		JPH::RMat44Arg inModelMatrix,
		const JPH::AABox& inWorldSpaceBounds,
		float inLODScaleSq,
		JPH::ColorArg inModelColor,
		const GeometryRef& inGeometry,
		ECullMode inCullMode,
		ECastShadow inCastShadow,
		EDrawMode inDrawMode) override;
	void  DrawText3D(
		JPH::RVec3Arg inPosition,
		const std::string_view& inString,
		JPH::ColorArg inColor,
		float inHeight) override;
};
