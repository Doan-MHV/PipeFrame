#ifndef PIPEFRAME_NATIVE_SIMULATION_DASHBOARD_H
#define PIPEFRAME_NATIVE_SIMULATION_DASHBOARD_H

#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>

#include <PipeFrame/Backend/SFML/UI/EdgeDrawer.h>
#include <PipeFrame/UI/View.h>
#include <PipeFrame/Backend/SFML/ViewPanelHost.h>
#include <PipeFrame/Backend/SFML/UI/Label.h>
#include <PipeFrame/Backend/SFML/UI/MetricCard.h>
#include <PipeFrame/Backend/SFML/UI/ScrollPanel.h>
#include <PipeFrame/Backend/SFML/UI/TabView.h>
#include <PipeFrame/Backend/SFML/UI/TextButton.h>
#include <PipeFrame/Backend/SFML/UI/UIManager.h>
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/GraphicsResourceAccess.h>
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <vector>

#include <PipeFrame/UI/DrawerTypes.h>

// Shared viewport-local drawer and scrolling pages. Projects own data and commands.
class NativeSimulationDashboard {
  public:
    NativeSimulationDashboard(const pipeframe::GraphicsResourceService &resources,pipeframe::FontHandle font,
                        const std::string &title,const UITheme &theme=UITheme::Dark())
        :NativeSimulationDashboard(*pipeframe::backend::sfml::GraphicsResourceAccess::Font(resources,font),title,theme) {}
    void Layout(const RenderContext &context){Layout(sf::FloatRect(pipeframe::backend::sfml::ViewportBounds(context)));}
    void Render(RenderContext &context){Render(pipeframe::backend::sfml::GetTarget(context));}
    bool ContainsPoint(pipeframe::Vector2i point) const {return Contains({static_cast<float>(point.x),static_cast<float>(point.y)});}
    bool HandleEvent(const pipeframe::InputEvent &event) {
        const auto translated=pipeframe::backend::sfml::ToBackend(event);
        return translated && HandleEvent(*translated);
    }
    explicit NativeSimulationDashboard(const sf::Font &font, const std::string &title,
                                 const UITheme &newTheme = UITheme::Dark())
        : font(font), theme(newTheme) {
        clip = &ui.CreateRoot<ScrollPanel>();
        Clear(*clip);
        clip->SetHitTestVisible(false);
        drawer = &clip->CreateChild<EdgeDrawer>(DrawerEdge::Left, theme);
        drawer->SetHandleExtent(28);
        drawer->SetHandleLength(76);
        drawer->Open(false);
        auto &handleLabel = drawer->GetHandle().CreateChild<Label>(font);
        handleLabel.SetText("UI");
        handleLabel.SetCharacterSize(11);
        handleLabel.SetAlignment(LabelAlignment::Center);
        handle = &handleLabel;
        frame = &drawer->CreateChild<Column>();
        Clear(*frame);
        frame->SetPadding(Thickness{10});
        frame->SetSpacing(8);
        auto &heading = Text(*frame, title, 28);
        heading.SetCharacterSize(theme.headingTextSize);
        heading.SetColor(theme.textPrimary);
        tabs = &frame->CreateChild<TabView>();
        frame->SetChildFlex(*tabs, 1);
        drawer->SetOnOpenChanged([this](bool open) {
            frame->SetVisible(open);
            if (!open)
                ui.HandleEvent(sf::Event::FocusLost{});
        });
    }
    void AddViewDrawer(DrawerEdge edge,const std::string &title,float width,float height,DrawerAnchor anchor,
                       std::function<pipeframe::ui::View()> builder) {
        auto &content=AddIndependentDrawer(edge,title,width,height,anchor,false);
        auto &view=content.CreateChild<pipeframe::ui::ViewBuilderPanel>(font,std::move(builder),theme);
        view.SetOutlineThickness(0); view.SetFillColor(sf::Color::Transparent);
        // The mounted view owns scrolling within the fixed drawer viewport.
        view.SetSize({0,std::max(0.0f,height-64)});
        viewPanels.push_back({&view,independentDrawers.size()-1});
    }
    void AddViewMetric(float width,float height,std::function<pipeframe::ui::View()> builder) {
        auto &view=clip->CreateChild<pipeframe::ui::ViewBuilderPanel>(font,std::move(builder),theme);
        view.SetOutlineThickness(0); view.SetFillColor(sf::Color::Transparent);
        view.SetSize({width,height}); view.SetEnabled(false); view.SetHitTestVisible(false);
        metricViews.push_back(&view);
    }
    void InvalidateViews() {
        for (auto &entry:viewPanels) entry.first->InvalidateView();
        for (auto *view:metricViews) view->InvalidateView();
    }
    Widget &RenderView(Widget &parent, const pipeframe::ui::View &view);
    Column &AddPage(const std::string &title) {
        auto &scroll = tabs->AddPage<ScrollPanel>();
        Clear(scroll);
        auto &content = scroll.CreateChild<Column>();
        Clear(content);
        content.SetSpacing(8);
        content.SetPadding(Thickness{4});
        content.SetSizePolicy(SizePolicy::Stretch, SizePolicy::FitContent);
        scroll.SetContent(content);
        auto &label = tabs->GetTab(tabs->GetPageCount() - 1)->CreateChild<Label>(font);
        label.SetText(title);
        label.SetCharacterSize(11);
        label.SetAlignment(LabelAlignment::Center);
        pages.push_back({&scroll, &content, &label});
        return content;
    }
    Label &Text(Widget &parent, const std::string &caption, float height = 28) {
        auto &label = parent.CreateChild<Label>(font);
        label.SetText(caption);
        label.SetCharacterSize(12);
        label.SetWrap(true);
        label.SetSize({0, height});
        label.SetColor(theme.textPrimary);
        return label;
    }
    TextButton &Action(Widget &parent, const std::string &caption, std::function<void()> action) {
        auto &button = parent.CreateChild<TextButton>(font);
        button.SetText(caption);
        button.SetTextCharacterSize(12);
        button.SetSize({0, 34});
        button.SetNormalColor(theme.controlNormal);
        button.SetHoveredColor(theme.controlHovered);
        button.SetPressedColor(theme.controlPressed);
        button.SetSelectedColor(theme.controlSelected);
        button.SetOutlineColor(theme.border);
        button.SetCornerRadius(theme.radiusMedium);
        button.SetOnClick(std::move(action));
        return button;
    }
    MetricCard &Metric(Widget &parent, const std::string &title) {
        auto &card = parent.CreateChild<MetricCard>(font, theme);
        card.SetTitle(title);
        card.SetSize({0, 88});
        return card;
    }
    MetricCard &AddPersistentMetric(const std::string &title) {
        return AddFloatingMetric(title, 240.0f, 76.0f, DrawerAnchor::Center, DrawerAnchor::Start);
    }
    MetricCard &AddFloatingMetric(const std::string &title, float width, float height,
                                  DrawerAnchor horizontal, DrawerAnchor vertical) {
        auto &card = clip->CreateChild<MetricCard>(font, theme);
        card.SetTitle(title);
        card.SetSize({width, height});
        card.SetHitTestVisible(false);
        card.SetEnabled(false);
        persistentMetrics.push_back({&card, width, height, horizontal, vertical});
        return card;
    }
    Column &AddIndependentDrawer(DrawerEdge edge, const std::string &handleText,
                                 float extent = 300.0f, float span = 0.0f,
                                 DrawerAnchor anchor = DrawerAnchor::Center,
                                 bool initiallyOpen = false) {
        auto &independent = clip->CreateChild<EdgeDrawer>(edge, theme);
        independent.SetHandleExtent(28.0f);
        const float handleLength = std::clamp(48.0f + handleText.size() * 5.0f, 68.0f, 132.0f);
        independent.SetHandleLength(handleLength);
        auto &handleLabel = independent.GetHandle().CreateChild<Label>(font);
        independent.GetHandle().SetNormalColor(sf::Color{242, 242, 238, 235});
        independent.GetHandle().SetHoveredColor(sf::Color{255, 255, 252, 250});
        independent.GetHandle().SetPressedColor(sf::Color{216, 218, 214, 245});
        independent.GetHandle().SetSelectedColor(sf::Color{245, 245, 240, 245});
        independent.GetHandle().SetOutlineColor(sf::Color{255, 255, 255, 180});
        handleLabel.SetText(handleText);
        handleLabel.SetCharacterSize(10);
        handleLabel.SetColor(sf::Color{54, 58, 64});
        handleLabel.SetAlignment(LabelAlignment::Center);
        if (edge == DrawerEdge::Left)
            handleLabel.SetRotation(90.0f);
        else if (edge == DrawerEdge::Right)
            handleLabel.SetRotation(-90.0f);
        auto &scroll = independent.CreateChild<ScrollPanel>();
        Clear(scroll);
        scroll.SetVisible(false);
        auto &content = scroll.CreateChild<Column>();
        Clear(content);
        content.SetPadding(Thickness{10});
        content.SetSpacing(8);
        content.SetSizePolicy(SizePolicy::Stretch, SizePolicy::FitContent);
        scroll.SetContent(content);
        content.SetVisible(false);
        auto &popupHeader = content.CreateChild<Row>();
        Clear(popupHeader);
        popupHeader.SetSize({0.0f, 34.0f});
        popupHeader.SetSpacing(8.0f);
        auto &popupTitle = popupHeader.CreateChild<Label>(font);
        popupTitle.SetText(handleText);
        popupTitle.SetCharacterSize(12);
        popupTitle.SetColor(theme.textSecondary);
        popupTitle.SetAlignment(LabelAlignment::Left);
        popupHeader.SetChildFlex(popupTitle, 1.0f);
        auto &closeButton = popupHeader.CreateChild<TextButton>(font);
        closeButton.SetText("CLOSE");
        closeButton.SetTextCharacterSize(10);
        closeButton.SetSize({64.0f, 30.0f});
        closeButton.SetNormalColor(theme.controlNormal);
        closeButton.SetHoveredColor(theme.controlHovered);
        closeButton.SetPressedColor(theme.controlPressed);
        closeButton.SetOutlineColor(theme.border);
        closeButton.SetCornerRadius(theme.radiusSmall);
        closeButton.SetOnClick([&independent] { independent.Close(); });
        independent.SetOnOpenChanged([this, &independent, &scroll, &content](const bool open) {
            scroll.SetVisible(open);
            content.SetVisible(open);
            if (open) {
                for (auto &entry : independentDrawers) {
                    if (entry.drawer == &independent || !entry.drawer->IsOpen())
                        continue;
                    // Hidden peer drawers must settle immediately. Otherwise
                    // their restored handles can move between press and release
                    // and cancel a valid click.
                    entry.drawer->Close(false);
                }
                // A popup is a modal dashboard surface.  Hide every collapsed
                // tab until it closes so tabs never paint over the panel.
                independent.GetHandle().SetVisible(false);
                for (auto &entry : independentDrawers)
                    entry.drawer->GetHandle().SetVisible(false);
            } else {
                const bool anotherPopupOpen = std::any_of(
                    independentDrawers.begin(), independentDrawers.end(),
                    [&independent](const IndependentDrawer &entry) {
                        return entry.drawer != &independent && entry.drawer->IsOpen();
                    });
                if (!anotherPopupOpen) {
                    independent.GetHandle().SetVisible(true);
                    for (auto &entry : independentDrawers)
                        entry.drawer->GetHandle().SetVisible(true);
                }
            }
            if (!open) {
                ui.HandleEvent(sf::Event::FocusLost{});
            }
        });
        independent.SetOpen(initiallyOpen, false);
        content.SetVisible(initiallyOpen);
        independentDrawers.push_back(
            {&independent, &scroll, &content, &handleLabel, extent, span, handleLength, anchor});
        return content;
    }
    void SetTabbedDrawerVisible(const bool shown) {
        tabbedDrawerVisible = shown;
        drawer->SetVisible(shown && visible);
    }
    void SetTabbedDrawerGeometry(float width, float height, DrawerAnchor anchor = DrawerAnchor::Center) {
        tabbedDrawerWidth = width;
        tabbedDrawerHeight = height;
        tabbedDrawerAnchor = anchor;
    }
    void Layout(sf::FloatRect viewport) {
        clip->Arrange(viewport);
        const float uiScale = std::clamp(viewport.size.y / 1200.0f, 0.6f, 1.5f);
        const float drawerWidth = std::max(0.f, std::min(tabbedDrawerWidth * uiScale, viewport.size.x - 40.f));
        const float drawerHeight = std::max(0.f, std::min(tabbedDrawerHeight * uiScale, viewport.size.y));
        const float drawerY = AnchorPosition(viewport.size.y, drawerHeight, tabbedDrawerAnchor);
        drawer->Arrange({{0.0f, drawerY}, {drawerWidth, drawerHeight}});
        frame->Arrange({{}, {std::max(0.f, drawer->GetSize().x - 28.f), drawer->GetSize().y}});
        handle->Arrange({{}, drawer->GetHandle().GetSize()});
        for (auto &page : pages) {
            page.label->Arrange({{}, page.label->GetParent()->GetSize()});
            const auto desired = page.content->Measure(
                {{page.scroll->GetSize().x, 0}, {page.scroll->GetSize().x, std::numeric_limits<float>::infinity()}});
            page.content->SetSize({page.scroll->GetSize().x, desired.y});
        }

        for (const auto &entry : persistentMetrics) {
            const float width = std::max(160.0f, entry.width * uiScale);
            const float height = std::max(72.0f, entry.height * uiScale);
            const float metricX = AnchorPosition(viewport.size.x, width, entry.horizontal);
            const float metricY = AnchorPosition(viewport.size.y, height, entry.vertical);
            entry.metric->Arrange({{metricX, metricY}, {width, height}});
        }

        const bool popupOpen=std::ranges::any_of(independentDrawers,[](const auto &entry){return entry.drawer->IsOpen();});
        for (auto *view:metricViews) {
            view->SetVisible(visible&&!popupOpen);
            view->SetPosition({std::max(0.0f,(viewport.size.x-view->GetSize().x)*0.5f),8});
        }
        std::array<std::size_t, 4> edgeCounts{};
        std::array<std::size_t, 4> edgeSlots{};
        for (const auto &entry : independentDrawers)
            ++edgeCounts[static_cast<std::size_t>(entry.drawer->GetEdge())];

        for (auto &entry : independentDrawers) {
            const float interactiveScale = std::max(1.0f, uiScale);
            entry.drawer->SetHandleExtent(28.0f * interactiveScale);
            entry.drawer->SetHandleLength(entry.handleLength * interactiveScale);
            const bool vertical = entry.drawer->GetEdge() == DrawerEdge::Left ||
                                  entry.drawer->GetEdge() == DrawerEdge::Right;
            // Popup contents are authored at their declared extent.  Shrinking
            // that extent made fixed-height rows wrap into one another; on a
            // small viewport the popup may cover more of the world instead.
            const float scaledExtent = entry.extent * std::max(1.0f, uiScale);
            const float primary = std::max(40.0f, std::min(scaledExtent,
                (vertical ? viewport.size.x : viewport.size.y) - 20.0f));
            const float availableSpan = vertical ? viewport.size.y : viewport.size.x;
            const float requestedSpan = entry.span > 0.0f
                ? entry.span * (vertical ? 1.0f : std::max(1.0f, uiScale))
                : availableSpan;
            const float cross = std::max(40.0f, std::min(requestedSpan, availableSpan));
            const sf::Vector2f drawerSize = vertical ? sf::Vector2f{primary, cross}
                                                     : sf::Vector2f{cross, primary};
            sf::Vector2f position{};
            if (vertical) {
                position.y = AnchorPosition(viewport.size.y, drawerSize.y, entry.anchor);
                if (entry.drawer->GetEdge() == DrawerEdge::Right)
                    position.x = viewport.size.x - drawerSize.x;
            } else {
                position.x = AnchorPosition(viewport.size.x, drawerSize.x, entry.anchor);
                if (entry.drawer->GetEdge() == DrawerEdge::Bottom)
                    position.y = viewport.size.y - drawerSize.y;
            }
            entry.drawer->Arrange({position, drawerSize});
            const auto edgeIndex = static_cast<std::size_t>(entry.drawer->GetEdge());
            const auto slot = edgeSlots[edgeIndex]++;
            auto handlePosition = entry.drawer->GetHandle().GetPosition();
            const auto handleSize = entry.drawer->GetHandle().GetSize();
            if (vertical) {
                const float center = viewport.size.y * static_cast<float>(slot + 1) /
                                     static_cast<float>(edgeCounts[edgeIndex] + 1);
                handlePosition.y = center - position.y - handleSize.y * 0.5f;
            } else {
                const float center = viewport.size.x * static_cast<float>(slot + 1) /
                                     static_cast<float>(edgeCounts[edgeIndex] + 1);
                handlePosition.x = center - position.x - handleSize.x * 0.5f;
            }
            entry.drawer->GetHandle().SetPosition(handlePosition);
            sf::Vector2f contentSize = drawerSize;
            entry.scroll->Arrange({{}, contentSize});
            for (auto &[view,index]:viewPanels) if (independentDrawers[index].drawer==entry.drawer)
                view->SetSize({std::max(0.0f,contentSize.x-20),std::max(0.0f,contentSize.y-64)});
            const auto desired = entry.content->Measure(
                {{contentSize.x, 0.0f}, {contentSize.x, std::numeric_limits<float>::infinity()}});
            entry.content->SetSize({contentSize.x, desired.y});
            entry.handleLabel->Arrange({{}, entry.drawer->GetHandle().GetSize()});
        }
    }
    void SetVisible(bool newVisible) {
        visible = newVisible;
        clip->SetVisible(newVisible);
        drawer->SetVisible(newVisible && tabbedDrawerVisible);
        for (const auto &entry : persistentMetrics) {
            entry.metric->SetVisible(newVisible);
        }
        for (auto &entry : independentDrawers) {
            entry.drawer->SetVisible(newVisible);
        }
    }
    bool IsVisible() const { return visible; }
    bool Contains(sf::Vector2f point) const {
        if (!IsVisible()) {
            return false;
        }
        if (drawer->IsVisible() &&
            (drawer->GetBounds().contains(point) || drawer->GetHandle().GetBounds().contains(point))) {
            return true;
        }
        return std::any_of(independentDrawers.begin(), independentDrawers.end(),
                           [point](const IndependentDrawer &entry) {
                               return entry.drawer->IsVisible() &&
                                      ((entry.drawer->IsOpen() && entry.drawer->GetBounds().contains(point)) ||
                                       (entry.drawer->GetHandle().IsVisible() &&
                                        entry.drawer->GetHandle().GetBounds().contains(point)));
                           });
    }
    bool HasKeyboardFocus() const { return IsVisible() && ui.HasKeyboardFocus(); }
    bool HandleEvent(const sf::Event &event) { return ui.HandleEvent(event); }
    void Update(float seconds) { ui.Update(seconds); }
    float GetFrameTimeMs() const { return frameTimeMs; }
    void Render(sf::RenderTarget &target) {
        const auto now = std::chrono::steady_clock::now();
        const float elapsed = std::chrono::duration<float>(now - lastFrame).count();
        frameTimeMs = frameTimeMs == 0 ? elapsed * 1000 : frameTimeMs * 0.9f + elapsed * 100;
        ui.Update(std::clamp(elapsed, 0.f, 0.1f));
        lastFrame = now;
        ui.Render(target);
    }
    Widget &Root() { return *clip; }
    TabView &Tabs() { return *tabs; }
    EdgeDrawer &Drawer() { return *drawer; }
    std::size_t GetPersistentMetricCount() const { return persistentMetrics.size(); }
    MetricCard &GetPersistentMetric(std::size_t index) { return *persistentMetrics.at(index).metric; }
    std::size_t GetIndependentDrawerCount() const { return independentDrawers.size(); }
    EdgeDrawer &GetIndependentDrawer(std::size_t index) { return *independentDrawers.at(index).drawer; }
    static void Clear(Panel &panel) {
        panel.SetFillColor(sf::Color::Transparent);
        panel.SetOutlineThickness(0);
    }

  private:
    struct Page {
        ScrollPanel *scroll;
        Column *content;
        Label *label;
    };
    struct PersistentMetric {
        MetricCard *metric;
        float width;
        float height;
        DrawerAnchor horizontal;
        DrawerAnchor vertical;
    };
    struct IndependentDrawer {
        EdgeDrawer *drawer;
        ScrollPanel *scroll;
        Column *content;
        Label *handleLabel;
        float extent;
        float span;
        float handleLength;
        DrawerAnchor anchor;
    };
    static float AnchorPosition(float available, float occupied, DrawerAnchor anchor) {
        if (anchor == DrawerAnchor::Start)
            return 8.0f;
        if (anchor == DrawerAnchor::End)
            return std::max(0.0f, available - occupied - 8.0f);
        return std::max(0.0f, (available - occupied) * 0.5f);
    }
    std::vector<std::pair<pipeframe::ui::ViewBuilderPanel *,std::size_t>> viewPanels;
    std::vector<pipeframe::ui::ViewBuilderPanel *> metricViews;
    const sf::Font &font;
    UITheme theme;
    UIManager ui;
    ScrollPanel *clip;
    EdgeDrawer *drawer;
    Column *frame;
    TabView *tabs;
    Label *handle;
    std::vector<Page> pages;
    std::vector<PersistentMetric> persistentMetrics;
    std::vector<IndependentDrawer> independentDrawers;
    bool tabbedDrawerVisible = true;
    float tabbedDrawerWidth = 310.0f;
    float tabbedDrawerHeight = 620.0f;
    DrawerAnchor tabbedDrawerAnchor = DrawerAnchor::Center;
    bool visible = true;
    float frameTimeMs = 0;
    std::chrono::steady_clock::time_point lastFrame = std::chrono::steady_clock::now();
};
#endif
