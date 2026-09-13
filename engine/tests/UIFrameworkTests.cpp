#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/Bindings.h>
#include <PipeFrame/Backend/SFML/UI/Button.h>
#include <PipeFrame/Backend/SFML/UI/Component.h>
#include <PipeFrame/Backend/SFML/UI/EdgeDrawer.h>
#include <PipeFrame/Backend/SFML/UI/Gauge.h>
#include <PipeFrame/Backend/SFML/UI/ListView.h>
#include <PipeFrame/Backend/SFML/UI/MetricCard.h>
#include <PipeFrame/Backend/SFML/UI/ModalBarrier.h>
#include <PipeFrame/Backend/SFML/UI/OverlayPanel.h>
#include <PipeFrame/Backend/SFML/UI/PopupLayer.h>
#include <PipeFrame/Backend/SFML/UI/ProgressBar.h>
#include <PipeFrame/Backend/SFML/UI/ScrollPanel.h>
#include <PipeFrame/Backend/SFML/UI/SegmentedControl.h>
#include <PipeFrame/Backend/SFML/UI/Separator.h>
#include <PipeFrame/Backend/SFML/UI/Slider.h>
#include <PipeFrame/Backend/SFML/UI/Spacer.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>
#include <PipeFrame/UI/State.h>
#include <PipeFrame/Backend/SFML/UI/Surface.h>
#include <PipeFrame/Backend/SFML/UI/TabView.h>
#include <PipeFrame/Backend/SFML/UI/Toggle.h>
#include <PipeFrame/Backend/SFML/UI/Toast.h>
#include <PipeFrame/Backend/SFML/UI/UIBuilder.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <PipeFrame/Backend/SFML/UI/ZenModeShell.h>

#include <cmath>
#include <chrono>
#include <iostream>
#include <string>

#include <SFML/Window/Mouse.hpp>

namespace {

bool NearlyEqual(const float left, const float right) { return std::abs(left - right) < 0.001f; }

bool Check(const bool condition, const std::string &message) {

    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';

        return false;
    }

    return true;
}

bool CheckVector(const sf::Vector2f actual, const sf::Vector2f expected, const std::string &message) {

    return Check(NearlyEqual(actual.x, expected.x) && NearlyEqual(actual.y, expected.y), message);
}

class ProbeWidget final : public Widget {
  public:
    bool consumeEvents = true;
    int pointerEnterCount = 0;
    int pointerExitCount = 0;
    int pointerMoveCount = 0;
    int pointerPressCount = 0;
    int pointerReleaseCount = 0;
    int textEventCount = 0;

  protected:
    void OnRender(sf::RenderTarget &target) const override { (void)target; }

    bool OnEvent(const sf::Event &event) override {
        pointerMoveCount += event.is<sf::Event::MouseMoved>() ? 1 : 0;
        pointerPressCount += event.is<sf::Event::MouseButtonPressed>() ? 1 : 0;
        pointerReleaseCount += event.is<sf::Event::MouseButtonReleased>() ? 1 : 0;
        textEventCount += event.is<sf::Event::TextEntered>() ? 1 : 0;
        return consumeEvents;
    }

    void OnPointerEntered() override { ++pointerEnterCount; }
    void OnPointerExited() override { ++pointerExitCount; }
};

bool TestDeclarativeBuilder() {
    UIManager manager;

    StackPanel &root = pipeframe::ui::BuildRoot<StackPanel>(manager, [](StackPanel &panel) {
        panel.SetSize({300.0f, 200.0f});
        panel.SetPadding(Thickness{10.0f});
        panel.SetSpacing(5.0f);
    });

    Panel &child = pipeframe::ui::BuildChild<Panel>(root, [](Panel &panel) { panel.SetSize({50.0f, 30.0f}); });

    bool passed = true;

    passed &= Check(manager.GetRootCount() == 1, "Builder should create one root.");

    passed &= Check(root.GetChildCount() == 1, "Builder should create one child.");

    passed &= CheckVector(child.GetPosition(), {10.0f, 10.0f}, "Builder child has the wrong layout position.");

    passed &= CheckVector(child.GetSize(), {280.0f, 30.0f}, "Builder child should stretch horizontally.");

    return passed;
}

bool TestBarrierKeyboardFocusAndTraversal() {
    UIManager manager;
    auto &world = manager.CreateRoot<ProbeWidget>();
    world.SetSize({500,400}); world.SetFocusable(true);
    manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,{10,10}});
    auto &popup = manager.CreateRoot<PopupLayer>();
    popup.SetSize({500,400}); popup.SetReducedMotion(true);
    auto &first = popup.GetContent().CreateChild<Button>(); first.SetSize({60,30});
    auto &second = popup.GetContent().CreateChild<Button>(); second.SetSize({60,30}); second.SetPosition({70,0});
    popup.Open();
    bool passed = true;
    passed &= Check(manager.HandleEvent(sf::Event::TextEntered{U'x'}) && world.textEventCount==0,
                    "Opening a popup must immediately block previously focused background text input.");
    manager.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,{450,350}});
    passed &= Check(world.pointerReleaseCount==0, "A new barrier must revoke background pointer capture.");
    manager.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Tab});
    passed &= Check(first.HasKeyboardFocus(), "Tab must enter the barrier's focusable children.");
    manager.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Tab});
    passed &= Check(second.HasKeyboardFocus(), "Tab must follow retained child order.");
    sf::Event::KeyPressed reverse{sf::Keyboard::Key::Tab}; reverse.shift=true;
    manager.HandleEvent(reverse);
    passed &= Check(first.HasKeyboardFocus(), "Shift-Tab must traverse backward within the barrier.");
    manager.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Escape});
    passed &= Check(!popup.IsVisible(), "Escape must bubble from a focused control to dismiss its popup.");
    passed &= Check(!manager.HandleEvent(sf::Event::TextEntered{U'y'}), "Hidden popup focus must relinquish keyboard input.");
    manager.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Tab});
    passed &= Check(world.HasKeyboardFocus(), "Tab must skip hidden popup controls.");
    auto &modal = manager.CreateRoot<ModalBarrier>(); modal.SetSize({500,400});
    auto &card = modal.CreateChild<Panel>(); card.SetPosition({50,50}); card.SetSize({100,100});
    int dismissals=0; modal.SetOnDismiss([&] { ++dismissals; modal.SetVisible(false); });
    manager.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,{80,80}});
    manager.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,{80,80}});
    passed &= Check(dismissals==0, "Bubbled inside clicks must not dismiss a modal.");
    manager.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Escape});
    passed &= Check(dismissals==1, "Escape must dismiss a modal once.");
    return passed;
}

bool TestRoundedSurfaceTokens() {
    const UITheme &theme = UITheme::Dark();
    Card card;
    card.SetSize({160.0f, 80.0f});

    bool passed = true;
    passed &= Check(card.GetVariant() == SurfaceVariant::Elevated,
                    "Card should use the elevated surface role.");
    passed &= Check(NearlyEqual(card.GetCornerRadius(), theme.radiusMedium),
                    "Card radius should come from the centralized theme.");
    passed &= Check(card.GetShadowColor() == theme.shadow &&
                        card.GetShadowOffset() == theme.shadowOffsetSmall,
                    "Elevated surfaces should use the themed restrained shadow.");

    card.SetVariant(SurfaceVariant::Glass);
    passed &= Check(card.GetFillColor() == theme.glassSurface,
                    "Glass surfaces should use the translucent theme role.");
    passed &= Check(NearlyEqual(card.GetCornerRadius(), theme.radiusLarge),
                    "Floating glass should use the large themed radius.");

    Separator horizontal;
    passed &= Check(horizontal.GetWidthPolicy() == SizePolicy::Stretch &&
                        horizontal.GetHeightPolicy() == SizePolicy::Fixed,
                    "A horizontal separator should stretch without consuming panel height.");
    return passed;
}

bool TestRealTimeButtonMotionAndReducedMotion() {
    UIManager manager;
    Button &button = manager.CreateRoot<Button>();
    button.SetSize({100.0f, 40.0f});
    button.SetNormalColor(sf::Color{10, 20, 30});
    button.SetHoveredColor(sf::Color{110, 120, 130});
    button.SetTransitionDuration(1.0f);
    button.SetReducedMotion(true);
    button.SetReducedMotion(false);

    const sf::Event moveInside{sf::Event::MouseMoved{{10, 10}}};
    manager.HandleEvent(moveInside);
    const sf::Color before = button.GetVisualColor();
    manager.Update(0.5f);
    const sf::Color midway = button.GetVisualColor();
    manager.Update(0.5f);

    bool passed = true;
    passed &= Check(before == sf::Color(10, 20, 30),
                    "A hover transition should begin from the current visual color.");
    passed &= Check(midway != before && midway != sf::Color(110, 120, 130),
                    "Button color should interpolate using real UI time.");
    passed &= Check(button.GetVisualColor() == sf::Color(110, 120, 130),
                    "Button motion should finish at the hover color.");

    button.SetReducedMotion(true);
    button.SetSelectedColor(sf::Color{40, 150, 90});
    manager.HandleEvent(sf::Event{sf::Event::MouseMoved{{200, 200}}});
    button.SetSelected(true);
    passed &= Check(button.GetState() == ButtonState::Selected &&
                        button.GetVisualColor() == sf::Color(40, 150, 90),
                    "Reduced motion should snap selected-state feedback immediately.");
    return passed;
}

bool TestWidgetLiftFadeAndInheritedOpacity() {
    Panel parent;
    parent.SetPosition({20.0f, 30.0f});
    Panel &child = parent.CreateChild<Panel>();
    child.SetPosition({5.0f, 7.0f});

    parent.AnimateVisualOffsetTo({0.0f, -4.0f}, 1.0f);
    parent.AnimateOpacityTo(0.5f, 1.0f);
    parent.Update(0.5f);

    bool passed = true;
    passed &= Check(parent.GetVisualOffset().y < 0.0f && parent.GetVisualOffset().y > -4.0f,
                    "Lift motion should interpolate from real UI time.");
    passed &= Check(parent.GetOpacity() < 1.0f && parent.GetOpacity() > 0.5f,
                    "Fade motion should interpolate rather than snap.");
    passed &= Check(NearlyEqual(child.GetEffectiveOpacity(), parent.GetOpacity()),
                    "A child surface should inherit its parent's animated opacity.");
    passed &= CheckVector(child.GetScreenPosition(),
                          {25.0f, 37.0f + parent.GetVisualOffset().y},
                          "A lifted parent should move its child without changing layout coordinates.");

    parent.SetReducedMotion(true);
    parent.AnimateVisualOffsetTo({0.0f, 0.0f}, 1.0f);
    parent.AnimateOpacityTo(1.0f, 1.0f);
    passed &= CheckVector(parent.GetVisualOffset(), {0.0f, 0.0f},
                          "Reduced motion should snap slide/lift feedback.");
    passed &= Check(NearlyEqual(parent.GetOpacity(), 1.0f),
                    "Reduced motion should snap fade feedback.");
    return passed;
}

bool TestModalBarrierInputOwnership() {
    UIManager manager;
    ProbeWidget &world = manager.CreateRoot<ProbeWidget>();
    world.SetSize({100.0f, 100.0f});

    ModalBarrier &barrier = manager.CreateRoot<ModalBarrier>();
    barrier.SetSize({100.0f, 100.0f});
    int dismissCount = 0;
    barrier.SetOnDismiss([&dismissCount]() { ++dismissCount; });

    const sf::Event press{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {20, 20}}};
    const sf::Event release{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {20, 20}}};

    bool passed = true;
    passed &= Check(manager.HandleEvent(press) && manager.HandleEvent(release),
                    "A modal barrier should capture background pointer input.");
    passed &= Check(dismissCount == 1, "A dismissible modal barrier should invoke its callback once.");
    passed &= Check(world.pointerPressCount == 0,
                    "Modal input must not leak into the simulation world.");

    barrier.SetVisible(false);
    passed &= Check(manager.HandleEvent(press),
                    "Hiding a modal barrier should restore the underlying world target.");
    passed &= Check(world.pointerPressCount == 1,
                    "The world should receive pointer input after the modal closes.");
    return passed;
}

bool TestPopupClampingAndInputOwnership() {
    UIManager manager;
    ProbeWidget &world = manager.CreateRoot<ProbeWidget>();
    world.SetSize({200.0f, 120.0f});

    PopupLayer &popup = manager.CreateRoot<PopupLayer>();
    popup.SetSize({200.0f, 120.0f});
    popup.SetContentBounds({{170.0f, 100.0f}, {80.0f, 50.0f}});
    popup.SetReducedMotion(true);
    int dismissCount = 0;
    popup.SetOnDismiss([&dismissCount]() { ++dismissCount; });
    popup.Open();

    bool passed = true;
    passed &= CheckVector(popup.GetContentBounds().position, {120.0f, 70.0f},
                          "Popup content should remain clamped to its viewport.");

    const sf::Event insidePress{
        sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {150, 90}}};
    const sf::Event insideRelease{
        sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {150, 90}}};
    manager.HandleEvent(insidePress);
    manager.HandleEvent(insideRelease);
    passed &= Check(popup.IsOpen() && world.pointerPressCount == 0,
                    "Popup content interaction should not dismiss or leak into the world.");

    const sf::Event outsidePress{
        sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {20, 20}}};
    const sf::Event outsideRelease{
        sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {20, 20}}};
    manager.HandleEvent(outsidePress);
    manager.HandleEvent(outsideRelease);
    passed &= Check(!popup.IsVisible() && dismissCount == 1,
                    "A reduced-motion popup should dismiss immediately after a background click.");

    manager.HandleEvent(outsidePress);
    passed &= Check(world.pointerPressCount == 1,
                    "World input should resume as soon as a popup has closed.");
    return passed;
}

bool TestToastUsesRealTimeAndPassesInputThrough() {
    UIManager manager;
    ProbeWidget &world = manager.CreateRoot<ProbeWidget>();
    world.SetSize({200.0f, 120.0f});

    Toast &toast = manager.CreateRoot<Toast>();
    toast.SetPosition({20.0f, 20.0f});
    toast.SetReducedMotion(true);
    int dismissCount = 0;
    toast.SetOnDismissed([&dismissCount]() { ++dismissCount; });
    toast.Show(0.5f);

    manager.HandleEvent(
        sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {30, 30}}});

    bool passed = true;
    passed &= Check(world.pointerPressCount == 1,
                    "A passive toast must never steal pointer input from the playfield.");
    manager.Update(0.49f);
    passed &= Check(toast.IsPresented(),
                    "Toast lifetime should advance using real UI time.");
    manager.Update(0.01f);
    passed &= Check(!toast.IsVisible() && dismissCount == 1,
                    "An expired reduced-motion toast should hide and report dismissal once.");
    return passed;
}

bool TestZenModePreservesEssentialLayerAndState() {
    ZenModeShell shell;
    shell.SetSize({320.0f, 180.0f});
    Panel &chrome = shell.GetChromeLayer().CreateChild<Panel>();
    Panel &transport = shell.GetEssentialLayer().CreateChild<Panel>();
    pipeframe::ui::State<bool> zen{false};
    pipeframe::ui::BindZenMode(shell, zen);

    bool passed = true;
    passed &= CheckVector(shell.GetPlayfieldLayer().GetSize(), {320.0f, 180.0f},
                          "The playfield layer should fill the complete Zen shell.");
    passed &= Check(chrome.IsVisible() && transport.IsVisible(),
                    "Normal mode should expose chrome and essential controls.");

    zen.Set(true);
    passed &= Check(shell.IsZenMode() && !shell.GetChromeLayer().IsVisible(),
                    "Zen mode should hide the chrome layer through typed state.");
    passed &= Check(shell.GetPlayfieldLayer().IsVisible() &&
                        shell.GetEssentialLayer().IsVisible() && transport.IsVisible(),
                    "Zen mode must preserve the playfield and essential controls.");

    zen.Set(false);
    passed &= Check(chrome.IsVisible(),
                    "Leaving Zen mode should restore the same retained chrome widgets.");
    return passed;
}

bool TestTabViewSelectionAndRetainedPages() {
    UIManager manager;
    TabView &tabs = manager.CreateRoot<TabView>();
    tabs.SetSize({200.0f, 120.0f});
    Panel &first = tabs.AddPage<Panel>();
    Panel &second = tabs.AddPage<Panel>();
    std::size_t selected = 0;
    tabs.SetOnSelectionChanged([&selected](const std::size_t index) { selected = index; });

    bool passed = true;
    passed &= Check(tabs.GetPageCount() == 2 && first.IsVisible() && !second.IsVisible(),
                    "A tab view should present its first retained page initially.");

    manager.HandleEvent(
        sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {150, 18}}});
    manager.HandleEvent(
        sf::Event{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {150, 18}}});
    passed &= Check(tabs.GetSelectedIndex() == 1 && selected == 1 &&
                        !first.IsVisible() && second.IsVisible(),
                    "Selecting a tab should swap retained pages and report the index.");

    pipeframe::ui::State<std::size_t> selection{0};
    pipeframe::ui::BindSelection(tabs, selection);
    passed &= Check(first.IsVisible() && !second.IsVisible(),
                    "A typed tab selection should immediately update page visibility.");
    return passed;
}

bool TestListViewPointerKeyboardAndBinding() {
    UIManager manager;
    ListView &list = manager.CreateRoot<ListView>();
    list.SetSize({160.0f, 112.0f});
    list.AddItem();
    list.AddItem();
    list.AddItem();

    manager.HandleEvent(
        sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {20, 56}}});
    manager.HandleEvent(
        sf::Event{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {20, 56}}});

    bool passed = true;
    passed &= Check(list.GetSelectedIndex() == 1 && list.GetItem(1)->IsSelected(),
                    "A list item should become the selected row after pointer activation.");
    manager.HandleEvent(sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Down}});
    passed &= Check(list.GetSelectedIndex() == 2 && list.GetItem(2)->IsSelected(),
                    "Down should move selection through the focused list.");

    pipeframe::ui::State<std::size_t> selection{0};
    pipeframe::ui::BindSelection(list, selection);
    passed &= Check(list.GetSelectedIndex() == 0 && list.GetItem(0)->IsSelected(),
                    "Typed list selection should update the retained selected row.");
    return passed;
}

bool TestProgressAndGaugeRealTimeMotion() {
    ProgressBar progress;
    progress.SetTransitionDuration(1.0f);
    progress.SetValue(1.0f);
    progress.Update(0.5f);

    Gauge gauge;
    gauge.SetRange(0.0f, 100.0f);
    gauge.SetTransitionDuration(1.0f);
    gauge.SetValue(100.0f);
    gauge.Update(0.5f);

    bool passed = true;
    passed &= Check(progress.GetVisualValue() > 0.0f && progress.GetVisualValue() < 1.0f,
                    "Progress fill should animate using real UI time.");
    passed &= Check(gauge.GetVisualValue() > 0.0f && gauge.GetVisualValue() < 100.0f,
                    "Gauge sweep should animate using real UI time.");

    progress.SetReducedMotion(true);
    gauge.SetReducedMotion(true);
    progress.SetValue(0.25f);
    gauge.SetValue(25.0f);
    passed &= Check(NearlyEqual(progress.GetVisualValue(), 0.25f) &&
                        NearlyEqual(gauge.GetVisualValue(), 25.0f),
                    "Reduced motion should snap monitoring values immediately.");

    pipeframe::ui::State<float> value{0.75f};
    pipeframe::ui::BindValue(progress, value);
    passed &= Check(NearlyEqual(progress.GetValue(), 0.75f),
                    "Progress should accept the common typed numeric binding.");
    return passed;
}

bool TestEdgeDrawerHandleAndMotion() {
    UIManager manager;
    EdgeDrawer &drawer = manager.CreateRoot<EdgeDrawer>(DrawerEdge::Left);
    drawer.SetSize({200.0f, 120.0f});
    drawer.SetHandleExtent(24.0f);
    drawer.SetTransitionDuration(1.0f);

    bool passed = true;
    passed &= CheckVector(drawer.GetVisualOffset(), {-176.0f, 0.0f},
                          "A closed left drawer should leave only its handle visible.");
    passed &= CheckVector(drawer.GetHandle().GetScreenPosition(), {0.0f, 12.0f},
                          "The compact closed handle should remain centered on the viewport edge.");

    manager.HandleEvent(sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {10, 20}}});
    manager.HandleEvent(sf::Event{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {10, 20}}});
    passed &= Check(drawer.IsOpen(), "Clicking the exposed handle should open the drawer.");

    manager.Update(0.5f);
    passed &= Check(drawer.GetVisualOffset().x > -176.0f && drawer.GetVisualOffset().x < 0.0f,
                    "Drawer opening should slide using real UI time.");
    manager.Update(0.5f);
    passed &= CheckVector(drawer.GetVisualOffset(), {0.0f, 0.0f},
                          "Drawer opening should end at its arranged position.");

    drawer.SetReducedMotion(true);
    drawer.Close();
    passed &= CheckVector(drawer.GetVisualOffset(), {-176.0f, 0.0f},
                          "Reduced motion should close an edge drawer immediately.");
    return passed;
}

bool TestTogglePointerAndKeyboardActivation() {
    UIManager manager;
    Toggle &toggle = manager.CreateRoot<Toggle>();
    int changeCount = 0;
    bool latestValue = false;
    toggle.SetOnChanged([&](const bool checked) {
        ++changeCount;
        latestValue = checked;
    });

    manager.HandleEvent(sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {10, 10}}});
    manager.HandleEvent(sf::Event{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {10, 10}}});

    bool passed = true;
    passed &= Check(toggle.IsChecked() && toggle.IsSelected() && latestValue && changeCount == 1,
                    "Pointer activation should toggle value, visuals, and callback together.");

    manager.HandleEvent(sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Space}});
    manager.HandleEvent(sf::Event{sf::Event::KeyReleased{sf::Keyboard::Key::Space}});
    passed &= Check(!toggle.IsChecked() && !toggle.IsSelected() && !latestValue && changeCount == 2,
                    "Space should toggle the focused control through the same callback.");
    return passed;
}

bool TestSliderPointerKeyboardAndStep() {
    UIManager manager;
    Slider &slider = manager.CreateRoot<Slider>();
    slider.SetSize({100.0f, 32.0f});
    slider.SetRange(0.0f, 10.0f);
    slider.SetStep(1.0f);
    int changeCount = 0;
    slider.SetOnValueChanged([&changeCount](float) { ++changeCount; });

    manager.HandleEvent(sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {75, 16}}});
    manager.HandleEvent(sf::Event{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {75, 16}}});

    bool passed = true;
    passed &= Check(NearlyEqual(slider.GetValue(), 8.0f) && changeCount == 1,
                    "Slider pointer input should clamp and snap to its configured step.");
    manager.HandleEvent(sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Right}});
    passed &= Check(NearlyEqual(slider.GetValue(), 9.0f) && changeCount == 2,
                    "Right arrow should advance a focused slider by one step.");
    manager.HandleEvent(sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::End}});
    passed &= Check(NearlyEqual(slider.GetValue(), 10.0f),
                    "End should move a focused slider to its maximum.");
    return passed;
}

bool TestSegmentedSelectionAndKeyboardNavigation() {
    UIManager manager;
    SegmentedControl &control = manager.CreateRoot<SegmentedControl>();
    control.SetSize({180.0f, 36.0f});
    control.AddSegment();
    control.AddSegment();
    control.AddSegment();
    std::size_t callbackIndex = SegmentedControl::NoSelection;
    control.SetOnSelectionChanged(
        [&callbackIndex](const std::size_t index) { callbackIndex = index; });

    manager.HandleEvent(sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {150, 18}}});
    manager.HandleEvent(sf::Event{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {150, 18}}});

    bool passed = true;
    passed &= Check(control.GetSelectedIndex() == 2 && callbackIndex == 2,
                    "Clicking a segment should select and report its stable index.");
    manager.HandleEvent(sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Left}});
    passed &= Check(control.GetSelectedIndex() == 1 && callbackIndex == 1,
                    "Arrow keys should navigate selection within the focused segment group.");
    passed &= Check(control.GetSegment(1) != nullptr && control.GetSegment(1)->IsSelected(),
                    "Only the selected segment should expose selected visual state.");
    return passed;
}

bool TestButtonKeyboardFocusAndActivation() {
    UIManager manager;
    Button &button = manager.CreateRoot<Button>();
    button.SetSize({100.0f, 40.0f});
    button.SetReducedMotion(true);

    int activationCount = 0;
    button.SetOnClick([&activationCount]() { ++activationCount; });
    manager.HandleEvent(sf::Event{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {10, 10}}});
    manager.HandleEvent(sf::Event{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {10, 10}}});

    bool passed = true;
    passed &= Check(manager.HasKeyboardFocus(), "Clicking a button should expose its focused treatment.");
    passed &= Check(activationCount == 1, "Pointer activation should invoke a focused button once.");

    manager.HandleEvent(sf::Event{sf::Event::KeyPressed{sf::Keyboard::Key::Enter}});
    manager.HandleEvent(sf::Event{sf::Event::KeyReleased{sf::Keyboard::Key::Enter}});
    passed &= Check(activationCount == 2,
                    "Enter should activate the focused button through the same callback.");
    passed &= Check(button.GetState() == ButtonState::Hovered,
                    "Pointer hover should remain the strongest treatment after keyboard activation.");
    return passed;
}

bool TestKeyedCompositionRetainsIdentity() {
    Panel parent;

    Panel &first = pipeframe::ui::BuildKeyedChild<Panel>(
        parent, "content", [](Panel &panel) { panel.SetSize({20.0f, 10.0f}); });
    Panel &second = pipeframe::ui::BuildKeyedChild<Panel>(
        parent, "content", [](Panel &panel) { panel.SetSize({40.0f, 30.0f}); });

    bool passed = true;
    passed &= Check(&first == &second, "A keyed composition pass should retain widget identity.");
    passed &= Check(parent.GetChildCount() == 1, "A keyed composition pass must not duplicate a child.");
    passed &= CheckVector(first.GetSize(), {40.0f, 30.0f},
                          "A retained keyed child should receive its latest configuration.");
    passed &= Check(parent.FindChildByKey("content") == &first,
                    "A keyed child should be discoverable by its stable identity.");
    return passed;
}

bool TestNestedComponentTree() {
    Panel root;
    auto tree = pipeframe::ui::Component<Column>(
        "shell", [](Column &column) {
            column.SetSize({240.0f, 160.0f});
            column.SetSpacing(8.0f);
        },
        pipeframe::ui::Component<Panel>("header", [](Panel &panel) { panel.SetSize({10.0f, 24.0f}); }),
        pipeframe::ui::Component<Row>(
            "body", [](Row &row) {
                row.SetSizePolicy(SizePolicy::Stretch, SizePolicy::Stretch);
                row.SetSpacing(4.0f);
            },
            pipeframe::ui::Component<Panel>("left", [](Panel &panel) {
                panel.SetSizePolicy(SizePolicy::Stretch, SizePolicy::Stretch);
            }),
            pipeframe::ui::Component<Panel>("right", [](Panel &panel) {
                panel.SetSizePolicy(SizePolicy::Stretch, SizePolicy::Stretch);
            })));

    pipeframe::ui::Compose(root, tree);
    Widget *shell = root.FindChildByKey("shell");
    Widget *body = shell == nullptr ? nullptr : shell->FindChildByKey("body");
    Widget *left = body == nullptr ? nullptr : body->FindChildByKey("left");
    const std::size_t childCount = root.GetChildCount();

    pipeframe::ui::Compose(root, tree);

    bool passed = true;
    passed &= Check(shell != nullptr && body != nullptr && left != nullptr,
                    "A nested component tree should materialize every keyed level.");
    passed &= Check(root.GetChildCount() == childCount && root.FindChildByKey("shell") == shell,
                    "Recomposing a component tree should preserve its stable widget instances.");
    passed &= Check(body->GetChildCount() == 2,
                    "A nested component should own its declared children.");
    return passed;
}

bool TestConditionalComponentReconciliation() {
    Panel root;
    auto expanded = pipeframe::ui::Component<Column>(
        "panel", [](Column &) {},
        pipeframe::ui::Component<Panel>("always", [](Panel &) {}),
        pipeframe::ui::Component<Panel>("details", [](Panel &) {}));
    pipeframe::ui::Compose(root, expanded);

    Widget *panel = root.FindChildByKey("panel");
    Widget *details = panel == nullptr ? nullptr : panel->FindChildByKey("details");

    auto collapsed = pipeframe::ui::Component<Column>(
        "panel", [](Column &) {},
        pipeframe::ui::Component<Panel>("always", [](Panel &) {}));
    pipeframe::ui::Compose(root, collapsed);

    bool passed = true;
    passed &= Check(details != nullptr && !details->IsVisible(),
                    "A conditional keyed child omitted by recomposition should be hidden.");

    pipeframe::ui::Compose(root, expanded);
    passed &= Check(panel->FindChildByKey("details") == details && details->IsVisible(),
                    "Restoring a conditional child should reuse its stable widget instance.");
    return passed;
}

bool TestReconciliationReleasesHiddenInputOwnership() {
    UIManager manager;
    ProbeWidget &root = manager.CreateRoot<ProbeWidget>();
    root.SetSize({100.0f, 100.0f});
    root.SetHitTestVisible(false);

    auto visibleTree = pipeframe::ui::Component<ProbeWidget>(
        "conditional", [](ProbeWidget &probe) {
            probe.SetSize({50.0f, 50.0f});
            probe.SetFocusable(true);
        });
    pipeframe::ui::Compose(root, visibleTree);

    auto *conditional = dynamic_cast<ProbeWidget *>(root.FindChildByKey("conditional"));
    const sf::Event press{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {10, 10}}};
    const sf::Event release{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {10, 10}}};
    manager.HandleEvent(press);

    pipeframe::ui::Compose(root);

    bool passed = true;
    passed &= Check(conditional != nullptr && !conditional->IsVisible(),
                    "The conditional input target should be hidden by reconciliation.");
    passed &= Check(!manager.HandleEvent(release),
                    "A hidden reconciled widget must release pointer capture before the next event.");
    passed &= Check(!manager.HasKeyboardFocus(),
                    "A hidden reconciled widget must release keyboard focus before the next event.");
    passed &= Check(conditional->pointerReleaseCount == 0,
                    "A hidden reconciled widget must not receive a stale pointer release.");
    return passed;
}

bool TestTypedStateBindingLifetime() {
    pipeframe::ui::State<bool> visible{false};
    pipeframe::ui::State<bool> enabled{true, pipeframe::ui::StateLifetime::Application};
    pipeframe::ui::State<float> opacity{0.25f};
    float appliedOpacity = 0.0f;

    bool passed = true;
    passed &= Check(visible.GetLifetime() == pipeframe::ui::StateLifetime::Visual &&
                        enabled.GetLifetime() == pipeframe::ui::StateLifetime::Application,
                    "Application and transient visual state should be explicitly distinguishable.");

    {
        Panel panel;
        pipeframe::ui::BindVisible(panel, visible);
        pipeframe::ui::BindEnabled(panel, enabled);
        pipeframe::ui::BindProperty(
            panel, opacity,
            [&appliedOpacity](Panel &, const float value) { appliedOpacity = value; });

        passed &= Check(!panel.IsVisible(), "A binding should apply its initial visibility value.");
        passed &= Check(panel.IsEnabled(), "A binding should apply its initial enabled value.");
        passed &= Check(NearlyEqual(appliedOpacity, 0.25f),
                        "A typed property binding should apply its initial value.");

        visible.Set(true);
        enabled.Set(false);
        opacity.Set(0.75f);
        passed &= Check(panel.IsVisible(), "Visibility should update only when its state changes.");
        passed &= Check(!panel.IsEnabled(), "Enabled state should update through its typed binding.");
        passed &= Check(NearlyEqual(appliedOpacity, 0.75f),
                        "A generic typed binding should update the configured property.");
        passed &= Check(visible.GetObserverCount() == 1 && enabled.GetObserverCount() == 1 &&
                            opacity.GetObserverCount() == 1,
                        "Each bound property should retain exactly one observer.");
        if (!passed) {
            return false;
        }
    }

    return Check(visible.GetObserverCount() == 0 && enabled.GetObserverCount() == 0 &&
                     opacity.GetObserverCount() == 0,
                 "Destroying a widget must detach all retained state bindings.");
}

bool TestReusableControlBindings() {
    pipeframe::ui::State<bool> checked{true};
    pipeframe::ui::State<float> value{0.25f};
    pipeframe::ui::State<std::size_t> selection{1};

    Toggle toggle;
    Slider slider;
    slider.SetRange(0.0f, 1.0f);
    SegmentedControl segmented;
    segmented.AddSegment();
    segmented.AddSegment();
    segmented.AddSegment();

    pipeframe::ui::BindChecked(toggle, checked);
    pipeframe::ui::BindValue(slider, value);
    pipeframe::ui::BindSelection(segmented, selection);

    bool passed = true;
    passed &= Check(toggle.IsChecked(),
                    "A toggle binding should apply its initial checked state.");
    passed &= Check(NearlyEqual(slider.GetValue(), 0.25f),
                    "A slider binding should apply its initial numeric value.");
    passed &= Check(segmented.GetSelectedIndex() == 1,
                    "A segmented binding should apply its initial selection.");

    checked.Set(false);
    value.Set(0.75f);
    selection.Set(2);
    passed &= Check(!toggle.IsChecked(),
                    "A toggle should react when its bound state changes.");
    passed &= Check(NearlyEqual(slider.GetValue(), 0.75f),
                    "A slider should react when its bound state changes.");
    passed &= Check(segmented.GetSelectedIndex() == 2 &&
                        segmented.GetSegment(2)->IsSelected(),
                    "A segmented control should react when its bound selection changes.");
    return passed;
}

bool TestVerticalFlexLayout() {
    StackPanel panel;

    panel.SetSize({300.0f, 300.0f});
    panel.SetPadding(Thickness{10.0f});
    panel.SetSpacing(10.0f);

    Panel &header = panel.CreateChild<Panel>();
    header.SetSize({100.0f, 40.0f});

    Panel &content = panel.CreateChild<Panel>();
    content.SetSize({100.0f, 1.0f});

    Panel &footer = panel.CreateChild<Panel>();
    footer.SetSize({100.0f, 30.0f});

    panel.SetChildFlex(content, 1.0f);

    bool passed = true;

    passed &= CheckVector(header.GetPosition(), {10.0f, 10.0f}, "Header position is incorrect.");

    passed &= CheckVector(header.GetSize(), {280.0f, 40.0f}, "Header size is incorrect.");

    passed &= CheckVector(content.GetPosition(), {10.0f, 60.0f}, "Flexible content position is incorrect.");

    passed &= CheckVector(content.GetSize(), {280.0f, 190.0f}, "Flexible content should consume remaining height.");

    passed &= CheckVector(footer.GetPosition(), {10.0f, 260.0f}, "Footer position is incorrect.");

    passed &= CheckVector(footer.GetSize(), {280.0f, 30.0f}, "Footer size is incorrect.");

    return passed;
}

bool TestHorizontalWeightedFlex() {
    StackPanel panel;

    panel.SetOrientation(StackOrientation::Horizontal);

    panel.SetSize({400.0f, 100.0f});
    panel.SetPadding(Thickness{10.0f});
    panel.SetSpacing(10.0f);

    Panel &left = panel.CreateChild<Panel>();
    Panel &center = panel.CreateChild<Panel>();
    Panel &right = panel.CreateChild<Panel>();

    panel.SetChildFlex(left, 1.0f);
    panel.SetChildFlex(center, 2.0f);
    panel.SetChildFlex(right, 1.0f);

    bool passed = true;

    passed &= CheckVector(left.GetSize(), {90.0f, 80.0f}, "Left flex size is incorrect.");

    passed &= CheckVector(center.GetSize(), {180.0f, 80.0f}, "Center flex size is incorrect.");

    passed &= CheckVector(right.GetSize(), {90.0f, 80.0f}, "Right flex size is incorrect.");

    passed &= CheckVector(center.GetPosition(), {110.0f, 10.0f}, "Center flex position is incorrect.");

    passed &= CheckVector(right.GetPosition(), {300.0f, 10.0f}, "Right flex position is incorrect.");

    return passed;
}

bool TestAlignment() {
    StackPanel panel;

    panel.SetSize({300.0f, 200.0f});
    panel.SetPadding(Thickness{10.0f});

    panel.SetMainAxisAlignment(MainAxisAlignment::Center);

    panel.SetCrossAxisAlignment(CrossAxisAlignment::Center);

    Panel &child = panel.CreateChild<Panel>();
    child.SetSize({100.0f, 40.0f});

    bool passed = true;

    passed &= CheckVector(child.GetPosition(), {100.0f, 80.0f}, "Centered child position is incorrect.");

    passed &= CheckVector(child.GetSize(), {100.0f, 40.0f}, "Centered child should preserve its size.");

    return passed;
}

bool TestHiddenChildLayout() {
    StackPanel panel;

    panel.SetSize({200.0f, 200.0f});
    panel.SetSpacing(10.0f);

    Panel &first = panel.CreateChild<Panel>();
    first.SetSize({100.0f, 30.0f});

    Panel &hidden = panel.CreateChild<Panel>();
    hidden.SetSize({100.0f, 50.0f});

    Panel &last = panel.CreateChild<Panel>();
    last.SetSize({100.0f, 30.0f});

    hidden.SetVisible(false);

    return CheckVector(last.GetPosition(), {0.0f, 40.0f}, "Hidden child should not consume layout space.");
}

bool TestNestedScreenPosition() {
    Panel root;
    root.SetPosition({20.0f, 30.0f});

    Panel &middle = root.CreateChild<Panel>();
    middle.SetPosition({10.0f, 15.0f});

    Panel &child = middle.CreateChild<Panel>();
    child.SetPosition({5.0f, 7.0f});

    return CheckVector(child.GetScreenPosition(), {35.0f, 52.0f}, "Nested screen position is incorrect.");
}

bool TestScrolling() {
    ScrollPanel scrollPanel;
    scrollPanel.SetSize({200.0f, 100.0f});

    Panel &content = scrollPanel.CreateChild<Panel>();

    content.SetSize({200.0f, 300.0f});

    scrollPanel.SetContent(content);
    scrollPanel.SetScrollOffset(500.0f);

    bool passed = true;

    passed &= Check(NearlyEqual(scrollPanel.GetMaximumScrollOffset(), 200.0f), "Maximum scroll offset is incorrect.");

    passed &= Check(NearlyEqual(scrollPanel.GetScrollOffset(), 200.0f), "Scroll offset should be clamped.");

    passed &= CheckVector(content.GetPosition(), {0.0f, -200.0f}, "Scrolled content position is incorrect.");

    content.SetSize({200.0f, 50.0f});

    passed &=
        Check(NearlyEqual(scrollPanel.GetScrollOffset(), 0.0f), "Scroll offset should reset when content shrinks.");

    return passed;
}

bool TestWorldInputPassThroughAndPointerCapture() {
    UIManager manager;

    ProbeWidget &overlayRoot = manager.CreateRoot<ProbeWidget>();
    overlayRoot.SetSize({200.0f, 200.0f});
    overlayRoot.SetHitTestVisible(false);

    ProbeWidget &button = overlayRoot.CreateChild<ProbeWidget>();
    button.SetPosition({20.0f, 20.0f});
    button.SetSize({50.0f, 50.0f});

    const sf::Event pressInside{sf::Event::MouseButtonPressed{sf::Mouse::Button::Right, {30, 30}}};
    const sf::Event dragOutside{sf::Event::MouseMoved{{150, 150}}};
    const sf::Event releaseOutside{sf::Event::MouseButtonReleased{sf::Mouse::Button::Right, {150, 150}}};
    const sf::Event pressEmpty{sf::Event::MouseButtonPressed{sf::Mouse::Button::Right, {150, 150}}};

    bool passed = true;
    passed &= Check(manager.HandleEvent(pressInside), "UI controls must consume world clicks inside their bounds.");
    passed &= Check(button.pointerPressCount == 1, "The hit child should receive the pointer press.");

    passed &= Check(manager.HandleEvent(dragOutside), "A captured pointer must remain owned while dragged outside.");
    passed &= Check(button.pointerMoveCount == 1, "The captured child should receive pointer movement.");
    passed &= Check(button.pointerExitCount == 1, "Dragging outside should clear the child's hover state.");

    passed &= Check(manager.HandleEvent(releaseOutside), "The captured child must receive the matching release.");
    passed &= Check(button.pointerReleaseCount == 1, "The captured child should receive one pointer release.");

    passed &= Check(!manager.HandleEvent(pressEmpty),
                    "Transparent overlay space must pass right-clicks through to the simulation world.");

    return passed;
}

bool TestTopmostOverlayAndKeyboardFocus() {
    UIManager manager;

    ProbeWidget &worldHud = manager.CreateRoot<ProbeWidget>();
    worldHud.SetSize({100.0f, 100.0f});

    ProbeWidget &popup = manager.CreateRoot<ProbeWidget>();
    popup.SetSize({100.0f, 100.0f});
    popup.SetFocusable(true);

    const sf::Event press{sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, {10, 10}}};
    const sf::Event release{sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, {10, 10}}};
    const sf::Event textEntered{sf::Event::TextEntered{U'a'}};
    const sf::Event focusLost{sf::Event::FocusLost{}};

    bool passed = true;
    passed &= Check(manager.HandleEvent(press), "The topmost overlay should receive pointer input.");
    passed &= Check(manager.HandleEvent(release), "The topmost overlay should receive pointer release.");
    passed &= Check(popup.pointerPressCount == 1, "The last root should be the topmost input target.");
    passed &= Check(worldHud.pointerPressCount == 0, "Input must not leak through an opaque popup.");
    passed &= Check(manager.HasKeyboardFocus(), "Clicking a focusable popup should give it keyboard focus.");
    passed &= Check(manager.HandleEvent(textEntered), "Keyboard input should route to the focused widget.");
    passed &= Check(popup.textEventCount == 1, "The focused popup should receive text input.");

    manager.HandleEvent(focusLost);
    passed &= Check(!manager.HasKeyboardFocus(), "Window focus loss should clear UI keyboard focus.");
    passed &= Check(!manager.HandleEvent(textEntered), "Unfocused keyboard input must remain available to the app.");

    return passed;
}

bool TestLargeLayoutBaseline() {
    constexpr std::size_t ChildCount = 2'000;

    StackPanel panel;
    panel.SetSize({600.0f, static_cast<float>(ChildCount) * 5.0f});
    panel.SetSpacing(1.0f);

    for (std::size_t index = 0; index < ChildCount; ++index) {
        Panel &child = panel.CreateChild<Panel>();
        child.SetSize({1.0f, 4.0f});
    }

    const auto started = std::chrono::steady_clock::now();
    panel.SetSize({800.0f, static_cast<float>(ChildCount) * 5.0f});
    const auto elapsed = std::chrono::steady_clock::now() - started;

    const Widget *last = panel.GetChild(ChildCount - 1);
    const double elapsedMilliseconds = std::chrono::duration<double, std::milli>(elapsed).count();

    std::cout << "UI layout baseline: " << ChildCount << " children relaid out in " << elapsedMilliseconds << " ms.\n";

    bool passed = true;
    passed &= Check(last != nullptr, "The large layout should retain every child.");
    passed &= CheckVector(last->GetPosition(), {0.0f, static_cast<float>(ChildCount - 1) * 5.0f},
                          "The large layout produced an incorrect final child position.");
    passed &= CheckVector(last->GetSize(), {800.0f, 4.0f},
                          "The large layout should stretch the final child across the panel.");

    return passed;
}

bool TestFitContentMeasurement() {
    StackPanel panel;
    panel.SetCrossAxisAlignment(CrossAxisAlignment::Start);
    panel.SetSizePolicy(SizePolicy::FitContent, SizePolicy::FitContent);
    panel.SetPadding(Thickness{10.0f});
    panel.SetSpacing(5.0f);

    Panel &first = panel.CreateChild<Panel>();
    first.SetSize({50.0f, 20.0f});

    Panel &second = panel.CreateChild<Panel>();
    second.SetSize({80.0f, 30.0f});

    bool passed = true;
    passed &= CheckVector(panel.GetSize(), {100.0f, 75.0f},
                          "Fit-content stack should include child extents, padding, and spacing.");
    passed &= CheckVector(second.GetPosition(), {10.0f, 35.0f},
                          "Fit-content stack should arrange children after measurement.");
    return passed;
}

bool TestImplicitStretchPolicy() {
    StackPanel panel;
    panel.SetSize({100.0f, 100.0f});
    panel.SetPadding(Thickness{10.0f});
    panel.SetSpacing(5.0f);

    Panel &header = panel.CreateChild<Panel>();
    header.SetSize({20.0f, 20.0f});

    Panel &content = panel.CreateChild<Panel>();
    content.SetSizePolicy(SizePolicy::Stretch, SizePolicy::Stretch);

    bool passed = true;
    passed &= CheckVector(content.GetPosition(), {10.0f, 35.0f}, "Stretch content position is incorrect.");
    passed &= CheckVector(content.GetSize(), {80.0f, 55.0f},
                          "Stretch policy should consume remaining main- and cross-axis space.");
    return passed;
}

bool TestMinimumAndMaximumConstraints() {
    StackPanel panel;
    panel.SetSize({300.0f, 100.0f});

    Panel &child = panel.CreateChild<Panel>();
    child.SetSize({200.0f, 10.0f});
    child.SetMinimumSize({40.0f, 25.0f});
    child.SetMaximumSize({120.0f, 50.0f});

    bool passed = true;
    passed &= CheckVector(child.GetDesiredSize(), {120.0f, 25.0f},
                          "Measurement should clamp requested size to min/max constraints.");
    passed &= CheckVector(child.GetSize(), {120.0f, 25.0f},
                          "Arrangement should honor min/max constraints even when the parent stretches.");
    return passed;
}

bool TestRequestedSizeSurvivesArrangement() {
    StackPanel panel;
    panel.SetSize({300.0f, 100.0f});

    Panel &child = panel.CreateChild<Panel>();
    child.SetSize({90.0f, 30.0f});

    bool passed = true;
    passed &= CheckVector(child.GetSize(), {300.0f, 30.0f},
                          "Default cross-axis alignment should arrange the child at panel width.");
    passed &= CheckVector(child.GetRequestedSize(), {90.0f, 30.0f},
                          "Parent arrangement must not overwrite a child's requested size.");

    panel.SetCrossAxisAlignment(CrossAxisAlignment::Start);
    passed &= CheckVector(child.GetSize(), {90.0f, 30.0f},
                          "Changing alignment should restore the measured requested width.");
    return passed;
}

bool TestOverlayAlignmentAndPadding() {
    PaddingPanel panel;
    panel.SetSize({200.0f, 120.0f});
    panel.SetPadding(Thickness{10.0f, 20.0f, 30.0f, 10.0f});

    Panel &child = panel.CreateChild<Panel>();
    child.SetSize({40.0f, 30.0f});
    panel.SetChildAlignment(child, {HorizontalAlignment::End, VerticalAlignment::Center});

    bool passed = true;
    passed &= CheckVector(child.GetPosition(), {130.0f, 50.0f},
                          "Overlay alignment should position a fixed child inside padded content.");
    passed &= CheckVector(child.GetSize(), {40.0f, 30.0f},
                          "A non-stretch overlay child should preserve its measured size.");

    Panel &background = panel.CreateChild<Panel>();
    passed &= CheckVector(background.GetPosition(), {10.0f, 20.0f},
                          "The default overlay alignment should begin at the padded origin.");
    passed &= CheckVector(background.GetSize(), {160.0f, 90.0f},
                          "The default overlay alignment should stretch through padded content.");
    return passed;
}

bool TestRowColumnAndSpacerPrimitives() {
    Row row;
    row.SetSize({300.0f, 40.0f});
    row.SetPadding(Thickness{10.0f});
    row.SetSpacing(5.0f);

    Panel &left = row.CreateChild<Panel>();
    left.SetSize({50.0f, 20.0f});
    Spacer &spacer = row.CreateChild<Spacer>();
    Panel &right = row.CreateChild<Panel>();
    right.SetSize({30.0f, 20.0f});

    bool passed = true;
    passed &= CheckVector(left.GetPosition(), {10.0f, 10.0f}, "Row should arrange its first child horizontally.");
    passed &= CheckVector(spacer.GetPosition(), {65.0f, 10.0f}, "Spacer should begin after the first row child.");
    passed &= CheckVector(spacer.GetSize(), {190.0f, 20.0f}, "Spacer should consume the row's remaining width.");
    passed &= CheckVector(right.GetPosition(), {260.0f, 10.0f}, "Row should place the trailing child after the spacer.");

    Column column;
    passed &= Check(column.GetOrientation() == StackOrientation::Vertical,
                    "Column should retain vertical stack orientation.");
    passed &= Check(row.GetOrientation() == StackOrientation::Horizontal,
                    "Row should configure horizontal stack orientation.");
    return passed;
}

} // namespace

int main() {
    bool passed = true;

    passed &= TestDeclarativeBuilder();
    passed &= TestBarrierKeyboardFocusAndTraversal();
    passed &= TestRoundedSurfaceTokens();
    passed &= TestRealTimeButtonMotionAndReducedMotion();
    passed &= TestWidgetLiftFadeAndInheritedOpacity();
    passed &= TestModalBarrierInputOwnership();
    passed &= TestPopupClampingAndInputOwnership();
    passed &= TestToastUsesRealTimeAndPassesInputThrough();
    passed &= TestZenModePreservesEssentialLayerAndState();
    passed &= TestTabViewSelectionAndRetainedPages();
    passed &= TestListViewPointerKeyboardAndBinding();
    passed &= TestProgressAndGaugeRealTimeMotion();
    passed &= TestEdgeDrawerHandleAndMotion();
    passed &= TestTogglePointerAndKeyboardActivation();
    passed &= TestSliderPointerKeyboardAndStep();
    passed &= TestSegmentedSelectionAndKeyboardNavigation();
    passed &= TestButtonKeyboardFocusAndActivation();
    passed &= TestKeyedCompositionRetainsIdentity();
    passed &= TestNestedComponentTree();
    passed &= TestConditionalComponentReconciliation();
    passed &= TestReconciliationReleasesHiddenInputOwnership();
    passed &= TestTypedStateBindingLifetime();
    passed &= TestReusableControlBindings();
    passed &= TestVerticalFlexLayout();
    passed &= TestHorizontalWeightedFlex();
    passed &= TestAlignment();
    passed &= TestHiddenChildLayout();
    passed &= TestNestedScreenPosition();
    passed &= TestScrolling();
    passed &= TestWorldInputPassThroughAndPointerCapture();
    passed &= TestTopmostOverlayAndKeyboardFocus();
    passed &= TestLargeLayoutBaseline();
    passed &= TestFitContentMeasurement();
    passed &= TestImplicitStretchPolicy();
    passed &= TestMinimumAndMaximumConstraints();
    passed &= TestRequestedSizeSurvivesArrangement();
    passed &= TestOverlayAlignmentAndPadding();
    passed &= TestRowColumnAndSpacerPrimitives();

    if (!passed) {
        return 1;
    }

    std::cout << "All UI framework tests passed.\n";

    return 0;
}
