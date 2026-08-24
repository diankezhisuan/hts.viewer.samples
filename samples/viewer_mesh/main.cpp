#include "SampleInput.h"

#include <HtsViewerSdk.h>

#include <iostream>

using namespace hts::viewer;

namespace
{
HtsMeshPartData makeQuad(const std::string& partId, float x0, float x1)
{
    HtsMeshPartData part;
    part.partId = partId;
    part.vertices = {
        {x0, -1.5f, 0.0f}, {x1, -1.5f, 0.0f},
        {x1,  1.5f, 0.0f}, {x0,  1.5f, 0.0f}
    };
    part.vertexIds = {1, 2, 3, 4};
    part.triangleIndices = {0, 1, 2, 0, 2, 3};
    part.triangleIds = {1, 2};
    part.edgeVertices = part.vertices;
    part.edgeIndices = {0, 1, 1, 2, 2, 3, 3, 0};
    part.edgeIds = {1, 2, 3, 4};
    return part;
}
}

int main()
{
    HtsViewerSdk viewer;
    if (!viewer.initializeStandalone(100, 100, 1024, 768)) {
        std::cerr << "Failed to initialize viewer_mesh." << std::endl;
        return 1;
    }

    const std::string cancelledMeshId = "Mesh_Cancelled";
    const HtsMeshPartData cancelledPart = makeQuad("face_1", -1.0f, 1.0f);
    if (!viewer.beginMeshUpdate(cancelledMeshId)
            || !viewer.appendMeshPart(cancelledMeshId, cancelledPart)
            || viewer.appendMeshPart(cancelledMeshId, cancelledPart)
            || !viewer.cancelMeshUpdate(cancelledMeshId)
            || viewer.hasMesh(cancelledMeshId)) {
        std::cerr << "Mesh transaction contract failed." << std::endl;
        return 2;
    }

    const std::string meshId = "Mesh_Demo";
    if (!viewer.beginMeshUpdate(meshId)
            || !viewer.appendMeshPart(meshId, makeQuad("face_1", -3.0f, 0.0f))
            || !viewer.appendMeshPart(meshId, makeQuad("face_2", 0.0f, 3.0f))
            || !viewer.endMeshUpdate(meshId)) {
        std::cerr << "Failed to submit mesh data." << std::endl;
        return 3;
    }

    HtsMeshPartData edited = makeQuad("face_2", 0.0f, 3.0f);
    edited.vertices[2].z = 0.35f;
    edited.vertices[3].z = 0.35f;
    viewer.upsertMeshPart(meshId, edited);

    HtsMeshPartStyle style;
    style.hasFaceColor = true;
    style.faceColor = {0.20f, 0.62f, 0.92f, 0.85f};
    viewer.setMeshPartStyle(meshId, "face_1", style);
    viewer.setSelectedMeshParts(meshId, {"face_2"});

    bool firstPartVisible = false;
    if (!viewer.hasMesh(meshId)
            || viewer.meshPartIds(meshId).size() != 2
            || viewer.selectedMeshPartIds(meshId) != std::vector<std::string>{"face_2"}
            || !viewer.meshPartVisible(meshId, "face_1", firstPartVisible)
            || !firstPartVisible) {
        std::cerr << "Mesh state query failed." << std::endl;
        return 4;
    }

    const HtsMeshStatistics stats = viewer.meshStatistics(meshId);
    std::cout << "parts=" << stats.partCount
              << " triangles=" << stats.triangleCount
              << " edges=" << stats.volumeEdgeCount << std::endl;

    viewer.fitView();
    bool firstVisible = true;
    bool selectionVisible = true;
    bool transparentMode = true;
    const std::string commands =
            "1  Show/hide face_1    2  Isolate face_2    3  Show both parts\n"
            "S  Select/clear face_2    C  Apply/clear face_1 style\n"
            "M  Toggle transparent/opaque mesh mode    F  Fit all\n"
            "P  Print mesh statistics\n";
    return samples::runInteractive(viewer, "viewer_mesh", commands,
            [&](samples::HotkeyState& keys) {
                if (keys.pressed('1')) { firstVisible = !firstVisible; viewer.setMeshPartsVisible(meshId, {"face_1"}, firstVisible); }
                if (keys.pressed('2')) viewer.showOnlyMeshParts(meshId, {"face_2"});
                if (keys.pressed('3')) viewer.setMeshPartsVisible(meshId, {"face_1", "face_2"}, true);
                if (keys.pressed('S')) {
                    selectionVisible = !selectionVisible;
                    if (selectionVisible) viewer.setSelectedMeshParts(meshId, {"face_2"});
                    else viewer.clearMeshSelection(meshId);
                }
                if (keys.pressed('C')) {
                    static bool styled = true;
                    styled = !styled;
                    if (styled) viewer.setMeshPartStyle(meshId, "face_1", style);
                    else viewer.clearMeshPartStyle(meshId, "face_1");
                }
                if (keys.pressed('M')) {
                    transparentMode = !transparentMode;
                    HtsMeshDisplayOptions options = viewer.meshDisplayOptions();
                    options.mode = transparentMode ? HtsMeshDisplayMode::TransparentWithMeshLines
                                                   : HtsMeshDisplayMode::ShadedWithMeshLines;
                    viewer.applyMeshDisplayOptions(options);
                }
                if (keys.pressed('F')) viewer.fitView();
                if (keys.pressed('P')) {
                    const HtsMeshStatistics current = viewer.meshStatistics(meshId);
                    std::cout << "parts=" << current.partCount << " triangles=" << current.triangleCount
                              << " edges=" << current.volumeEdgeCount << '\n';
                }
            });
}
