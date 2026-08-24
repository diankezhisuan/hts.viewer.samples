#pragma once

#include "HtsViewerTypes.h"

#include <cmath>

namespace hts::viewer
{
/**
 * Per-Viewer appearance settings.
 *
 * This is an OSG-free value object. Constructing it produces the SDK factory
 * defaults; modifying a copy has no effect until it is passed to
 * HtsViewerSdk::applySettings().
 */
struct HtsViewerSettings
{
    HtsColor4f DEFAULT_COLOR{0.8f, 0.8f, 0.8f, 1.0f};
    HtsColor4f DEFAULT_VERTEX_COLOR{0.0f, 0.0f, 0.0f, 1.0f};
    HtsColor4f DEFAULT_EDGE_COLOR{0.0f, 0.0f, 0.0f, 1.0f};
    HtsColor4f DEFAULT_FACE_COLOR{1.0f, 1.0f, 1.0f, 1.0f};

    HtsColor4f DEFAULT_SELECTED_COLOR{1.0f, 1.0f, 0.0f, 1.0f};
    HtsColor4f DEFAULT_PREVIEW_COLOR{0.5f, 0.15f, 0.8f, 1.0f};
    HtsColor4f DEFAULT_MOUSE_OVER_COLOR{0.5f, 0.5f, 0.0f, 1.0f};
    HtsColor4f SCALE_TEXT_COLOR{1.0f, 1.0f, 1.0f, 1.0f};
    HtsColor4f SCALE_POLYGON_START_COLOR{1.0f, 1.0f, 1.0f, 1.0f};
    HtsColor4f SCALE_POLYGON_END_COLOR{0.0f, 0.0f, 0.0f, 1.0f};
    HtsColor4f DEFAULT_WARNING_COLOR{1.0f, 0.0f, 0.0f, 1.0f};

    /** Legacy-compatible percentage in [0, 100]; 0 is opaque. */
    int DEFAULT_TRANSPARENCE = 20;
    /** Application refresh policy retained for source-level migration. */
    int DEFAULT_REFRESH_FREQ_IN_MS = 200;
    float DEFAULT_EDGE_LINE_WIDTH = 1.0f;
    float DEFAULT_MOUSE_OVER_LINE_WIDTH = 1.6f;
    float DEFAULT_SELECTED_EDGE_LINE_WIDTH = 1.0f;
    float DEFAULT_SELECTED_FACE_OPACITY = 0.42f;
    float DEFAULT_SELECTED_VERTEX_POINT_SIZE = 7.0f;
};

inline bool isValidViewerColor(const HtsColor4f& color)
{
    return std::isfinite(color.r) && std::isfinite(color.g)
            && std::isfinite(color.b) && std::isfinite(color.a)
            && color.r >= 0.0f && color.r <= 1.0f
            && color.g >= 0.0f && color.g <= 1.0f
            && color.b >= 0.0f && color.b <= 1.0f
            && color.a >= 0.0f && color.a <= 1.0f;
}

/** Validate a complete setting value before it crosses into the renderer. */
inline bool isValidViewerSettings(const HtsViewerSettings& value)
{
    return isValidViewerColor(value.DEFAULT_COLOR)
            && isValidViewerColor(value.DEFAULT_VERTEX_COLOR)
            && isValidViewerColor(value.DEFAULT_EDGE_COLOR)
            && isValidViewerColor(value.DEFAULT_FACE_COLOR)
            && isValidViewerColor(value.DEFAULT_SELECTED_COLOR)
            && isValidViewerColor(value.DEFAULT_PREVIEW_COLOR)
            && isValidViewerColor(value.DEFAULT_MOUSE_OVER_COLOR)
            && isValidViewerColor(value.SCALE_TEXT_COLOR)
            && isValidViewerColor(value.SCALE_POLYGON_START_COLOR)
            && isValidViewerColor(value.SCALE_POLYGON_END_COLOR)
            && isValidViewerColor(value.DEFAULT_WARNING_COLOR)
            && value.DEFAULT_TRANSPARENCE >= 0
            && value.DEFAULT_TRANSPARENCE <= 100
            && value.DEFAULT_REFRESH_FREQ_IN_MS > 0
            && std::isfinite(value.DEFAULT_EDGE_LINE_WIDTH)
            && value.DEFAULT_EDGE_LINE_WIDTH > 0.0f
            && std::isfinite(value.DEFAULT_MOUSE_OVER_LINE_WIDTH)
            && value.DEFAULT_MOUSE_OVER_LINE_WIDTH > 0.0f
            && std::isfinite(value.DEFAULT_SELECTED_EDGE_LINE_WIDTH)
            && value.DEFAULT_SELECTED_EDGE_LINE_WIDTH > 0.0f
            && std::isfinite(value.DEFAULT_SELECTED_FACE_OPACITY)
            && value.DEFAULT_SELECTED_FACE_OPACITY >= 0.0f
            && value.DEFAULT_SELECTED_FACE_OPACITY <= 1.0f
            && std::isfinite(value.DEFAULT_SELECTED_VERTEX_POINT_SIZE)
            && value.DEFAULT_SELECTED_VERTEX_POINT_SIZE > 0.0f;
}
}
