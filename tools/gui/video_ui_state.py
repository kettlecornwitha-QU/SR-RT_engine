#!/usr/bin/env python3
from __future__ import annotations


class VideoWindowStateController:
    def __init__(self, window: object) -> None:
        self.window = window

    def _set_main_controls_enabled(self, enabled: bool) -> None:
        self.window.right_body.setEnabled(enabled)
        self.window.scene_combo.setEnabled(enabled)
        self.window.variant_combo.setEnabled(enabled)
        self.window.palette_combo.setEnabled(enabled)
        self.window.fps_combo.setEnabled(enabled)
        self.window.total_frames.setEnabled(enabled)
        self.window.settings_btn.setEnabled(enabled)

    def show_rendering(self) -> None:
        self.window.overlay_title.setText("Rendering...")
        self.window.overlay_msg.setText("")
        self.window.progress.setValue(0)
        self.window.abort_btn.show()
        self.window.pause_btn.show()
        self.window.return_btn.hide()
        self.window.overlay.show()
        self._set_main_controls_enabled(False)
        self.window.render_btn.setVisible(False)
        self.window.resume_btn.setVisible(False)

    def show_paused(self) -> None:
        self.window.overlay.hide()
        self._set_main_controls_enabled(True)
        self.window.render_btn.setVisible(False)
        self.window.resume_btn.setVisible(True)

    def show_idle(self) -> None:
        self.window.overlay.hide()
        self._set_main_controls_enabled(True)
        self.window.render_btn.setVisible(True)
        self.window.resume_btn.setVisible(False)

    def show_success(self, video_path: str) -> None:
        self.window.overlay_title.setText("Successfully rendered")
        self.window.overlay_msg.setText(video_path)
        self.window.abort_btn.hide()
        self.window.pause_btn.hide()
        self.window.return_btn.show()
        self.window.progress.setValue(100)

    def show_failure(self) -> None:
        self.show_idle()

    def show_abort_result(self) -> None:
        self.show_idle()
