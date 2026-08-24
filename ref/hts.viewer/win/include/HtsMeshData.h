#pragma once

#include "HtsViewerTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hts::viewer
{
enum class HtsMeshElementType
{
    Surface,
    Volume
};

enum class HtsMeshDisplayMode
{
    ShadedWithMeshLines = 0,
    Shaded = 1,
    Wireframe = 2,
    TransparentWithMeshLines = 3
};

struct HtsMeshDisplayOptions
{
    HtsMeshDisplayMode mode = HtsMeshDisplayMode::TransparentWithMeshLines;
    HtsColor4f faceColor{0.66f, 0.74f, 0.84f, 1.0f};
    HtsColor4f edgeColor{0.08f, 0.10f, 0.13f, 0.45f};
    HtsColor4f volumeEdgeColor{0.10f, 0.14f, 0.20f, 0.18f};
    HtsColor4f selectedFaceColor{1.0f, 0.62f, 0.05f, 1.0f};
    HtsColor4f selectedEdgeColor{1.0f, 0.88f, 0.08f, 1.0f};
    float opacity = 1.0f;
    float transparentOpacity = 0.28f;
    float edgeWidthPixels = 0.40f;
    float volumeEdgeWidthPixels = 1.0f;
    float selectedEdgeWidthScale = 1.30f;
    float edgeSoftnessPixels = 0.10f;
    float ambientStrength = 0.90f;
    float diffuseStrength = 0.10f;
    bool visible = true;
    bool showVolumeEdges = true;
    bool volumeEdgesXRay = true;
    bool useLighting = false;
    bool twoSided = true;
    bool shaderEnabled = true;
};

/**
 * A stable, independently editable and styleable part of one mesh result.
 *
 * Geometry rules:
 * - partId must be non-empty and unique within its HtsMeshDisplayData.
 * - triangleIndices, when present, contains triples and every index must be
 *   smaller than vertices.size(). When it is empty, vertices is interpreted
 *   as triangle soup and vertices.size() must be a multiple of three.
 * - vertexIds is optional; when present its size must equal vertices.size().
 * - triangleIds is optional; when present its size must equal the triangle
 *   count after applying the indexed/triangle-soup rule above.
 * - edgeIndices, when present, contains pairs and every index must be smaller
 *   than edgeVertices.size(). When it is empty, edgeVertices is interpreted
 *   as line-segment soup and edgeVertices.size() must be a multiple of two.
 * - edgeIds is optional; when present its size must equal the edge count.
 * - At least one triangle or edge must be present.
 */
struct HtsMeshPartData
{
    std::string partId;
    std::string objectId;
    std::string bodyId;
    int faceTag = -1;
    std::vector<HtsVec3f> vertices;
    std::vector<std::uint32_t> triangleIndices;

    /** Optional stable IDs reserved for selection and future mesh editing. */
    std::vector<std::uint64_t> vertexIds;
    std::vector<std::uint64_t> triangleIds;

    std::vector<HtsVec3f> edgeVertices;
    std::vector<std::uint32_t> edgeIndices;
    std::vector<std::uint64_t> edgeIds;
    bool visible = true;
};

struct HtsMeshDisplayData
{
    std::string meshId;
    /** The business type of the complete mesh result, not its render primitive. */
    HtsMeshElementType elementType = HtsMeshElementType::Surface;
    std::vector<HtsMeshPartData> parts;
    bool visible = true;
};

struct HtsMeshPartStyle
{
    bool hasFaceColor = false;
    HtsColor4f faceColor{};
    bool hasEdgeColor = false;
    HtsColor4f edgeColor{};
};

struct HtsMeshStatistics
{
    std::size_t partCount = 0;
    std::size_t vertexCount = 0;
    std::size_t triangleCount = 0;
    std::size_t volumeEdgeCount = 0;
    std::size_t estimatedMemoryBytes = 0;
};
}
