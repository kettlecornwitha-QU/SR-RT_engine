#!/usr/bin/env python3
from __future__ import annotations


IMAGE_RENDERER_TITLE = "Image Renderer"
VIDEO_RENDERER_TITLE = "Video Renderer"
SETTINGS_DIALOG_TITLE = "Render Settings"

IMAGE_WINDOW_SIZE = (1560, 920)
VIDEO_WINDOW_SIZE = (1700, 1000)
SETTINGS_DIALOG_SIZE = (520, 700)

MAIN_LAYOUT_MARGIN = 8
SETTINGS_DIALOG_MARGIN = 12
SETTINGS_DIALOG_SPACING = 10

LEFT_PANEL_SPACING = 10
RIGHT_PANEL_SPACING = 12
SECTION_PADDING_SPACING = 6

LEFT_PANEL_MAX_WIDTH_IMAGE = 460
LEFT_PANEL_MAX_WIDTH_VIDEO = 470

FORMULA_SECTION_SPACING = 8
FORMULA_ROOT_SPACING = 4
FORMULA_LINE_SPACING = 8
FORMULA_FIELD_COLUMN_SPACING = 6

OVERLAY_BUTTON_MIN_HEIGHT = 32
OVERLAY_PROGRESS_WIDTH = 780

STATUS_IDLE = "Idle"
STATUS_RENDERING = "Rendering..."
IMAGE_PLACEHOLDER_TEXT = "Render output will appear here"
PRESET_PLACEHOLDER_TEXT = "Preset: ________"
TOTAL_FRAMES_PLACEHOLDER = "Math expression, e.g. int(2*fps*halfway)"

GROUPBOX_STYLE = (
    "QGroupBox { margin-top: 16px; }"
    "QGroupBox::title {"
    "font-size: 14px; font-weight: 600;"
    "subcontrol-origin: margin;"
    "subcontrol-position: top left;"
    "padding: 0 4px;"
    "top: 0px;"
    "}"
)

OVERLAY_STYLE = "background-color: rgba(0,0,0,200);"
OVERLAY_TITLE_STYLE = "color: white; font-size: 18px; font-weight: 600;"
OVERLAY_MESSAGE_STYLE = "color: white;"
