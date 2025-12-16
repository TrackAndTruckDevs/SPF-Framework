#include "SPF/UI/UIManager.hpp"
#include "SPF/Logging/Logger.hpp" // Added include for GetAllLogLevels and LogLevelToString
#include "SPF/UI/MainWindow.hpp"      // Added for MainWindow creation
#include "SPF/UI/PluginsWindow.hpp"     // Added for PluginsWindow creation
#include "SPF/UI/SettingsWindow.hpp"    // Added for SettingsWindow creation
#include "SPF/UI/LoggerWindow.hpp"      // Added for LoggerWindow creation
#include "SPF/UI/CameraWindow.hpp"      // Added for CameraWindow creation
#include "SPF/UI/GameConsoleWindow.hpp" // Added for GameConsoleWindow creation
#include "SPF/UI/HooksWindow.hpp"       // Added for HooksWindow creation
#include "SPF/UI/TelemetryWindow.hpp"   // Added for TelemetryWindow creation

#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Events/EventManager.hpp"
#include <SPF/UI/UIStyle.hpp>
#include "SPF/UI/Icons.hpp"
#include "SPF/Config/IConfigService.hpp"
#include "SPF/Events/PluginEvents.hpp"
#include "SPF/UI/PluginProxyWindow.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Input/InputManager.hpp"


// --- Embedded Font Data ---
#include "SPF/UI/Fonts/FontAwesome7.h"
#include "SPF/UI/Fonts/FontAwesome7Brands.h"
#include "SPF/UI/Fonts/NotoSansBold.h"
#include "SPF/UI/Fonts/NotoSansBoldItalic.h"
#include "SPF/UI/Fonts/NotoSansItalic.h"
#include "SPF/UI/Fonts/NotoSansMedium.h"
#include "SPF/UI/Fonts/NotoSansRegular.h"
#include "SPF/UI/Fonts/NotoSansSCRegular.h"
#include "SPF/UI/Fonts/NotoSansKRRegular.h"
#include "SPF/UI/Fonts/NotoSansMediumItalic.h"
#include "SPF/UI/Fonts/RobotoMonoRegular.h"
// --- End Embedded Font Data ---

#include <set>
#include <string>
#include <imgui_internal.h>

#include "SPF/GameCamera/GameCameraManager.hpp"  // For animation input blocking

#include "SPF/UI/MainWindow.hpp"      // Required for dynamic_cast and GetMainDockspaceID
#include "SPF/UI/SettingsWindow.hpp"  // Required for dynamic_cast
SPF_NS_BEGIN

namespace UI {
using namespace SPF::Logging;
using namespace SPF::Input;

// Static method to get the singleton instance
UIManager& UIManager::GetInstance() {
  static UIManager instance;
  return instance;
}

// Parameterless constructor for Singleton pattern
UIManager::UIManager()
    : m_eventManager(nullptr),  // Initialize all member pointers to nullptr
      m_inputManager(nullptr),
      m_configService(nullptr),
      m_keyBindsManager(nullptr),
      m_pluginManager(nullptr),
      m_onPluginDidLoadSink(nullptr),        // will be initialized in Init()
      m_onPluginWillBeUnloadedSink(nullptr)  // will be initialized in Init()
{
  // No dependencies are passed here, they will be passed via Init()
}

void UIManager::Init(Events::EventManager& eventManager, Input::InputManager& inputManager, Config::IConfigService& configService, Modules::KeyBindsManager& keyBindsManager,
            Modules::PluginManager& pluginManager, Logging::LoggerFactory& loggerFactory, Modules::ITelemetryService& telemetryService) {
  m_eventManager = &eventManager;
  m_inputManager = &inputManager;
  m_configService = &configService;
  m_keyBindsManager = &keyBindsManager;
  m_pluginManager = &pluginManager;
  m_loggerFactory = &loggerFactory;
  m_telemetryService = &telemetryService;
  // Initialize and connect sinks
  m_onPluginDidLoadSink = std::make_unique<Utils::Sink<void(const Events::OnPluginDidLoad&)>>(m_eventManager->System.OnPluginDidLoad);
  m_onPluginWillBeUnloadedSink = std::make_unique<Utils::Sink<void(const Events::OnPluginWillBeUnloaded&)>>(m_eventManager->System.OnPluginWillBeUnloaded);

  m_onPluginDidLoadSink->Connect<&UIManager::OnPluginLoaded>(this);
  m_onPluginWillBeUnloadedSink->Connect<&UIManager::OnPluginUnloaded>(this);
}

void UIManager::CloseFocusedWindow() {
    ImGuiContext& g = *ImGui::GetCurrentContext();

    // Priority 1: Check for and close any open popups/modals
    if (g.OpenPopupStack.Size > 0) {
        g.OpenPopupStack.clear(); // Brute-force close. May have side effects.
        return; // Popups handled, do nothing else.
    }

    IWindow* focusedWindow = nullptr;
    ImGuiWindow* navWindow = g.NavWindow;

    if (navWindow) {
        const char* navWindowName = navWindow->Name;
        for (const auto& window : m_windows) {
            if (!window || !window->IsVisible()) {
                continue;
            }
            
            std::string expectedName = std::string(window->GetWindowTitle()) + "###" + window->GetComponentName() + "_" + window->GetWindowId();
            if (strcmp(navWindowName, expectedName.c_str()) == 0) {
                focusedWindow = window.get();
                break;
            }
        }
    }

    // If no specific window is focused, but the UI is active, close the main window.
    if (!focusedWindow) {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
            if (auto* mainWindow = dynamic_cast<MainWindow*>(GetWindow("framework", "main_window"))) {
                if (mainWindow->IsVisible()) {
                    mainWindow->SetVisibility(false);
                }
            }
        }
        return;
    }

    auto* baseWindow = dynamic_cast<BaseWindow*>(focusedWindow);
    if (!baseWindow) {
        return; // Not a BaseWindow, do nothing.
    }
    
    // If it's the main window, close it.
    if (baseWindow->GetWindowId() == "main_window") {
        baseWindow->SetVisibility(false);
        return;
    }

    // If the window is configured to be dockable AND it's currently undocked, then re-dock it.
    if (baseWindow->IsConfiguredAsDockable() && !baseWindow->IsDocked()) {
        baseWindow->SetDocked(true);
    } 
    // Otherwise (it's either a docked window or a floating plugin window)
    else {
        // If it's docked, close the main window.
        if (baseWindow->IsDocked()) {
            if (auto* mainWindow = dynamic_cast<MainWindow*>(GetWindow("framework", "main_window"))) {
                mainWindow->SetVisibility(false);
            }
        } 
        // If it's a floating window not configured for docking, just hide it.
        else {
            baseWindow->SetVisibility(false);
        }
    }
}

UIManager::~UIManager() {
  // Ensure shutdown is called, even if not explicitly done.
  Shutdown();
}

Core::InitializationReport UIManager::Initialize(const std::map<std::string, nlohmann::json>* allUIConfigs) {
  m_allUIConfigs = allUIConfigs;  // Still needed for settings application

  auto logger = LoggerFactory::GetInstance().GetLogger("UIManager");
  logger->Info("Initializing UIManager...");
  InitializeImGui();

  // NOTE: The loop that created plugin windows has been removed.
  // That logic will be moved to the OnPluginLoaded event handler.

  Core::InitializationReport report;
  report.ServiceName = "UIManager";
  report.InfoMessages.push_back("ImGui context initialized successfully.");

  // Register framework-specific actions now that the KeyBindsManager is initialized.
  m_keyBindsManager->RegisterAction("framework.input.toggle_mouse_overridden", [this]() { ToggleMouseOverridden(); });
  m_keyBindsManager->RegisterAction("framework.ui.close_focused", [this]() { CloseFocusedWindow(); });

  return report;
}

void UIManager::Shutdown() {
  ShutdownImGui();
  m_windows.clear();
  m_fonts.clear();

  // Reset state variables
  m_allUIConfigs = nullptr;
  m_windowToFocus.clear();
  m_lastFocusedDockedWindowId.clear();
  m_wasShellVisibleLastFrame = false;
  m_isMouseControlOverridden = false;
}

void UIManager::RegisterWindow(std::shared_ptr<IWindow> window) {
  if (!window) return;

  // Apply settings before adding the window
  if (m_allUIConfigs) {
    const auto& componentName = window->GetComponentName();
    const auto& windowId = window->GetWindowId();

    auto compIt = m_allUIConfigs->find(componentName);
    if (compIt != m_allUIConfigs->end()) {
      const auto& componentUIConfig = compIt->second;
      if (componentUIConfig.contains("windows") && componentUIConfig["windows"].contains(windowId)) {
        const auto& windowConfig = componentUIConfig["windows"][windowId];
        nlohmann::json strippedConfig;

        for (const auto& [key, node] : windowConfig.items()) {
          if (node.is_object() && node.contains("_value")) {
            strippedConfig[key] = node["_value"];
          } else {
            // Keep non-_value properties (like _meta) if needed, but for ApplySettings we only need values.
            // For now, we can ignore them, or copy them if the window needs them.
            // Let's assume ApplySettings only wants simple values.
          }
        }
        window->ApplySettings(strippedConfig);
      }
    }
  }

  m_windows.push_back(std::move(window));
}

std::map<std::string, nlohmann::json> UIManager::GetAllWindowSettings() const {
  std::map<std::string, nlohmann::json> allSettings;
  for (const auto& window : m_windows) {
    if (!window) continue;
    // This structure assumes settings are grouped by component
    allSettings[window->GetComponentName()]["windows"][window->GetWindowId()] = window->GetCurrentSettings();
  }
  return allSettings;
}

void UIManager::ToggleMouseOverridden() {
  auto* mainWindow = dynamic_cast<MainWindow*>(GetWindow("framework", "main_window"));
  const bool isShellVisible = mainWindow && mainWindow->IsVisible();

  bool isAnyWindowTrulyVisible = false;
  for (const auto& window : m_windows) {
    bool should_render = false;
    if (auto* baseWindow = dynamic_cast<BaseWindow*>(window.get())) {
      should_render = baseWindow->IsVisible() && (!baseWindow->IsDocked() || isShellVisible);
    } else {
      should_render = window->IsVisible();
    }

    if (should_render) {
      isAnyWindowTrulyVisible = true;
      break;
    }
  }

  if (!isAnyWindowTrulyVisible) {
    return;  // Do nothing if no UI is actually visible
  }

  m_isMouseControlOverridden = !m_isMouseControlOverridden;
  auto logger = LoggerFactory::GetInstance().GetLogger("UIManager");
  if (logger) logger->Debug("Mouse control override toggled by hotkey. New state: {}", m_isMouseControlOverridden);
}

ImFont* UIManager::GetFont(const std::string& name) const {
  auto it = m_fonts.find(name);
  if (it != m_fonts.end()) {
    return it->second;
  }
  return nullptr;
}

IWindow* UIManager::GetWindow(const std::string& componentName, const std::string& windowId) const {
  for (const auto& window : m_windows) {
    if (window->GetComponentName() == componentName && window->GetWindowId() == windowId) {
      return window.get();
    }
  }
  return nullptr;
}

// Helper function to process rendering for a single window
void RenderWindow(IWindow* window, ImGuiID mainDockspaceId, bool isShellVisible) {
  if (!window) return;

  auto* baseWindow = dynamic_cast<BaseWindow*>(window);

  bool should_render = false;
  bool is_docked = false;
  if (baseWindow) {
    is_docked = baseWindow->IsDocked();
    should_render = baseWindow->IsVisible() && (!is_docked || isShellVisible);
  } else {
    should_render = window->IsVisible();
  }

  if (should_render) {
    // If the window is docked, make its background transparent
    // so the main window's background shows through.
    if (is_docked) {
      ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    if (baseWindow && baseWindow->IsConfiguredAsDockable() && is_docked) {
      ImGui::SetNextWindowDockID(mainDockspaceId, ImGuiCond_Always);
    }

    window->Render();

    // Pop the style color if it was pushed
    if (is_docked) {
      ImGui::PopStyleColor();
    }
  }
}

void UIManager::RenderAll() {
  // Process unload queue at the beginning of the frame
  m_pluginManager->ProcessUnloadQueue();

  // --- Declarative Rendering Logic ---
  auto* mainWindow = dynamic_cast<MainWindow*>(GetWindow("framework", "main_window"));
  const bool isShellVisible = mainWindow && mainWindow->IsVisible();
  const ImGuiID mainDockspaceId = isShellVisible ? mainWindow->GetMainDockspaceID() : 0;

  // --- Master Visibility & Cursor Control Logic ---
  bool isAnyWindowTrulyVisible = false;
  for (const auto& window : m_windows) {
    bool should_render = false;
    if (auto* baseWindow = dynamic_cast<BaseWindow*>(window.get())) {
      should_render = baseWindow->IsVisible() && (!baseWindow->IsDocked() || isShellVisible);
    } else {
      should_render = window->IsVisible();
    }

    if (should_render) {
      isAnyWindowTrulyVisible = true;
      break;
    }
  }

  bool isAnimationPlaying = false;
  if (auto* animController = GameCamera::GameCameraManager::GetInstance().GetDebugAnimationController()) {
    if (animController->GetPlaybackState() == GameCamera::GameCameraDebugAnimation::PlaybackState::Playing) {
      isAnimationPlaying = true;
    }
  }

  if (!isAnyWindowTrulyVisible && !isAnimationPlaying) {
    // If no UI is visible and no animation is playing, reset override and give game control.
    m_isMouseControlOverridden = false;
    m_inputManager->SetMouseAxesControl(true);
    m_inputManager->SetMouseButtonsControl(true);
    m_inputManager->SetMouseWheelControl(true);
    ImGui::GetIO().MouseDrawCursor = false;
  } else {
    // Special case: Animation is playing, but no windows are visible.
    // We block the mouse from the game, but we don't draw a cursor.
    if (isAnimationPlaying && !isAnyWindowTrulyVisible) {
      m_isMouseControlOverridden = false;            // Reset override here too
      m_inputManager->SetMouseAxesControl(false);    // Block game camera
      m_inputManager->SetMouseButtonsControl(true);  // Allow buttons
      m_inputManager->SetMouseWheelControl(true);    // Allow wheel
      ImGui::GetIO().MouseDrawCursor = false;        // Don't draw UI cursor
    } else                                           // Normal case: UI is visible.
    {
      // If UI is visible, perform the detailed logic check.
      bool isAnyInteractiveWindowVisible = false;
      for (const auto& window : m_windows) {
        bool should_render = false;
        if (auto* baseWindow = dynamic_cast<BaseWindow*>(window.get())) {
          if (baseWindow->IsDocked()) {
            should_render = isShellVisible;
          } else {
            should_render = baseWindow->IsVisible();
          }
        } else {
          should_render = window->IsVisible();
        }

        if (should_render && window->IsInteractive()) {
          isAnyInteractiveWindowVisible = true;
          break;
        }
      }

      // 1. Determine the automatic state (UI should have control if an interactive window is up OR an animation is playing)
      bool auto_uiShouldHaveControl = isAnyInteractiveWindowVisible || isAnimationPlaying;

      // 2. Apply the toggle override using XOR.
      bool final_uiShouldHaveControl = auto_uiShouldHaveControl ^ m_isMouseControlOverridden;

      // 3. Apply the final state
      ImGui::GetIO().MouseDrawCursor = final_uiShouldHaveControl;
      m_inputManager->SetMouseAxesControl(!final_uiShouldHaveControl);
      m_inputManager->SetMouseButtonsControl(!final_uiShouldHaveControl);
      m_inputManager->SetMouseWheelControl(!final_uiShouldHaveControl);
    }
  }

  // --- Focus Management (Second Frame) ---
  if (!m_windowToFocus.empty()) {
    ImGui::SetWindowFocus(m_windowToFocus.c_str());
    m_windowToFocus.clear();
  }

  // --- Focus Management (First Frame) ---
  if (isShellVisible && !m_wasShellVisibleLastFrame)  // Shell was just opened
  {
    IWindow* windowToFocusPtr = nullptr;
    if (!m_lastFocusedDockedWindowId.empty()) {
      // Find the window pointer from the stored ID
      for (const auto& windowPtr : m_windows) {
        // HACK: This currently only finds framework windows. A more robust solution would be needed
        // to uniquely identify plugin windows if they can also be the last focused.
        if (windowPtr->GetWindowId() == m_lastFocusedDockedWindowId && windowPtr->GetComponentName() == "framework") {
          windowToFocusPtr = windowPtr.get();
          break;
        }
      }
    }

    // If we couldn't find the last focused, or it wasn't set, find the highest priority
    if (!windowToFocusPtr) {
      IWindow* highestPriorityWindow = nullptr;
      int highestPriority = 9999;

      for (const auto& windowPtr : m_windows) {
        if (auto* baseWindow = dynamic_cast<BaseWindow*>(windowPtr.get())) {
          if (baseWindow->IsConfiguredAsDockable() && baseWindow->IsDocked()) {
            if (baseWindow->GetDockPriority() < highestPriority) {
              highestPriority = baseWindow->GetDockPriority();
              highestPriorityWindow = baseWindow;
            }
          }
        }
      }
      windowToFocusPtr = highestPriorityWindow;
    }

    // If we have a window to focus, construct its full ImGui ID and queue it for the next frame.
    if (windowToFocusPtr) {
      m_windowToFocus = std::string(windowToFocusPtr->GetWindowTitle()) + "###" + windowToFocusPtr->GetComponentName() + "_" + windowToFocusPtr->GetWindowId();
    }
  }

  // Render the shell itself if it's visible
  if (isShellVisible) {
    mainWindow->Render();
  }

  // Sort windows by dock priority before rendering
  std::vector<IWindow*> windowsToRender;
  for (const auto& windowPtr : m_windows) {
    if (windowPtr.get() != mainWindow) {
      windowsToRender.push_back(windowPtr.get());
    }
  }

  std::sort(windowsToRender.begin(), windowsToRender.end(), [](IWindow* a, IWindow* b) {
    auto* baseA = dynamic_cast<BaseWindow*>(a);
    auto* baseB = dynamic_cast<BaseWindow*>(b);
    if (baseA && baseB) {
      return baseA->GetDockPriority() < baseB->GetDockPriority();
    }
    return false;
  });

  // --- Pass 1: Render all windows ---
  for (IWindow* window : windowsToRender) {
    RenderWindow(window, mainDockspaceId, isShellVisible);
  }

  // --- Pass 2: Update focused tab state using robust method ---
  if (isShellVisible) {
    for (IWindow* window : windowsToRender) {
      if (!window->IsVisible()) continue;

      if (auto* baseWindow = dynamic_cast<BaseWindow*>(window)) {
        if (baseWindow->IsConfiguredAsDockable() && baseWindow->IsDocked()) {
          std::string fullId = std::string(baseWindow->GetWindowTitle()) + "###" + baseWindow->GetComponentName() + "_" + baseWindow->GetWindowId();
          ImGuiWindow* imguiWindow = ImGui::FindWindowByName(fullId.c_str());

          if (imguiWindow && imguiWindow->DockNode && imguiWindow->DockNode->TabBar && imguiWindow->DockNode->VisibleWindow == imguiWindow) {
            // This is the currently visible tab in its dock node.
            m_lastFocusedDockedWindowId = baseWindow->GetWindowId();
            break;  // Found the one, stop searching.
          }
        }
      }
    }
  }

  m_wasShellVisibleLastFrame = isShellVisible;
}

void UIManager::InitializeImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  //ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard; не працює, видалити
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGuiIO& io = ImGui::GetIO();
  io.IniFilename = nullptr;
  Style::ApplyGameStyle();

  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("UIManager");
  logger->Info("Loading and preparing embedded fonts...");

  // Get Cyrillic glyph ranges.
  const ImWchar* glyph_ranges_cyrillic = io.Fonts->GetGlyphRangesCyrillic();
  const ImWchar* glyph_ranges_chinese = io.Fonts->GetGlyphRangesChineseFull();
  const ImWchar* glyph_ranges_japanese = io.Fonts->GetGlyphRangesJapanese();
  const ImWchar* glyph_ranges_korean = io.Fonts->GetGlyphRangesKorean();

  // 1. Main UI Font (Regular) with Cyrillic support
  // This is the default font at index 0 of the font atlas.
  m_fonts["regular"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansRegular_compressed_data, Font_NotoSansRegular_compressed_size, 18.0f, nullptr, glyph_ranges_cyrillic);
  io.FontDefault = m_fonts["regular"];  // Set as default
  logger->Info("Successfully loaded main font (NotoSans-Regular) from memory.");

  // 2. Icon Fonts (Merged with Main Font)
  ImFontConfig icon_config_fa;
  icon_config_fa.MergeMode = true;
  icon_config_fa.PixelSnapH = true;
  static const ImWchar icon_ranges_fa[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_FontAwesome7_compressed_data, Font_FontAwesome7_compressed_size, 15.0f, &icon_config_fa, icon_ranges_fa);
  logger->Info("Successfully merged FontAwesome7 (Regular) icon font from memory.");

  ImFontConfig icon_config_fab;
  icon_config_fab.MergeMode = true;
  icon_config_fab.PixelSnapH = true;
  static const ImWchar icon_ranges_fab[] = {ICON_MIN_FAB, ICON_MAX_FAB, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_FontAwesome7Brands_compressed_data, Font_FontAwesome7Brands_compressed_size, 15.0f, &icon_config_fab, icon_ranges_fab);
  logger->Info("Successfully merged FontAwesome7 (Brands) icon font from memory.");

  // Merge CJK font
  ImFontConfig cjk_config;
  cjk_config.MergeMode = true;
  cjk_config.PixelSnapH = true;
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansSCRegular_compressed_data, Font_NotoSansSCRegular_compressed_size, 18.0f, &cjk_config, glyph_ranges_chinese);
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansSCRegular_compressed_data, Font_NotoSansSCRegular_compressed_size, 18.0f, &cjk_config, glyph_ranges_japanese);
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansKRRegular_compressed_data, Font_NotoSansKRRegular_compressed_size, 18.0f, &cjk_config, glyph_ranges_korean);
  logger->Info("Successfully merged CJK font into the main font.");

  // 3. Typist Fonts (Bold, Italic, etc.)
  m_fonts["bold"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansBold_compressed_data, Font_NotoSansBold_compressed_size, 18.0f, nullptr, glyph_ranges_cyrillic);
  // Merge Font Awesome icons into the bold font
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_FontAwesome7_compressed_data, Font_FontAwesome7_compressed_size, 16.0f, &icon_config_fa, icon_ranges_fa);
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_FontAwesome7Brands_compressed_data, Font_FontAwesome7Brands_compressed_size, 16.0f, &icon_config_fab, icon_ranges_fab);
  logger->Info("Successfully loaded bold font (NotoSans-Bold) and merged icons from memory.");

  // Merge CJK font into bold
  ImFontConfig cjk_config_bold;
  cjk_config_bold.MergeMode = true;
  cjk_config_bold.PixelSnapH = true;
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansSCRegular_compressed_data, Font_NotoSansSCRegular_compressed_size, 18.0f, &cjk_config_bold, glyph_ranges_chinese);
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansSCRegular_compressed_data, Font_NotoSansSCRegular_compressed_size, 18.0f, &cjk_config_bold, glyph_ranges_japanese);
  io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansKRRegular_compressed_data, Font_NotoSansKRRegular_compressed_size, 18.0f, &cjk_config_bold, glyph_ranges_korean);
  logger->Info("Successfully merged CJK font into the bold font.");

  m_fonts["italic"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansItalic_compressed_data, Font_NotoSansItalic_compressed_size, 18.0f, nullptr, glyph_ranges_cyrillic);
  logger->Info("Successfully loaded italic font (NotoSans-Italic) from memory.");

  m_fonts["bold_italic"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansBoldItalic_compressed_data, Font_NotoSansBoldItalic_compressed_size, 18.0f, nullptr, glyph_ranges_cyrillic);
  logger->Info("Successfully loaded bold-italic font (NotoSans-BoldItalic) from memory.");

  m_fonts["medium"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansMedium_compressed_data, Font_NotoSansMedium_compressed_size, 18.0f, nullptr, glyph_ranges_cyrillic);
  logger->Info("Successfully loaded medium font (NotoSans-Medium) from memory.");

  m_fonts["medium_italic"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansMediumItalic_compressed_data, Font_NotoSansMediumItalic_compressed_size, 18.0f, nullptr, glyph_ranges_cyrillic);
  logger->Info("Successfully loaded medium-italic font (NotoSans-MediumItalic) from memory.");

  // 4. Heading Fonts (Medium)
  m_fonts["h1"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansMedium_compressed_data, Font_NotoSansMedium_compressed_size, 24.0f, nullptr, glyph_ranges_cyrillic);
  m_fonts["h2"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansMedium_compressed_data, Font_NotoSansMedium_compressed_size, 22.0f, nullptr, glyph_ranges_cyrillic);
  m_fonts["h3"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_NotoSansMedium_compressed_data, Font_NotoSansMedium_compressed_size, 20.0f, nullptr, glyph_ranges_cyrillic);
  logger->Info("Successfully loaded heading fonts (h1, h2, h3) from memory.");

  // 5. Monospaced Font
  m_fonts["monospace"] = io.Fonts->AddFontFromMemoryCompressedTTF(Font_RobotoMonoRegular_compressed_data, Font_RobotoMonoRegular_compressed_size, 17.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
  logger->Info("Successfully loaded monospaced font (RobotoMono-Regular) from memory.");
}

void UIManager::ShutdownImGui() {
  if (ImGui::GetCurrentContext()) {
    ImGui::DestroyContext();
  }
}

void UIManager::OnPluginLoaded(const Events::OnPluginDidLoad& e) {
  auto logger = LoggerFactory::GetInstance().GetLogger("UIManager");
  if (logger) logger->Info("Plugin '{}' loaded, creating its windows...", e.pluginName);

  if (!m_allUIConfigs) return;

  auto componentConfigIt = m_allUIConfigs->find(e.pluginName);
  if (componentConfigIt == m_allUIConfigs->end()) return;

  const auto& config = componentConfigIt->second;
  if (config.contains("windows")) {
    for (auto const& [windowId, windowConfig] : config["windows"].items()) {
      logger->Info("Creating declared window: '{}' for component '{}'", windowId, e.pluginName);
      // For now, all non-framework windows are PluginProxyWindows.
      auto window = std::make_shared<PluginProxyWindow>(e.pluginName, windowId);
      RegisterWindow(window);
    }
  }
}

void UIManager::OnPluginUnloaded(const Events::OnPluginWillBeUnloaded& e) {
  auto logger = LoggerFactory::GetInstance().GetLogger("UIManager");
  if (logger) logger->Info("Plugin '{}' unloading, destroying its windows...", e.pluginName);
  DestroyWindowsForOwner(e.pluginName);
}

void UIManager::DestroyWindowsForOwner(const std::string& owner) {
  std::erase_if(m_windows, [&](const std::shared_ptr<IWindow>& window) { return window->GetComponentName() == owner; });
}

bool UIManager::OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) {
  if (systemName != "ui") {
    return false;  // This component only handles UI settings.
  }

  // Example keyPath: "windows.main_window.is_visible"
  static const std::string prefix = "windows.";
  if (keyPath.rfind(prefix, 0) != 0) {
    return false;  // Not a window setting.
  }

  std::string windowIdAndProp = keyPath.substr(prefix.length());
  size_t lastDot = windowIdAndProp.rfind('.');
  if (lastDot == std::string::npos) {
    return false;  // Malformed key path.
  }

  std::string windowId = windowIdAndProp.substr(0, lastDot);
  std::string propertyName = windowIdAndProp.substr(lastDot + 1);

  IWindow* window = GetWindow(componentName, windowId);
  if (window) {
    // Create a mini-json with just the changed property to pass to the window
    nlohmann::json settingUpdate;
    const nlohmann::json* valueToApply = &newValue;
    if (newValue.is_object() && newValue.contains("_value")) {
      valueToApply = &newValue["_value"];
    }
    settingUpdate[propertyName] = *valueToApply;
    window->ApplySettings(settingUpdate);
    return true;  // The setting was handled.
  }

  return false;  // A UI setting, but for a window that doesn't exist.
}

void UIManager::NotifyInputCaptured(const Input::InputCaptured& e) {
  for (const auto& window : m_windows) {
    if (auto* settingsWindow = dynamic_cast<SettingsWindow*>(window.get())) {
      settingsWindow->OnInputCaptured(e);
    }
  }
}

void UIManager::NotifyInputCaptureCancelled(const Input::InputCaptureCancelled& e) {
  for (const auto& window : m_windows) {
    if (auto* settingsWindow = dynamic_cast<SettingsWindow*>(window.get())) {
      settingsWindow->OnInputCaptureCancelled(e);
    }
  }
}

void UIManager::NotifyInputCaptureConflict(const Input::InputCaptureConflict& e) {
  for (const auto& window : m_windows) {
    if (auto* settingsWindow = dynamic_cast<SettingsWindow*>(window.get())) {
      settingsWindow->OnInputCaptureConflict(e);
    }
  }
}

void UIManager::NotifyUpdateCheckSucceeded(const Events::System::OnUpdateCheckSucceeded& e) {
    auto logger = LoggerFactory::GetInstance().GetLogger("UIManager");
    logger->Debug("Received OnUpdateCheckSucceeded event. Notifying {} windows.", m_windows.size());
    for (const auto& window : m_windows) {
        window->OnUpdateCheckSucceeded(e);
    }
}

void UIManager::NotifyUpdateCheckFailed(const Events::System::OnUpdateCheckFailed& e) {
    auto logger = LoggerFactory::GetInstance().GetLogger("UIManager");
    logger->Debug("Received OnUpdateCheckFailed event. Notifying {} windows.", m_windows.size());
    for (const auto& window : m_windows) {
        window->OnUpdateCheckFailed(e);
    }
}

void UIManager::NotifyPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e) {
    auto logger = LoggerFactory::GetInstance().GetLogger("UIManager");
    logger->Debug("Received OnPatronsFetchCompleted event. Notifying {} windows.", m_windows.size());
    for (const auto& window : m_windows) {
        window->OnPatronsFetchCompleted(e);
    }
}
void UIManager::CreateAndRegisterFrameworkWindows() {
  // Ensure dependencies are valid
  assert(m_eventManager);
  assert(m_inputManager);
  assert(m_configService);
  assert(m_keyBindsManager);
  assert(m_pluginManager);
  assert(m_loggerFactory);
  assert(m_telemetryService);

  // Prepare log levels for SettingsWindow
  std::vector<std::string> logLevels;
  for (const auto& level : SPF::Logging::GetAllLogLevels()) {
    logLevels.push_back(SPF::Logging::LogLevelToString(level));
  }

  // Create and register framework-owned windows.
  // The order here matters for potential window layering or initial focus.

  // Main Window
  auto mainWindow = std::make_shared<MainWindow>(*m_eventManager, *m_inputManager, *m_keyBindsManager, *m_configService, *m_telemetryService);
  RegisterWindow(mainWindow);

  // Logger Window
  if (auto loggerSink = m_loggerFactory->GetInstance().GetUISink()) {
    auto loggerWindow = std::make_shared<LoggerWindow>("framework", "logger_window", *loggerSink, *m_configService);
    RegisterWindow(loggerWindow);
  }

  // Plugins Window
  auto pluginsWindow = std::make_shared<PluginsWindow>(*m_configService, *m_eventManager, "framework", "plugins_window");
  RegisterWindow(pluginsWindow);

  // Settings Window
  auto settingsWindow = std::make_shared<SettingsWindow>(*m_configService, logLevels, *m_eventManager, "framework", "settings_window");
  RegisterWindow(settingsWindow);

  // Hooks Window
  auto hooksWindow = std::make_shared<HooksWindow>(*this, *m_eventManager, "framework", "hooks_window");
  RegisterWindow(hooksWindow);

  // Telemetry Window
  auto telemetryWindow = std::make_shared<TelemetryWindow>("framework", "telemetry_window", *m_telemetryService);
  RegisterWindow(telemetryWindow);

  // Game Console Window
  auto gameConsoleWindow = std::make_shared<GameConsoleWindow>("framework", "game_console_window", *m_eventManager);
  RegisterWindow(gameConsoleWindow);

  // Camera Window
  auto cameraWindow = std::make_shared<CameraWindow>(GameCamera::GameCameraManager::GetInstance(), "framework", "camera_window");
  RegisterWindow(cameraWindow);
}

}  // namespace UI

SPF_NS_END

