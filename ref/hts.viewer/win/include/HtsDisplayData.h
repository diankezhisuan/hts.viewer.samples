#pragma once

#include "HtsViewerTypes.h"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace hts::viewer
{
enum class HtsDisplayShapeType
{
    Unknown,
    Solid,
    Face,
    Edge,
    Vertex
};

enum class HtsDisplayBodyKind
{
    Unknown,
    Solid,
    Shell,
    Sheet,
    AirBoxCandidate
};

struct HtsImportedMaterialData
{
    std::string materialId;
    std::string name;
    HtsColor4f baseColor{0.70f, 0.77f, 0.88f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.55f;
    float specular = 0.5f;
    float opacity = 1.0f;
    bool fromFile = false;
};

struct HtsDisplayFaceData
{
    std::string objectId;
    std::string bodyId;
    int solidTag = -1;
    HtsDisplayShapeType shapeType = HtsDisplayShapeType::Face;
    int faceTag = -1;
    std::vector<HtsVec3f> positions;
    std::vector<HtsVec3f> normals;
    std::vector<HtsVec3f> barycentric;
    std::vector<unsigned int> indices;
    std::vector<int> edgeTags;
    HtsColor4f color{0.70f, 0.77f, 0.88f, 1.0f};
    std::string materialId;
    std::array<HtsVec3f, 4> corners{};
    bool visible = true;
};

struct HtsDisplayEdgeData
{
    std::string objectId;
    std::string bodyId;
    int edgeTag = -1;
    std::vector<int> ownerFaceTags;
    std::vector<HtsVec3f> polyline;
    HtsColor4f color{0.10f, 0.16f, 0.22f, 1.0f};
    bool visible = true;
};

struct HtsDisplayBodyData
{
    std::string objectId;
    std::string bodyId;
    int solidTag = -1;
    HtsDisplayBodyKind kind = HtsDisplayBodyKind::Unknown;
    HtsBounds3d bounds;
    std::vector<int> faceTags;
    std::vector<int> edgeTags;
    std::vector<int> vertexTags;
    bool visible = true;
};

struct HtsDisplayVertexData
{
    std::string objectId;
    std::string bodyId;
    int solidTag = -1;
    int vertexTag = -1;
    HtsVec3f position;
    HtsColor4f color{0.12f, 0.22f, 0.32f, 1.0f};
    bool visible = true;
};

struct HtsDisplayObjectData
{
    std::string objectId;
    std::string bodyId;
    HtsDisplayShapeType shapeType = HtsDisplayShapeType::Unknown;
    std::vector<HtsDisplayFaceData> faces;
    std::vector<HtsDisplayEdgeData> edges;
    std::vector<HtsDisplayVertexData> vertices;
    std::vector<HtsDisplayBodyData> bodies;
    std::map<std::string, HtsImportedMaterialData> importedMaterials;
    bool visible = true;
};

struct HtsDisplayMeshData
{
    std::string sceneType = "DisplayData";
    int shapeVersion = 1;
    std::vector<HtsDisplayObjectData> objects;
};

/** Precomputed face annotation geometry; line vertices are stored in pairs. */
struct HtsFaceOverlayData
{
    std::string objectId;
    int faceTag = -1;
    std::vector<HtsVec3d> lineVertices;
    std::vector<HtsVec3d> fillTriangleVertices;

    bool empty() const
    {
        return lineVertices.empty() && fillTriangleVertices.empty();
    }
};

enum class HtsGlyphSizeMode
{
    Automatic,
    FixedWorld
};

enum class HtsWavePolarization
{
    LeftElliptical,
    Linear,
    RightElliptical
};

/** Parameters for a Viewer-rendered incident plane-wave annotation. */
struct HtsPlaneWaveGlyph
{
    std::string id;
    HtsVec3d anchor{};
    HtsVec3d propagationDirection{0.0, 0.0, -1.0};
    HtsVec3d electricDirection{1.0, 0.0, 0.0};
    HtsGlyphSizeMode sizeMode = HtsGlyphSizeMode::Automatic;
    bool loopDirections = false;
    double thetaStartDegrees = 0.0;
    double phiStartDegrees = 0.0;
    double thetaEndDegrees = 0.0;
    double phiEndDegrees = 0.0;
    double thetaStepDegrees = 1.0;
    double phiStepDegrees = 0.0;
    double polarizationAngleDegrees = 0.0;
    double modelDiagonal = 100.0;
    double fixedArrowLength = 100.0;
    double amplitude = 1.0;
    double phaseDegrees = 0.0;
    HtsWavePolarization polarization = HtsWavePolarization::Linear;
    bool calculateOrthogonalPolarizations = false;
    double ellipticity = 0.0;
    bool showPropagation = true;
    bool showElectric = true;
    bool showMagnetic = true;
    bool showWaveFront = true;
    bool showLabel = true;
    bool alwaysOnTop = false;
    HtsColor4f propagationColor{1.0f, 0.75f, 0.10f, 1.0f};
    HtsColor4f electricColor{1.0f, 0.10f, 0.10f, 1.0f};
    HtsColor4f magneticColor{0.10f, 0.35f, 1.0f, 1.0f};
    HtsColor4f waveFrontColor{0.30f, 0.65f, 1.0f, 0.18f};
};
}
