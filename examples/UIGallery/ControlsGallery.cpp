#include "ControlsGallery.h"

#include <PipeFrame/Backend/SFML/UI/CollapsiblePanel.h>
#include <PipeFrame/Backend/SFML/UI/EdgeDrawer.h>
#include <PipeFrame/Backend/SFML/UI/ListView.h>
#include <PipeFrame/Backend/SFML/UI/ModalBarrier.h>
#include <PipeFrame/Backend/SFML/UI/NumericField.h>
#include <PipeFrame/Backend/SFML/UI/PopupLayer.h>
#include <PipeFrame/Backend/SFML/UI/SimulationTransport.h>
#include <PipeFrame/Backend/SFML/UI/SegmentedControl.h>
#include <PipeFrame/Backend/SFML/UI/Toast.h>
#include <PipeFrame/Backend/SFML/UI/Toggle.h>
#include <PipeFrame/Backend/SFML/UI/Tooltip.h>
#include <PipeFrame/Backend/SFML/UI/ZenModeShell.h>
#include <array>
#include <cmath>
#include <iostream>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/View.hpp>

namespace {
void ClearPanel(Panel &panel) {
    panel.SetFillColor(sf::Color::Transparent);
    panel.SetOutlineThickness(0);
    panel.SetHitTestVisible(false);
}
Label &LabelIn(Widget &parent, const sf::Font &font, const std::string &text, float height = 28) {
    auto &label = parent.CreateChild<Label>(font);
    label.SetText(text); label.SetSize({0, height}); label.SetCharacterSize(14);
    return label;
}
TextButton &Action(Widget &parent, const sf::Font &font, const std::string &text, std::function<void()> callback) {
    auto &button = parent.CreateChild<TextButton>(font);
    button.SetText(text); button.SetSize({0, 36}); button.SetOnClick(std::move(callback));
    return button;
}
}

struct ControlsGallery::Impl {
    UIManager ui;
    const sf::Font &font;
    ZenModeShell *shell;
    Column *chrome;
    OverlayPanel *drawerLayer;
    ScrollPanel *drawerClip;
    ScrollPanel *scroll;
    Column *content;
    Column *essential;
    Label *world;
    Label *status;
    CollapsiblePanel *collapse;
    NumericField *numeric;
    ListView *list;
    ScrollPanel *listScroll;
    Toggle *reduced;
    TextButton *zenButton;
    TextButton *popupButton;
    TextButton *modalButton;
    TextButton *toastButton;
    TextButton *tipButton;
    PopupLayer *popup;
    ModalBarrier *modal;
    Column *dialog;
    Toast *toast;
    Tooltip *tooltip;
    SimulationTransport *transport;
    std::array<EdgeDrawer *, 4> drawers{};
    sf::Vector2u size{1100, 800};
    SimulationController controller;
    float accumulator = 0;
    int worldClicks = 0;
    std::function<void()> onMonitoring;

    explicit Impl(const sf::Font &newFont) : font(newFont) {
        shell = &ui.CreateRoot<ZenModeShell>();
        world = &LabelIn(shell->GetPlayfieldLayer(), font, "Playfield: click here in Zen mode", 60);
        world->SetSize({440, 60});
        world->SetAlignment(LabelAlignment::Center);
        shell->GetPlayfieldLayer().SetChildAlignment(*world, {HorizontalAlignment::Center, VerticalAlignment::Center});
        chrome = &shell->GetChromeLayer().CreateChild<Column>();
        chrome->SetFillColor(UITheme::Dark().applicationBackground); chrome->SetOutlineThickness(0);
        chrome->SetPadding(Thickness{24, 24, 24, 232}); chrome->SetSpacing(12);
        auto &title = LabelIn(*chrome, font, "PipeFrame / Controls & surfaces", 38);
        title.SetCharacterSize(24);
        Action(*chrome, font, "Back to charts, network and table", [this] { if (onMonitoring) onMonitoring(); });
        scroll = &chrome->CreateChild<ScrollPanel>(); ClearPanel(*scroll);
        chrome->SetChildFlex(*scroll, 1);
        content = &scroll->CreateChild<Column>(); ClearPanel(*content);
        content->SetSpacing(10); content->SetSizePolicy(SizePolicy::Stretch, SizePolicy::FitContent);
        scroll->SetContent(*content);
        LabelIn(*content, font, "Overlays / Escape closes dialogs / scroll for drawers");
        auto &actions = content->CreateChild<Row>(); ClearPanel(actions); actions.SetSpacing(8); actions.SetSize({0,36});
        popupButton = &Action(actions, font, "Popup", [this] { popup->Open(); });
        modalButton = &Action(actions, font, "Modal", [this] { modal->SetVisible(true); });
        toastButton = &Action(actions, font, "Toast", [this] { toast->Show(2); });
        tipButton = &Action(actions, font, "Tooltip", [this] { ShowTip(); });
        for (Widget *button : {popupButton, modalButton, toastButton, tipButton}) actions.SetChildFlex(*button, 1);
        collapse = &content->CreateChild<CollapsiblePanel>(font);
        collapse->SetCaption("Collapsible settings"); collapse->SetExpandedHeight(140);
        LabelIn(collapse->GetContent(), font, "Numeric field / type a value, then Enter");
        numeric = &collapse->GetContent().CreateChild<NumericField>(font);
        numeric->SetSize({0,36}); numeric->SetValue(12);
        auto &segments = collapse->GetContent().CreateChild<SegmentedControl>();
        segments.SetSize({0,36});
        for (const char *name : {"First", "Second", "Third"}) {
            auto &button = segments.AddSegment();
            auto &label = LabelIn(button, font, name); label.SetSize({100, 32});
        }
        collapse->SetExpanded(true, false);
        LabelIn(*content, font, "Retained list inside a scroll view / select with mouse or arrows");
        listScroll = &content->CreateChild<ScrollPanel>(); listScroll->SetSize({0,116});
        listScroll->SetOutlineThickness(0);
        list = &listScroll->CreateChild<ListView>();
        for (int i = 0; i < 12; ++i) {
            auto &item = list->AddItem();
            auto &label = LabelIn(item, font, "Item " + std::to_string(i + 1)); label.SetSize({260,36});
        }
        list->SetSize({300, 12 * 38.0f - 2}); listScroll->SetContent(*list);
        list->SetOnSelectionChanged([this](std::size_t index) {
            const float top = static_cast<float>(index) * 38;
            if (top < listScroll->GetScrollOffset()) listScroll->SetScrollOffset(top);
            else if (top + 36 > listScroll->GetScrollOffset() + listScroll->GetSize().y)
                listScroll->SetScrollOffset(top + 36 - listScroll->GetSize().y);
        });
        LabelIn(*content, font, "Edge drawers / each handle remains available when closed");
        auto &drawerActions = content->CreateChild<Row>(); ClearPanel(drawerActions);
        drawerActions.SetSize({0,36}); drawerActions.SetSpacing(8);
        const std::array<const char *, 4> names{"Left", "Right", "Top", "Bottom"};
        auto &drawerFrame = content->CreateChild<PaddingPanel>(); ClearPanel(drawerFrame);
        drawerFrame.SetSize({0,240});
        drawerClip = &drawerFrame.CreateChild<ScrollPanel>(); ClearPanel(*drawerClip);
        drawerLayer = &drawerClip->CreateChild<OverlayPanel>(); ClearPanel(*drawerLayer);
        drawerClip->SetContent(*drawerLayer);
        for (std::size_t i = 0; i < 4; ++i) {
            drawers[i] = &drawerLayer->CreateChild<EdgeDrawer>(static_cast<DrawerEdge>(i));
            auto &label = LabelIn(*drawers[i], font, std::string(names[i]) + " drawer");
            label.SetPosition({36,36}); label.SetSize({160,32});
            Action(drawerActions, font, names[i], [this,i] { drawers[i]->Toggle(); });
            drawerActions.SetChildFlex(*drawerActions.GetChild(i), 1);
        }
        essential = &shell->GetEssentialLayer().CreateChild<Column>();
        essential->SetFillColor(UITheme::Dark().applicationBackground); essential->SetOutlineThickness(0);
        essential->SetPadding(Thickness{24,8,24,16}); essential->SetSpacing(8); essential->SetSize({0,216});
        shell->GetEssentialLayer().SetChildAlignment(*essential, {HorizontalAlignment::Stretch, VerticalAlignment::End});
        auto &recovery = essential->CreateChild<Row>(); ClearPanel(recovery); recovery.SetSpacing(12); recovery.SetSize({0,36});
        zenButton = &Action(recovery, font, "Enter Zen", [this] {
            shell->ToggleZenMode(); zenButton->SetText(shell->IsZenMode() ? "Exit Zen" : "Enter Zen");
            if (shell->IsZenMode()) tooltip->Hide();
        });
        recovery.SetChildFlex(*zenButton,1);
        reduced = &recovery.CreateChild<Toggle>(); reduced->SetSize({48,28});
        auto &reducedLabel = LabelIn(recovery,font,"Reduced motion"); reducedLabel.SetSize({150,28});
        reduced->SetOnChanged([this](bool enabled) {
            shell->SetReducedMotion(enabled); popup->SetReducedMotion(enabled);
            modal->SetReducedMotion(enabled); toast->SetReducedMotion(enabled); tooltip->SetReducedMotion(enabled);
        });
        status = &LabelIn(*essential,font, "Paused / ticks 0", 20);
        transport = &essential->CreateChild<SimulationTransport>(font);
        controller.Pause();
        transport->SetOnPlayPause([this] { controller.TogglePlayPause(); Refresh(); });
        transport->SetOnSingleStep([this] { controller.RequestSingleStep(); controller.ConsumeTick(); Refresh(); });
        transport->SetOnReset([this] { controller.ResetTickCount(); accumulator = 0; Refresh(); });
        transport->SetOnSpeedSelected([this](SimulationSpeed speed) { controller.SetSpeed(speed); Refresh(); });
        popup = &ui.CreateRoot<PopupLayer>();
        auto &popupText = LabelIn(popup->GetContent(),font,"Popup / click outside or press Escape");
        popupText.SetPosition({12,12}); popupText.SetSize({330,36});
        auto &popupClose = Action(popup->GetContent(),font,"Close popup",[this] { popup->Dismiss(); });
        popupClose.SetPosition({12,70}); popupClose.SetSize({240,36});
        modal = &ui.CreateRoot<ModalBarrier>(); modal->SetVisible(false);
        modal->SetOnDismiss([this] { modal->SetVisible(false); });
        dialog = &modal->CreateChild<Column>(); dialog->SetFillColor(UITheme::Dark().floatingSurface);
        dialog->SetPadding(Thickness{12}); dialog->SetSpacing(12);
        LabelIn(*dialog,font,"Modal / background tools are blocked",36);
        Action(*dialog,font,"Close modal",[this] { modal->SetVisible(false); });
        toast = &ui.CreateRoot<Toast>();
        auto &toastText = LabelIn(*toast,font,"Saved / this toast does not capture input"); toastText.SetSize({340,44});
        tooltip = &ui.CreateRoot<Tooltip>(font); tooltip->SetText("Delayed tooltip / pointer input passes through");
        tooltip->SetSize({360,44});
        Refresh(); Resize(size);
    }
    void Refresh() {
        transport->SetSimulationState(controller.IsPlaying(),controller.GetTickCount()>0,controller.GetSpeed());
        status->SetText(std::string(controller.IsPlaying() ? "Playing" : "Paused") + " / ticks " +
                       std::to_string(controller.GetTickCount()) + " / " + controller.GetSpeedName());
    }
    void Resize(sf::Vector2u value) {
        size = value; const auto s = sf::Vector2f(size); shell->SetSize(s);
        const auto measured = content->Measure({{0,0},{scroll->GetSize().x,std::numeric_limits<float>::infinity()}});
        content->SetSize({scroll->GetSize().x,measured.y});
        drawerLayer->SetSize(drawerClip->GetSize());
        list->SetSize({listScroll->GetSize().x,12*38.0f-2});
        // Only the overlay anchors use viewport coordinates; their contents use engine layout.
        for (std::size_t i=0;i<4;++i) {
            auto &layer = *drawerLayer;
            layer.SetChildAlignment(*drawers[i],{HorizontalAlignment::Start,VerticalAlignment::Start});
            drawers[i]->SetSize(i<2 ? sf::Vector2f{220,200} : sf::Vector2f{260,120});
        }
        // Use alignment/padding containers for edge anchors rather than manual child positioning.
        drawerLayer->SetChildAlignment(*drawers[0],{HorizontalAlignment::Start,VerticalAlignment::Center});
        drawerLayer->SetChildAlignment(*drawers[1],{HorizontalAlignment::End,VerticalAlignment::Center});
        drawerLayer->SetChildAlignment(*drawers[2],{HorizontalAlignment::Center,VerticalAlignment::Start});
        drawerLayer->SetChildAlignment(*drawers[3],{HorizontalAlignment::Center,VerticalAlignment::End});
        popup->SetSize(s); popup->SetContentBounds({{(s.x-360)*0.5f,120},{360,130}});
        modal->SetSize(s); dialog->SetSize({std::min(380.0f,s.x),140});
        dialog->SetPosition({std::max(0.0f,(s.x-dialog->GetSize().x)*0.5f),120});
        toast->SetPosition({std::max(0.0f,(s.x-360)*0.5f),std::max(0.0f,s.y-280)}); toast->SetSize({360,44});
    }
    void ShowTip() { tooltip->ShowAt(tipButton->GetScreenPosition()+sf::Vector2f{0,40},{{0,0},sf::Vector2f(size)}); }
    void Update(float delta) {
        ui.Update(delta);
        const auto desired = content->Measure({{0,0},{scroll->GetSize().x,std::numeric_limits<float>::infinity()}});
        if (desired.y != content->GetSize().y) content->SetSize({scroll->GetSize().x,desired.y});
        // Demonstration policy: max runs up to 32 ticks per displayed frame.
        if (controller.IsPlaying()) {
            accumulator += delta * (controller.IsFullSpeed() ? 32.0f : controller.GetTimeScale());
            int budget = 32;
            while (accumulator >= 1.0f/60 && budget-- > 0) { controller.ConsumeTick(); accumulator -= 1.0f/60; }
            accumulator = std::min(accumulator,1.0f);
            Refresh();
        }
    }
    void HandleEvent(const sf::Event &event) {
        const bool consumed = ui.HandleEvent(event);
        if (const auto *move=event.getIf<sf::Event::MouseMoved>(); move && !popup->IsVisible() && !modal->IsVisible()) {
            if (!shell->IsZenMode() && tipButton->Contains(sf::Vector2f(move->position))) {
                if (!tooltip->IsVisible() && !tooltip->IsOpening()) ShowTip();
            } else tooltip->Hide();
        }
        if (!consumed && event.is<sf::Event::MouseButtonPressed>()) {
            world->SetText("Playfield clicks: "+std::to_string(++worldClicks));
        }
    }
    void Click(Widget &widget) {
        const auto point = widget.GetScreenPosition()+widget.GetSize()*0.5f;
        const sf::Vector2i pixel{static_cast<int>(point.x),static_cast<int>(point.y)};
        HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,pixel});
        HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,pixel});
    }
};

ControlsGallery::ControlsGallery(const sf::Font &font) : impl(std::make_unique<Impl>(font)) {}
ControlsGallery::~ControlsGallery() = default;
void ControlsGallery::Resize(sf::Vector2u size) { impl->Resize(size); }
void ControlsGallery::Update(float delta) { impl->Update(delta); }
void ControlsGallery::HandleEvent(const sf::Event &event) { impl->HandleEvent(event); }
void ControlsGallery::Render(sf::RenderTarget &target) { impl->ui.Render(target); }
void ControlsGallery::SetOnMonitoring(std::function<void()> callback) { impl->onMonitoring = std::move(callback); }
void ControlsGallery::Present(const std::string &scene) {
    if (scene=="zen") impl->Click(*impl->zenButton);
    else if (scene=="popup") impl->popup->Open();
    else if (scene=="modal") impl->modal->SetVisible(true);
    else if (scene=="toast") impl->toast->Show(3);
    else if (scene=="tooltip") impl->ShowTip();
    else if (scene=="drawers") {
        impl->scroll->SetScrollOffset(impl->scroll->GetMaximumScrollOffset());
        impl->drawers[0]->Open(false);
    }
}

bool ControlsGallery::CheckInteractions() {
    auto &g=*impl; bool passed=true;
    const auto check=[&](bool value,const char *message) { if(!value) { std::cerr<<"FAILED: "<<message<<'\n'; passed=false; } };
    g.Resize({1100,800});
    g.Click(*g.popupButton); g.ui.Update(0.3f);
    check(g.popup->IsOpen(),"Gallery popup action must open its layer");
    g.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Escape}); g.ui.Update(0.3f); g.ui.Update(0.01f);
    check(!g.popup->IsVisible(),"Escape must dismiss a popup without a preliminary click");
    g.Click(*g.modalButton);
    const sf::Vector2i inside{static_cast<int>(g.dialog->GetScreenPosition().x+20),static_cast<int>(g.dialog->GetScreenPosition().y+20)};
    g.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,inside});
    g.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,inside});
    check(g.modal->IsVisible(),"Inside modal clicks must not dismiss it");
    g.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Escape});
    check(!g.modal->IsVisible(),"Escape must dismiss the modal");
    g.Click(*g.toastButton); check(g.toast->IsPresented(),"Toast action must present feedback");
    g.ui.Update(3); g.ui.Update(0.3f); check(!g.toast->IsVisible(),"Toast must expire in real UI time");
    g.ShowTip(); g.ui.Update(0.6f); g.ui.Update(0.3f); check(g.tooltip->IsVisible(),"Tooltip must appear after its delay");
    g.tooltip->Hide(); g.ui.Update(0.3f);
    const float openHeight=g.collapse->GetSize().y;
    g.Click(g.collapse->GetHeader()); g.ui.Update(0.04f);
    check(g.collapse->GetSize().y < openHeight && g.collapse->GetSize().y > g.collapse->GetHeader().GetSize().y,
          "Collapse must interpolate height using real UI time");
    check(!g.collapse->GetContent().GetParent()->IsEnabled(),"Closing content must stop accepting input immediately");
    g.ui.Update(0.3f);
    check(!g.collapse->IsExpanded() && !g.collapse->GetContent().GetParent()->IsVisible(),"Collapse must hide its retained body");
    g.Click(g.collapse->GetHeader()); g.ui.Update(0.3f);
    check(g.collapse->IsExpanded(),"Collapse header must reopen the panel");
    g.Click(*g.numeric); g.HandleEvent(sf::Event::TextEntered{U'7'});
    g.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Enter});
    check(g.numeric->GetValue()==7,"Numeric field must accept and commit keyboard input");
    g.Click(*g.list->GetItem(0));
    g.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::End});
    check(g.list->GetSelectedIndex()==11 && g.listScroll->GetScrollOffset()>0,"List End must select and reveal the final item");
    g.HandleEvent(sf::Event::KeyPressed{sf::Keyboard::Key::Home});
    check(g.list->GetSelectedIndex()==0 && g.listScroll->GetScrollOffset()==0,"List Home must return to the first item");
    g.Click(*g.reduced); check(g.shell->IsReducedMotion(),"Reduced-motion toggle must reach the shell");
    g.scroll->SetScrollOffset(g.scroll->GetMaximumScrollOffset());
    for(auto *drawer:g.drawers) {
        g.Click(drawer->GetHandle()); check(drawer->IsOpen() && drawer->GetVisualOffset()==sf::Vector2f{},"Every drawer handle must open with reduced-motion snapping");
        g.Click(drawer->GetHandle()); check(!drawer->IsOpen(),"Every drawer handle must close");
    }
    g.scroll->SetScrollOffset(0);
    // Locate public view keys, not the retained adapter's private child indexes.
    const std::function<Widget *(Widget &,const std::string &)> findView=[&](Widget &root,const std::string &key)->Widget * {
        if(root.GetKey()==key)return &root;
        for(std::size_t i=0;i<root.GetChildCount();++i)if(auto *found=findView(*root.GetChild(i),key))return found;
        return nullptr;
    };
    const auto transportAction=[&](const std::string &key)->Widget & {
        g.ui.Update(0);
        auto *found=findView(*g.transport,key);
        if(!found)throw std::runtime_error("Missing transport action: "+key);
        return *found;
    };
    g.Click(transportAction("step")); check(g.controller.GetTickCount()==1,"Step must advance exactly one tick");
    g.Click(transportAction("reset")); check(g.controller.GetTickCount()==0,"Reset must clear the preview");
    for(std::size_t i=0;i<4;++i) { g.Click(transportAction("speed:"+std::to_string(i))); check(g.controller.GetSpeed()==static_cast<SimulationSpeed>(i),"Every transport speed must be wired"); }
    g.Click(transportAction("play")); g.Update(0.1f); check(g.controller.GetTickCount()>0,"Play must advance the demonstration");
    g.Click(transportAction("play")); check(g.controller.IsPaused(),"Pause must stop the demonstration");
    g.Click(*g.zenButton); check(g.shell->IsZenMode() && !g.shell->GetChromeLayer().IsVisible(),"Zen must hide optional chrome");
    g.HandleEvent(sf::Event::MouseButtonPressed{sf::Mouse::Button::Left,{500,300}});
    g.HandleEvent(sf::Event::MouseButtonReleased{sf::Mouse::Button::Left,{500,300}});
    check(g.worldClicks==1,"Zen playfield must retain world input");
    g.Click(*g.zenButton); check(!g.shell->IsZenMode(),"Visible recovery control must exit Zen");
    // A scaled view must clip scroll children in target pixels, not logical coordinates.
    ScrollPanel clipped;
    clipped.SetPosition({20,20}); clipped.SetSize({60,40}); ClearPanel(clipped);
    auto &oversized=clipped.CreateChild<Panel>(); oversized.SetSize({120,120});
    oversized.SetFillColor(sf::Color::Green); oversized.SetOutlineThickness(0); clipped.SetContent(oversized);
    sf::RenderTexture target({200,160}); target.setView(sf::View(sf::FloatRect{{0,0},{100,80}}));
    target.clear(sf::Color::Black); clipped.Render(target); target.display();
    const auto pixels=target.getTexture().copyToImage();
    check(pixels.getPixel({140,100})==sf::Color::Green && pixels.getPixel({170,100})==sf::Color::Black,
          "Scroll view clipping must scale with the render view");
    if(passed) std::cout<<"All gallery acceptance interactions passed.\n";
    return passed;
}
