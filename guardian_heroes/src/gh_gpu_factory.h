#pragma once

#include <memory>
#include <string_view>

namespace rex::system {
class IGraphicsSystem;
}

namespace gh::graphics {

std::unique_ptr<rex::system::IGraphicsSystem> CreateGraphicsSystem(
    std::string_view plugin, std::string_view backend = "any");

}  // namespace gh::graphics
