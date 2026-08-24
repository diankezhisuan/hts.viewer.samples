#pragma once

#include "HtsDisplayData.h"
#include "HtsMeshData.h"
#include "HtsViewerApi.h"
#include "HtsViewerTypes.h"
#include "HtsViewerSettings.h"

#include <memory>
#include <string>
#include <vector>

namespace hts::viewer
{
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable: 4251)
#endif
class HTS_VIEWER_API HtsViewerSdk
{
public:
    HtsViewerSdk();
    ~HtsViewerSdk();

    HtsViewerSdk(const HtsViewerSdk&) = delete;
    HtsViewerSdk& operator=(const HtsViewerSdk&) = delete;
    HtsViewerSdk(HtsViewerSdk&&) noexcept;
    HtsViewerSdk& operator=(HtsViewerSdk&&) noexcept;

    bool initializeEmbedded(int x, int y, int width, int height);
    bool initializeStandalone(int x, int y, int width, int height);
    bool frame();
    bool frame(double simulationTime);
    /** Return true while internal chunked display work needs more frames. */
    bool hasPendingFrameWork() const;
    bool shouldClose() const;
    bool isAuthorized() const;
    bool submitInput(const HtsInputEvent& event);
    bool resizeViewport(int x, int y, int width, int height);

    /** Apply an OSG-free setting value to this Viewer instance. */
    bool applySettings(const HtsViewerSettings& settings);
    /** Return the settings currently owned by this Viewer instance. */
    HtsViewerSettings settings() const;
    /** Load a settings INI file and apply it atomically on success. */
    bool loadSettings(const std::string& filePath);
    /** Save this Viewer instance's current settings to an INI file. */
    bool saveSettings(const std::string& filePath) const;

    std::string machineCode() const;
    bool exportMachineCode(const std::string& filePath) const;
    bool importLicense(const std::string& directoryPath);
    bool checkAuthorization() const;
    /**
     * Return the authorization expiration as Unix time in seconds.
     * Trial mode returns the trial end time; formal mode returns the license
     * expiration. Zero means that the value is unavailable.
     */
    std::int64_t authorizationExpirationTime() const;
    /** Return whether the current authorization is trial or formal. */
    HtsAuthorizationType authorizationType() const;

    void clearScene();
    bool upsertDisplayData(const HtsDisplayMeshData& data);
    bool removeDisplayObject(const std::string& objectId);
    bool removeDisplayObjects(const std::vector<std::string>& objectIds);

    /** Add or replace one complete mesh result in the Viewer Mesh layer. */
    bool upsertMeshData(const HtsMeshDisplayData& data);
    /**
     * Start a staged mesh update without affecting the live mesh.
     * replaceExisting=true starts empty; false snapshots the current mesh and
     * permits appending only new partIds. A new begin for the same meshId
     * replaces its previous staged transaction.
     */
    bool beginMeshUpdate(const std::string& meshId, bool replaceExisting = true);
    /** Append one complete part; partId must be unique in the staged update. */
    bool appendMeshPart(const std::string& meshId, const HtsMeshPartData& part);
    /**
     * Atomically publish the staged mesh. On failure, both the live mesh and
     * staged transaction remain unchanged so the caller may retry or cancel.
     */
    bool endMeshUpdate(const std::string& meshId);
    /** Discard staged data without changing the currently displayed mesh. */
    bool cancelMeshUpdate(const std::string& meshId);
    /** Add or replace one part without rebuilding unrelated mesh parts. */
    bool upsertMeshPart(const std::string& meshId, const HtsMeshPartData& part);
    bool removeMeshPart(const std::string& meshId, const std::string& partId);
    bool removeMesh(const std::string& meshId);
    void clearMeshes();
    bool hasMesh(const std::string& meshId) const;
    std::vector<std::string> meshPartIds(const std::string& meshId) const;
    std::vector<std::string> selectedMeshPartIds(const std::string& meshId) const;
    bool meshPartVisible(const std::string& meshId,
                         const std::string& partId,
                         bool& visible) const;

    bool setMeshVisible(const std::string& meshId, bool visible);
    bool setMeshPartsVisible(const std::string& meshId,
                             const std::vector<std::string>& partIds,
                             bool visible);
    bool showOnlyMeshParts(const std::string& meshId,
                           const std::vector<std::string>& partIds);
    bool setSelectedMeshParts(const std::string& meshId,
                              const std::vector<std::string>& partIds);
    bool clearMeshSelection(const std::string& meshId);
    bool setMeshPartStyle(const std::string& meshId,
                          const std::string& partId,
                          const HtsMeshPartStyle& style);
    bool clearMeshPartStyle(const std::string& meshId,
                            const std::string& partId);
    bool applyMeshDisplayOptions(const HtsMeshDisplayOptions& options);
    HtsMeshDisplayOptions meshDisplayOptions() const;
    HtsMeshStatistics meshStatistics(const std::string& meshId) const;

    /** Add or replace temporary preview geometry identified by a stable tag. */
    bool upsertPreview(const std::string& tag,
                       const HtsDisplayMeshData& data,
                       const HtsPreviewStyle& style = {});

    /** Add or replace a Viewer-styled coordinate-axis preview. */
    bool upsertPreviewAxis(const std::string& tag,
                           const HtsVec3f& origin,
                           const HtsVec3f& x,
                           const HtsVec3f& y,
                           const HtsVec3f& z,
                           double axisLength = 2.0,
                           const HtsPreviewStyle& style = {});

    bool setPreviewVisible(const std::string& tag, bool visible);
    bool setPreviewTransform(const std::string& tag,
                             const HtsTransform& transform);
    bool hasPreview(const std::string& tag) const;
    bool upsertPreviewMarker(const std::string& tag,
                             const HtsVec3d& worldPoint,
                             HtsFeaturePointKind kind,
                             bool hollow = false);
    bool removePreview(const std::string& tag);
    void clearPreviews();

    void setSelectionMode(HtsSelectionMode mode);
    HtsSelectionMode selectionMode() const;
    void clearSelection();
    /** Query the target under a screen position without changing selection. */
    bool pickTarget(double x, double y, HtsSelectionTarget& target) const;
    /** Query once with an explicit mode and return its semantic target and hit point. */
    bool pickTarget(double x,
                    double y,
                    HtsSelectionMode mode,
                    HtsSelectionTarget& target,
                    HtsVec3d& worldPoint) const;
    bool pickFeaturePoint(double x,
                          double y,
                          HtsVec3d& worldPoint,
                          HtsFeaturePointKind& kind) const;
    bool screenToWorldRay(double x,
                          double y,
                          HtsVec3d& origin,
                          HtsVec3d& direction) const;
    /** Project a world point to top-left-origin framebuffer coordinates. */
    bool projectWorldToScreen(const HtsVec3d& worldPoint,
                              HtsVec3d& screenPoint) const;
    bool selectAt(double x, double y, bool additive = false);
    bool boxSelect(double x0, double y0, double x1, double y1);
    bool selectAllVisible();
    bool hasSelection() const;
    HtsSelectionTarget selectionTarget() const;
    /** Return the complete current semantic selection set without picking. */
    std::vector<HtsSelectionTarget> selectionTargets() const;
    HtsSelectionDisplayState selectionDisplayState() const;
    bool setSelectionTargets(const std::vector<HtsSelectionTarget>& targets);
    /** Enable or disable selection dimming without changing selection targets. */
    bool setSelectionDimEnabled(bool enabled);

    bool updateHover(double x, double y);
    bool hasHover() const;
    /** Return the current semantic hover target without running another pick. */
    HtsSelectionTarget hoverTarget() const;
    void clearHover();

    void setDisplayStyle(HtsDisplayStyle style);
    HtsDisplayStyle displayStyle() const;
    void showCadEdges(bool visible);
    bool cadEdgesVisible() const;
    void showTriangleWireframe(bool visible);
    bool triangleWireframeVisible() const;
    void setAirBoxTransparent(bool transparent);
    bool airBoxTransparent() const;
    bool hideSelected();
    bool showSelected();
    bool isolateSelected();
    bool clearIsolate();
    bool showAll();
    bool resetVisibility();

    bool setTargetVisible(const HtsSelectionTarget& target, bool visible);
    bool targetVisible(const HtsSelectionTarget& target, bool& visible) const;
    bool setObjectsVisible(const std::vector<std::string>& objectIds,
                           bool visible);
    bool setTargetsVisible(const std::vector<HtsSelectionTarget>& targets,
                           bool visible);
    bool setObjectPartVisible(const std::string& objectId,
                              HtsObjectDisplayPart part,
                              bool visible);
    bool setObjectTopologyVisible(
            const std::vector<std::string>& objectIds,
            bool visible);
    bool resetDisplayOverrides();
    bool setTargetColorAndTransparency(const HtsSelectionTarget& target,
                                       const HtsColor4f& color,
                                       float transparency);
    /** Sets persistent color, transparency and PBR appearance for one surface target. */
    bool setTargetMaterialAppearance(const HtsSelectionTarget& target,
                                     const HtsMaterialAppearance& appearance);
    bool targetColorAndTransparency(const HtsSelectionTarget& target,
                                    HtsColor4f& color,
                                    float& transparency) const;
    bool setObjectsColorAndTransparency(
            const std::vector<std::string>& objectIds,
            const HtsColor4f& color,
            float transparency);
    /** Sets persistent color, transparency and PBR appearance for objects. */
    bool setObjectsMaterialAppearance(
            const std::vector<std::string>& objectIds,
            const HtsMaterialAppearance& appearance);
    bool setSelectedColor(const HtsColor4f& color);
    bool setSelectedOpacity(float opacity);
    bool setSelectedTransparent();
    bool setSelectedMaterialPreset(HtsMaterialPreset preset);
    bool setSelectedCustomMaterial(const HtsRenderMaterial& material);
    bool resetSelectedStyle();

    bool setAxisVisible(bool visible);
    bool setScaleBarVisible(bool visible);
    bool setHudAxisVisible(bool visible);
    /** Show or hide the filled floor plane without changing the ground grid. */
    bool setFloorVisible(bool visible);
    /** Show or hide the ground grid lines without changing the floor plane. */
    bool setGroundGridVisible(bool visible);
    bool setDebugEnabled(bool enabled);
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
    HtsGridPlane currentGridPlane() const;
    double currentGridSpacing() const;

    bool showPlaneWaveGlyph(const HtsPlaneWaveGlyph& glyph);
    bool removePlaneWaveGlyph(const std::string& id);
    bool showBoundaryFaceOverlay(const std::string& key,
                                 const HtsFaceOverlayData& data);
    bool showExcitationFaceOverlay(const std::string& key,
                                   const HtsFaceOverlayData& data);
    /** Remove only the face overlay previously submitted with this key. */
    bool removeFaceOverlay(const std::string& key);
    bool clearFaceOverlays();
    bool flyToTargets(const std::vector<HtsSelectionTarget>& targets);

    void fitView();
    void fitSelection();
    void setTopView();
    void setBottomView();
    void setFrontView();
    void setRearView();
    void setRightView();
    void setLeftView();
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
