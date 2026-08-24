#pragma once

#include <HtsViewerSdk.h>

#include <array>
#include <string>
#include <utility>

namespace hts::viewer::samples
{
inline HtsDisplayFaceData makeFace(const std::string& objectId,
                                   const std::string& bodyId,
                                   int faceTag,
                                   const std::array<HtsVec3f, 4>& corners,
                                   const HtsVec3f& normal,
                                   const HtsColor4f& color)
{
    HtsDisplayFaceData face;
    face.objectId = objectId;
    face.bodyId = bodyId;
    face.solidTag = 1;
    face.faceTag = faceTag;
    face.positions.assign(corners.begin(), corners.end());
    face.normals.assign(4, normal);
    face.indices = {0, 1, 2, 0, 2, 3};
    face.corners = corners;
    face.color = color;
    return face;
}

inline HtsDisplayObjectData makeBox(const std::string& objectId,
                                    const HtsVec3f& center,
                                    float size,
                                    const HtsColor4f& faceColor)
{
    const float h = size * 0.5f;
    const std::array<HtsVec3f, 8> p = {
        center + HtsVec3f{-h, -h, -h}, center + HtsVec3f{h, -h, -h},
        center + HtsVec3f{h, h, -h}, center + HtsVec3f{-h, h, -h},
        center + HtsVec3f{-h, -h, h}, center + HtsVec3f{h, -h, h},
        center + HtsVec3f{h, h, h}, center + HtsVec3f{-h, h, h}
    };

    HtsDisplayObjectData object;
    object.objectId = objectId;
    object.bodyId = objectId + "_Body";
    object.faces = {
        makeFace(objectId, object.bodyId, 1, {p[0], p[3], p[2], p[1]}, {0, 0, -1}, faceColor),
        makeFace(objectId, object.bodyId, 2, {p[4], p[5], p[6], p[7]}, {0, 0, 1}, faceColor),
        makeFace(objectId, object.bodyId, 3, {p[0], p[1], p[5], p[4]}, {0, -1, 0}, faceColor),
        makeFace(objectId, object.bodyId, 4, {p[3], p[7], p[6], p[2]}, {0, 1, 0}, faceColor),
        makeFace(objectId, object.bodyId, 5, {p[0], p[4], p[7], p[3]}, {-1, 0, 0}, faceColor),
        makeFace(objectId, object.bodyId, 6, {p[1], p[2], p[6], p[5]}, {1, 0, 0}, faceColor)
    };

    static constexpr int edgeIndices[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    for (int i = 0; i < 12; ++i) {
        HtsDisplayEdgeData edge;
        edge.objectId = objectId;
        edge.bodyId = object.bodyId;
        edge.edgeTag = i + 1;
        edge.polyline = {p[edgeIndices[i][0]], p[edgeIndices[i][1]]};
        object.edges.push_back(std::move(edge));
    }

    for (int i = 0; i < 8; ++i) {
        HtsDisplayVertexData vertex;
        vertex.objectId = objectId;
        vertex.bodyId = object.bodyId;
        vertex.solidTag = 1;
        vertex.vertexTag = i + 1;
        vertex.position = p[i];
        object.vertices.push_back(vertex);
    }

    HtsDisplayBodyData body;
    body.objectId = objectId;
    body.bodyId = object.bodyId;
    body.solidTag = 1;
    body.kind = HtsDisplayBodyKind::Solid;
    for (const auto& point : p) body.bounds.expandBy(point);
    body.faceTags = {1, 2, 3, 4, 5, 6};
    body.edgeTags = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    body.vertexTags = {1, 2, 3, 4, 5, 6, 7, 8};
    object.bodies.push_back(std::move(body));
    return object;
}

inline bool initializeViewer(HtsViewerSdk& viewer,
                             HtsDisplayMeshData scene,
                             const char* sampleName)
{
    if (!viewer.initializeStandalone(100, 100, 1024, 768)) return false;
    scene.sceneType = sampleName;
    if (!viewer.upsertDisplayData(scene)) return false;
    viewer.fitView();
    return true;
}
}
