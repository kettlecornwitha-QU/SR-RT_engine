#!/usr/bin/env python3
import re
import sys
import time
from pathlib import Path
from typing import Dict, List, Tuple

from PySide6.QtCore import QProcess, Qt
from PySide6.QtGui import QPixmap
from PySide6.QtWidgets import (
    QApplication,
    QComboBox,
    QFormLayout,
    QFrame,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMainWindow,
    QMessageBox,
    QPushButton,
    QRadioButton,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)
from button_styles import style_button
from gui_constants import (
    IMAGE_PLACEHOLDER_TEXT,
    IMAGE_RENDERER_TITLE,
    IMAGE_WINDOW_SIZE,
    LEFT_PANEL_MAX_WIDTH_IMAGE,
    LEFT_PANEL_SPACING,
    STATUS_IDLE,
    STATUS_RENDERING,
)
from gui_runtime import (
    gui_temp_output_dir,
    new_image_output_base,
    packaged_runtime,
    pick_output_image,
    project_root_path,
    raytracer_binary_path,
)
from image_config_builder import build_image_cli_args, build_image_render_config
from oidn_postprocess import can_run_oidn_postprocess, run_oidn_postprocess
from options_schema import load_gui_options_schema
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


class RaytracerGui(QMainWindow):
    def __init__(self, project_root: Path) -> None:
        super().__init__()
        self.project_root = project_root
        self.raytracer_bin = raytracer_binary_path(project_root, Path(__file__))
        self.options_schema = load_gui_options_schema(self.raytracer_bin)
        self.scene_registry: SceneRegistry = load_scene_registry(self.raytracer_bin)
        self.process: QProcess | None = None
        self.render_start = 0.0
        self.latest_pixmap: QPixmap | None = None
        self.current_output_base: Path | None = None
        self.temp_output_dir = gui_temp_output_dir("sr_rt_gui")
        self.stdout_buffer = ""
        self.last_progress_text = ""
        self.pending_python_denoise = False
        self.scene_variant_map: Dict[str, List[Tuple[str, str]]] = to_scene_variant_map(self.scene_registry)

        self.setWindowTitle(IMAGE_RENDERER_TITLE)
        self.resize(*IMAGE_WINDOW_SIZE)
        self._build_ui()
        self._populate_scene_controls()
        self._on_scene_changed()

    def _build_ui(self) -> None:
        root = QWidget()
        self.setCentralWidget(root)
        layout = QHBoxLayout(root)

        controls = QVBoxLayout()
        controls.setSpacing(LEFT_PANEL_SPACING)

        scene_box = QGroupBox("Scene")
        scene_form = QFormLayout(scene_box)
        self.scene_combo = QComboBox()
        self.scene_combo.currentIndexChanged.connect(self._on_scene_changed)
        self.variant_combo = QComboBox()
        self.variant_combo.currentIndexChanged.connect(self._on_variant_changed)
        self.big_scatter_palette_combo = QComboBox()
        for p in self.options_schema.choices.big_scatter_palette:
            self.big_scatter_palette_combo.addItem(p)
        palette_idx = self.big_scatter_palette_combo.findText(self.options_schema.defaults.big_scatter_palette)
        if palette_idx >= 0:
            self.big_scatter_palette_combo.setCurrentIndex(palette_idx)
        scene_form.addRow("Scene", self.scene_combo)
        scene_form.addRow("Variant", self.variant_combo)
        scene_form.addRow("Big Scatter Palette", self.big_scatter_palette_combo)
        controls.addWidget(scene_box)

        vel_box = QGroupBox("Player Velocity")
        vel_layout = QGridLayout(vel_box)
        self.vel_mode_turns = QRadioButton("Speed + Yaw + Pitch")
        self.vel_mode_vec = QRadioButton("Vector (vx, vy, vz)")
        self.vel_mode_turns.setChecked(True)
        self.vel_mode_turns.toggled.connect(self._on_velocity_mode_changed)
        self.vel_mode_vec.toggled.connect(self._on_velocity_mode_changed)
        vel_layout.addWidget(self.vel_mode_turns, 0, 0, 1, 2)
        vel_layout.addWidget(self.vel_mode_vec, 1, 0, 1, 2)

        self.ab_speed = QLineEdit("0.0")
        self.ab_yaw = QLineEdit("0.0")
        self.ab_pitch = QLineEdit("0.0")
        self.vx = QLineEdit("0.0")
        self.vy = QLineEdit("0.0")
        self.vz = QLineEdit("0.0")
        vel_layout.addWidget(QLabel("Speed"), 2, 0)
        vel_layout.addWidget(self.ab_speed, 2, 1)
        vel_layout.addWidget(QLabel("Yaw turns"), 3, 0)
        vel_layout.addWidget(self.ab_yaw, 3, 1)
        vel_layout.addWidget(QLabel("Pitch turns"), 4, 0)
        vel_layout.addWidget(self.ab_pitch, 4, 1)
        vel_layout.addWidget(QLabel("vx"), 5, 0)
        vel_layout.addWidget(self.vx, 5, 1)
        vel_layout.addWidget(QLabel("vy"), 6, 0)
        vel_layout.addWidget(self.vy, 6, 1)
        vel_layout.addWidget(QLabel("vz"), 7, 0)
        vel_layout.addWidget(self.vz, 7, 1)
        controls.addWidget(vel_box)

        cam_box = QGroupBox("Camera View")
        cam_form = QFormLayout(cam_box)
        self.cam_x = QLineEdit("")
        self.cam_y = QLineEdit("")
        self.cam_z = QLineEdit("")
        self.cam_yaw = QLineEdit("0.0")
        self.cam_pitch = QLineEdit("0.0")
        self.cam_x.setPlaceholderText("scene default")
        self.cam_y.setPlaceholderText("scene default")
        self.cam_z.setPlaceholderText("scene default")
        cam_form.addRow("X", self.cam_x)
        cam_form.addRow("Y", self.cam_y)
        cam_form.addRow("Z", self.cam_z)
        cam_form.addRow("Yaw turns", self.cam_yaw)
        cam_form.addRow("Pitch turns", self.cam_pitch)
        controls.addWidget(cam_box)

        self.settings_dialog = RenderSettingsDialog(
            self,
            self.options_schema,
            self._on_settings_changed,
        )
        self.settings_summary = QLabel("")
        self.settings_summary.setWordWrap(True)
        controls.addWidget(self.settings_summary)

        button_row = QHBoxLayout()
        self.settings_button = QPushButton("Settings...")
        style_button(self.settings_button, primary=False)
        self.settings_button.clicked.connect(self._on_settings_clicked)
        self.render_button = QPushButton("Render")
        style_button(self.render_button, primary=True)
        self.render_button.clicked.connect(self._on_render_clicked)
        button_row.addWidget(self.settings_button)
        button_row.addWidget(self.render_button)
        controls.addLayout(button_row)

        self.status_label = QLabel(STATUS_IDLE)
        self.status_label.setWordWrap(True)
        controls.addWidget(self.status_label)
        controls.addStretch(1)

        left_panel = QFrame()
        left_panel.setLayout(controls)
        left_panel.setMaximumWidth(LEFT_PANEL_MAX_WIDTH_IMAGE)
        layout.addWidget(left_panel, 0)

        self.image_label = QLabel(IMAGE_PLACEHOLDER_TEXT)
        self.image_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.image_label.setMinimumSize(1, 1)
        self.image_scroll = QScrollArea()
        self.image_scroll.setWidgetResizable(True)
        self.image_scroll.setWidget(self.image_label)
        layout.addWidget(self.image_scroll, 1)

        self._on_velocity_mode_changed()
        self._refresh_settings_summary()

    def _populate_scene_controls(self) -> None:
        populate_scene_combo(self.scene_combo, self.scene_registry)

    def _on_scene_changed(self) -> None:
        populate_variant_combo(self.scene_combo, self.variant_combo, self.scene_variant_map)
        self._on_variant_changed()

    def _on_variant_changed(self) -> None:
        scene_name = selected_scene_name(self.scene_combo, self.variant_combo)
        update_palette_enabled(self.big_scatter_palette_combo, scene_name)
        update_scene_override_controls(self.settings_dialog, scene_name)
        self._refresh_settings_summary()

    def _on_velocity_mode_changed(self) -> None:
        turns_mode = self.vel_mode_turns.isChecked()
        for w in [self.ab_speed, self.ab_yaw, self.ab_pitch]:
            w.setEnabled(turns_mode)
        for w in [self.vx, self.vy, self.vz]:
            w.setEnabled(not turns_mode)

    def _on_settings_clicked(self) -> None:
        self.settings_dialog.exec()

    def _on_settings_changed(self) -> None:
        self._refresh_settings_summary()

    def _refresh_settings_summary(self) -> None:
        self.settings_summary.setText(build_settings_summary(self.settings_dialog))

    def _build_config(self):
        scene = self.variant_combo.currentData() or str(self.scene_combo.currentData() or self.scene_combo.currentText())
        config = build_image_render_config(
            scene_name=str(scene),
            big_scatter_palette=self.big_scatter_palette_combo.currentText(),
            turns_mode=self.vel_mode_turns.isChecked(),
            ab_speed=self.ab_speed,
            ab_yaw=self.ab_yaw,
            ab_pitch=self.ab_pitch,
            vx=self.vx,
            vy=self.vy,
            vz=self.vz,
            cam_yaw=self.cam_yaw,
            cam_pitch=self.cam_pitch,
            cam_x=self.cam_x,
            cam_y=self.cam_y,
            cam_z=self.cam_z,
            settings_dialog=self.settings_dialog,
            options_schema=self.options_schema,
        )
        if packaged_runtime() and config.denoise and can_run_oidn_postprocess():
            config.denoise = False
            config.ppm_denoised_only = False
            self.pending_python_denoise = True
        else:
            self.pending_python_denoise = False
        return config

    def _parse_progress(self, text: str) -> None:
        match = re.findall(r"Progress:\s*([0-9.]+)%.*ETA:\s*([0-9.]+)s", text)
        if match:
            pct, eta = match[-1]
            self.last_progress_text = f"Progress {pct}% | ETA {eta}s"
            self.status_label.setText(self.last_progress_text)

    def _display_image(self, image_path: Path) -> None:
        pix = QPixmap(str(image_path))
        if pix.isNull():
            self.status_label.setText(f"Render finished, but failed to load image: {image_path}")
            return
        self.latest_pixmap = pix
        self._refresh_display_pixmap()

    def _refresh_display_pixmap(self) -> None:
        if self.latest_pixmap is None:
            return

        viewport_size = self.image_scroll.viewport().size()
        if viewport_size.width() <= 1 or viewport_size.height() <= 1:
            self.image_label.setPixmap(self.latest_pixmap)
            return

        pix = self.latest_pixmap
        if pix.width() > viewport_size.width() or pix.height() > viewport_size.height():
            scaled = pix.scaled(
                viewport_size,
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation,
            )
            self.image_label.setPixmap(scaled)
        else:
            self.image_label.setPixmap(pix)
        self.image_label.adjustSize()

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        self._refresh_display_pixmap()

    def _on_render_clicked(self) -> None:
        if self.process and self.process.state() != QProcess.ProcessState.NotRunning:
            self.process.kill()
            return

        if not self.raytracer_bin.exists():
            QMessageBox.critical(self, "Missing Binary", f"Could not find: {self.raytracer_bin}")
            return

        self.current_output_base = new_image_output_base(self.temp_output_dir)

        config = self._build_config()
        args = build_image_cli_args(config, self.current_output_base)
        self.process = QProcess(self)
        self.process.setProgram(str(self.raytracer_bin))
        self.process.setArguments(args)
        self.process.setWorkingDirectory(str(self.project_root))
        self.process.readyReadStandardOutput.connect(self._on_process_output)
        self.process.readyReadStandardError.connect(self._on_process_output)
        self.process.finished.connect(self._on_process_finished)
        self.process.start()

        self.render_start = time.time()
        self.stdout_buffer = ""
        self.last_progress_text = ""
        self.status_label.setText(STATUS_RENDERING)
        self.render_button.setText("Cancel")

    def _on_process_output(self) -> None:
        if not self.process:
            return
        out = bytes(self.process.readAllStandardOutput()).decode("utf-8", errors="ignore")
        err = bytes(self.process.readAllStandardError()).decode("utf-8", errors="ignore")
        chunk = out + ("\n" + err if err else "")
        if chunk:
            self.stdout_buffer += chunk
            self._parse_progress(self.stdout_buffer[-4000:])

    def _on_process_finished(self, exit_code: int) -> None:
        elapsed = time.time() - self.render_start
        self.render_button.setText("Render")
        if exit_code != 0:
            self.status_label.setText(f"Render failed (exit {exit_code})")
            QMessageBox.warning(self, "Render Failed", self.stdout_buffer[-2000:])
            return

        if not self.current_output_base:
            self.status_label.setText("Render finished, but output path unknown.")
            return

        image_path = pick_output_image(self.current_output_base)
        if self.pending_python_denoise:
            ok, denoise_msg, denoised_path = run_oidn_postprocess(self.current_output_base)
            if ok and denoised_path is not None:
                image_path = denoised_path
            else:
                QMessageBox.warning(self, "Denoise Failed", denoise_msg)
        self._display_image(image_path)
        self.status_label.setText(f"Done in {elapsed:.2f}s | {image_path}")

def main() -> int:
    project_root = project_root_path(Path(__file__))
    app = QApplication(sys.argv)
    win = RaytracerGui(project_root)
    win.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
