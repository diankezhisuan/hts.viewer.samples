#include "SampleInput.h"
#include "SampleScene.h"

#include <iostream>

using namespace hts::viewer;

int main()
{
    HtsDisplayMeshData scene;
    scene.objects.push_back(samples::makeBox(
            "Appearance_A", {-2.4f, 0.0f, 0.0f}, 1.8f, {0.68f, 0.77f, 0.90f, 1.0f}));
    scene.objects.push_back(samples::makeBox(
            "Appearance_B", {0.0f, 0.0f, 0.0f}, 1.8f, {0.74f, 0.82f, 0.70f, 1.0f}));
    scene.objects.push_back(samples::makeBox(
            "Appearance_C", {2.4f, 0.0f, 0.0f}, 1.8f, {0.88f, 0.75f, 0.68f, 1.0f}));
    HtsViewerSdk viewer;
    if (!samples::initializeViewer(viewer, std::move(scene), "viewer_appearance")) {
        std::cerr << "Failed to initialize viewer_appearance.\n";
        return 1;
    }
    viewer.setDisplayStyle(HtsDisplayStyle::ShadedWithCadEdges);
    HtsSelectionTarget target;
    target.type = HtsSelectionTargetType::Object;
    target.objectId = "Appearance_B";
    bool cadEdges = true, triangleWireframe = false, darkBackground = true;

    const std::string commands =
            "1  Engineering style    2  Shaded    3  Shaded with CAD edges\n"
            "E  Toggle CAD edges    W  Toggle triangle wireframe\n"
            "C  Blue color    T  55% transparent    M  Polished metal PBR\n"
            "D  Rough dielectric PBR    R  Reset all persistent overrides\n"
            "B  Toggle light/dark background    S  Print display statistics\n";

    return samples::runInteractive(viewer, "viewer_appearance", commands,
            [&](samples::HotkeyState& keys) {
                if (keys.pressed('1')) viewer.setDisplayStyle(HtsDisplayStyle::EngineeringDefault);
                if (keys.pressed('2')) viewer.setDisplayStyle(HtsDisplayStyle::Shaded);
                if (keys.pressed('3')) viewer.setDisplayStyle(HtsDisplayStyle::ShadedWithCadEdges);
                if (keys.pressed('E')) { cadEdges = !cadEdges; viewer.showCadEdges(cadEdges); }
                if (keys.pressed('W')) { triangleWireframe = !triangleWireframe; viewer.showTriangleWireframe(triangleWireframe); }
                if (keys.pressed('C')) viewer.setTargetColorAndTransparency(target, {0.18f, 0.48f, 0.92f, 1.0f}, 0.0f);
                if (keys.pressed('T')) viewer.setTargetColorAndTransparency(target, {0.20f, 0.72f, 0.92f, 1.0f}, 0.55f);
                if (keys.pressed('M')) viewer.setTargetMaterialAppearance(target,
                        {{0.82f, 0.58f, 0.20f, 1.0f}, 0.0f, 0.90f, 0.16f, 0.88f});
                if (keys.pressed('D')) viewer.setTargetMaterialAppearance(target,
                        {{0.25f, 0.62f, 0.82f, 1.0f}, 0.0f, 0.02f, 0.78f, 0.35f});
                if (keys.pressed('R')) viewer.resetDisplayOverrides();
                if (keys.pressed('B')) {
                    darkBackground = !darkBackground;
                    viewer.setBackgroundColor(darkBackground
                            ? HtsColor4f{0.10f, 0.12f, 0.16f, 1.0f}
                            : HtsColor4f{0.82f, 0.86f, 0.92f, 1.0f});
                }
                if (keys.pressed('S')) std::cout << viewer.displayStatsText() << '\n';
            });
}
