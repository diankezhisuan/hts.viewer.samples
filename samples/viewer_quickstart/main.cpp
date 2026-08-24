#include "SampleInput.h"
#include "SampleScene.h"

#include <iostream>

using namespace hts::viewer;

int main()
{
    const HtsDisplayObjectData boxC = samples::makeBox(
            "Quickstart_C", {3.6f, 0.0f, 0.0f}, 1.8f, {0.86f, 0.58f, 0.42f, 1.0f});
    HtsDisplayMeshData scene;
    scene.objects.push_back(samples::makeBox(
            "Quickstart_A", {-1.2f, 0.0f, 0.0f}, 1.8f, {0.44f, 0.68f, 0.88f, 1.0f}));
    scene.objects.push_back(samples::makeBox(
            "Quickstart_B", {1.2f, 0.0f, 0.0f}, 1.8f, {0.72f, 0.78f, 0.56f, 1.0f}));

    HtsViewerSdk viewer;
    if (!samples::initializeViewer(viewer, std::move(scene), "viewer_quickstart")) {
        std::cerr << "Failed to initialize viewer_quickstart. Check authorization.\n";
        return 1;
    }
    viewer.setDisplayStyle(HtsDisplayStyle::ShadedWithCadEdges);
    viewer.setFloorVisible(true);
    viewer.setGroundGridVisible(true);

    bool boxBVisible = true, boxCAdded = false, floorVisible = true;
    bool gridVisible = true, axisVisible = true, hudVisible = true;
    const std::string commands =
            "1  Show/hide Quickstart_B\n2  Add/remove Quickstart_C incrementally\n"
            "3  Restore all object visibility\nF  Fit all    I  Iso    T  Top    R  Front\n"
            "G  Ground grid    L  Floor    A  World axis    U  HUD/scale bar\n"
            "S  Print display statistics\n";

    return samples::runInteractive(viewer, "viewer_quickstart", commands,
            [&](samples::HotkeyState& keys) {
                if (keys.pressed('1')) {
                    boxBVisible = !boxBVisible;
                    viewer.setObjectsVisible({"Quickstart_B"}, boxBVisible);
                    std::cout << "Quickstart_B visible=" << boxBVisible << '\n';
                }
                if (keys.pressed('2')) {
                    if (boxCAdded) viewer.removeDisplayObject("Quickstart_C");
                    else { HtsDisplayMeshData data; data.objects.push_back(boxC); viewer.upsertDisplayData(data); }
                    boxCAdded = !boxCAdded; viewer.fitView();
                    std::cout << "Quickstart_C present=" << boxCAdded << '\n';
                }
                if (keys.pressed('3')) { viewer.showAll(); boxBVisible = true; }
                if (keys.pressed('F')) viewer.fitView();
                if (keys.pressed('I')) { viewer.setIsoView(); viewer.fitView(); }
                if (keys.pressed('T')) { viewer.setTopView(); viewer.fitView(); }
                if (keys.pressed('R')) { viewer.setFrontView(); viewer.fitView(); }
                if (keys.pressed('G')) { gridVisible = !gridVisible; viewer.setGroundGridVisible(gridVisible); }
                if (keys.pressed('L')) { floorVisible = !floorVisible; viewer.setFloorVisible(floorVisible); }
                if (keys.pressed('A')) { axisVisible = !axisVisible; viewer.setAxisVisible(axisVisible); }
                if (keys.pressed('U')) { hudVisible = !hudVisible; viewer.setHudAxisVisible(hudVisible); viewer.setScaleBarVisible(hudVisible); }
                if (keys.pressed('S')) std::cout << viewer.displayStatsText() << '\n';
            });
}
