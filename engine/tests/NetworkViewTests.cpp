#include <PipeFrame/Backend/SFML/UI/NetworkView.h>
#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
bool Check(bool condition, const char *message) {
    if (!condition) std::cerr << "FAILED: " << message << '\n';
    return condition;
}
sf::Image Render(const Widget &widget) {
    sf::RenderTexture target({160, 160});
    target.clear(sf::Color::Black);
    widget.Render(target);
    target.display();
    return target.getTexture().copyToImage();
}
}

int main() {
    bool passed = true;
    Panel parent;
    parent.SetPosition({20, 20});
    auto &view = parent.CreateChild<NetworkView>();
    view.SetSize({100, 100});
    const std::vector<NetworkNode> nodes{{1, {0, 0.5f}, sf::Color::White, "Input", "0.50"},
                                         {2, {1, 0.5f}, sf::Color::White, "Output", "-0.25"}};
    view.SetGraph(nodes, {{1, 2, sf::Color::White, true, 2.0f}});
    passed &= Check(view.GetNodes()[0].label == "Input" && view.GetNodes()[1].value == "-0.25" &&
                        view.GetEdges()[0].strength == 2.0f,
                    "Network presentation metadata must preserve labels, live values, and edge strength");
    passed &= Check(view.FindNodeAt({31, 70}) == 1 && view.FindNodeAt({109, 70}) == 2,
                    "Picking must use parent position and inset node centers");
    auto image = Render(view);
    passed &= Check(image.getPixel({31, 70}).r == 255 && image.getPixel({70, 70}).r == 255,
                    "Nodes and connections must render");
    passed &= Check(image.getPixel({92, 67}).r > 100 && image.getPixel({45, 67}).r == 0,
                    "Directed edges must place the arrowhead at the target");
    parent.SetOpacity(0.5f);
    image = Render(view);
    passed &= Check(image.getPixel({31, 70}).r >= 126 && image.getPixel({31, 70}).r <= 129,
                    "Nodes must inherit opacity");
    view.SetSelectedNode(1);
    passed &= Check(Render(view).getPixel({21, 70}).r > 100, "Selected nodes must have a visible ring");
    for (int invalid = 0; invalid < 4; ++invalid) {
        auto badNodes = nodes;
        std::vector<NetworkEdge> badEdges{{1, 2}};
        if (invalid == 0) badNodes[1].id = 1;
        if (invalid == 1) badNodes[1].position.x = std::numeric_limits<float>::quiet_NaN();
        if (invalid == 2) badEdges[0].target = 3;
        if (invalid == 3) badEdges[0].target = 1;
        bool rejected = false;
        try { view.SetGraph(badNodes, badEdges); } catch (const std::invalid_argument &) { rejected = true; }
        passed &= Check(rejected && view.GetNodes().size() == 2 && view.GetSelectedNode() == 1,
                        "Invalid graph replacement must preserve graph and selection");
    }
    parent.SetVisualOffset({10, 0});
    view.SetSize({120, 80});
    passed &= Check(view.FindNodeAt({41, 60}) == 1, "Picking must follow resize and visual motion");
    parent.SetVisible(false);
    passed &= Check(!view.FindNodeAt({41, 60}), "Hidden ancestors must disable node inspection");
    parent.SetVisible(true);
    view.SetGraph({{2, {0.5f, 0.5f}}, {3, {0.5f, 0.5f}}}, {});
    passed &= Check(!view.GetSelectedNode() && view.FindNodeAt({90, 60}) == 3,
                    "Removed selection must clear and overlapping nodes pick in paint order");
    view.SetSize({0, 0});
    passed &= Check(!view.FindNodeAt({90, 60}) && Render(view).getPixel({90, 60}) == sf::Color::Black,
                    "Collapsed graphs must clear geometry and picking");
    view.SetGraph({}, {});
    UIManager manager;
    auto &passive = manager.CreateRoot<NetworkView>();
    passive.SetGraph(nodes, {});
    passed &= Check(!manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {11, 80}}),
                    "Passive network surface must not capture world input");
    if (!passed) return 1;
    std::cout << "All network view tests passed.\n";
}
