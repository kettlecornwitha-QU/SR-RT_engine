#!/usr/bin/env python3
from PySide6.QtGui import QFont
from PySide6.QtWidgets import QApplication, QPushButton

REGULAR_BUTTON_STYLE = (
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

PRIMARY_BUTTON_STYLE = (
    "QPushButton {"
    "background-color: #2d7df6;"
    "color: white;"
    "border: 1px solid #1d5fc5;"
    "border-top-color: #6fa7ff;"
    "border-bottom-color: #174c9f;"
    "border-radius: 6px;"
    "padding: 6px 14px;"
    "}"
    "QPushButton:hover { background-color: #3b89ff; }"
    "QPushButton:pressed {"
    "background-color: #1f6ede;"
    "border-top-color: #1757b8;"
    "border-bottom-color: #8ab8ff;"
    "}"
    "QPushButton:disabled {"
    "background-color: #7fa9e8;"
    "color: #eaf1ff;"
    "}"
)

ICON_BUTTON_STYLE = (
    "QPushButton {"
    "background-color: #bfbfbf;"
    "border: 1px solid #6f6f6f;"
    "border-top-color: #e7e7e7;"
    "border-bottom-color: #585858;"
    "border-radius: 6px;"
    "padding: 0px;"
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


def style_button(button: QPushButton, *, primary: bool = False, icon: bool = False) -> None:
    app = QApplication.instance()
    if app is not None:
        base_font = app.font()
        button_font = QFont(base_font)
        button_font.setBold(False)
        button.setFont(button_font)
    if icon:
        button.setStyleSheet(ICON_BUTTON_STYLE)
    else:
        button.setStyleSheet(PRIMARY_BUTTON_STYLE if primary else REGULAR_BUTTON_STYLE)
