#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QApplication,
    QComboBox,
    QFormLayout,
    QFileDialog,
    QFrame,
    QGroupBox,
    QHBoxLayout,
    QInputDialog,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QProgressBar,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from button_styles import style_button
from gui_constants import (
    GROUPBOX_STYLE,
    LEFT_PANEL_MAX_WIDTH_VIDEO,
    LEFT_PANEL_SPACING,
    MAIN_LAYOUT_MARGIN,
    OVERLAY_BUTTON_MIN_HEIGHT,
    OVERLAY_MESSAGE_STYLE,
    OVERLAY_PROGRESS_WIDTH,
    OVERLAY_STYLE,
    OVERLAY_TITLE_STYLE,
    PRESET_PLACEHOLDER_TEXT,
    RIGHT_PANEL_SPACING,
    SECTION_PADDING_SPACING,
    STATUS_IDLE,
    STATUS_RENDERING,
    TOTAL_FRAMES_PLACEHOLDER,
    VIDEO_RENDERER_TITLE,
    VIDEO_WINDOW_SIZE,
)
from gui_runtime import outputs_dir, project_root_path, raytracer_binary_path
from options_schema import GuiOptionsSchema, load_gui_options_schema
from render_settings_dialog import RenderSettingsDialog
from scene_controls import (
    populate_scene_combo,
    populate_variant_combo,
    selected_scene_name,
    update_palette_enabled,
    update_scene_override_controls,
)
from scene_registry import SceneRegistry, load_scene_registry, to_scene_variant_map
from settings_summary import build_settings_summary
from video_config_builder import RenderTaskConfig, build_render_task_config
from video_preset_manager import VideoPresetManager
from video_widgets import DefinitionSection, TimedVectorSection
from video_worker import RenderWorker


class AnimationFormulaGui(QMainWindow):
    def __init__(self, project_root: Path) -> None:
        super().__init__()
        self.project_root = project_root
        self.raytracer_bin = raytracer_binary_path(project_root, Path(__file__))
        self.output_dir = outputs_dir(project_root)
        self.options_schema = load_gui_options_schema(self.raytracer_bin)
        self.scene_registry: SceneRegistry = load_scene_registry(self.raytracer_bin)
        self.scene_variant_map: Dict[str, List[Tuple[str, str]]] = to_scene_variant_map(self.scene_registry)
        self.preset_manager = VideoPresetManager(project_root, self.options_schema)

        self.settings_dialog = RenderSettingsDialog(
            self,
            self.options_schema,
            self._refresh_settings_summary,
            include_keep_frames=True,
            auto_ppm_denoised_only_with_denoise=True,
        )
        self.worker: Optional[RenderWorker] = None
        self.last_video_path: Optional[str] = None
        self.current_preset_path: Optional[Path] = None
        self.current_preset_name: Optional[str] = None
        self._abort_requested = False

        self.setWindowTitle(VIDEO_RENDERER_TITLE)
        self.resize(*VIDEO_WINDOW_SIZE)
        self.setStyleSheet(GROUPBOX_STYLE)
        self._build_ui()
        self._populate_scene_controls()
        self._on_scene_changed()

    def _build_ui(self) -> None:
        root = QWidget()
        self.setCentralWidget(root)
        layout = QHBoxLayout(root)
        layout.setContentsMargins(MAIN_LAYOUT_MARGIN, MAIN_LAYOUT_MARGIN, MAIN_LAYOUT_MARGIN, MAIN_LAYOUT_MARGIN)

        left = QVBoxLayout()
        left.setSpacing(LEFT_PANEL_SPACING)

        scene_box = QGroupBox("Scene")
        scene_form = QFormLayout(scene_box)
        self.scene_combo = QComboBox()
        self.scene_combo.currentIndexChanged.connect(self._on_scene_changed)
        self.variant_combo = QComboBox()
        self.variant_combo.currentIndexChanged.connect(self._on_variant_changed)
        self.palette_combo = QComboBox()
        self.palette_combo.addItems(self.options_schema.big_scatter_palette_choices)
        palette_idx = self.palette_combo.findText(self.options_schema.big_scatter_palette_default)
        if palette_idx >= 0:
            self.palette_combo.setCurrentIndex(palette_idx)
        scene_form.addRow("Scene", self.scene_combo)
        scene_form.addRow("Variant", self.variant_combo)
        scene_form.addRow("Color Palette", self.palette_combo)
        left.addWidget(scene_box)
        left.addSpacing(SECTION_PADDING_SPACING)

        video_box = QGroupBox("Video")
        video_form = QFormLayout(video_box)
        self.fps_combo = QComboBox()
        self.fps_combo.addItems([str(v) for v in self.options_schema.video_fps_choices])
        if self.options_schema.video_fps_choices:
            self.fps_combo.setCurrentText(str(self.options_schema.video_fps_choices[-1]))
        self.total_frames = QLineEdit("")
        self.total_frames.setPlaceholderText(TOTAL_FRAMES_PLACEHOLDER)
        video_form.addRow("FPS", self.fps_combo)
        video_form.addRow("Total number of frames", self.total_frames)
        left.addWidget(video_box)

        self.preset_label = QLabel(PRESET_PLACEHOLDER_TEXT)
        self.preset_label.setWordWrap(True)
        left.addWidget(self.preset_label)

        self.settings_summary = QLabel("")
        self.settings_summary.setWordWrap(True)
        left.addWidget(self.settings_summary)

        btn_row = QHBoxLayout()
        self.settings_btn = QPushButton("Settings...")
        style_button(self.settings_btn, primary=False)
        self.settings_btn.clicked.connect(self.settings_dialog.exec)
        self.render_btn = QPushButton("Render")
        style_button(self.render_btn, primary=True)
        self.render_btn.clicked.connect(self._on_render_clicked)
        self.resume_btn = QPushButton("Resume")
        style_button(self.resume_btn, primary=True)
        self.resume_btn.setVisible(False)
        self.resume_btn.clicked.connect(self._on_resume_clicked)
        btn_row.addWidget(self.settings_btn)
        btn_row.addWidget(self.render_btn)
        btn_row.addWidget(self.resume_btn)
        left.addLayout(btn_row)

        self.status = QLabel(STATUS_IDLE)
        self.status.setWordWrap(True)
        left.addWidget(self.status)
        left.addStretch(1)

        left_panel = QFrame()
        left_panel.setLayout(left)
        left_panel.setMaximumWidth(LEFT_PANEL_MAX_WIDTH_VIDEO)
        layout.addWidget(left_panel, 0)

        self.right_scroll = QScrollArea()
        self.right_scroll.setWidgetResizable(True)
        self.right_body = QWidget()
        right_layout = QVBoxLayout(self.right_body)
        right_layout.setSpacing(RIGHT_PANEL_SPACING)

        self.definitions = DefinitionSection(max_rows=10)
        right_layout.addWidget(self.definitions)
        right_layout.addSpacing(SECTION_PADDING_SPACING)

        self.camera_location = TimedVectorSection("Camera Location", ["x", "y", "z"], max_rows=10)
        right_layout.addWidget(self.camera_location)
        right_layout.addSpacing(SECTION_PADDING_SPACING)

        self.camera_velocity = TimedVectorSection("Camera Velocity (scene frame)", ["x", "y", "z"], max_rows=10)
        right_layout.addWidget(self.camera_velocity)
        right_layout.addSpacing(SECTION_PADDING_SPACING)

        self.camera_orientation = TimedVectorSection("Camera Orientation (turns)", ["pitch", "yaw"], max_rows=10)
        right_layout.addWidget(self.camera_orientation)

        preset_row = QHBoxLayout()
        self.save_preset_btn = QPushButton("Save Preset")
        style_button(self.save_preset_btn, primary=False)
        self.save_preset_btn.clicked.connect(self._on_save_preset_clicked)
        self.save_preset_as_btn = QPushButton("Save Preset As...")
        style_button(self.save_preset_as_btn, primary=False)
        self.save_preset_as_btn.clicked.connect(self._on_save_preset_as_clicked)
        self.load_preset_btn = QPushButton("Load Preset")
        style_button(self.load_preset_btn, primary=False)
        self.load_preset_btn.clicked.connect(self._on_load_preset_clicked)
        preset_row.addWidget(self.save_preset_btn)
        preset_row.addWidget(self.save_preset_as_btn)
        preset_row.addWidget(self.load_preset_btn)
        preset_row.addStretch(1)
        right_layout.addLayout(preset_row)

        right_layout.addStretch(1)
        self.right_scroll.setWidget(self.right_body)
        layout.addWidget(self.right_scroll, 1)

        self.overlay = QWidget(root)
        self.overlay.setStyleSheet(OVERLAY_STYLE)
        self.overlay.hide()
        ov = QVBoxLayout(self.overlay)
        ov.setAlignment(Qt.AlignmentFlag.AlignCenter)

        self.overlay_title = QLabel(STATUS_RENDERING)
        self.overlay_title.setStyleSheet(OVERLAY_TITLE_STYLE)
        ov.addWidget(self.overlay_title, alignment=Qt.AlignmentFlag.AlignHCenter)

        self.progress = QProgressBar()
        self.progress.setFixedWidth(OVERLAY_PROGRESS_WIDTH)
        self.progress.setRange(0, 100)
        ov.addWidget(self.progress, alignment=Qt.AlignmentFlag.AlignHCenter)

        self.overlay_msg = QLabel("")
        self.overlay_msg.setStyleSheet(OVERLAY_MESSAGE_STYLE)
        ov.addWidget(self.overlay_msg, alignment=Qt.AlignmentFlag.AlignHCenter)

        self.overlay_btn_row = QHBoxLayout()
        self.abort_btn = QPushButton("Abort")
        self.pause_btn = QPushButton("Pause")
        self.return_btn = QPushButton("Return")
        overlay_btn_style = (
            "QPushButton {"
            "background-color: #bfbfbf;"
            "border: 1px solid #6f6f6f;"
            "border-top-color: #e7e7e7;"
            "border-bottom-color: #585858;"
            "border-radius: 6px;"
            "padding: 6px 14px;"
            "color: #111111;"
            "}"
            "QPushButton:hover { background-color: #c9c9c9; }"
            "QPushButton:pressed {"
            "background-color: #a9a9a9;"
            "border-top-color: #666666;"
            "border-bottom-color: #dddddd;"
            "}"
            "QPushButton:disabled {"
            "background-color: #8f8f8f;"
            "color: #444444;"
            "}"
        )
        for btn in [self.abort_btn, self.pause_btn, self.return_btn]:
            btn.setStyleSheet(overlay_btn_style)
            btn.setMinimumHeight(OVERLAY_BUTTON_MIN_HEIGHT)
        self.return_btn.hide()
        self.abort_btn.clicked.connect(self._on_abort_clicked)
        self.pause_btn.clicked.connect(self._on_pause_clicked)
        self.return_btn.clicked.connect(self._on_return_clicked)
        self.overlay_btn_row.addWidget(self.abort_btn)
        self.overlay_btn_row.addWidget(self.pause_btn)
        self.overlay_btn_row.addWidget(self.return_btn)
        ov.addLayout(self.overlay_btn_row)

        self._wire_settings_summary_updates()
        self._refresh_settings_summary()

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        if self.centralWidget() is not None:
            self.overlay.setGeometry(self.centralWidget().rect())

    def _populate_scene_controls(self) -> None:
        populate_scene_combo(self.scene_combo, self.scene_registry)

    def _on_scene_changed(self) -> None:
        populate_variant_combo(self.scene_combo, self.variant_combo, self.scene_variant_map)
        self._on_variant_changed()
        self._refresh_settings_summary()

    def _on_variant_changed(self) -> None:
        selected = selected_scene_name(self.scene_combo, self.variant_combo)
        update_palette_enabled(self.palette_combo, selected)
        update_scene_override_controls(self.settings_dialog, selected)
        self._refresh_settings_summary()

    def _wire_settings_summary_updates(self) -> None:
        for w in [self.scene_combo, self.variant_combo, self.palette_combo, self.fps_combo, self.total_frames]:
            if hasattr(w, "valueChanged"):
                w.valueChanged.connect(self._refresh_settings_summary)
            if hasattr(w, "currentIndexChanged"):
                w.currentIndexChanged.connect(self._refresh_settings_summary)
            if hasattr(w, "textChanged"):
                w.textChanged.connect(self._refresh_settings_summary)
            if hasattr(w, "stateChanged"):
                w.stateChanged.connect(self._refresh_settings_summary)

    def _refresh_settings_summary(self) -> None:
        self.settings_summary.setText(
            build_settings_summary(self.settings_dialog, include_keep_frames=True, include_lighting=True)
        )

    def _preset_dir(self) -> Path:
        return self.preset_manager.preset_dir()

    def _update_preset_label(self) -> None:
        name = (self.current_preset_name or "").strip()
        self.preset_label.setText(f"Preset: {name}" if name else PRESET_PLACEHOLDER_TEXT)

    def _set_current_preset(self, path: Path, name: str) -> None:
        self.current_preset_path = path
        self.current_preset_name = (name or path.stem).strip()
        self._update_preset_label()

    def _set_definition_row_count(self, target: int) -> None:
        target = max(1, min(10, target))
        while len(self.definitions.rows) < target:
            self.definitions.add_row()
        while len(self.definitions.rows) > target:
            self.definitions.remove_last_row()

    def _set_section_row_count(self, section: TimedVectorSection, target: int) -> None:
        target = max(1, min(10, target))
        while len(section.rows) < target:
            section.add_row()
        while len(section.rows) > target:
            section.remove_last_row()

    def _serialize_section(self, section: TimedVectorSection) -> Dict[str, object]:
        return {
            "rows": [
                {"fields": {k: e.text() for k, e in row.fields.items()}, "range_text": row.range_block.current_text()}
                for row in section.rows
            ]
        }

    def _apply_section_data(self, section: TimedVectorSection, data: Dict[str, object]) -> None:
        rows_data = data.get("rows", [])
        if not isinstance(rows_data, list) or not rows_data:
            return
        self._set_section_row_count(section, len(rows_data))
        for i, row_data in enumerate(rows_data):
            if not isinstance(row_data, dict):
                continue
            row_widget = section.rows[i]
            fields = row_data.get("fields", {})
            if isinstance(fields, dict):
                for key, edit in row_widget.fields.items():
                    edit.setText(str(fields.get(key, "")))
            row_widget.range_block.apply_value(str(row_data.get("range_text", "")))
        section._refresh_ranges()

    def _collect_preset_data(self) -> Dict[str, object]:
        return self.preset_manager.collect_preset_data(self)

    def _migrate_preset_data(self, data: Dict[str, object]) -> Dict[str, object]:
        return self.preset_manager.migrate_preset_data(data)

    def _apply_preset_data(self, data: Dict[str, object]) -> None:
        self.preset_manager.apply_preset_data(self, data)

    def _on_save_preset_clicked(self) -> None:
        if self.current_preset_path is None:
            self._on_save_preset_as_clicked()
            return
        preset_name = (self.current_preset_name or self.current_preset_path.stem).strip()
        self._write_preset_file(self.current_preset_path, preset_name)
        self.status.setText(f"Saved preset: {self.current_preset_path}")

    def _on_save_preset_as_clicked(self) -> None:
        name, ok = QInputDialog.getText(self, "Save Preset", "Preset name:")
        if not ok:
            return
        name = name.strip()
        if not name:
            QMessageBox.warning(self, "Save Preset", "Preset name cannot be empty.")
            return
        safe_name = re.sub(r"[^A-Za-z0-9._-]+", "_", name).strip("_")
        if not safe_name:
            QMessageBox.warning(self, "Save Preset", "Preset name is invalid after sanitization.")
            return
        path = self._preset_dir() / f"{safe_name}.json"
        if path.exists():
            overwrite = QMessageBox.question(
                self,
                "Overwrite Preset",
                f"Preset '{safe_name}' already exists. Overwrite?",
                QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            )
            if overwrite != QMessageBox.StandardButton.Yes:
                return
        self._write_preset_file(path, name)
        self._set_current_preset(path, name)
        self.status.setText(f"Saved preset: {path}")

    def _write_preset_file(self, path: Path, preset_name: str) -> None:
        self.preset_manager.write_preset_file(path, preset_name, self)

    def _on_load_preset_clicked(self) -> None:
        path_str, _ = QFileDialog.getOpenFileName(self, "Load Preset", str(self._preset_dir()), "Preset JSON (*.json)")
        if not path_str:
            return
        path = Path(path_str)
        try:
            migrated = self.preset_manager.read_preset_file(path)
            self._apply_preset_data(migrated)
            preset_name = str(migrated.get("preset_name", "")).strip() or path.stem
            self._set_current_preset(path, preset_name)
            self.status.setText(f"Loaded preset: {path}")
        except Exception as exc:
            QMessageBox.critical(self, "Load Preset Failed", str(exc))

    def _show_input_error(self, text: str) -> None:
        QMessageBox.critical(self, "Could not begin rendering", text)

    def _build_config(self) -> Optional[RenderTaskConfig]:
        try:
            return build_render_task_config(
                scene_name=str(self.variant_combo.currentData() or self.scene_combo.currentData() or self.scene_combo.currentText()),
                palette=self.palette_combo.currentText(),
                fps_text=self.fps_combo.currentText(),
                total_frames_expr=self.total_frames.text(),
                definitions=self.definitions.get_definitions(),
                location_rows=self.camera_location.get_rows(),
                velocity_rows=self.camera_velocity.get_rows(),
                orientation_rows=self.camera_orientation.get_rows(),
                settings_dialog=self.settings_dialog,
                options_schema=self.options_schema,
                scene_registry=self.scene_registry,
                raytracer_bin=self.raytracer_bin,
                output_dir=self.output_dir,
            )
        except Exception as exc:
            self._show_input_error(str(exc))
            return None

    def _enter_render_mode(self) -> None:
        self.overlay_title.setText(STATUS_RENDERING)
        self.overlay_msg.setText("")
        self.progress.setValue(0)
        self.abort_btn.show()
        self.pause_btn.show()
        self.return_btn.hide()
        self.overlay.show()
        self.right_body.setEnabled(False)
        self.scene_combo.setEnabled(False)
        self.variant_combo.setEnabled(False)
        self.palette_combo.setEnabled(False)
        self.fps_combo.setEnabled(False)
        self.total_frames.setEnabled(False)
        self.settings_btn.setEnabled(False)
        self.render_btn.setVisible(False)
        self.resume_btn.setVisible(False)

    def _leave_render_mode(self) -> None:
        self.overlay.hide()
        self.right_body.setEnabled(True)
        self.scene_combo.setEnabled(True)
        self.variant_combo.setEnabled(True)
        self.palette_combo.setEnabled(True)
        self.fps_combo.setEnabled(True)
        self.total_frames.setEnabled(True)
        self.settings_btn.setEnabled(True)
        self.render_btn.setVisible(True)
        self.resume_btn.setVisible(False)

    def _on_render_clicked(self) -> None:
        if self.worker is not None:
            return
        cfg = self._build_config()
        if cfg is None:
            return
        if not cfg.raytracer_bin.exists():
            self._show_input_error(f"Raytracer binary not found: {cfg.raytracer_bin}")
            return

        self.worker = RenderWorker(cfg)
        self.worker.progress.connect(self._on_worker_progress)
        self.worker.failed.connect(self._on_worker_failed)
        self.worker.done.connect(self._on_worker_done)
        self.worker.aborted.connect(self._on_worker_aborted)
        self.worker.finished.connect(self._on_worker_finished)
        self.worker.start()

        self._abort_requested = False
        self.status.setText("Rendering started...")
        self._enter_render_mode()

    def _on_worker_progress(self, done: int, total: int, msg: str) -> None:
        pct = int(100.0 * done / max(1, total))
        self.progress.setValue(pct)
        self.overlay_msg.setText(msg)
        self.status.setText(msg)

    def _on_worker_failed(self, err: str) -> None:
        if self._abort_requested:
            self.status.setText("Render aborted.")
            return
        self.status.setText("Render failed.")
        self._leave_render_mode()
        QMessageBox.critical(self, "Could not complete render", err)

    def _on_worker_done(self, video_path: str) -> None:
        if self._abort_requested:
            self.status.setText("Render aborted.")
            return
        self.last_video_path = video_path
        self.status.setText(f"Successfully rendered: {video_path}")
        self.overlay_title.setText("Successfully rendered")
        self.overlay_msg.setText(video_path)
        self.abort_btn.hide()
        self.pause_btn.hide()
        self.return_btn.show()
        self.progress.setValue(100)

    def _on_worker_aborted(self, video_path: str) -> None:
        if video_path:
            self.last_video_path = video_path
            self.status.setText(f"Render aborted. Partial video saved: {video_path}")
        else:
            self.status.setText("Render aborted.")

    def _on_worker_finished(self) -> None:
        self.worker = None
        if self._abort_requested:
            self._abort_requested = False

    def _on_return_clicked(self) -> None:
        self._leave_render_mode()

    def _on_pause_clicked(self) -> None:
        if self.worker is None:
            return
        self.worker.request_pause(True)
        self.overlay.hide()
        self.right_body.setEnabled(True)
        self.scene_combo.setEnabled(True)
        self.variant_combo.setEnabled(True)
        self.palette_combo.setEnabled(True)
        self.fps_combo.setEnabled(True)
        self.total_frames.setEnabled(True)
        self.settings_btn.setEnabled(True)
        self.render_btn.setVisible(False)
        self.resume_btn.setVisible(True)
        self.status.setText("Paused (will pause at frame boundary).")

    def _on_resume_clicked(self) -> None:
        if self.worker is None:
            return
        self.worker.request_pause(False)
        self._enter_render_mode()
        self.status.setText("Rendering resumed...")

    def _on_abort_clicked(self) -> None:
        if self.worker is None:
            return
        resp = QMessageBox.question(
            self,
            "Confirm Abort",
            "Abort current video render?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        )
        if resp != QMessageBox.StandardButton.Yes:
            return
        self.worker.request_abort()
        self._abort_requested = True
        self.status.setText("Aborting render...")
        self._leave_render_mode()


def main() -> int:
    project_root = project_root_path(Path(__file__))
    app = QApplication(sys.argv)
    win = AnimationFormulaGui(project_root)
    win.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
