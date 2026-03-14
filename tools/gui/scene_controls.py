#!/usr/bin/env python3
from __future__ import annotations

from PySide6.QtWidgets import QCheckBox, QComboBox, QLineEdit


def populate_scene_combo(scene_combo: QComboBox, scene_registry: object) -> None:
    scene_combo.blockSignals(True)
    scene_combo.clear()
    groups = list(scene_registry.groups)
    groups.sort(key=lambda g: (0 if g.key == "row_scatter" else 1))
    for group in groups:
        scene_combo.addItem(group.label, group.key)
    if scene_combo.count() > 0:
        scene_combo.setCurrentIndex(0)
    scene_combo.blockSignals(False)


def populate_variant_combo(scene_combo: QComboBox, variant_combo: QComboBox, scene_variant_map: dict) -> str:
    scene = str(scene_combo.currentData() or scene_combo.currentText())
    variants = scene_variant_map.get(scene, [("default", scene)])
    variant_combo.clear()
    for label, scene_name in variants:
        variant_combo.addItem(label, scene_name)
    variant_combo.setEnabled(len(variants) > 1)
    return scene


def selected_scene_name(scene_combo: QComboBox, variant_combo: QComboBox) -> str:
    return str(variant_combo.currentData() or scene_combo.currentData() or scene_combo.currentText())


def update_palette_enabled(palette_combo: QComboBox, scene_name: str) -> None:
    palette_combo.setEnabled(str(scene_name).startswith("big_scatter"))


def set_override_control_enabled(check: QCheckBox, edit: QLineEdit, enabled: bool) -> None:
    check.setEnabled(enabled)
    edit.setEnabled(enabled and check.isChecked())
    if not enabled:
        check.setChecked(False)


def update_scene_override_controls(settings_dialog: object, scene_name: str) -> None:
    is_scatter = scene_name.startswith("scatter")
    is_big_scatter = scene_name.startswith("big_scatter")
    is_row_scatter = scene_name == "row_scatter"

    set_override_control_enabled(settings_dialog.scatter_spacing_check, settings_dialog.scatter_spacing_edit, is_scatter)
    set_override_control_enabled(
        settings_dialog.scatter_max_radius_check,
        settings_dialog.scatter_max_radius_edit,
        is_scatter,
    )
    set_override_control_enabled(
        settings_dialog.scatter_growth_scale_check,
        settings_dialog.scatter_growth_scale_edit,
        is_scatter,
    )
    set_override_control_enabled(
        settings_dialog.big_scatter_spacing_check,
        settings_dialog.big_scatter_spacing_edit,
        is_big_scatter,
    )
    set_override_control_enabled(
        settings_dialog.big_scatter_max_radius_check,
        settings_dialog.big_scatter_max_radius_edit,
        is_big_scatter,
    )
    set_override_control_enabled(
        settings_dialog.row_scatter_xmax_check,
        settings_dialog.row_scatter_xmax_edit,
        is_row_scatter,
    )
    set_override_control_enabled(
        settings_dialog.row_scatter_z_front_check,
        settings_dialog.row_scatter_z_front_edit,
        is_row_scatter,
    )
    set_override_control_enabled(
        settings_dialog.row_scatter_z_back_check,
        settings_dialog.row_scatter_z_back_edit,
        is_row_scatter,
    )
    set_override_control_enabled(
        settings_dialog.row_scatter_replacement_rate_check,
        settings_dialog.row_scatter_replacement_rate_edit,
        is_row_scatter,
    )
