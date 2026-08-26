#include "import/HtsOccShapeDisplayDataBuilder.h"
#include "import/HtsImportMemoryGuard.h"

#ifdef HTS_ENABLE_OCCT_IMPORT

#include <BRepAdaptor_Curve.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <GeomAbs_CurveType.hxx>
#include <Poly_Polygon3D.hxx>

#include <Poly_Triangle.hxx>

#include <Poly_Triangulation.hxx>

#include <Standard_Failure.hxx>
#include <TColgp_Array1OfPnt.hxx>

#include <TopAbs_Orientation.hxx>

#include <TopAbs_ShapeEnum.hxx>

#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopTools_DataMapOfShapeInteger.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#endif

#include <cstddef>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

namespace hts::viewer::importing
{
#ifdef HTS_ENABLE_OCCT_IMPORT
namespace
{
class ImportStageTimer
{
public:
    explicit ImportStageTimer(const std::string& name)
            : m_Name(name),
              m_Start(std::chrono::steady_clock::now())
    {
        std::cout << "[ImportStage] begin name=" << m_Name << std::endl;
    }
    ~ImportStageTimer()
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_Start).count();
        std::cout << "[ImportStage] end name=" << m_Name
                  << " elapsedMs=" << elapsed << std::endl;
    }

private:
    std::string m_Name;
    std::chrono::steady_clock::time_point m_Start;
};


hts::viewer::HtsVec3f normalizedCross(const hts::viewer::HtsVec3f& lhs,
                                      const hts::viewer::HtsVec3f& rhs)
{
    hts::viewer::HtsVec3f normal{
            lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x};
    const float lengthSquared = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
    if (lengthSquared <= 1.0e-12f) {
        return {0.0f, 0.0f, 1.0f};
    }
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    normal = normal * inverseLength;
    return normal;
}

hts::viewer::HtsVec3f computedTriangleNormal(const hts::viewer::HtsVec3f& a,
                                             const hts::viewer::HtsVec3f& b,
                                             const hts::viewer::HtsVec3f& c,
                                             bool reversed)
{
    return reversed ? normalizedCross(c - a, b - a) : normalizedCross(b - a, c - a);
}

hts::viewer::HtsVec3f transformedOccNormal(const Poly_Triangulation& triangulation,
                                           Standard_Integer nodeIndex,
                                           const TopLoc_Location& location,
                                           bool reversed)
{
    gp_Dir normal = triangulation.Normal(nodeIndex).Transformed(location.Transformation());
    hts::viewer::HtsVec3f result(static_cast<float>(normal.X()),
                                 static_cast<float>(normal.Y()),
                                 static_cast<float>(normal.Z()));
    if (reversed) {
        result = result * -1.0f;
    }
    const float lengthSquared = result.x * result.x + result.y * result.y + result.z * result.z;
    if (lengthSquared <= 1.0e-12f) {
        return {0.0f, 0.0f, 1.0f};
    }
    return result * (1.0f / std::sqrt(lengthSquared));

}

void updateCorners(hts::viewer::HtsDisplayFaceData& face)
{
    if (face.positions.empty()) {
        return;
    }
    face.corners[0] = face.positions[0];
    face.corners[1] = face.positions.size() > 1 ? face.positions[1] : face.positions[0];
    face.corners[2] = face.positions.size() > 2 ? face.positions[2] : face.corners[1];
    face.corners[3] = face.positions.size() > 3 ? face.positions[3] : face.corners[0];
}

hts::viewer::HtsColor4f engineeringPaletteColor(int seed, bool sheetFace)
{
    static const hts::viewer::HtsColor4f solidPalette[] = {
            {0.66f, 0.72f, 0.80f, 1.0f}, {0.72f, 0.70f, 0.65f, 1.0f},
            {0.64f, 0.73f, 0.68f, 1.0f}, {0.72f, 0.68f, 0.76f, 1.0f},
            {0.68f, 0.75f, 0.78f, 1.0f}
    };

    static const hts::viewer::HtsColor4f sheetPalette[] = {
            {0.76f, 0.80f, 0.86f, 1.0f}, {0.80f, 0.82f, 0.78f, 1.0f},
            {0.74f, 0.82f, 0.80f, 1.0f}
    };

    if (sheetFace) {
        const int index = std::abs(seed) % (sizeof(sheetPalette) / sizeof(sheetPalette[0]));
        return sheetPalette[index];
    }
    const int index = std::abs(seed) % (sizeof(solidPalette) / sizeof(solidPalette[0]));
    return solidPalette[index];
}

bool appendPolygon3D(
        const TopoDS_Edge& edge,
        hts::viewer::HtsDisplayEdgeData& displayEdge)
{
    try {
        TopLoc_Location location;
        Handle(Poly_Polygon3D) polygon =
                BRep_Tool::Polygon3D(edge, location);

        if (polygon.IsNull() || polygon->NbNodes() < 2) {
            return false;
        }

        displayEdge.polyline.reserve(
                static_cast<std::size_t>(polygon->NbNodes()));

        const TColgp_Array1OfPnt& nodes = polygon->Nodes();

        for (Standard_Integer i = nodes.Lower();
             i <= nodes.Upper();
             ++i) {
            const gp_Pnt point =
                    nodes.Value(i).Transformed(
                            location.Transformation());

            displayEdge.polyline.push_back({
                                                   static_cast<float>(point.X()),
                                                   static_cast<float>(point.Y()),
                                                   static_cast<float>(point.Z())
                                           });
        }

        return displayEdge.polyline.size() >= 2;
    }
    catch (const Standard_Failure&) {
        displayEdge.polyline.clear();
        return false;
    }
    catch (...) {
        displayEdge.polyline.clear();
        return false;
    }
}

bool appendSampledCurve(
        const TopoDS_Edge& edge,
        hts::viewer::HtsDisplayEdgeData& displayEdge,
        int maxSamplePoints)
{
    try {
        BRepAdaptor_Curve curve(edge);

        const double first = curve.FirstParameter();
        const double last = curve.LastParameter();

        if (!std::isfinite(first)
            || !std::isfinite(last)
            || first == last) {
            return false;
        }

        const int sampleCount =
                curve.GetType() == GeomAbs_Line
                ? 1
                : std::max(2, maxSamplePoints);

        displayEdge.polyline.reserve(
                static_cast<std::size_t>(sampleCount + 1));

        for (int i = 0; i <= sampleCount; ++i) {
            const double t =
                    first
                    + (last - first)
                      * (static_cast<double>(i)
                         / static_cast<double>(sampleCount));

            gp_Pnt point;
            curve.D0(t, point);

            if (!std::isfinite(point.X())
                || !std::isfinite(point.Y())
                || !std::isfinite(point.Z())) {
                displayEdge.polyline.clear();
                return false;
            }

            displayEdge.polyline.push_back({
                                                   static_cast<float>(point.X()),
                                                   static_cast<float>(point.Y()),
                                                   static_cast<float>(point.Z())
                                           });
        }

        return displayEdge.polyline.size() >= 2;
    }
    catch (const Standard_Failure&) {
        displayEdge.polyline.clear();
        return false;
    }
    catch (...) {
        displayEdge.polyline.clear();
        return false;
    }
}

struct BodyBuildInfo
{
    TopoDS_Shape shape;
    std::string bodyId;
    int solidTag = -1;
    hts::viewer::HtsDisplayBodyKind kind = hts::viewer::HtsDisplayBodyKind::Unknown;
};

int findBodyIndexForOwnerList(const TopTools_DataMapOfShapeInteger& bodyIndexMap,
                              const TopTools_ListOfShape& owners)
{
    for (TopTools_ListIteratorOfListOfShape ownerIt(owners); ownerIt.More(); ownerIt.Next()) {
        const TopoDS_Shape& owner = ownerIt.Value();
        if (bodyIndexMap.IsBound(owner)) {
            return bodyIndexMap.Find(owner);
        }
    }
    return -1;
}

int findBodyIndex(const TopTools_DataMapOfShapeInteger& bodyIndexMap,
                  const TopoDS_Shape& subShape,
                  const TopTools_IndexedDataMapOfShapeListOfShape& solidOwnerMap)
{
    if (solidOwnerMap.Contains(subShape)) {
        const int bodyIndex = findBodyIndexForOwnerList(
                bodyIndexMap, solidOwnerMap.FindFromKey(subShape));
        if (bodyIndex >= 0) {
            return bodyIndex;
        }
    }
    if (bodyIndexMap.IsBound(subShape)) {
        return bodyIndexMap.Find(subShape);
    }
    return -1;
}

std::vector<BodyBuildInfo> buildBodyInfos(const TopoDS_Shape& shape)
{
    std::vector<BodyBuildInfo> bodies;
    int solidTag = 0;
    for (TopExp_Explorer explorer(shape, TopAbs_SOLID); explorer.More(); explorer.Next()) {
        ++solidTag;
        std::ostringstream id;
        id << "Solid_" << std::setw(4) << std::setfill('0') << solidTag;
        BodyBuildInfo info;
        info.shape = explorer.Current();
        info.bodyId = id.str();
        info.solidTag = solidTag;
        info.kind = hts::viewer::HtsDisplayBodyKind::Solid;
        bodies.push_back(info);
    }

    // Body 只表示真实的 Solid 拓扑。Shell、Sheet 和独立 Face 不人为
    // 包装成 Body；它们在对象选择模式下应保持各自的最高真实拓扑层级：
    // Face 命中返回单个 Face，只有独立 Edge 时返回单个 Edge。
    return bodies;
}
}
#endif

bool HtsOccShapeDisplayDataBuilder::isAvailable() const
{
#ifdef HTS_ENABLE_OCCT_IMPORT
    return true;

#else
    return false;

#endif

}

#ifdef HTS_ENABLE_OCCT_IMPORT
bool HtsOccShapeDisplayDataBuilder::build(const TopoDS_Shape& shape,
                                          const HtsImportOptions& options,
                                          const std::string& displayName,
                                          HtsImportedModel& outModel,
                                          std::string& errorMessage,
                                          const HtsOccFaceMaterialOverrides* materialOverrides) const
{
    outModel = HtsImportedModel();
    if (shape.IsNull()) {
        errorMessage = "OCCT shape is null.";
        return false;
    }
    auto rejectIfLiveMemoryUnsafe = [&options, &errorMessage](const std::string& stageName) -> bool {
        const HtsImportMemoryBudgetDecision memoryDecision =
                evaluateImportMemoryBudget(options, 0.0, 0.0, stageName);
        logImportMemoryLive(stageName, options, memoryDecision);
        if (memoryDecision.allowed) {
            return false;
        }
        std::cout << "[ImportRejected]"
                  << " reason=live_memory_budget"
                  << " stage=" << stageName
                  << " availableMB=" << memoryDecision.snapshot.availablePhysicalMB
                  << " privateBytesMB=" << memoryDecision.snapshot.privateBytesMB
                  << " safeLimitMB=" << memoryDecision.safeLimitMB
                  << std::endl;
        errorMessage = "Import rejected by live memory budget at stage=" + stageName
                       + " reason=" + memoryDecision.reason;
        return true;
    };

    if (rejectIfLiveMemoryUnsafe("before BRepMesh triangulation")) {
        return false;
    }
    {
        ImportStageTimer stage("BRepMesh triangulation");
        try {
            ImportStageTimer stage("BRepMesh triangulation");

            try {
                BRepMesh_IncrementalMesh mesher(
                        shape,
                        options.linearDeflection,
                        options.relativeDeflection,
                        options.angularDeflection,
                        options.meshInParallel);

                /*
                 * 带 Shape 参数的 BRepMesh_IncrementalMesh 构造函数
                 * 已经自动执行 Perform()，这里不能再次调用 mesher.Perform()。
                 */
                if (!mesher.IsDone()) {
                    errorMessage = "BRepMesh_IncrementalMesh did not complete.";
                    return false;
                }
            }
            catch (const Standard_Failure& failure) {
                const char* message = failure.GetMessageString();
                errorMessage =
                        std::string("BRepMesh_IncrementalMesh failed: ")
                        + (message ? message : "unknown OCCT failure");
                return false;
            }
            catch (...) {
                errorMessage =
                        "BRepMesh_IncrementalMesh failed with unknown exception.";
                return false;
            }

            std::cout << "[ImportStats] meshParallel="
                      << (options.meshInParallel ? "true" : "false")
                      << std::endl;
        }
        catch (const Standard_Failure& failure) {
            errorMessage = std::string("BRepMesh_IncrementalMesh failed: ") + failure.GetMessageString();
            return false;
        }
        catch (...) {
            errorMessage = "BRepMesh_IncrementalMesh failed with unknown exception.";
            return false;
        }
        std::cout << "[ImportStats] meshParallel=" << (options.meshInParallel ? "true" : "false") << std::endl;
    }
    if (rejectIfLiveMemoryUnsafe("after BRepMesh triangulation")) {
        return false;
    }
    hts::viewer::HtsDisplayObjectData object;
    object.objectId = options.defaultObjectId.empty() ? "ImportedOccShape" : options.defaultObjectId;
    // HtsDisplayObjectData 只是显示数据容器，不代表 OCCT 拓扑层级。
    object.bodyId.clear();
    object.shapeType = hts::viewer::HtsDisplayShapeType::Unknown;
    std::vector<BodyBuildInfo> bodyInfos;
    TopTools_DataMapOfShapeInteger bodyIndexMap;
    {
        ImportStageTimer stage("solid split / body index build");
        bodyInfos = buildBodyInfos(shape);
        for (std::size_t i = 0; i < bodyInfos.size(); ++i) {
            if (!bodyIndexMap.IsBound(bodyInfos[i].shape)) {
                bodyIndexMap.Bind(bodyInfos[i].shape, static_cast<int>(i));
            }
        }
        std::cout << "[ImportStats] bodyIndexCount=" << bodyIndexMap.Size() << std::endl;
    }
    TopTools_IndexedMapOfShape edgeTagMap;
    TopTools_IndexedMapOfShape faceTagMap;
    TopTools_IndexedMapOfShape vertexTagMap;
    TopTools_IndexedDataMapOfShapeListOfShape faceSolidOwnerMap;
    TopTools_IndexedDataMapOfShapeListOfShape edgeSolidOwnerMap;
    TopTools_IndexedDataMapOfShapeListOfShape vertexSolidOwnerMap;
    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceOwnerMap;
    {
        ImportStageTimer stage("topology owner map build");
        TopExp::MapShapes(shape, TopAbs_FACE, faceTagMap);
        TopExp::MapShapes(shape, TopAbs_EDGE, edgeTagMap);
        TopExp::MapShapes(shape, TopAbs_VERTEX, vertexTagMap);
        TopExp::MapShapesAndAncestors(shape, TopAbs_FACE, TopAbs_SOLID, faceSolidOwnerMap);
        if (options.buildCadEdges) {
            TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_SOLID, edgeSolidOwnerMap);
            TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceOwnerMap);
        }
        if (options.buildVertexDisplayData) {
            TopExp::MapShapesAndAncestors(shape, TopAbs_VERTEX, TopAbs_SOLID, vertexSolidOwnerMap);
        }
        std::cout << "[ImportStats] buildCadEdges=" << (options.buildCadEdges ? "true" : "false")
                  << " buildVertexDisplayData=" << (options.buildVertexDisplayData ? "true" : "false") << std::endl;
    }
    object.bodies.reserve(bodyInfos.size());
    object.faces.reserve(static_cast<std::size_t>(faceTagMap.Extent()));
    if (options.buildCadEdges) {
        object.edges.reserve(static_cast<std::size_t>(edgeTagMap.Extent()));
    }
    if (options.buildVertexDisplayData) {
        object.vertices.reserve(static_cast<std::size_t>(vertexTagMap.Extent()));
    }
    for (const BodyBuildInfo& bodyInfo : bodyInfos) {
        hts::viewer::HtsDisplayBodyData body;
        body.objectId = object.objectId;
        body.bodyId = bodyInfo.bodyId;
        body.solidTag = bodyInfo.solidTag;
        body.kind = bodyInfo.kind;
        object.bodies.push_back(body);
    }
    std::size_t faceCount = 0;
    std::size_t triangulatedFaceCount = 0;
    std::size_t skippedFaceCount = 0;
    std::size_t triangleCount = 0;
    std::size_t vertexCount = 0;
    std::size_t occtNormalTriangleCount = 0;
    std::size_t computedNormalTriangleCount = 0;
    std::size_t reversedFaceCount = 0;
    std::size_t edgeCount = 0;
    std::size_t polylineEdgeCount = 0;
    std::size_t sampledEdgeCount = 0;
    std::size_t skippedEdgeCount = 0;
    std::size_t vertexTopologyCount = 0;
    std::size_t importedFaceColorCount = 0;
    std::size_t autoMaterialFaceCount = 0;
    std::size_t faceEdgeRelationCount = 0;
    std::size_t edgeOwnerFaceRelationCount = 0;
    std::size_t solidOwnedEdgeCount = 0;
    std::size_t sheetOwnedEdgeCount = 0;
    std::size_t totalEdgePolylinePointCount = 0;
    std::size_t maxEdgePolylinePointCount = 0;
    int faceTag = 0;
    {
        ImportStageTimer stage("face mesh extraction");
        for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More(); explorer.Next()) {
            ++faceCount;
            ++faceTag;
            const TopoDS_Face face = TopoDS::Face(explorer.Current());
            const int bodyIndex = findBodyIndex(
                    bodyIndexMap, face, faceSolidOwnerMap);
            TopLoc_Location location;
            Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, location);
            if (triangulation.IsNull() || triangulation->NbTriangles() <= 0) {
                ++skippedFaceCount;
                continue;
            }
            hts::viewer::HtsDisplayFaceData displayFace;
            displayFace.objectId = object.objectId;
            displayFace.bodyId = bodyIndex >= 0 ? bodyInfos[bodyIndex].bodyId : std::string();
            displayFace.solidTag = bodyIndex >= 0 ? bodyInfos[bodyIndex].solidTag : -1;
            displayFace.faceTag = faceTag;
            displayFace.color = options.defaultColor;
            displayFace.edgeTags.reserve(8);
            bool hasImportedMaterial = false;
            if (materialOverrides != nullptr) {
                const auto materialIt = materialOverrides->faceColorsByTag.find(faceTag);
                if (materialIt != materialOverrides->faceColorsByTag.end()) {
                    displayFace.color = materialIt->second;
                    displayFace.materialId = "xcaf_face_" + std::to_string(faceTag);
                    hts::viewer::HtsImportedMaterialData materialData;
                    materialData.materialId = displayFace.materialId;
                    materialData.name = displayFace.materialId;
                    materialData.baseColor = displayFace.color;
                    materialData.opacity = displayFace.color.a;
                    materialData.fromFile = true;
                    object.importedMaterials[materialData.materialId] = materialData;
                    ++importedFaceColorCount;
                    hasImportedMaterial = true;
                }
            }
            if (!hasImportedMaterial) {
                if (options.enableDebugBodyColors) {
                    const bool sheetFace = bodyIndex < 0
                                           || bodyInfos[bodyIndex].kind == hts::viewer::HtsDisplayBodyKind::Sheet
                                           || bodyInfos[bodyIndex].kind == hts::viewer::HtsDisplayBodyKind::Shell;
                    const int materialSeed = sheetFace ? faceTag : bodyInfos[bodyIndex].solidTag;
                    displayFace.color = engineeringPaletteColor(materialSeed, sheetFace);
                    displayFace.materialId = sheetFace
                                             ? "debug_sheet_face_" + std::to_string(faceTag)
                                             : "debug_solid_" + std::to_string(bodyInfos[bodyIndex].solidTag);
                }
                else {
                    displayFace.color = options.defaultColor;
                    displayFace.materialId = "engineering_default";
                }
                if (object.importedMaterials.find(displayFace.materialId) == object.importedMaterials.end()) {
                    hts::viewer::HtsImportedMaterialData materialData;
                    materialData.materialId = displayFace.materialId;
                    materialData.name = displayFace.materialId;
                    materialData.baseColor = displayFace.color;
                    materialData.opacity = displayFace.color.a;
                    materialData.fromFile = false;
                    object.importedMaterials[materialData.materialId] = materialData;
                }
                ++autoMaterialFaceCount;
            }
            for (TopExp_Explorer edgeExplorer(face, TopAbs_EDGE); edgeExplorer.More(); edgeExplorer.Next()) {
                const int edgeTag = edgeTagMap.FindIndex(edgeExplorer.Current());
                if (edgeTag > 0
                    && std::find(displayFace.edgeTags.begin(), displayFace.edgeTags.end(), edgeTag)
                       == displayFace.edgeTags.end()) {
                    displayFace.edgeTags.push_back(edgeTag);
                    ++faceEdgeRelationCount;
                }
            }
            const Standard_Integer nodeCount = triangulation->NbNodes();
            const Standard_Integer sourceTriangleCount = triangulation->NbTriangles();

            displayFace.positions.reserve(static_cast<std::size_t>(nodeCount));
            displayFace.normals.reserve(static_cast<std::size_t>(nodeCount));
            displayFace.indices.reserve(static_cast<std::size_t>(sourceTriangleCount) * 3);

            /*
             * 普通 CAD Surface 不再常驻 barycentric。
             * HtsDisplayFaceData::barycentric 字段暂时保留，避免修改公开 DTO；
             * 此处不填充即可。
             */

            const bool reversed = face.Orientation() == TopAbs_REVERSED;
            if (reversed) {
                ++reversedFaceCount;
            }

            bool hasOcctNormals = triangulation->HasNormals();

            /*
             * 一个 OCCT triangulation node 只生成一个 Display vertex。
             * 顶点共享限定在当前 Face 内，不跨 Face 合并：
             *
             * - 保留 Face -> contiguous vertex range；
             * - RangeTable / Selection / Partial Material 仍可按 Face 工作；
             * - 不会把两个 CAD Face 的法向或显示属性焊接到一起。
             */
            for (Standard_Integer nodeIndex = 1;
                 nodeIndex <= nodeCount;
                 ++nodeIndex) {
                const gp_Pnt point =
                        triangulation->Node(nodeIndex)
                                .Transformed(location.Transformation());

                displayFace.positions.push_back({
                                                        static_cast<float>(point.X()),
                                                        static_cast<float>(point.Y()),
                                                        static_cast<float>(point.Z())
                                                });

                if (hasOcctNormals) {
                    try {
                        displayFace.normals.push_back(
                                transformedOccNormal(
                                        *triangulation,
                                        nodeIndex,
                                        location,
                                        reversed));
                    }
                    catch (const Standard_Failure&) {
                        /*
                         * 当前 Face 的 OCCT Normal 数据不可可靠读取时，
                         * 整个 Face 统一退回 Triangle Normal 累加路径。
                         */
                        hasOcctNormals = false;
                        displayFace.normals.assign(
                                displayFace.positions.size(),
                                hts::viewer::HtsVec3f(
                                        0.0f, 0.0f, 0.0f));
                    }
                }
                else {
                    displayFace.normals.push_back(
                            hts::viewer::HtsVec3f(
                                    0.0f, 0.0f, 0.0f));
                }
            }

            for (Standard_Integer triangleIndex = 1;
                 triangleIndex <= sourceTriangleCount;
                 ++triangleIndex) {
                const Poly_Triangle& triangle =
                        triangulation->Triangle(triangleIndex);

                Standard_Integer index1 = 0;
                Standard_Integer index2 = 0;
                Standard_Integer index3 = 0;
                triangle.Get(index1, index2, index3);

                if (index1 < 1 || index1 > nodeCount
                    || index2 < 1 || index2 > nodeCount
                    || index3 < 1 || index3 > nodeCount) {
                    continue;
                }

                const unsigned int vertexIndex1 =
                        static_cast<unsigned int>(index1 - 1);
                const unsigned int vertexIndex2 =
                        static_cast<unsigned int>(index2 - 1);
                const unsigned int vertexIndex3 =
                        static_cast<unsigned int>(index3 - 1);

                /*
                 * OCCT REVERSED Face需要反转三角形绕序。
                 * Node Array本身不复制，只调整Index Buffer。
                 */
                if (reversed) {
                    displayFace.indices.push_back(vertexIndex1);
                    displayFace.indices.push_back(vertexIndex3);
                    displayFace.indices.push_back(vertexIndex2);
                }
                else {
                    displayFace.indices.push_back(vertexIndex1);
                    displayFace.indices.push_back(vertexIndex2);
                    displayFace.indices.push_back(vertexIndex3);
                }

                if (hasOcctNormals) {
                    ++occtNormalTriangleCount;
                    continue;
                }

                const hts::viewer::HtsVec3f& v1 =
                        displayFace.positions[vertexIndex1];
                const hts::viewer::HtsVec3f& v2 =
                        displayFace.positions[vertexIndex2];
                const hts::viewer::HtsVec3f& v3 =
                        displayFace.positions[vertexIndex3];

                const hts::viewer::HtsVec3f triangleNormal =
                        computedTriangleNormal(v1, v2, v3, reversed);

                /*
                 * Face-local indexed mesh：
                 * 同一 Face 内共享节点的 Normal由相邻三角形累加后归一化。
                 *
                 * 不跨 Face共享，因此 CAD Face之间的法向断点仍然保留。
                 */
                hts::viewer::HtsVec3f& normal1 =
                        displayFace.normals[vertexIndex1];
                hts::viewer::HtsVec3f& normal2 =
                        displayFace.normals[vertexIndex2];
                hts::viewer::HtsVec3f& normal3 =
                        displayFace.normals[vertexIndex3];

                normal1.x += triangleNormal.x;
                normal1.y += triangleNormal.y;
                normal1.z += triangleNormal.z;

                normal2.x += triangleNormal.x;
                normal2.y += triangleNormal.y;
                normal2.z += triangleNormal.z;

                normal3.x += triangleNormal.x;
                normal3.y += triangleNormal.y;
                normal3.z += triangleNormal.z;

                ++computedNormalTriangleCount;
            }

            if (!hasOcctNormals) {
                for (hts::viewer::HtsVec3f& normal : displayFace.normals) {
                    const float lengthSquared =
                            normal.x * normal.x
                            + normal.y * normal.y
                            + normal.z * normal.z;

                    if (lengthSquared <= 1.0e-12f) {
                        normal = hts::viewer::HtsVec3f(
                                0.0f, 0.0f, 1.0f);
                        continue;
                    }

                    const float inverseLength =
                            1.0f / std::sqrt(lengthSquared);

                    normal.x *= inverseLength;
                    normal.y *= inverseLength;
                    normal.z *= inverseLength;
                }
            }
            if (displayFace.indices.empty()) {
                ++skippedFaceCount;
                continue;
            }
            updateCorners(displayFace);
            triangleCount += displayFace.indices.size() / 3;
            vertexCount += displayFace.positions.size();
            ++triangulatedFaceCount;
            if (bodyIndex >= 0 && bodyIndex < static_cast<int>(object.bodies.size())) {
                object.bodies[bodyIndex].faceTags.push_back(faceTag);
                for (const hts::viewer::HtsVec3f& position : displayFace.positions) {
                    object.bodies[bodyIndex].bounds.expandBy(position);
                }
            }
            object.faces.push_back(std::move(displayFace));
        }
    }
    if (rejectIfLiveMemoryUnsafe("after face mesh extraction")) {
        return false;
    }
    if (rejectIfLiveMemoryUnsafe("before edge extraction")) {
        return false;
    }
    if (options.buildCadEdges) {
        ImportStageTimer stage("edge polyline extraction / sampling");
        for (TopExp_Explorer explorer(shape, TopAbs_EDGE); explorer.More(); explorer.Next()) {
            ++edgeCount;
            const TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
            const int edgeTag = edgeTagMap.FindIndex(edge);
            if (edgeTag <= 0) {
                continue;
            }
            const int bodyIndex = findBodyIndex(
                    bodyIndexMap, edge, edgeSolidOwnerMap);
            hts::viewer::HtsDisplayEdgeData displayEdge;
            displayEdge.objectId = object.objectId;
            displayEdge.bodyId = bodyIndex >= 0 ? bodyInfos[bodyIndex].bodyId : std::string();
            displayEdge.edgeTag = edgeTag;
            displayEdge.color = {0.08f, 0.12f, 0.16f, 1.0f};
            displayEdge.ownerFaceTags.reserve(2);
            if (edgeFaceOwnerMap.Contains(edge)) {
                const TopTools_ListOfShape& ownerFaces = edgeFaceOwnerMap.FindFromKey(edge);
                for (TopTools_ListIteratorOfListOfShape faceIt(ownerFaces); faceIt.More(); faceIt.Next()) {
                    const int ownerFaceTag = faceTagMap.FindIndex(faceIt.Value());
                    if (ownerFaceTag > 0
                        && std::find(displayEdge.ownerFaceTags.begin(),
                                     displayEdge.ownerFaceTags.end(),
                                     ownerFaceTag) == displayEdge.ownerFaceTags.end()) {
                        displayEdge.ownerFaceTags.push_back(ownerFaceTag);
                        ++edgeOwnerFaceRelationCount;
                    }
                }
            }
            if (appendPolygon3D(edge, displayEdge)) {
                ++polylineEdgeCount;
            }
            else {
                displayEdge.polyline.clear();
                if (appendSampledCurve(edge, displayEdge, options.maxEdgeSamplePoints)) {
                    ++sampledEdgeCount;
                }
            }
            if (displayEdge.polyline.size() < 2) {
                ++skippedEdgeCount;
                continue;
            }
            totalEdgePolylinePointCount += displayEdge.polyline.size();
            maxEdgePolylinePointCount = std::max(maxEdgePolylinePointCount, displayEdge.polyline.size());
            if (bodyIndex >= 0 && bodyIndex < static_cast<int>(object.bodies.size())) {
                object.bodies[bodyIndex].edgeTags.push_back(edgeTag);
                const hts::viewer::HtsDisplayBodyKind bodyKind = bodyInfos[bodyIndex].kind;
                if (bodyKind == hts::viewer::HtsDisplayBodyKind::Sheet
                    || bodyKind == hts::viewer::HtsDisplayBodyKind::Shell) {
                    ++sheetOwnedEdgeCount;
                }
                else {
                    ++solidOwnedEdgeCount;
                }
            }
            else if (!displayEdge.ownerFaceTags.empty()) {
                ++sheetOwnedEdgeCount;
            }
            object.edges.push_back(std::move(displayEdge));
        }
    }
    else {
        std::cout << "[ImportStats] cadEdgeDisplayData=false edgeTopologyCount="
                  << edgeTagMap.Extent() << std::endl;
    }
    if (rejectIfLiveMemoryUnsafe("before DisplayData finalize")) {
        return false;
    }
    int vertexTag = 0;
    {
        ImportStageTimer stage("vertex topology extraction");
        vertexTopologyCount = static_cast<std::size_t>(vertexTagMap.Extent());
        if (options.buildVertexDisplayData) {
            for (TopExp_Explorer explorer(shape, TopAbs_VERTEX); explorer.More(); explorer.Next()) {
                ++vertexTag;
                const TopoDS_Vertex vertex = TopoDS::Vertex(explorer.Current());
                const int bodyIndex = findBodyIndex(
                        bodyIndexMap, vertex, vertexSolidOwnerMap);
                const gp_Pnt point = BRep_Tool::Pnt(vertex);
                // Sandbox fallback: vertexTag follows OCCT explorer order.
                // Main CAD migration should inject TopoDS_Shape_TopologyMap vertex tags here.
                hts::viewer::HtsDisplayVertexData displayVertex;
                displayVertex.objectId = object.objectId;
                displayVertex.bodyId = bodyIndex >= 0 ? bodyInfos[bodyIndex].bodyId : std::string();
                displayVertex.solidTag = bodyIndex >= 0 ? bodyInfos[bodyIndex].solidTag : -1;
                displayVertex.vertexTag = vertexTag;
                displayVertex.position = {static_cast<float>(point.X()),
                                          static_cast<float>(point.Y()),
                                          static_cast<float>(point.Z())};
                if (bodyIndex >= 0 && bodyIndex < static_cast<int>(object.bodies.size())) {
                    object.bodies[bodyIndex].vertexTags.push_back(vertexTag);
                    object.bodies[bodyIndex].bounds.expandBy(displayVertex.position);
                }
                object.vertices.push_back(displayVertex);
            }
        }
        else {
            std::cout << "[ImportStats] vertexDisplayData=false vertexTopologyCount="
                      << vertexTopologyCount << std::endl;
        }
    }
    if (object.faces.empty() || triangleCount == 0) {
        errorMessage = "OCCT shape has no triangulated faces.";
        return false;
    }
    const std::size_t positionMemoryBytes = vertexCount * sizeof(hts::viewer::HtsVec3f);
    const std::size_t normalMemoryBytes = vertexCount * sizeof(hts::viewer::HtsVec3f);
    const std::size_t indexMemoryBytes = triangleCount * 3 * sizeof(unsigned int);
    const std::size_t edgePolylineMemoryBytes = totalEdgePolylinePointCount * sizeof(hts::viewer::HtsVec3f);
    const std::size_t rangeTableMemoryBytes =
            (triangulatedFaceCount + object.edges.size()) * sizeof(std::size_t) * 2;
    const std::size_t materialBucketMemoryBytes =
            object.importedMaterials.size() * sizeof(hts::viewer::HtsImportedMaterialData);
    const std::size_t estimatedCpuBytes =
            positionMemoryBytes
            + normalMemoryBytes
            + indexMemoryBytes
            + edgePolylineMemoryBytes
            + rangeTableMemoryBytes
            + materialBucketMemoryBytes;
    const double estimatedCpuMB = double(estimatedCpuBytes) / (1024.0 * 1024.0);
    const double estimatedGpuMB =
            double(positionMemoryBytes
                   + normalMemoryBytes
                   + indexMemoryBytes)
            / (1024.0 * 1024.0);
    const HtsImportMemoryBudgetDecision memoryDecision =
            evaluateImportMemoryBudget(options, estimatedCpuMB, estimatedGpuMB, "DisplayData finalize");
    logImportMemoryLive("DisplayData finalize", options, memoryDecision);
    if (!memoryDecision.allowed) {
        std::cout << "[ImportRejected]"
                  << " reason=memory_budget"
                  << " estimatedCpuMB=" << estimatedCpuMB
                  << " estimatedGpuMB=" << estimatedGpuMB
                  << " budgetMB=" << options.memoryBudgetMB
                  << " decisionReason=" << memoryDecision.reason
                  << std::endl;
        errorMessage = "Import rejected by memory budget. estimatedCpuMB="
                       + std::to_string(estimatedCpuMB)
                       + " estimatedGpuMB=" + std::to_string(estimatedGpuMB)
                       + " reason=" + memoryDecision.reason;
        return false;
    }
    {
        ImportStageTimer stage("DisplayData finalize");
        outModel.displayName = displayName.empty() ? "ImportedOccShape" : displayName;
        outModel.meshData.sceneType = "ImportedOccShape";
        outModel.messages.push_back("faceCount=" + std::to_string(faceCount)
                                    + " triangulatedFaceCount=" + std::to_string(triangulatedFaceCount)
                                    + " skippedFaceCount=" + std::to_string(skippedFaceCount)
                                    + " triangleCount=" + std::to_string(triangleCount)
                                    + " vertexCount=" + std::to_string(vertexCount)
                                    + " hasNormals=true"
                                    + " normalSource=" + std::string(occtNormalTriangleCount > 0 ? "OCCT" : "Computed")
                                    + " occtNormalTriangleCount=" + std::to_string(occtNormalTriangleCount)
                                    + " computedNormalTriangleCount=" + std::to_string(computedNormalTriangleCount)
                                    + " reversedFaceCount=" + std::to_string(reversedFaceCount)
                                    + " edgeCount=" + std::to_string(edgeCount)
                                    + " polylineEdgeCount=" + std::to_string(polylineEdgeCount)
                                    + " sampledEdgeCount=" + std::to_string(sampledEdgeCount)
                                    + " skippedEdgeCount=" + std::to_string(skippedEdgeCount)
                                    + " vertexTopologyCount=" + std::to_string(vertexTopologyCount)
                                    + " importedFaceColorCount=" + std::to_string(importedFaceColorCount)
                                    + " autoDefaultMaterialFaceCount=" + std::to_string(autoMaterialFaceCount));
        std::cout << "OccImportStats"
                  << " faceCount=" << faceCount
                  << " triangulatedFaceCount=" << triangulatedFaceCount
                  << " skippedFaceCount=" << skippedFaceCount
                  << " triangleCount=" << triangleCount
                  << " vertexCount=" << vertexCount
                  << " hasNormals=true"
                  << " normalSource=" << (occtNormalTriangleCount > 0 ? "OCCT" : "Computed")
                  << " occtNormalTriangleCount=" << occtNormalTriangleCount
                  << " computedNormalTriangleCount=" << computedNormalTriangleCount
                  << " reversedFaceCount=" << reversedFaceCount
                  << " edgeCount=" << edgeCount
                  << " polylineEdgeCount=" << polylineEdgeCount
                  << " sampledEdgeCount=" << sampledEdgeCount
                  << " skippedEdgeCount=" << skippedEdgeCount
                  << " vertexTopologyCount=" << vertexTopologyCount
                  << " importedFaceColorCount=" << importedFaceColorCount
                  << " autoDefaultMaterialFaceCount=" << autoMaterialFaceCount
                  << std::endl;
        std::size_t solidOwnedFaceCount = 0;
        for (const hts::viewer::HtsDisplayBodyData& body : object.bodies) {
            solidOwnedFaceCount += body.faceTags.size();
        }
        const std::size_t standaloneFaceCount =
                triangulatedFaceCount >= solidOwnedFaceCount ? triangulatedFaceCount - solidOwnedFaceCount : 0;
        std::cout << "[ImportStats]"
                  << " solidCount=" << bodyInfos.size()
                  << " standaloneFaceCount=" << standaloneFaceCount
                  << " faceCount=" << faceCount
                  << " edgeCount=" << edgeCount
                  << " triangleCount=" << triangleCount
                  << std::endl;
        std::cout << "[ImportEdgeStats]"
                  << " edgeCount=" << edgeCount
                  << " polygonEdgeCount=" << polylineEdgeCount
                  << " sampledEdgeCount=" << sampledEdgeCount
                  << " ownerRelationCount=" << edgeOwnerFaceRelationCount
                  << " faceEdgeRelationCount=" << faceEdgeRelationCount
                  << " solidOwnedEdgeCount=" << solidOwnedEdgeCount
                  << " sheetOwnedEdgeCount=" << sheetOwnedEdgeCount
                  << " averagePointsPerEdge="
                  << (object.edges.empty() ? 0.0 : double(totalEdgePolylinePointCount) / double(object.edges.size()))
                  << " maxPointsPerEdge=" << maxEdgePolylinePointCount
                  << " edgeMemoryEstimateMB="
                  << (double(totalEdgePolylinePointCount) * double(sizeof(hts::viewer::HtsVec3f)) / (1024.0 * 1024.0))
                  << std::endl;
        std::cout << "[ImportMemory]"
                  << " estimatedCpuMB=" << estimatedCpuMB
                  << " estimatedGpuMB=" << estimatedGpuMB
                  << " positionMB=" << (double(positionMemoryBytes) / (1024.0 * 1024.0))
                  << " normalMB=" << (double(normalMemoryBytes) / (1024.0 * 1024.0))
                  << " indexMB=" << (double(indexMemoryBytes) / (1024.0 * 1024.0))
                  << " edgePolylineMB=" << (double(edgePolylineMemoryBytes) / (1024.0 * 1024.0))
                  << " rangeTableMB=" << (double(rangeTableMemoryBytes) / (1024.0 * 1024.0))
                  << " materialBucketMB=" << (double(materialBucketMemoryBytes) / (1024.0 * 1024.0))
                  << " budgetMB=" << options.memoryBudgetMB
                  << std::endl;
        outModel.meshData.objects.push_back(std::move(object));
    }
    errorMessage.clear();
    return true;
}

#endif

bool HtsOccShapeDisplayDataBuilder::buildFromUnavailableOccShape(HtsImportedModel& outModel,
                                                                 std::string& errorMessage) const
{
    outModel = HtsImportedModel();
    errorMessage = "OCCT shape to DisplayData builder is not available because HTS_ENABLE_OCCT_IMPORT is OFF.";
    return false;

}

}
