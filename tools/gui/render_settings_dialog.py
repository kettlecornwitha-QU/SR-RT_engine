#!/usr/bin/env python3
from __future__ import annotations

from typing import Callable, Optional

from PySide6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLineEdit,
    QPushButton,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

from button_styles import style_button
from gui_constants import SETTINGS_DIALOG_MARGIN, SETTINGS_DIALOG_SIZE, SETTINGS_DIALOG_SPACING, SETTINGS_DIALOG_TITLE
from options_schema import GuiOptionsSchema


def _float_text(v: float) -> str:
    return f"{v:.6f}".rstrip("0").rstrip(".") or "0"


class RenderSettingsDialog(QDialog):
    def __init__(
        self,
        parent: QWidget,
        schema: GuiOptionsSchema,
        on_change: Optional[Callable[[], None]] = None,
        *,
        title: str = SETTINGS_DIALOG_TITLE,
        include_keep_frames: bool = False,
        auto_ppm_denoised_only_with_denoise: bool = False,
        allow_denoise: bool = True,
        denoise_disabled_reason: str = "",
    ) -> None:
        super().__init__(parent)
        self.schema = schema
        self._on_change = on_change
        self.setWindowTitle(title)
        self.setModal(True)
        self.resize(*SETTINGS_DIALOG_SIZE)

        root = QVBoxLayout(self)
        root.setContentsMargins(
            SETTINGS_DIALOG_MARGIN,
            SETTINGS_DIALOG_MARGIN,
            SETTINGS_DIALOG_MARGIN,
            SETTINGS_DIALOG_MARGIN,
        )
        root.setSpacing(SETTINGS_DIALOG_SPACING)

        self.width_spin = QSpinBox()
        self.width_spin.setRange(64, 8192)
        self.width_spin.setValue(schema.width)
        self.height_spin = QSpinBox()
        self.height_spin.setRange(64, 8192)
        self.height_spin.setValue(schema.height)
        self.spp_spin = QSpinBox()
        self.spp_spin.setRange(1, 4096)
        self.spp_spin.setValue(schema.spp)
        self.max_depth_spin = QSpinBox()
        self.max_depth_spin.setRange(1, 64)
        self.max_depth_spin.setValue(schema.max_depth)
        self.adaptive_check = QCheckBox("Adaptive")
        self.adaptive_check.setChecked(schema.adaptive)
        self.adaptive_threshold = QLineEdit(_float_text(schema.adaptive_threshold))
        self.denoise_check = QCheckBox("Denoise")
        self.denoise_check.setChecked(schema.denoise)
        self.lighting_preset_combo = QComboBox()
        self.lighting_preset_combo.addItems(schema.lighting_preset_choices)
        lighting_idx = self.lighting_preset_combo.findText(schema.lighting_preset_default)
        if lighting_idx >= 0:
            self.lighting_preset_combo.setCurrentIndex(lighting_idx)

        self.ppm_denoised_only = QCheckBox("Only write denoised PPM")
        self.ppm_denoised_only.setChecked(schema.ppm_denoised_only)
        self.atmosphere_check = QCheckBox("Enable fog / atmosphere")
        self.atmosphere_check.setChecked(schema.atmosphere_enabled)
        self.atmosphere_density = QLineEdit(_float_text(schema.atmosphere_density))
        self.atmosphere_color = QLineEdit(schema.atmosphere_color)

        self.keep_frames = QCheckBox("Keep individual frame files")
        self.keep_frames.setChecked(schema.video_keep_frames_default)

        render_box = QGroupBox("Render")
        render_form = QFormLayout(render_box)
        render_form.addRow("Width", self.width_spin)
        render_form.addRow("Height", self.height_spin)
        render_form.addRow("SPP", self.spp_spin)
        render_form.addRow("Max depth", self.max_depth_spin)
        render_form.addRow(self.adaptive_check)
        render_form.addRow("Adaptive threshold", self.adaptive_threshold)
        render_form.addRow("Lighting preset", self.lighting_preset_combo)
        root.addWidget(render_box)

        output_box = QGroupBox("Output")
        output_form = QFormLayout(output_box)
        output_form.addRow(self.denoise_check)
        output_form.addRow(self.ppm_denoised_only)
        output_form.addRow(self.atmosphere_check)
        output_form.addRow("Fog density", self.atmosphere_density)
        output_form.addRow("Fog color R,G,B", self.atmosphere_color)
        if include_keep_frames:
            output_form.addRow(self.keep_frames)
        root.addWidget(output_box)

        if not allow_denoise:
            self.denoise_check.setChecked(False)
            self.denoise_check.setEnabled(False)
            self.ppm_denoised_only.setChecked(False)
            self.ppm_denoised_only.setEnabled(False)
            if denoise_disabled_reason:
                self.denoise_check.setToolTip(denoise_disabled_reason)
                self.ppm_denoised_only.setToolTip(denoise_disabled_reason)

        if auto_ppm_denoised_only_with_denoise:
            self.denoise_check.toggled.connect(self._on_denoise_toggled_force_denoised_only)
            if self.denoise_check.isChecked() and not self.ppm_denoised_only.isChecked():
                self.ppm_denoised_only.setChecked(True)

        scene_override_box = QGroupBox("Scene Layout Overrides")
        scene_override_form = QFormLayout(scene_override_box)

        self.scatter_spacing_check = QCheckBox("Scatter spacing")
        self.scatter_spacing_edit = QLineEdit(_float_text(schema.scatter_spacing))
        scene_override_form.addRow(self.scatter_spacing_check, self.scatter_spacing_edit)

        self.scatter_max_radius_check = QCheckBox("Scatter max radius")
        self.scatter_max_radius_edit = QLineEdit(_float_text(schema.scatter_max_radius))
        scene_override_form.addRow(self.scatter_max_radius_check, self.scatter_max_radius_edit)

        self.scatter_growth_scale_check = QCheckBox("Scatter growth scale")
        self.scatter_growth_scale_edit = QLineEdit(_float_text(schema.scatter_growth_scale))
        scene_override_form.addRow(self.scatter_growth_scale_check, self.scatter_growth_scale_edit)

        self.big_scatter_spacing_check = QCheckBox("Big scatter spacing")
        self.big_scatter_spacing_edit = QLineEdit(_float_text(schema.big_scatter_spacing))
        scene_override_form.addRow(self.big_scatter_spacing_check, self.big_scatter_spacing_edit)

        self.big_scatter_max_radius_check = QCheckBox("Big scatter max radius")
        self.big_scatter_max_radius_edit = QLineEdit(_float_text(schema.big_scatter_max_radius))
        scene_override_form.addRow(self.big_scatter_max_radius_check, self.big_scatter_max_radius_edit)

        self.row_scatter_xmax_check = QCheckBox("Row scatter x max")
        self.row_scatter_xmax_edit = QLineEdit(_float_text(schema.row_scatter_xmax))
        scene_override_form.addRow(self.row_scatter_xmax_check, self.row_scatter_xmax_edit)

        self.row_scatter_z_front_check = QCheckBox("Row scatter z front")
        self.row_scatter_z_front_edit = QLineEdit(_float_text(schema.row_scatter_z_front))
        scene_override_form.addRow(self.row_scatter_z_front_check, self.row_scatter_z_front_edit)

        self.row_scatter_z_back_check = QCheckBox("Row scatter z back")
        self.row_scatter_z_back_edit = QLineEdit(_float_text(schema.row_scatter_z_back))
        scene_override_form.addRow(self.row_scatter_z_back_check, self.row_scatter_z_back_edit)

        self.row_scatter_replacement_rate_check = QCheckBox("Row scatter replacement rate")
        self.row_scatter_replacement_rate_edit = QLineEdit(_float_text(schema.row_scatter_replacement_rate))
        scene_override_form.addRow(self.row_scatter_replacement_rate_check, self.row_scatter_replacement_rate_edit)

        self._bind_override_pair(self.scatter_spacing_check, self.scatter_spacing_edit)
        self._bind_override_pair(self.scatter_max_radius_check, self.scatter_max_radius_edit)
        self._bind_override_pair(self.scatter_growth_scale_check, self.scatter_growth_scale_edit)
        self._bind_override_pair(self.big_scatter_spacing_check, self.big_scatter_spacing_edit)
        self._bind_override_pair(self.big_scatter_max_radius_check, self.big_scatter_max_radius_edit)
        self._bind_override_pair(self.row_scatter_xmax_check, self.row_scatter_xmax_edit)
        self._bind_override_pair(self.row_scatter_z_front_check, self.row_scatter_z_front_edit)
        self._bind_override_pair(self.row_scatter_z_back_check, self.row_scatter_z_back_edit)
        self._bind_override_pair(
            self.row_scatter_replacement_rate_check,
            self.row_scatter_replacement_rate_edit,
        )

        root.addWidget(scene_override_box)

        close_row = QHBoxLayout()
        close_row.addStretch(1)
        close_btn = QPushButton("Close")
        style_button(close_btn, primary=False)
        close_btn.clicked.connect(self.accept)
        close_row.addWidget(close_btn)
        root.addLayout(close_row)

        if self._on_change is not None:
            self._wire_change_signals(include_keep_frames=include_keep_frames)

        # Compatibility aliases so existing callers can use either naming scheme.
        self.width = self.width_spin
        self.height = self.height_spin
        self.spp = self.spp_spin
        self.max_depth = self.max_depth_spin
        self.adaptive = self.adaptive_check
        self.denoise = self.denoise_check
        self.lighting_preset = self.lighting_preset_combo

    def _on_denoise_toggled_force_denoised_only(self, checked: bool) -> None:
        if checked and not self.ppm_denoised_only.isChecked():
            self.ppm_denoised_only.setChecked(True)

    def _wire_change_signals(self, *, include_keep_frames: bool) -> None:
        assert self._on_change is not None
        watched = [
            self.width_spin,
            self.height_spin,
            self.spp_spin,
            self.max_depth_spin,
            self.adaptive_check,
            self.adaptive_threshold,
            self.denoise_check,
            self.lighting_preset_combo,
            self.ppm_denoised_only,
            self.atmosphere_check,
            self.atmosphere_density,
            self.atmosphere_color,
            self.scatter_spacing_check,
            self.scatter_spacing_edit,
            self.scatter_max_radius_check,
            self.scatter_max_radius_edit,
            self.scatter_growth_scale_check,
            self.scatter_growth_scale_edit,
            self.big_scatter_spacing_check,
            self.big_scatter_spacing_edit,
            self.big_scatter_max_radius_check,
            self.big_scatter_max_radius_edit,
            self.row_scatter_xmax_check,
            self.row_scatter_xmax_edit,
            self.row_scatter_z_front_check,
            self.row_scatter_z_front_edit,
            self.row_scatter_z_back_check,
            self.row_scatter_z_back_edit,
            self.row_scatter_replacement_rate_check,
            self.row_scatter_replacement_rate_edit,
        ]
        if include_keep_frames:
            watched.append(self.keep_frames)

        for w in watched:
            if hasattr(w, "valueChanged"):
                w.valueChanged.connect(self._on_change)
            if hasattr(w, "currentIndexChanged"):
                w.currentIndexChanged.connect(self._on_change)
            if hasattr(w, "toggled"):
                w.toggled.connect(self._on_change)
            if hasattr(w, "stateChanged"):
                w.stateChanged.connect(self._on_change)
            if hasattr(w, "editingFinished"):
                w.editingFinished.connect(self._on_change)

    def _bind_override_pair(self, check: QCheckBox, edit: QLineEdit) -> None:
        edit.setEnabled(False)
        check.setChecked(False)
        check.toggled.connect(lambda checked, c=check, e=edit: e.setEnabled(c.isEnabled() and checked))
