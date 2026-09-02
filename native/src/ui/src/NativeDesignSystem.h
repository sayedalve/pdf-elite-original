#pragma once
#include <d2d1_1.h>
#include <dwrite_3.h>
#include <wrl/client.h>

namespace design {

struct Colors {
    // Exact from target: dark purple-blue
    static constexpr D2D1_COLOR_F Background = {0x1E/255.0f, 0x1E/255.0f, 0x2F/255.0f, 1.0f}; // #1e1e2f
    static constexpr D2D1_COLOR_F Workspace = {0x1E/255.0f, 0x1E/255.0f, 0x2F/255.0f, 1.0f}; // #1e1e2f
    static constexpr D2D1_COLOR_F Surface = {0x28/255.0f, 0x28/255.0f, 0x3E/255.0f, 1.0f}; // #28283e
    static constexpr D2D1_COLOR_F SurfaceElevated = {0x17/255.0f, 0x17/255.0f, 0x29/255.0f, 1.0f}; // SIDEBAR_BG #171729
    static constexpr D2D1_COLOR_F SidebarBg = SurfaceElevated; // Alias for exact mock UI code
    static constexpr D2D1_COLOR_F SurfaceHover = {0x32/255.0f, 0x32/255.0f, 0x4E/255.0f, 1.0f}; // #32324e
    static constexpr D2D1_COLOR_F SurfaceActiveTab = {0x2A/255.0f, 0x2A/255.0f, 0x40/255.0f, 1.0f}; // CARD #2a2a40
    static constexpr D2D1_COLOR_F SurfacePressed = {0x33/255.0f, 0x33/255.0f, 0x4F/255.0f, 1.0f}; // CARD_HOVER #33334f
    
    static constexpr D2D1_COLOR_F Control = {0x2B/255.0f, 0x2B/255.0f, 0x42/255.0f, 1.0f}; // TOOLBAR_BG
    static constexpr D2D1_COLOR_F ControlHover = {255/255.0f, 255/255.0f, 255/255.0f, 0.06f}; 
    static constexpr D2D1_COLOR_F ControlPressed = {255/255.0f, 255/255.0f, 255/255.0f, 0.03f}; 
    static constexpr D2D1_COLOR_F ControlActive = {0x6B/255.0f, 0x8C/255.0f, 0xFF/255.0f, 0.25f}; // #6b8cff with 25% opacity
    
    static constexpr D2D1_COLOR_F AccentPrimary = {0x6B/255.0f, 0x8C/255.0f, 0xFF/255.0f, 1.0f}; // #6b8cff
    static constexpr D2D1_COLOR_F AccentHover = {0x7D/255.0f, 0x9B/255.0f, 0xFF/255.0f, 1.0f}; // #7d9bff
    static constexpr D2D1_COLOR_F AccentPressed = {0x5A/255.0f, 0x7B/255.0f, 0xEE/255.0f, 1.0f};
    
    static constexpr D2D1_COLOR_F TextPrimary = {0xE8/255.0f, 0xE8/255.0f, 0xF0/255.0f, 1.0f}; // #e8e8f0
    static constexpr D2D1_COLOR_F TextSecondary = {0x9A/255.0f, 0x9A/255.0f, 0xB0/255.0f, 1.0f}; // #9a9ab0
    static constexpr D2D1_COLOR_F TextMuted = {0x6B/255.0f, 0x6B/255.0f, 0x80/255.0f, 1.0f}; // #6b6b80
    static constexpr D2D1_COLOR_F TextDisabled = {0x9A/255.0f, 0x9A/255.0f, 0xB0/255.0f, 0.5f};
    
    static constexpr D2D1_COLOR_F Border = {255/255.0f, 255/255.0f, 255/255.0f, 0.12f};
    static constexpr D2D1_COLOR_F BorderSubtle = {255/255.0f, 255/255.0f, 255/255.0f, 0.06f};
    static constexpr D2D1_COLOR_F BorderStrong = {255/255.0f, 255/255.0f, 255/255.0f, 0.18f};
    static constexpr D2D1_COLOR_F Focus = {59/255.0f, 130/255.0f, 246/255.0f, 0.5f};
    
    static constexpr D2D1_COLOR_F Success = {52/255.0f, 211/255.0f, 153/255.0f, 1.0f};
    static constexpr D2D1_COLOR_F Warning = {251/255.0f, 146/255.0f, 60/255.0f, 1.0f};
    static constexpr D2D1_COLOR_F Error = {248/255.0f, 113/255.0f, 113/255.0f, 1.0f};
};

struct Spacing {
    static constexpr float XSmall = 4.0f;
    static constexpr float Small = 8.0f;
    static constexpr float Medium1 = 12.0f;
    static constexpr float Medium2 = 16.0f;
    static constexpr float Large1 = 20.0f;
    static constexpr float Large2 = 24.0f;
    static constexpr float XLarge = 32.0f;
    static constexpr float XXLarge = 40.0f;
    static constexpr float XXXLarge = 48.0f;
};

struct Radius {
    static constexpr float R4 = 4.0f;
    static constexpr float R6 = 6.0f;
    static constexpr float R8 = 8.0f;
    static constexpr float R10 = 10.0f;
    static constexpr float R12 = 12.0f;
    static constexpr float R16 = 16.0f;
};

struct Metrics {
    static constexpr float SidebarWidth = 64.0f;
    static constexpr float HomeSidebarWidth = 240.0f;
    static constexpr float RightRailWidth = 68.0f;
    static constexpr float TopbarHeight = 48.0f;
    static constexpr float ToolbarHeight = 48.0f;
    static constexpr float ContentPadding = Spacing::Large2;
    static constexpr float SectionGap = Spacing::Large2;
    
    static constexpr float ControlHeightLarge = 40.0f;
    static constexpr float ControlHeightNormal = 32.0f;
    static constexpr float ControlHeightSmall = 24.0f;
    
    static constexpr float IconSizeLarge = 24.0f;
    static constexpr float IconSizeMedium = 20.0f;
    static constexpr float IconSizeSmall = 16.0f;
};

struct TypographySize {
    static constexpr float AppTitle = 24.0f;
    static constexpr float PageTitle = 20.0f;
    static constexpr float SectionHeading = 16.0f;
    static constexpr float Body = 13.0f;
    static constexpr float Secondary = 11.0f;
    static constexpr float Toolbar = 13.0f;
    static constexpr float Navigation = 14.0f;
    static constexpr float Metadata = 12.0f;
    static constexpr float Caption = 11.0f;
    static constexpr float Tooltip = 12.0f;
};

struct Layout {
    static constexpr float TopbarHeight = 48.0f;
    static constexpr float TopBarHeight = 48.0f;
    static constexpr float ContentPadding = 32.0f;
    static constexpr float SectionGap = 24.0f;
    static constexpr float ControlHeightNormal = 32.0f;
    static constexpr float ControlHeightLarge = 40.0f;
    static constexpr float ToolbarHeight = 48.0f;
    static constexpr float SidebarWidth = 64.0f;
    static constexpr float RightRailWidth = 68.0f;
    static constexpr float RightPanelWidth = 280.0f;
    static constexpr float SearchBoxWidth = 320.0f;
    static constexpr float SearchBoxHeight = 32.0f;
    static constexpr float ButtonSizeSmall = 24.0f;
    static constexpr float ButtonSizeNormal = 32.0f;
    static constexpr float ButtonSizeLarge = 40.0f;
    static constexpr float IconSizeSmall = 16.0f;
    static constexpr float IconSizeNormal = 20.0f;
    static constexpr float IconSizeLarge = 24.0f;
};

class FontManager {
public:
    static FontManager& Instance();
    
    void Initialize(IDWriteFactory* writeFactory);
    
    IDWriteTextFormat* GetAppTitle() const { return m_appTitle.Get(); }
    IDWriteTextFormat* GetPageTitle() const { return m_pageTitle.Get(); }
    IDWriteTextFormat* GetSectionHeading() const { return m_sectionHeading.Get(); }
    IDWriteTextFormat* GetBody() const { return m_body.Get(); }
    IDWriteTextFormat* GetSecondary() const { return m_secondary.Get(); }
    IDWriteTextFormat* GetToolbar() const { return m_toolbar.Get(); }
    IDWriteTextFormat* GetNavigation() const { return m_navigation.Get(); }
    IDWriteTextFormat* GetMetadata() const { return m_metadata.Get(); }
    IDWriteTextFormat* GetCaption() const { return m_caption.Get(); }
    IDWriteTextFormat* GetTooltip() const { return m_tooltip.Get(); }

private:
    FontManager() = default;
    
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_appTitle;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_pageTitle;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_sectionHeading;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_body;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_secondary;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_toolbar;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_navigation;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_metadata;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_caption;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_tooltip;
};

} // namespace design
