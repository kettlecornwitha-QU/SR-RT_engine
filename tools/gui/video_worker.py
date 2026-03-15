#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import threading
from typing import Optional

from PySide6.QtCore import QThread, Signal

from video_config_builder import RenderTaskConfig
from video_job_runner import VideoJobRunner


class RenderWorker(QThread):
    progress = Signal(int, int, str)
    failed = Signal(str)
    paused = Signal(bool)
    done = Signal(str)
    aborted = Signal(str)

    def __init__(self, cfg: RenderTaskConfig) -> None:
        super().__init__()
        self.cfg = cfg
        self._abort = False
        self._pause = False
        self._lock = threading.Lock()
        self._current_render_proc: Optional[subprocess.Popen[str]] = None
        self._current_encoder_proc: Optional[subprocess.Popen[bytes]] = None

    def request_abort(self) -> None:
        with self._lock:
            self._abort = True
            render_proc = self._current_render_proc
        self._terminate_process(render_proc)

    def request_pause(self, pause: bool) -> None:
        with self._lock:
            self._pause = pause
        self.paused.emit(pause)

    def _is_abort(self) -> bool:
        with self._lock:
            return self._abort

    def _is_pause(self) -> bool:
        with self._lock:
            return self._pause

    def _set_render_process(self, proc: Optional[subprocess.Popen[str]]) -> None:
        with self._lock:
            self._current_render_proc = proc

    def _set_encoder_process(self, proc: Optional[subprocess.Popen[bytes]]) -> None:
        with self._lock:
            self._current_encoder_proc = proc

    def _terminate_process(self, proc: Optional[subprocess.Popen[str]]) -> None:
        if proc is None:
            return
        if proc.poll() is None:
            try:
                proc.terminate()
                proc.wait(timeout=5)
            except Exception:
                try:
                    proc.kill()
                except Exception:
                    pass

    def run(self) -> None:
        runner = VideoJobRunner(
            self.cfg,
            is_abort=self._is_abort,
            is_pause=self._is_pause,
            set_render_process=self._set_render_process,
            set_encoder_process=self._set_encoder_process,
            progress_cb=lambda done, total, msg: self.progress.emit(done, total, msg),
        )
        try:
            result = runner.run()
            if result.status == "aborted":
                self.aborted.emit(result.video_path)
            else:
                self.done.emit(result.video_path)
        except Exception as exc:
            self.failed.emit(str(exc))
        finally:
            self._set_render_process(None)
            self._set_encoder_process(None)
