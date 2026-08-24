#pragma once

#include "HtsDisplayData.h"
#include "HtsViewerApi.h"
#include "HtsViewerTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace hts::viewer
{
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable: 4251)
#endif
class HTS_VIEWER_API HtsViewer
{
public:
    HtsViewer();
    ~HtsViewer();

    HtsViewer(const HtsViewer&) = delete;
    HtsViewer& operator=(const HtsViewer&) = delete;
    HtsViewer(HtsViewer&&) noexcept;
    HtsViewer& operator=(HtsViewer&&) noexcept;

    bool initializeEmbedded(int x, int y, int width, int height);
    bool initializeStandalone(int x, int y, int width, int height);
    bool frame();
    bool frame(double simulationTime);
    bool shouldClose() const;
    bool isAuthorized() const;
    bool submitInput(const HtsInputEvent& event);
    bool resizeViewport(int x, int y, int width, int height);

    std::string machineCode() const;
    bool exportMachineCode(const std::string& filePath) const;
    bool importLicense(const std::string& directoryPath);
    bool checkAuthorization() const;

    void clearScene();
    bool upsertDisplayData(const HtsDisplayMeshData& data);
    bool removeDisplayObject(const std::string& objectId);
    bool removeDisplayObjects(const std::vector<std::string>& objectIds);

    void setSelectionMode(HtsSelectionMode mode);
    HtsSelectionMode selectionMode() const;
    void clearSelection();
    bool selectAt(double x, double y, bool additive = false);
    bool boxSelect(double x0, double y0, double x1, double y1);
    bool selectAllVisible();
    bool hasSelection() const;
    HtsSelectionTarget selectionTarget() const;
    HtsSelectionDisplayState selectionDisplayState() const;
    bool setSelectionTargets(const std::vector<HtsSelectionTarget>& targets);

    bool updateHover(double x, double y);
    bool hasHover() const;
    void clearHover();

    void setDisplayStyle(HtsDisplayStyle style);
    HtsDisplayStyle displayStyle() const;
    void showCadEdges(bool visible);
    void showTriangleWireframe(bool visible);
    bool hideSelected();
    bool showSelected();
    bool isolateSelected();
    bool clearIsolate();
    bool showAll();
    bool resetVisibility();

    bool setTargetVisible(const HtsSelectionTarget& target, bool visible);
    bool setTargetColorAndTransparency(const HtsSelectionTarget& target,
                                       const HtsColor4f& color,
                                       float transparency);
    bool setSelectedColor(const HtsColor4f& color);
    bool setSelectedOpacity(float opacity);
    bool setSelectedMaterialPreset(HtsMaterialPreset preset);
    bool setSelectedCustomMaterial(const HtsRenderMaterial& material);
    bool resetSelectedStyle();

    bool setAxisVisible(bool visible);
    bool setScaleBarVisible(bool visible);
    bool setHudAxisVisible(bool visible);
    bool setBackgroundColor(const HtsColor4f& color);
    bool setGridPlaneColor(const HtsColor4f& color);
    bool setCoordinateSystem(const std::string& id,
                             const HtsVec3f& origin,
                             const HtsVec3f& x,
                             const HtsVec3f& y,
                             const HtsVec3f& z,
                             double axisLength = 2.0);
    bool setGridFrame(const HtsVec3f& origin,
                      const HtsVec3f& x,
                      const HtsVec3f& y,
                      const HtsVec3f& z,
                      HtsGridPlane plane);

    void fitView();
    void fitSelection();
    void setTopView();
    void setFrontView();
    void setRightView();
    void setIsoView();
    void setViewCenterToSelection();
    void zoomAroundModelCenter(double factor);

    std::string displayStatsText() const;
    std::string selectionSummary() const;
    std::string hoverSummary() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif
}
