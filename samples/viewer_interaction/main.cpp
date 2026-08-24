#include "SampleInput.h"
#include "SampleScene.h"

#include <iostream>

using namespace hts::viewer;

int main()
{
    HtsDisplayMeshData scene;
    scene.objects.push_back(samples::makeBox(
            "Interaction_A", {-1.4f, 0.0f, 0.0f}, 1.8f, {0.64f, 0.76f, 0.90f, 1.0f}));
    scene.objects.push_back(samples::makeBox(
            "Interaction_B", {1.4f, 0.0f, 0.0f}, 1.8f, {0.78f, 0.82f, 0.68f, 1.0f}));
    HtsViewerSdk viewer;
    if (!samples::initializeViewer(viewer, std::move(scene), "viewer_interaction")) {
        std::cerr << "Failed to initialize viewer_interaction.\n";
        return 1;
    }
    viewer.setSelectionMode(HtsSelectionMode::Object);

    HtsDisplayMeshData preview;
    preview.objects.push_back(samples::makeBox(
            "Preview", {0.0f, 0.0f, 0.0f}, 1.2f, {0.0f, 0.72f, 1.0f, 0.35f}));
    HtsPreviewStyle previewStyle;
    previewStyle.color = {0.0f, 0.72f, 1.0f, 0.35f};
    previewStyle.lineWidth = 2.0f;
    viewer.upsertPreview("MovePreview", preview, previewStyle);
    viewer.setPreviewTransform("MovePreview", HtsTransform::translation(0.0, 2.4, 0.0));
    HtsPreviewStyle axisStyle;
    axisStyle.layer = HtsPreviewLayer::Scene;
    axisStyle.depthTest = true;
    viewer.upsertPreviewAxis("LocalAxis", {0.0f, 2.4f, 0.0f},
            {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, 0.8, axisStyle);
    viewer.setPreviewVisible("LocalAxis", false);

    bool dimEnabled = true, previewVisible = true, axisVisible = false;
    const std::string commands =
            "Mouse left click selects; Ctrl+left click toggles multiple targets.\n"
            "1  Object mode    2  Face mode    3  Edge mode\n"
            "A  Select all visible    C  Clear selection\n"
            "D  Toggle dim unrelated    F  Fit selection    V  Fit all\n"
            "P  Show/hide preview geometry    X  Show/hide preview axis\n"
            "S  Print complete selection summary\n";

    return samples::runInteractive(viewer, "viewer_interaction", commands,
            [&](samples::HotkeyState& keys) {
                if (keys.pressed('1')) { viewer.setSelectionMode(HtsSelectionMode::Object); viewer.clearSelection(); std::cout << "Mode=Object\n"; }
                if (keys.pressed('2')) { viewer.setSelectionMode(HtsSelectionMode::Face); viewer.clearSelection(); std::cout << "Mode=Face\n"; }
                if (keys.pressed('3')) { viewer.setSelectionMode(HtsSelectionMode::Edge); viewer.clearSelection(); std::cout << "Mode=Edge\n"; }
                if (keys.pressed('A')) viewer.selectAllVisible();
                if (keys.pressed('C')) viewer.clearSelection();
                if (keys.pressed('D')) { dimEnabled = !dimEnabled; viewer.setSelectionDimEnabled(dimEnabled); }
                if (keys.pressed('F')) viewer.fitSelection();
                if (keys.pressed('V')) viewer.fitView();
                if (keys.pressed('P')) { previewVisible = !previewVisible; viewer.setPreviewVisible("MovePreview", previewVisible); }
                if (keys.pressed('X')) { axisVisible = !axisVisible; viewer.setPreviewVisible("LocalAxis", axisVisible); }
                if (keys.pressed('S')) std::cout << viewer.selectionSummary() << '\n';
            });
}
