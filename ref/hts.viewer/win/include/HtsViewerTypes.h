#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>

namespace hts::viewer
{
/** Identifies the source of the Viewer authorization. */
enum class HtsAuthorizationType
{
    Unknown,
    Trial,
    Formal
};

struct HtsVec3f
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr HtsVec3f() = default;
    constexpr HtsVec3f(float xValue, float yValue, float zValue)
        : x(xValue), y(yValue), z(zValue) {}
};

inline constexpr HtsVec3f operator+(const HtsVec3f& lhs, const HtsVec3f& rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

inline constexpr HtsVec3f operator-(const HtsVec3f& lhs, const HtsVec3f& rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

inline constexpr HtsVec3f operator*(const HtsVec3f& value, float scale)
{
    return {value.x * scale, value.y * scale, value.z * scale};
}

struct HtsVec3d
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    constexpr HtsVec3d() = default;
    constexpr HtsVec3d(double xValue, double yValue, double zValue)
        : x(xValue), y(yValue), z(zValue) {}
};

struct HtsColor4f
{
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    constexpr HtsColor4f() = default;
    constexpr HtsColor4f(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
};

struct HtsBounds3d
{
    HtsVec3d minimum{};
    HtsVec3d maximum{};
    bool valid = false;

    void expandBy(const HtsVec3f& point)
    {
        expandBy(HtsVec3d{point.x, point.y, point.z});
    }

    void expandBy(const HtsVec3d& point)
    {
        if (!valid) {
            minimum = point;
            maximum = point;
            valid = true;
            return;
        }
        minimum.x = std::min(minimum.x, point.x);
        minimum.y = std::min(minimum.y, point.y);
        minimum.z = std::min(minimum.z, point.z);
        maximum.x = std::max(maximum.x, point.x);
        maximum.y = std::max(maximum.y, point.y);
        maximum.z = std::max(maximum.z, point.z);
    }
};

enum class HtsSelectionMode
{
    Object,
    Body,
    Face,
    Edge,
    Vertex
};

enum class HtsSelectionTargetType
{
    None,
    Object,
    Body,
    Face,
    Edge,
    Vertex
};

struct HtsSelectionTarget
{
    HtsSelectionTargetType type = HtsSelectionTargetType::None;
    std::string objectId;
    std::string bodyId;
    int faceTag = -1;
    int edgeTag = -1;
    int vertexTag = -1;
};

/** Return whether a selection target contains the identity required by its type. */
inline bool isValidSelectionTarget(const HtsSelectionTarget& target)
{
    switch (target.type) {
        case HtsSelectionTargetType::Object:
            return !target.objectId.empty();
        case HtsSelectionTargetType::Body:
            return !target.objectId.empty() && !target.bodyId.empty();
        case HtsSelectionTargetType::Face:
            return !target.objectId.empty() && target.faceTag > 0;
        case HtsSelectionTargetType::Edge:
            return !target.objectId.empty() && target.edgeTag > 0;
        case HtsSelectionTargetType::Vertex:
            return !target.objectId.empty() && target.vertexTag > 0;
        case HtsSelectionTargetType::None:
        default:
            return false;
    }
}

/** Compare the complete semantic identity of two selection targets. */
inline bool isSameSelectionTarget(const HtsSelectionTarget& lhs,
                                  const HtsSelectionTarget& rhs)
{
    return lhs.type == rhs.type
            && lhs.objectId == rhs.objectId
            && lhs.bodyId == rhs.bodyId
            && lhs.faceTag == rhs.faceTag
            && lhs.edgeTag == rhs.edgeTag
            && lhs.vertexTag == rhs.vertexTag;
}

struct HtsSelectionDisplayState
{
    bool visible = true;
    bool hasPersistentOverride = false;
    float opacity = 1.0f;
    HtsColor4f color{};
};

/** Geometric feature represented by a query pick or preview marker. */
enum class HtsFeaturePointKind
{
    None,
    Vertex,
    EdgeEndpoint,
    EdgeMidpoint,
    FaceCenter
};

enum class HtsDisplayStyle
{
    Shaded,
    ShadedWithCadEdges,
    TriangleWireframe,
    TransparentAirBox,
    EngineeringDefault
};

enum class HtsGridPlane
{
    XOY,
    YOZ,
    XOZ
};

enum class HtsObjectDisplayPart
{
    Object,
    Faces,
    Edges,
    Vertices
};

enum class HtsMaterialPreset
{
    EngineeringDefault,
    NeutralSolid,
    AirBoxTransparent,
    PECMetal,
    Aluminum,
    Copper,
    Dielectric,
    FR4,
    RubberAbsorber,
    Custom,
    SelectedFill,
    HoverOutline,
    CADBoundaryEdge,
    TriangleWireframe,
    VertexCandidate,
    VertexHover,
    VertexSelected,
    Preview,
    OverlayBoundary,
    OverlayExcitation
};

struct HtsRenderMaterial
{
    HtsColor4f baseColor{0.70f, 0.77f, 0.88f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.55f;
    float specular = 0.5f;
    float opacity = 1.0f;
    HtsVec3f emissive{};
    bool useLighting = true;
    bool doubleSided = true;
    bool transparent = false;
    bool wireframeEnabled = false;
};

/**
 * Persistent CAD material appearance exposed by the Viewer SDK.
 *
 * Transparency uses the CAD-facing convention: 0.0 means opaque and 1.0
 * means fully transparent.  The alpha component of color is retained for
 * convenient DTO transport; transparency is the authoritative alpha value
 * for this API.
 */
struct HtsMaterialAppearance
{
    HtsColor4f color{};
    float transparency = 0.0f;
    float metallic = 0.0f;
    float roughness = 0.55f;
    float specular = 0.50f;
};

/** Controls which Viewer-managed layer receives temporary preview geometry. */
enum class HtsPreviewLayer
{
    Scene,
    Overlay
};

enum class HtsPreviewLinePattern
{
    Solid,
    Dashed
};

/**
 * Rendering appearance for temporary geometry.
 * Geometry generation remains the caller's responsibility; OSG state remains
 * entirely inside the Viewer DLL.
 */
struct HtsPreviewStyle
{
    HtsColor4f color{0.0f, 0.65f, 1.0f, 1.0f};
    float lineWidth = 1.8f;
    bool drawFaces = true;
    bool drawEdges = true;
    bool useLighting = false;
    bool depthTest = true;
    bool depthWrite = false;
    HtsPreviewLayer layer = HtsPreviewLayer::Scene;
    HtsPreviewLinePattern linePattern = HtsPreviewLinePattern::Solid;
};

/** Column-major 4x4 affine transform used by Viewer-managed previews. */
struct HtsTransform
{
    std::array<double, 16> matrix{
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    static HtsTransform identity() { return {}; }

    static HtsTransform translation(double x, double y, double z)
    {
        HtsTransform value;
        value.matrix[12] = x;
        value.matrix[13] = y;
        value.matrix[14] = z;
        return value;
    }
};

enum class HtsInputEventType
{
    MouseMove = 0,
    MouseButtonPress = 1,
    MouseButtonRelease = 2,
    MouseDoubleClick = 3,
    MouseWheel = 4,
    KeyPress = 5,
    KeyRelease = 6,
    Resize = 7,
    Close = 8,
    MouseWarped = 9,
    PenPressure = 10,
    PenOrientation = 11,
    PenProximity = 12,
    TouchBegan = 13,
    TouchMoved = 14,
    TouchEnded = 15,
    Quit = 16
};

enum class HtsMouseButton
{
    None = 0,
    Left = 1,
    Middle = 2,
    Right = 3
};

/** Direction or mode of a mouse-wheel/trackpad scroll event. */
enum class HtsScrollMotion
{
    None,
    Left,
    Right,
    Up,
    Down,
    TwoDimensional
};

/** Tablet tool currently entering or leaving proximity. */
enum class HtsTabletPointerType
{
    Unknown,
    Pen,
    Puck,
    Eraser
};

/** Lifecycle phase of a touch point. */
enum class HtsTouchPhase
{
    Unknown,
    Began,
    Moved,
    Stationary,
    Ended
};

struct HtsInputEvent
{
    HtsInputEventType type = HtsInputEventType::MouseMove;
    double x = 0.0;
    double y = 0.0;
    HtsMouseButton button = HtsMouseButton::None;
    double wheelDelta = 0.0;
    int key = 0;
    int unmodifiedKey = 0;
    int windowX = 0;
    int windowY = 0;
    int width = 0;
    int height = 0;
    HtsScrollMotion scrollMotion = HtsScrollMotion::None;
    double scrollDeltaX = 0.0;
    double scrollDeltaY = 0.0;
    float pressure = 0.0f;
    float tiltX = 0.0f;
    float tiltY = 0.0f;
    float rotation = 0.0f;
    HtsTabletPointerType tabletPointerType = HtsTabletPointerType::Unknown;
    bool entering = false;
    unsigned int touchId = 0;
    HtsTouchPhase touchPhase = HtsTouchPhase::Unknown;
    unsigned int tapCount = 0;
};
}
