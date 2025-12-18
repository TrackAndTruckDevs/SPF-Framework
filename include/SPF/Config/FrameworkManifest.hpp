#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Config/ManifestData.hpp"
SPF_NS_BEGIN

namespace Config {

/**
 * @brief Provides the default manifest for the framework as a C++ structure.
 *
 * @return A constant reference to a statically initialized ManifestData object.
 */
inline const ManifestData& GetFrameworkManifestData() {
  static ManifestData manifest;
    static bool initialized = false;

    if (!initialized) {
        manifest = {
      // .info
      .info =
          {
              .name = "SPF Framework",
              .version = "1.0.4",
              .author = "Track'n'Truck Devs",
              .descriptionKey = "description.detailed",  // key in the translation file
              .descriptionLiteral = "",                  // if there is no translation, you can write a description here
              .email = "mailto:spf.framework@gmail.com",
              .discordUrl = "",
              .steamProfileUrl = "",
              .githubUrl = "https://github.com/TrackAndTruckDevs/SPF-Framework",
              .youtubeUrl = "https://www.youtube.com/@TrackAndTruck",
              .scsForumUrl = "",
              .patreonUrl = "https://www.patreon.com/TrackAndTruckDevs",
              .websiteUrl = "https://trucksimhub.top",

          },
      // .configPolicy
      .configPolicy =
          {
              .allowUserConfig = true,                                                   // allow creating a configuration file
              .userConfigurableSystems = {"localization"},  // which settings to show in the UI. "keybinds" if present, will always be displayed
              .requiredHooks = {}                                                        // Framework doesn't require hooks in its own manifest
                                                                                        //"ui",  "settings", "logging"
          },
      // .settings (top-level settings block, including config.settings)
      .settings = nlohmann::json::parse(R"json(
            {
              "plugin_states": {},
              "hook_states": {}
            }
        )json"),
      // .logging
      .logging = {.level = "trace",  // logging level, for sink file, UI has its own filter ("trace", "debug", "info", "warn", "error", "critical")
                  .sinks =
                      {
                          .file = true,  // create a log file
                          .ui = true,    // display logging in UI

                      }},
      // .localization
      .localization = {.language = "en"},
      // .keybinds
      .keybinds = {.actions = {{"framework.ui.main_window",
                                {{"toggle",
                                  {{
                                      .type = "keyboard",       // keyboard, gamepad
                                      .key = "KEY_DELETE",      // VirtualKeyMapping.cpp, GamepadButtonMapping.cpp
                                      .pressType = "short",     // short, long
                                      .pressThresholdMs = 500,  // How long does it take to get a long press to work
                                      .consume = "always",      // never, on_ui_focus, always
                                      .behavior = "toggle",     // toggle, hold
                                  }}}}},
                               {"framework.ui",
                                {{"close_focused",
                                  {{
                                      .type = "keyboard",
                                      .key = "KEY_ESCAPE",
                                      .pressType = "short",
                                      .consume = "on_ui_focus",
                                  }}}}},
                               {"framework.input",
                                {{"toggle_mouse_overridden",
                                  {{
                                      .type = "mouse",
                                      .key = "MOUSE_MIDDLE",
                                      .pressType = "short",
                                      .pressThresholdMs = 500,
                                      .consume = "on_ui_focus",
                                      .behavior = "hold",
                                  }}}}}}},
      // .ui
      .ui = {.windows = {{"main_window",
                          {
                              .isVisible = false,
                              .isInteractive = true,
                              .posX = 100,
                              .posY = 100,
                              .sizeW = 800,
                              .sizeH = 600,
                              .isCollapsed = false,
                              .isDocked = false,
                              .dockPriority = 0,
                              .allowUndocking = false,
                              .autoScroll = false,
                          }},
                         {"plugins_window",
                          {
                              .isVisible = true,
                              .isInteractive = false,
                              .posX = 0,
                              .posY = 0,
                              .sizeW = 0,
                              .sizeH = 0,
                              .isCollapsed = false,
                              .isDocked = true,
                              .dockPriority = 1,
                              .allowUndocking = false,
                              .autoScroll = false,
                          }},
                         {"settings_window",
                          {
                              .isVisible = true,
                              .isInteractive = false,
                              .posX = 0,
                              .posY = 0,
                              .sizeW = 0,
                              .sizeH = 0,
                              .isCollapsed = false,
                              .isDocked = true,
                              .dockPriority = 2,
                              .allowUndocking = false,
                              .autoScroll = false,
                          }},
                         {"logger_window",
                          {
                              .isVisible = true,
                              .isInteractive = false,
                              .posX = 0,
                              .posY = 0,
                              .sizeW = 0,
                              .sizeH = 0,
                              .isCollapsed = false,
                              .isDocked = true,
                              .dockPriority = 3,
                              .allowUndocking = true,
                              .autoScroll = true,
                          }},
                         {"telemetry_window",
                          {
                              .isVisible = true,
                              .isInteractive = false,
                              .posX = 0,
                              .posY = 0,
                              .sizeW = 0,
                              .sizeH = 0,
                              .isCollapsed = false,
                              .isDocked = true,
                              .dockPriority = 4,
                              .allowUndocking = true,
                              .autoScroll = false,
                          }},
                         {"hooks_window",
                          {
                              .isVisible = true,
                              .isInteractive = false,
                              .posX = 0,
                              .posY = 0,
                              .sizeW = 0,
                              .sizeH = 0,
                              .isCollapsed = false,
                              .isDocked = true,
                              .dockPriority = 5,
                              .allowUndocking = false,
                              .autoScroll = false,
                          }},
                         {"game_console_window",
                          {
                              .isVisible = true,
                              .isInteractive = false,
                              .posX = 0,
                              .posY = 0,
                              .sizeW = 0,
                              .sizeH = 0,
                              .isCollapsed = false,
                              .isDocked = true,
                              .dockPriority = 6,
                              .allowUndocking = true,
                              .autoScroll = false,
                          }},
                         {"camera_window",
                          {
                              .isVisible = true,
                              .isInteractive = false,
                              .posX = 0,
                              .posY = 0,
                              .sizeW = 0,
                              .sizeH = 0,
                              .isCollapsed = false,
                              .isDocked = true,
                              .dockPriority = 7,
                              .allowUndocking = false,
                              .autoScroll = false,
                          }}}},
        
        // --- Metadata for framework's own settings ---
        .customSettingsMetadata = {
            {"plugin_states", "settings_window.setting_names.settings.plugin_states.title", "settings_window.setting_names.settings.plugin_states.description"},
            {"hook_states", "settings_window.setting_names.settings.hook_states.title", "settings_window.setting_names.settings.hook_states.description"}
        },
        .keybindsMetadata = {
            {"framework.ui.main_window", "toggle", "keybind_actions.ui.main_window.toggle.title", "keybind_actions.ui.main_window.toggle.description"},
            {"framework.ui", "close_focused", "keybind_actions.ui.close_focused.title", "keybind_actions.ui.close_focused.description"},
            {"framework.input", "toggle_mouse_overridden", "keybind_actions.input.toggle_mouse_overridden.title", "keybind_actions.input.toggle_mouse_overridden.description"}
        },
        .loggingMetadata = {
            {"level", "settings_window.setting_names.logging.level.title", "settings_window.setting_names.logging.level.description"},
            {"sinks", "settings_window.setting_names.logging.sinks.title", ""},
            {"sinks.file", "settings_window.setting_names.logging.sinks.file.title", "settings_window.setting_names.logging.sinks.file.description"},
            {"sinks.ui", "settings_window.setting_names.logging.sinks.ui.title", "settings_window.setting_names.logging.sinks.ui.description"}
        },
        .localizationMetadata = {
            {"language", "settings_window.setting_names.localization.language.title", "settings_window.setting_names.localization.language.description"}
        },
        .uiMetadata = {
            // Metadata for the 'windows' group itself
            {"windows", "settings_window.setting_names.ui.windows.title", ""},

            // Metadata for individual windows
            {"main_window", "settings_window.setting_names.ui.windows.main_window.title", ""},
            {"plugins_window", "settings_window.setting_names.ui.windows.plugins_window.title", ""},
            {"settings_window", "settings_window.setting_names.ui.windows.settings_window.title", ""},
            {"logger_window", "settings_window.setting_names.ui.windows.logger_window.title", ""},
            {"telemetry_window", "settings_window.setting_names.ui.windows.telemetry_window.title", ""},
            {"hooks_window", "settings_window.setting_names.ui.windows.hooks_window.title", ""},
            {"game_console_window", "settings_window.setting_names.ui.windows.game_console_window.title", ""},
            {"camera_window", "settings_window.setting_names.ui.windows.camera_window.title", ""},

            // Generic metadata for window properties
            {"is_visible", "settings_window.setting_names.ui.properties.is_visible.title", "settings_window.setting_names.ui.properties.is_visible.description"},
            {"is_interactive", "settings_window.setting_names.ui.properties.is_interactive.title", "settings_window.setting_names.ui.properties.is_interactive.description"},
            {"pos_x", "settings_window.setting_names.ui.properties.pos_x.title", "settings_window.setting_names.ui.properties.pos_x.description"},
            {"pos_y", "settings_window.setting_names.ui.properties.pos_y.title", "settings_window.setting_names.ui.properties.pos_y.description"},
            {"size_w", "settings_window.setting_names.ui.properties.size_w.title", "settings_window.setting_names.ui.properties.size_w.description"},
            {"size_h", "settings_window.setting_names.ui.properties.size_h.title", "settings_window.setting_names.ui.properties.size_h.description"},
            {"is_collapsed", "settings_window.setting_names.ui.properties.is_collapsed.title", "settings_window.setting_names.ui.properties.is_collapsed.description"},
            {"is_docked", "settings_window.setting_names.ui.properties.is_docked.title", "settings_window.setting_names.ui.properties.is_docked.description"},
            {"dock_priority", "settings_window.setting_names.ui.properties.dock_priority.title", "settings_window.setting_names.ui.properties.dock_priority.description"},
            {"allow_undocking", "settings_window.setting_names.ui.properties.allow_undocking.title", "settings_window.setting_names.ui.properties.allow_undocking.description"},
            {"auto_scroll", "settings_window.setting_names.ui.properties.auto_scroll.title", "settings_window.setting_names.ui.properties.auto_scroll.description"},
        }
        };
        
        initialized = true;
    }

    return manifest;
}

}  // namespace Config

SPF_NS_END
