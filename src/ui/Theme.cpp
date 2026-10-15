#include "ui/Theme.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

namespace {
const QColor kWindow(26, 26, 26);
const QColor kWindowText(224, 224, 224);
const QColor kBase(16, 16, 16);
const QColor kAltBase(30, 30, 30);
const QColor kText(224, 224, 224);
const QColor kButton(45, 45, 45);
const QColor kButtonText(224, 224, 224);
const QColor kHighlight(38, 79, 120);
const QColor kHighlightedText(255, 255, 255);
const QColor kDisabled(120, 120, 120);
} // namespace

void Theme::applyDark(QApplication& app) {
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QPalette p;
    p.setColor(QPalette::Window, kWindow);
    p.setColor(QPalette::WindowText, kWindowText);
    p.setColor(QPalette::Base, kBase);
    p.setColor(QPalette::AlternateBase, kAltBase);
    p.setColor(QPalette::ToolTipBase, kAltBase);
    p.setColor(QPalette::ToolTipText, kWindowText);
    p.setColor(QPalette::Text, kText);
    p.setColor(QPalette::PlaceholderText, QColor(130, 130, 130));
    p.setColor(QPalette::Button, kButton);
    p.setColor(QPalette::ButtonText, kButtonText);
    p.setColor(QPalette::BrightText, QColor(255, 90, 90));
    p.setColor(QPalette::Link, QColor(90, 160, 255));
    p.setColor(QPalette::Highlight, kHighlight);
    p.setColor(QPalette::HighlightedText, kHighlightedText);

    p.setColor(QPalette::Light, QColor(70, 70, 70));
    p.setColor(QPalette::Midlight, QColor(55, 55, 55));
    p.setColor(QPalette::Mid, QColor(35, 35, 35));
    p.setColor(QPalette::Dark, QColor(10, 10, 10));
    p.setColor(QPalette::Shadow, QColor(0, 0, 0));

    p.setColor(QPalette::Disabled, QPalette::WindowText, kDisabled);
    p.setColor(QPalette::Disabled, QPalette::Text, kDisabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, kDisabled);
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(180, 180, 180));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor(24, 24, 24));

    app.setPalette(p);
}
