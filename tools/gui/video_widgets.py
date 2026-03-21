#!/usr/bin/env python3
from __future__ import annotations

from typing import Dict, List, Optional

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QSizePolicy,
    QVBoxLayout,
    QWidget,
)

from button_styles import style_button
from gui_constants import (
    FORMULA_FIELD_COLUMN_SPACING,
    FORMULA_LINE_SPACING,
    FORMULA_ROOT_SPACING,
    FORMULA_SECTION_SPACING,
)
from video_formula import sanitize_frontend_expr


class DefinitionRow(QWidget):
    lock_changed = Signal()

    def __init__(self, on_add, on_remove) -> None:
        super().__init__()
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        self.edit = QLineEdit()
        self.edit.setPlaceholderText("name = expression")
        self.edit.setMinimumWidth(570)
        layout.addWidget(self.edit, 1)

        self.remove_btn = QPushButton("-")
        self.remove_btn.setFixedWidth(34)
        self.remove_btn.setFixedHeight(28)
        style_button(self.remove_btn, primary=False, icon=True)
        self.remove_btn.clicked.connect(on_remove)
        layout.addWidget(self.remove_btn)

        self.add_btn = QPushButton("+")
        self.add_btn.setFixedWidth(34)
        self.add_btn.setFixedHeight(28)
        style_button(self.add_btn, primary=False, icon=True)
        self.add_btn.clicked.connect(on_add)
        layout.addWidget(self.add_btn)

    def set_controls(self, show_remove: bool, show_add: bool) -> None:
        self.remove_btn.setVisible(show_remove)
        self.add_btn.setVisible(show_add)


class DefinitionSection(QGroupBox):
    def __init__(self, max_rows: int = 15) -> None:
        super().__init__("Definitions")
        self.max_rows = max_rows
        self.rows: List[DefinitionRow] = []
        self.vlayout = QVBoxLayout(self)
        self.vlayout.setSpacing(FORMULA_SECTION_SPACING)
        self.add_row()

    def add_row(self) -> None:
        if len(self.rows) >= self.max_rows:
            return
        row = DefinitionRow(self.add_row, self.remove_last_row)
        self.rows.append(row)
        self.vlayout.addWidget(row)
        self._refresh_controls()

    def remove_last_row(self) -> None:
        if len(self.rows) <= 1:
            return
        row = self.rows.pop()
        row.setParent(None)
        row.deleteLater()
        self._refresh_controls()

    def _refresh_controls(self) -> None:
        for i, row in enumerate(self.rows):
            is_last = i == len(self.rows) - 1
            if len(self.rows) == 1:
                row.set_controls(False, True)
            else:
                row.set_controls(is_last, is_last and len(self.rows) < self.max_rows)

    def get_definitions(self) -> List[str]:
        out: List[str] = []
        for row in self.rows:
            t = row.edit.text().strip()
            if t:
                out.append(t)
        return out


class RangeBlock(QWidget):
    changed = Signal()

    def __init__(self) -> None:
        super().__init__()
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        self.prefix = QLabel("")
        self.prefix.setAlignment(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter)
        layout.addWidget(self.prefix)

        self.edit = QLineEdit()
        self.edit.setMinimumWidth(420)
        self.edit.setPlaceholderText("expression")
        self.edit.textChanged.connect(self.changed)
        layout.addWidget(self.edit, 1)

    def set_mode(self, show_editor: bool, prefix_text: str, placeholder: str = "expression") -> None:
        self.prefix.setText(prefix_text)
        self.edit.setVisible(show_editor)
        self.edit.setPlaceholderText(placeholder)

    def current_text(self) -> str:
        return sanitize_frontend_expr(self.edit.text())

    def apply_value(self, text: str) -> None:
        self.edit.setText(sanitize_frontend_expr(text))


class TimedVectorRow(QWidget):
    range_changed = Signal()

    def __init__(self, labels: List[str], on_add, on_remove) -> None:
        super().__init__()
        self.fields: Dict[str, QLineEdit] = {}

        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(FORMULA_ROOT_SPACING)

        self.range_block = RangeBlock()
        self.range_block.changed.connect(self.range_changed)
        root.addWidget(self.range_block)

        row = QHBoxLayout()
        row.setContentsMargins(0, 0, 0, 0)
        row.setSpacing(FORMULA_LINE_SPACING)

        fields_col = QVBoxLayout()
        fields_col.setContentsMargins(0, 0, 0, 0)
        fields_col.setSpacing(FORMULA_FIELD_COLUMN_SPACING)
        for label in labels:
            line = QHBoxLayout()
            line.setContentsMargins(0, 0, 0, 0)
            line.setSpacing(FORMULA_LINE_SPACING)
            lbl = QLabel(f"{label} =")
            line.addWidget(lbl)
            edit = QLineEdit()
            edit.setPlaceholderText(f"{label} expression")
            edit.setMinimumWidth(0)
            edit.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
            self.fields[label] = edit
            line.addWidget(edit, 1)
            fields_col.addLayout(line)
        row.addLayout(fields_col, 1)

        self.remove_btn = QPushButton("-")
        self.remove_btn.setFixedWidth(34)
        self.remove_btn.setFixedHeight(28)
        style_button(self.remove_btn, primary=False, icon=True)
        self.remove_btn.clicked.connect(on_remove)
        btn_col = QVBoxLayout()
        btn_col.setContentsMargins(0, 0, 0, 0)
        btn_col.addWidget(self.remove_btn)

        self.add_btn = QPushButton("+")
        self.add_btn.setFixedWidth(34)
        self.add_btn.setFixedHeight(28)
        style_button(self.add_btn, primary=False, icon=True)
        self.add_btn.clicked.connect(on_add)
        btn_col.addWidget(self.add_btn)
        btn_col.addStretch(1)
        row.addLayout(btn_col)

        root.addLayout(row)

    def set_controls(self, show_remove: bool, show_add: bool) -> None:
        self.remove_btn.setVisible(show_remove)
        self.add_btn.setVisible(show_add)


class TimedVectorSection(QGroupBox):
    def __init__(self, title: str, labels: List[str], max_rows: int = 10) -> None:
        super().__init__(title)
        self.labels = labels
        self.max_rows = max_rows
        self.rows: List[TimedVectorRow] = []
        self.vlayout = QVBoxLayout(self)
        self.vlayout.setSpacing(FORMULA_SECTION_SPACING)
        self.add_row()

    def add_row(self) -> None:
        if len(self.rows) >= self.max_rows:
            return
        row = TimedVectorRow(self.labels, self.add_row, self.remove_last_row)
        row.range_changed.connect(self._refresh_ranges)
        self.rows.append(row)
        self.vlayout.addWidget(row)
        self._refresh_controls()
        self._refresh_ranges()

    def remove_last_row(self) -> None:
        if len(self.rows) <= 1:
            return
        row = self.rows.pop()
        row.setParent(None)
        row.deleteLater()
        self._refresh_controls()
        self._refresh_ranges()

    def _refresh_controls(self) -> None:
        for i, row in enumerate(self.rows):
            is_last = i == len(self.rows) - 1
            if len(self.rows) == 1:
                row.set_controls(False, True)
            else:
                row.set_controls(is_last, is_last and len(self.rows) < self.max_rows)

    def _prev_locked_or_placeholder(self, i: int) -> str:
        txt = self.rows[i - 1].range_block.current_text().strip()
        return txt if txt else "enter ending frame above"

    def _refresh_ranges(self) -> None:
        n = len(self.rows)
        if n == 1:
            self.rows[0].range_block.setVisible(False)
            return

        for i, row in enumerate(self.rows):
            rb = row.range_block
            rb.setVisible(True)
            if i == 0:
                rb.set_mode(True, "Range: frame 0 through and including frame ", "end frame expression")
            elif i == n - 1:
                rb.set_mode(False, f"Range: frame [1 + {self._prev_locked_or_placeholder(i)}] to end")
            else:
                rb.set_mode(
                    True,
                    f"Range: frame [1 + {self._prev_locked_or_placeholder(i)}] through and including frame ",
                    "end frame expression",
                )

    def get_rows(self) -> List[Dict[str, object]]:
        out: List[Dict[str, object]] = []
        n = len(self.rows)
        for i, row in enumerate(self.rows):
            fields = {k: e.text().strip() for k, e in row.fields.items()}
            range_expr: Optional[str] = None
            if n > 1 and i < n - 1:
                range_expr = row.range_block.current_text().strip()
            out.append({"fields": fields, "range_expr": range_expr})
        return out
