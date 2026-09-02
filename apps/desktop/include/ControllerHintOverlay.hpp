#pragma once

#include "ControllerArtworkLayer.hpp"

#include "pocket/input/ControllerLayout.hpp"

#include <QSize>

#include <memory>
#include <optional>

class QPainter;

namespace Pocket::Input {
class ControllerMapping;
}

namespace Pocket::App {

class ControllerHintOverlay {
public:
    void setSystem(const QString& system);
    void setMapping(std::shared_ptr<Pocket::Input::ControllerMapping> mapping);
    bool isValid() const;

    void paint(QPainter& painter, const QRect& bounds) const;
    QSize preferredSize(const QSize& available) const;

private:
    ControllerArtworkLayer m_artwork;
    std::optional<Pocket::Input::ControllerLayout> m_layout;
    std::shared_ptr<Pocket::Input::ControllerMapping> m_mapping;
};

} // namespace Pocket::App
