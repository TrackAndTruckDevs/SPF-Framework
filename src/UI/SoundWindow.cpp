#include "SPF/UI/SoundWindow.hpp"

#include "SPF/Data/GameData/SoundService.hpp"
#include "SPF/Fmod/FmodApi.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"

#include "SPF/Fmod/FmodStudioHook.hpp"

#include "imgui.h"

#include <cfloat>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace SPF::UI {
using namespace Localization;

SoundWindow::SoundWindow(const std::string& componentName, const std::string& windowId, Data::GameData::SoundService& soundService) : BaseWindow(componentName, windowId), m_soundService(soundService) {
  m_titleLocalizationKey = "sound_window.title";
  RefreshLocalization();
}

void SoundWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locNotReady = loc.Get("sound_window.not_ready");
  m_locBankComboLabel = loc.Get("sound_window.bank_combo_label");
  m_locEventComboLabel = loc.Get("sound_window.event_combo_label");
  m_locNone = loc.Get("sound_window.none");
  m_locBanksFound = loc.Get("sound_window.banks_found");
  m_locEventsFound = loc.Get("sound_window.events_found");
  m_locBankInfoTitle = loc.Get("sound_window.bank_info_title");
  m_locEventInfoTitle = loc.Get("sound_window.event_info_title");
  m_locBankPath = loc.Get("sound_window.bank_path");
  m_locEventCount = loc.Get("sound_window.event_count");
  m_locEventPath = loc.Get("sound_window.event_path");
  m_locGUID = loc.Get("sound_window.guid");
  m_locRefresh = loc.Get("sound_window.refresh");
  m_locNoSelection = loc.Get("sound_window.no_selection");
  m_locDuration = loc.Get("sound_window.duration");
  m_locIs3D = loc.Get("sound_window.is_3d");
  m_locIsLooping = loc.Get("sound_window.is_looping");
  m_locIsOneshot = loc.Get("sound_window.is_oneshot");
  m_locIsStream = loc.Get("sound_window.is_stream");
  m_locHasSustainPoint = loc.Get("sound_window.has_sustain_point");
  m_locYes = loc.Get("sound_window.yes");
  m_locNo = loc.Get("sound_window.no");
  m_locNA = loc.Get("sound_window.na");
  m_locDumpToLog = loc.Get("sound_window.dump_to_log");
  m_locParameters = loc.Get("sound_window.parameters");
  m_locParamName = loc.Get("sound_window.param_name");
  m_locParamUnits = loc.Get("sound_window.param_units");
  m_locParamMin = loc.Get("sound_window.param_min");
  m_locParamMax = loc.Get("sound_window.param_max");
  m_locParamDefault = loc.Get("sound_window.param_default");
  m_locBuses = loc.Get("sound_window.buses");
  m_locBusPath = loc.Get("sound_window.bus_path");
  m_locGlobalParameters = loc.Get("sound_window.global_parameters");
  m_locGlobalParamPath = loc.Get("sound_window.global_param_path");
  m_locPlay = loc.Get("sound_window.play");
  m_locStop = loc.Get("sound_window.stop");
  m_locPlayback = loc.Get("sound_window.playback");
  m_locNoParameters = loc.Get("sound_window.no_parameters");
  m_locVolume = loc.Get("sound_window.volume");
  m_locMute = loc.Get("sound_window.mute");
  m_locUnmute = loc.Get("sound_window.unmute");
  m_locCurrentValue = loc.Get("sound_window.current_value");
  m_locPause = loc.Get("sound_window.pause");
  m_locResume = loc.Get("sound_window.resume");
  m_locAutoLoop = loc.Get("sound_window.auto_loop");
  m_locValueChanged = loc.Get("sound_window.value_changed");
  m_locSelectParam = loc.Get("sound_window.select_param");
  m_locVCAs = loc.Get("sound_window.vcas");
  m_locVCAPath = loc.Get("sound_window.vca_path");
  m_locSelectVCA = loc.Get("sound_window.select_vca");
  m_locListener = loc.Get("sound_window.listener");
  m_locNumListeners = loc.Get("sound_window.num_listeners");
  m_locPosition = loc.Get("sound_window.position");
  m_locVelocity = loc.Get("sound_window.velocity");
  m_locForward = loc.Get("sound_window.forward");
  m_locUp = loc.Get("sound_window.up");
  m_locSearchFilter = loc.Get("sound_window.search_filter");
  m_locPitch = loc.Get("sound_window.pitch");
  m_locTimelinePosition = loc.Get("sound_window.timeline_position");
  m_locInstances = loc.Get("sound_window.instances");
  m_locInstanceCount = loc.Get("sound_window.instance_count");
  m_locIsSnapshot = loc.Get("sound_window.is_snapshot");
  m_locIsDoppler = loc.Get("sound_window.is_doppler");
  m_locMinMaxDistance = loc.Get("sound_window.min_max_distance");
  m_locSoundSize = loc.Get("sound_window.sound_size");
  m_locSampleState = loc.Get("sound_window.sample_state");
  m_locListenerLabel = loc.Get("sound_window.listener_label");
  m_locColBus = loc.Get("sound_window.col_bus");
  m_locColParameter = loc.Get("sound_window.col_parameter");
  m_locColType = loc.Get("sound_window.col_type");
  m_locColRange = loc.Get("sound_window.col_range");
  m_locColDefault = loc.Get("sound_window.col_default");
  m_locColValue = loc.Get("sound_window.col_value");
  m_locTypeUser = loc.Get("sound_window.type_user");
  m_locTypeAuto = loc.Get("sound_window.type_auto");
  m_locColFader = loc.Get("sound_window.col_fader");
  m_locColBypass = loc.Get("sound_window.col_bypass");
  m_locUserProperties = loc.Get("sound_window.user_properties");
  m_locActiveInstances = loc.Get("sound_window.active_instances");
  m_locAttach = loc.Get("sound_window.attach");
  m_locAttached = loc.Get("sound_window.attached");
  m_locDetach = loc.Get("sound_window.detach");
  m_locResetToGame = loc.Get("sound_window.reset_to_game");
  m_locCreateNewInstance = loc.Get("sound_window.create_new_instance");
  m_locLoadingState = loc.Get("sound_window.loading_state");
  m_locSampleLoadingState = loc.Get("sound_window.sample_loading_state");
  m_locBankBusCount = loc.Get("sound_window.bank_bus_count");
  m_locBankVcaCount = loc.Get("sound_window.bank_vca_count");
  m_locPropBoolean = loc.Get("sound_window.prop_boolean");
  m_locPropInteger = loc.Get("sound_window.prop_integer");
  m_locPropFloat = loc.Get("sound_window.prop_float");
  m_locPropString = loc.Get("sound_window.prop_string");
}

void SoundWindow::RefreshSoundList() {
  if (m_activeInstance) {
    if (m_ownsInstance) {
      m_soundService.StopEvent(m_activeInstance, true);
      m_soundService.ReleaseEventInstance(m_activeInstance);
    } else {
      m_soundService.StopEvent(m_activeInstance, true);
    }
    m_activeInstance = nullptr;
    m_ownsInstance = false;
    m_playbackState = -1;
    Fmod::FmodStudioHook::GetInstance().RemoveAllOverrides();
  }
  m_banks = m_soundService.GetSoundBankGroups();
  m_bankLoadInfos = m_soundService.GetLoadedBanksInfo();
  m_enriched = false;
  m_parametersEnriched = false;

  m_bankComboItems.clear();
  m_bankComboItems.push_back(m_locNone.c_str());
  for (const auto& bank : m_banks) {
    m_bankComboItems.push_back(bank.bankPath.c_str());
  }

  if (m_selectedBank >= (int)m_banks.size()) m_selectedBank = -1;
  m_selectedEvent = -1;
  m_eventComboItems.clear();

  RefreshBusAndParamLists();
  RefreshVCAList();
}

void SoundWindow::RefreshBusAndParamLists() {
  m_buses = m_soundService.GetBuses();
  m_globalParams = m_soundService.GetGlobalParameters();

  m_globalParamValues.clear();
  m_globalParamValues.resize(m_globalParams.size());
  for (size_t i = 0; i < m_globalParams.size(); ++i) {
    m_soundService.GetGlobalParamValue(m_globalParams[i].paramPath, m_globalParamValues[i]);
  }

  m_busSortOrder.resize(m_buses.size());
  for (size_t i = 0; i < m_buses.size(); ++i) m_busSortOrder[i] = (int)i;
  std::sort(m_busSortOrder.begin(), m_busSortOrder.end(), [this](int a, int b) {
    return m_buses[a].busPath < m_buses[b].busPath;
  });
}

void SoundWindow::RefreshVCAList() {
  m_vcas = m_soundService.GetVCAs();

  m_vcaComboItems.clear();
  m_vcaComboItems.push_back(m_locNone.c_str());
  for (const auto& vca : m_vcas) {
    m_vcaComboItems.push_back(vca.vcaPath.c_str());
  }
  if (m_selectedVCA >= (int)m_vcas.size()) m_selectedVCA = -1;

  m_vcaInfos.clear();
  m_vcaInfos.resize(m_vcas.size());
  for (size_t i = 0; i < m_vcas.size(); ++i) {
    m_vcaInfos[i].vcaPath = m_vcas[i].vcaPath;
    void* vcaHandle = m_soundService.GetVCAByPath(m_vcas[i].vcaPath.c_str());
    if (vcaHandle) {
      m_soundService.GetVCAVolume(vcaHandle, m_vcaInfos[i].volume, m_vcaInfos[i].volume);
    }
  }
}

void SoundWindow::RenderContent() {
  if (!m_soundService.IsReady()) {
    if (m_activeInstance) {
      if (m_ownsInstance) m_soundService.ReleaseEventInstance(m_activeInstance);
      m_activeInstance = nullptr;
      m_ownsInstance = false;
      m_playbackState = -1;
      m_autoLoop = false;
      Fmod::FmodStudioHook::GetInstance().RemoveAllOverrides();
    }
    m_listInitialized = false;
    m_enriched = false;
    m_parametersEnriched = false;
    m_banks.clear();
    m_bankLoadInfos.clear();
    m_buses.clear();
    m_globalParams.clear();
    m_globalParamValues.clear();
    m_vcas.clear();
    m_vcaInfos.clear();
    m_selectedBank = -1;
    m_selectedEvent = -1;
    m_selectedBus = -1;
    m_selectedParam = -1;
    m_selectedVCA = -1;
    m_paramComboItems.clear();
    m_vcaComboItems.clear();

    Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", m_locNotReady.c_str());
    return;
  }

  if (!m_listInitialized) {
    RefreshSoundList();
    m_listInitialized = true;
  }

  if (!m_enriched) {
    m_soundService.EnrichEventsWithFmodData(m_banks);
    for (const auto& group : m_banks) {
      for (const auto& ev : group.events) {
        if (ev.eventDesc) {
          Fmod::FmodStudioHook::GetInstance().PopulateDescPathCache(ev.eventDesc, ev.eventPath.c_str());
        }
      }
    }
    m_enriched = true;
  }

  // --- Toolbar ---
  if (ImGui::Button(m_locRefresh.c_str())) {
    RefreshSoundList();
  }
  ImGui::SameLine();
  if (ImGui::Button(m_locDumpToLog.c_str())) {
    m_soundService.DumpAllEventsToLog();
  }

  int totalEvents = 0;
  for (const auto& b : m_banks) totalEvents += (int)b.events.size();
  ImGui::SameLine();
  ImGui::TextDisabled("  ");
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::GREEN), m_locBanksFound.c_str(), (int)m_banks.size());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::GREEN), m_locEventsFound.c_str(), totalEvents);

  // --- Tab bar ---
  ImGui::Spacing();
  if (ImGui::BeginTabBar("SoundTabs")) {
    if (ImGui::BeginTabItem("Events")) {
      m_activeTab = Tab::Events;
      RenderTabEvents();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Buses")) {
      m_activeTab = Tab::Buses;
      RenderTabBuses();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("VCAs")) {
      m_activeTab = Tab::VCAs;
      RenderTabVCAs();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Global Params")) {
      m_activeTab = Tab::GlobalParams;
      RenderTabGlobalParams();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Listener")) {
      m_activeTab = Tab::Listener;
      RenderTabListener();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void SoundWindow::RenderEventDetail(const Data::GameData::SoundEvent& ev) {
  const float tableWidth = ImGui::GetContentRegionAvail().x;

  char guidHex[33];
  for (int i = 0; i < 16; ++i) snprintf(guidHex + i * 2, 3, "%02x", ev.guid[i]);
  guidHex[32] = '\0';

  if (ImGui::BeginTable("event_detail", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX)) {
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, tableWidth * 0.35f);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);

    auto rowLabel = [&](const char* label) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", label);
      ImGui::TableSetColumnIndex(1);
    };

    rowLabel(m_locBankPath.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.bankPath.c_str());

    rowLabel(m_locEventPath.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.eventPath.c_str());

    rowLabel(m_locGUID.c_str());
    Typography::Text(TextStyle::Monospace().Color(Colors::YELLOW), "%s", guidHex);

    if (ev.hasDuration) {
      rowLabel(m_locDuration.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%u ms", ev.durationMs);
    }
    if (ev.hasIs3D) {
      rowLabel(m_locIs3D.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.is3D ? m_locYes.c_str() : m_locNo.c_str());
    }
    if (ev.hasIsOneshot) {
      rowLabel(m_locIsOneshot.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.isOneshot ? m_locYes.c_str() : m_locNo.c_str());
    }
    if (ev.hasIsStream) {
      rowLabel(m_locIsStream.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.isStream ? m_locYes.c_str() : m_locNo.c_str());
    }
    if (ev.hasIsSnapshot) {
      rowLabel(m_locIsSnapshot.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.isSnapshot ? m_locYes.c_str() : m_locNo.c_str());
    }
    if (ev.hasIsDopplerEnabled) {
      rowLabel(m_locIsDoppler.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.isDopplerEnabled ? m_locYes.c_str() : m_locNo.c_str());
    }
    if (ev.hasHasSustainPoint) {
      rowLabel(m_locHasSustainPoint.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", ev.hasSustainPoint ? m_locYes.c_str() : m_locNo.c_str());
    }
    if (ev.hasIs3D) {
      rowLabel(m_locMinMaxDistance.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%.2f / %.2f", ev.minDistance, ev.maxDistance);
    }
    if (ev.soundSize > 0) {
      rowLabel(m_locSoundSize.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%u bytes", ev.soundSize);
    }
    if (ev.sampleLoadingState != 0) {
      rowLabel(m_locSampleState.c_str());
      Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%d", ev.sampleLoadingState);
    }

    rowLabel(m_locInstanceCount.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%d", ev.instanceCount);

    ImGui::EndTable();
  }

  if (!ev.userProperties.empty()) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "%s (%d)", m_locUserProperties.c_str(), (int)ev.userProperties.size());
    ImGui::Spacing();

    if (ImGui::BeginTable("user_props", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
      ImGui::TableSetupColumn("type", ImGuiTableColumnFlags_WidthStretch, 1.5f);
      ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 2.0f);

      ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s", m_locParamName.c_str());
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%s", m_locColType.c_str());
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%s", m_locColValue.c_str());

      for (const auto& up : ev.userProperties) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%s", up.name.c_str());
        ImGui::TableSetColumnIndex(1);
        const char* typeName = m_locPropString.c_str();
        if (up.type == 0) typeName = m_locPropBoolean.c_str();
        else if (up.type == 1) typeName = m_locPropInteger.c_str();
        else if (up.type == 2) typeName = m_locPropFloat.c_str();
        ImGui::Text("%s", typeName);
        ImGui::TableSetColumnIndex(2);
        if (up.type == 0) ImGui::Text("%s", up.boolValue ? m_locYes.c_str() : m_locNo.c_str());
        else if (up.type == 1) ImGui::Text("%d", up.intValue);
        else if (up.type == 2) ImGui::Text("%.3f", up.floatValue);
        else if (up.type == 3) ImGui::Text("%s", up.stringValue.c_str());
      }
      ImGui::EndTable();
    }
  }
}

void SoundWindow::RenderInstanceControls(const std::string& eventPath, bool is3D) {
  if (!m_activeInstance) return;

  m_playbackState = m_soundService.GetEventPlaybackState(m_activeInstance);
  const char* stateStr = "Unknown";
  if (m_playbackState == 0)
    stateStr = "Playing";
  else if (m_playbackState == 1)
    stateStr = "Sustaining";
  else if (m_playbackState == 2)
    stateStr = "Stopped";
  else if (m_playbackState == 3)
    stateStr = "Starting";
  else if (m_playbackState == 4)
    stateStr = "Stopping";

  ImGui::Text("State: %s", stateStr);

  // --- Volume ---
  float volume = 0, finalVolume = 0;
  m_soundService.GetEventVolume(m_activeInstance, volume, finalVolume);
  ImGui::SetNextItemWidth(200);
  if (ImGui::SliderFloat(m_locVolume.c_str(), &volume, 0.0f, 10.0f, "%.2f")) {
    m_soundService.SetEventVolume(m_activeInstance, volume);
  }

  // --- Pitch ---
  float pitch = 0, finalPitch = 0;
  m_soundService.GetEventPitch(m_activeInstance, pitch, finalPitch);
  ImGui::SetNextItemWidth(200);
  if (ImGui::SliderFloat(m_locPitch.c_str(), &pitch, -8.0f, 8.0f, "%.2f")) {
    m_soundService.SetEventPitch(m_activeInstance, pitch);
  }

  // --- Timeline position ---
  if (m_playbackState == 0 || m_playbackState == 1) {
    int pos = 0;
    m_soundService.GetEventTimelinePosition(m_activeInstance, pos);
    ImGui::Text("%s: %d ms", m_locTimelinePosition.c_str(), pos);
  }

  bool isGameInstance = !m_ownsInstance && !eventPath.empty();

  // --- 3D Attributes (only for 3D events) ---
  if (is3D) {
    bool isPlaying = (m_playbackState == 0 || m_playbackState == 1);
    if (isPlaying && !m_has3DCache) {
      float px = 0, py = 0, pz = 0;
      float vx = 0, vy = 0, vz = 0;
      float fx = 0, fy = 0, fz = 0;
      float ux = 0, uy = 0, uz = 0;
      if (m_soundService.GetEvent3DAttributes(m_activeInstance, px, py, pz, vx, vy, vz, fx, fy, fz, ux, uy, uz)) {
        m_cached3DPos[0] = px; m_cached3DPos[1] = py; m_cached3DPos[2] = pz;
        m_cached3DVel[0] = vx; m_cached3DVel[1] = vy; m_cached3DVel[2] = vz;
        m_cached3DFwd[0] = fx; m_cached3DFwd[1] = fy; m_cached3DFwd[2] = fz;
        m_cached3DUp[0] = ux; m_cached3DUp[1] = uy; m_cached3DUp[2] = uz;
        m_has3DCache = true;
      }
    }

    if (ImGui::TreeNode("3D Attributes")) {
      if (m_has3DCache) {
        const float colW = 100.0f;
        if (ImGui::BeginTable("##3dtable", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX)) {
          ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 120.0f);
          ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, colW + 20.0f);
          ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed, colW + 20.0f);
          ImGui::TableSetupColumn("Z", ImGuiTableColumnFlags_WidthFixed, colW + 20.0f);
          ImGui::TableHeadersRow();

          auto dragRow = [&](int row, const char* label, float* v) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", label);
            for (int c = 0; c < 3; ++c) {
              ImGui::TableSetColumnIndex(c + 1);
              ImGui::SetNextItemWidth(colW);
              char id[32];
              snprintf(id, sizeof(id), "##3d_%d_%d", row, c);
              ImGui::DragFloat(id, &v[c], 0.1f, 0.0f, 0.0f, "%.2f");
            }
          };

          dragRow(0, m_locPosition.c_str(), m_cached3DPos);
          dragRow(1, m_locForward.c_str(), m_cached3DFwd);
          dragRow(2, m_locUp.c_str(), m_cached3DUp);
          dragRow(3, m_locVelocity.c_str(), m_cached3DVel);

          ImGui::EndTable();
        }

        ImGui::Spacing();
        if (isPlaying) {
          if (isGameInstance) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.25f, 1.0f));
            if (ImGui::Button("Apply 3D")) {
              auto& hook = Fmod::FmodStudioHook::GetInstance();
              FMOD_3D_ATTRIBUTES attrs = {};
              attrs.position = {m_cached3DPos[0], m_cached3DPos[1], m_cached3DPos[2]};
              attrs.velocity = {m_cached3DVel[0], m_cached3DVel[1], m_cached3DVel[2]};
              attrs.forward = {m_cached3DFwd[0], m_cached3DFwd[1], m_cached3DFwd[2]};
              attrs.up = {m_cached3DUp[0], m_cached3DUp[1], m_cached3DUp[2]};
              hook.Override3DAttributes(eventPath, attrs);
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
            if (ImGui::SmallButton(m_locResetToGame.c_str())) {
              auto& hook = Fmod::FmodStudioHook::GetInstance();
              hook.Reset3DToOriginal(eventPath);
              m_has3DCache = false;
            }
            ImGui::PopStyleColor(3);
          } else {
            if (ImGui::Button("Apply 3D")) {
              m_soundService.SetEvent3DAttributes(m_activeInstance,
                m_cached3DPos[0], m_cached3DPos[1], m_cached3DPos[2],
                m_cached3DVel[0], m_cached3DVel[1], m_cached3DVel[2],
                m_cached3DFwd[0], m_cached3DFwd[1], m_cached3DFwd[2],
                m_cached3DUp[0], m_cached3DUp[1], m_cached3DUp[2]);
            }
          }
        } else {
          ImGui::TextDisabled("%s", m_locNoSelection.c_str());
        }
      } else {
        ImGui::TextDisabled("Attach to instance to edit 3D attributes");
      }
      ImGui::TreePop();
    }
  }

  // --- Per-event parameters ---
  {
    bool isPlaying = (m_playbackState == 0 || m_playbackState == 1);
    if (m_selectedBank >= 0 && m_selectedBank < (int)m_banks.size() && m_selectedEvent >= 0 && m_selectedEvent < (int)m_banks[m_selectedBank].events.size()) {
      const auto& ev = m_banks[m_selectedBank].events[m_selectedEvent];
      if (!ev.parameters.empty()) {
        if (ImGui::TreeNode("Event Parameters")) {
          if (isPlaying) {
            for (const auto& p : ev.parameters) {
              float val = 0, fval = 0;
              m_soundService.GetEventParameterByName(m_activeInstance, p.name.c_str(), val, fval);
              ImGui::SetNextItemWidth(200);
              if (ImGui::SliderFloat(p.name.c_str(), &val, p.minimum, p.maximum, "%.2f")) {
                if (isGameInstance) {
                  Fmod::FmodStudioHook::GetInstance().OverrideParameter(eventPath, p.name, val);
                } else {
                  m_soundService.SetEventParameterByName(m_activeInstance, p.name.c_str(), val, false);
                }
              }
              if (isGameInstance) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.3f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.4f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.25f, 0.1f, 1.0f));
                char resetLabel[64];
                snprintf(resetLabel, sizeof(resetLabel), "R##%s", p.name.c_str());
                if (ImGui::SmallButton(resetLabel)) {
                  Fmod::FmodStudioHook::GetInstance().RemoveParameterOverride(eventPath, p.name);
                }
                ImGui::PopStyleColor(3);
              }
            }
          } else {
            for (const auto& p : ev.parameters) {
              ImGui::Text("%s: %.2f [%.2f .. %.2f]", p.name.c_str(), p.defaultvalue, p.minimum, p.maximum);
            }
            ImGui::TextDisabled("Attach to instance to control parameters");
          }
          ImGui::TreePop();
        }
      }
    }
  }
}

void SoundWindow::RenderTabEvents() {
  ImGui::Spacing();

  // --- Search filter ---
  ImGui::SetNextItemWidth(300);
  if (ImGui::InputTextWithHint("##search", m_locSearchFilter.c_str(), m_searchFilter, sizeof(m_searchFilter))) {
    if (std::strcmp(m_searchFilter, m_lastSearchFilter) != 0) {
      std::strncpy(m_lastSearchFilter, m_searchFilter, sizeof(m_lastSearchFilter));
      if (m_selectedBank >= 0 && m_selectedBank < (int)m_banks.size()) {
        m_eventComboItems.clear();
        m_eventComboItems.push_back(m_locNone.c_str());
        for (const auto& ev : m_banks[m_selectedBank].events) {
          if (m_searchFilter[0] == '\0' || ev.eventPath.find(m_searchFilter) != std::string::npos) {
            m_eventComboItems.push_back(ev.eventPath.c_str());
          }
        }
        m_selectedEvent = -1;
      }
    }
  }

  // --- Bank combo ---
  int bankIdx = m_selectedBank + 1;
  if (ImGui::Combo(m_locBankComboLabel.c_str(), &bankIdx, m_bankComboItems.data(), (int)m_bankComboItems.size())) {
    m_selectedBank = bankIdx - 1;
      if (m_activeInstance) {
      m_soundService.StopEvent(m_activeInstance, true);
      if (m_ownsInstance) m_soundService.ReleaseEventInstance(m_activeInstance);
      m_activeInstance = nullptr;
      m_ownsInstance = false;
      m_playbackState = -1;
      Fmod::FmodStudioHook::GetInstance().RemoveAllOverrides();
    }
    m_selectedEvent = -1;
    m_parametersEnriched = false;

    m_eventComboItems.clear();
    if (m_selectedBank >= 0 && m_selectedBank < (int)m_banks.size()) {
      m_eventComboItems.push_back(m_locNone.c_str());
      for (const auto& ev : m_banks[m_selectedBank].events) {
        if (m_searchFilter[0] == '\0' || ev.eventPath.find(m_searchFilter) != std::string::npos) {
          m_eventComboItems.push_back(ev.eventPath.c_str());
        }
      }
    }
  }

  if (m_selectedBank < 0 || m_selectedBank >= (int)m_banks.size()) {
    Typography::Text(TextStyle::Regular().Color(Colors::GRAY), "%s", m_locNoSelection.c_str());
    return;
  }

  const auto& bank = m_banks[m_selectedBank];

  // --- Event combo ---
  if (m_eventComboItems.empty()) {
    m_eventComboItems.push_back(m_locNone.c_str());
    for (const auto& ev : bank.events) {
      if (m_searchFilter[0] == '\0' || ev.eventPath.find(m_searchFilter) != std::string::npos) {
        m_eventComboItems.push_back(ev.eventPath.c_str());
      }
    }
  }

  int eventIdx = m_selectedEvent + 1;
  if (ImGui::Combo(m_locEventComboLabel.c_str(), &eventIdx, m_eventComboItems.data(), (int)m_eventComboItems.size())) {
    if (m_selectedEvent != eventIdx - 1) {
      if (m_activeInstance) {
        if (m_ownsInstance) {
          m_soundService.StopEvent(m_activeInstance, true);
          m_soundService.ReleaseEventInstance(m_activeInstance);
        } else {
          m_soundService.StopEvent(m_activeInstance, true);
        }
        m_activeInstance = nullptr;
        m_ownsInstance = false;
        m_playbackState = -1;
        Fmod::FmodStudioHook::GetInstance().RemoveAllOverrides();
      }
      m_selectedEvent = eventIdx - 1;
      m_parametersEnriched = false;
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Bank info ---
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s", m_locBankInfoTitle.c_str());
  ImGui::Spacing();

  const float tableWidth = ImGui::GetContentRegionAvail().x;
  if (ImGui::BeginTable("bank_info", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX)) {
    ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, tableWidth * 0.35f);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
    auto rowLabel = [&](const char* label) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", label);
      ImGui::TableSetColumnIndex(1);
    };
    rowLabel(m_locBankPath.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", bank.bankPath.c_str());
    rowLabel(m_locEventCount.c_str());
    Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%d", (int)bank.events.size());

    for (const auto& li : m_bankLoadInfos) {
      if (li.bankPath == bank.bankPath) {
        rowLabel(m_locLoadingState.c_str());
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%d", li.loadingState);
        rowLabel(m_locSampleLoadingState.c_str());
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%d", li.sampleLoadingState);
        rowLabel(m_locBankBusCount.c_str());
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%d", li.busCount);
        rowLabel(m_locBankVcaCount.c_str());
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%d", li.vcaCount);
        break;
      }
    }

    ImGui::EndTable();
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (m_selectedEvent < 0 || m_selectedEvent >= (int)bank.events.size()) {
    Typography::Text(TextStyle::Regular().Color(Colors::GRAY), "%s", m_locNoSelection.c_str());
    return;
  }

  const auto& ev = bank.events[m_selectedEvent];

  Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "%s", m_locEventInfoTitle.c_str());
  ImGui::Spacing();
  RenderEventDetail(ev);

  // --- Playback controls ---
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  Typography::Text(TextStyle::H3().Color(Colors::GREEN), "%s", m_locPlayback.c_str());
  ImGui::Spacing();

  // --- Active game instances ---
  {
    int activeCount = m_soundService.GetEventInstanceCount(ev.eventDesc);
    ImGui::Text("%s %d", m_locActiveInstances.c_str(), activeCount);
    if (activeCount > 0) {
      auto instances = m_soundService.GetEventInstanceList(ev.eventDesc);
      for (int i = 0; i < (int)instances.size(); ++i) {
        bool isAttached = (instances[i] == m_activeInstance);
        char btnLabel[64];
        ImGui::SameLine();
        if (isAttached) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
          snprintf(btnLabel, sizeof(btnLabel), "%s ##inst_%d", m_locAttached.c_str(), i);
          ImGui::SmallButton(btnLabel);
          ImGui::PopStyleColor(3);
        } else {
          snprintf(btnLabel, sizeof(btnLabel), "%s ##inst_%d", m_locAttach.c_str(), i);
          if (ImGui::SmallButton(btnLabel)) {
            if (m_activeInstance && m_ownsInstance) {
              m_soundService.StopEvent(m_activeInstance, true);
              m_soundService.ReleaseEventInstance(m_activeInstance);
            }
            m_activeInstance = instances[i];
            m_ownsInstance = false;
            m_has3DCache = false;
            m_playbackState = m_soundService.GetEventPlaybackState(m_activeInstance);
          }
        }
      }
    }
  }

  ImGui::Spacing();

  // --- Detach button (only when attached to a game instance) ---
  if (m_activeInstance && !m_ownsInstance) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.3f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.4f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.25f, 0.1f, 1.0f));
    if (ImGui::Button(m_locDetach.c_str())) {
      m_activeInstance = nullptr;
      m_ownsInstance = false;
      m_playbackState = -1;
      m_has3DCache = false;
      Fmod::FmodStudioHook::GetInstance().RemoveAllOverrides();
    }
    ImGui::PopStyleColor(3);
    ImGui::Spacing();
  }

  // --- Playback controls ---
  if (!m_activeInstance) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.65f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.55f, 0.75f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.35f, 0.55f, 1.0f));
    if (ImGui::Button(m_locPlay.c_str())) {
      m_activeInstance = m_soundService.CreateEventInstance(ev.guid);
      if (m_activeInstance) {
        m_soundService.StartEvent(m_activeInstance);
        m_ownsInstance = true;
        m_has3DCache = false;
      }
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", m_locCreateNewInstance.c_str());
  } else {
    if (m_autoLoop && m_playbackState == 2) {
      m_soundService.StartEvent(m_activeInstance);
      m_playbackState = m_soundService.GetEventPlaybackState(m_activeInstance);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.65f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.55f, 0.75f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.35f, 0.55f, 1.0f));
    if (m_playbackState == 2 || m_playbackState == -1) {
      if (ImGui::Button(m_locPlay.c_str())) {
        m_soundService.StartEvent(m_activeInstance);
      }
    } else {
      if (ImGui::Button(m_locPause.c_str())) {
        m_soundService.PauseEvent(m_activeInstance, true);
      }
      ImGui::SameLine();
      if (ImGui::Button(m_locResume.c_str())) {
        m_soundService.PauseEvent(m_activeInstance, false);
      }
    }
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button(m_locStop.c_str())) {
      if (m_ownsInstance) {
        m_soundService.StopEvent(m_activeInstance, true);
        m_soundService.ReleaseEventInstance(m_activeInstance);
      }
      m_autoLoop = false;
      m_activeInstance = nullptr;
      m_ownsInstance = false;
      m_playbackState = -1;
      m_has3DCache = false;
      Fmod::FmodStudioHook::GetInstance().RemoveAllOverrides();
    }
    ImGui::PopStyleColor(6);
  }

  ImGui::SameLine();
  ImGui::Checkbox(m_locAutoLoop.c_str(), &m_autoLoop);

  // --- Instance controls (volume, pitch, 3D, params) ---
  RenderInstanceControls(ev.eventPath, ev.is3D);

  // --- Enrich parameters if not yet ---
  if (!m_parametersEnriched) {
    m_soundService.EnrichEventParameters(m_banks);
    m_parametersEnriched = true;
  }

  // --- Parameters table ---
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "%s (%d)", m_locParameters.c_str(), (int)ev.parameters.size());
  ImGui::Spacing();

  if (ev.parameters.empty()) {
    Typography::Text(TextStyle::Regular().Color(Colors::GRAY), "%s", m_locNoParameters.c_str());
  } else {
    if (ImGui::BeginTable("event_params", 5, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingStretchProp)) {
      ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
      ImGui::TableSetupColumn("units", ImGuiTableColumnFlags_WidthStretch, 1.5f);
      ImGui::TableSetupColumn("min", ImGuiTableColumnFlags_WidthStretch, 1.0f);
      ImGui::TableSetupColumn("max", ImGuiTableColumnFlags_WidthStretch, 1.0f);
      ImGui::TableSetupColumn("default", ImGuiTableColumnFlags_WidthStretch, 1.0f);

      ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
      ImGui::TableSetColumnIndex(0);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", m_locParamName.c_str());
      ImGui::TableSetColumnIndex(1);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", m_locParamUnits.c_str());
      ImGui::TableSetColumnIndex(2);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", m_locParamMin.c_str());
      ImGui::TableSetColumnIndex(3);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", m_locParamMax.c_str());
      ImGui::TableSetColumnIndex(4);
      Typography::Text(TextStyle::Regular().Color(Colors::LIGHT_GRAY), "%s", m_locParamDefault.c_str());

      for (const auto& param : ev.parameters) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", param.name.c_str());
        ImGui::TableSetColumnIndex(1);
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "-");
        ImGui::TableSetColumnIndex(2);
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%.2f", param.minimum);
        ImGui::TableSetColumnIndex(3);
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%.2f", param.maximum);
        ImGui::TableSetColumnIndex(4);
        Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%.2f", param.defaultvalue);
      }
      ImGui::EndTable();
    }
  }
}

void SoundWindow::RenderTabBuses() {
  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s (%d)", m_locBuses.c_str(), (int)m_buses.size());
  ImGui::Spacing();

  if (m_busSortOrder.empty()) return;

  if (ImGui::BeginTable("##busTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg, ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()))) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(m_locBusPath.c_str(), ImGuiTableColumnFlags_WidthStretch, 0.5f);
    ImGui::TableSetupColumn(m_locVolume.c_str(), ImGuiTableColumnFlags_WidthFixed, 140.0f);
    ImGui::TableSetupColumn(m_locMute.c_str(), ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn(m_locColFader.c_str(), ImGuiTableColumnFlags_WidthFixed, 140.0f);
    ImGui::TableSetupColumn(m_locColBypass.c_str(), ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableHeadersRow();

    for (int si = 0; si < (int)m_busSortOrder.size(); ++si) {
      size_t busIdx = (size_t)m_busSortOrder[si];
      auto& bus = m_buses[busIdx];

      ImGui::TableNextRow();
      ImGui::PushID((int)busIdx);

      ImGui::TableSetColumnIndex(0);
      int depth = 0;
      for (char c : bus.busPath) { if (c == '/') depth++; }
      if (depth > 0) ImGui::Indent(depth * 16.0f);
      ImGui::Text("%s", bus.busPath.c_str());
      if (depth > 0) ImGui::Unindent(depth * 16.0f);

      ImGui::TableSetColumnIndex(1);
      float vol = bus.volume;
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (ImGui::SliderFloat("##vol", &vol, 0.0f, 1.0f, "%.2f")) {
        if (m_soundService.SetBusVolume(bus.busPath, vol)) bus.volume = vol;
      }

      ImGui::TableSetColumnIndex(2);
      bool muted = bus.isMuted;
      if (ImGui::Checkbox("##mute", &muted)) {
        if (m_soundService.SetBusMute(bus.busPath, muted)) bus.isMuted = muted;
      }

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%.3f", bus.faderLevel);
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("Fader level: secondary volume control (final = volume * fader)");

      ImGui::TableSetColumnIndex(4);
      ImGui::Text("%s", bus.isBypassed ? m_locYes.c_str() : m_locNo.c_str());
      if (ImGui::IsItemHovered()) ImGui::SetTooltip("Bypass: skips all audio processing on this bus");

      ImGui::PopID();
    }

    ImGui::EndTable();
  }
}

void SoundWindow::RenderTabVCAs() {
  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s (%d)", m_locVCAs.c_str(), (int)m_vcas.size());
  ImGui::Spacing();

  int vcaIdx = m_selectedVCA + 1;
  if (ImGui::Combo(m_locSelectVCA.c_str(), &vcaIdx, m_vcaComboItems.data(), (int)m_vcaComboItems.size())) {
    m_selectedVCA = vcaIdx - 1;
  }

  if (m_selectedVCA >= 0 && m_selectedVCA < (int)m_vcas.size()) {
    auto& info = m_vcaInfos[m_selectedVCA];
    ImGui::Text("%s: %s", m_locVCAPath.c_str(), info.vcaPath.c_str());
    ImGui::Spacing();

    float vol = info.volume;
    ImGui::SetNextItemWidth(300);
    if (ImGui::SliderFloat(m_locVolume.c_str(), &vol, 0.0f, 10.0f, "%.2f")) {
      void* vcaHandle = m_soundService.GetVCAByPath(info.vcaPath.c_str());
      if (vcaHandle && m_soundService.SetVCAVolume(vcaHandle, vol)) {
        info.volume = vol;
      }
    }
  }
}

void SoundWindow::RenderTabGlobalParams() {
  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s (%d)", m_locGlobalParameters.c_str(), (int)m_globalParams.size());
  ImGui::Spacing();

  if (m_globalParams.empty()) return;

  if (ImGui::BeginTable("##paramTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg, ImVec2(0, -ImGui::GetTextLineHeightWithSpacing()))) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(m_locColParameter.c_str(), ImGuiTableColumnFlags_WidthStretch, 0.4f);
    ImGui::TableSetupColumn(m_locColType.c_str(), ImGuiTableColumnFlags_WidthFixed, 80.0f);
    ImGui::TableSetupColumn(m_locColRange.c_str(), ImGuiTableColumnFlags_WidthFixed, 140.0f);
    ImGui::TableSetupColumn(m_locColDefault.c_str(), ImGuiTableColumnFlags_WidthFixed, 70.0f);
    ImGui::TableSetupColumn(m_locColValue.c_str(), ImGuiTableColumnFlags_WidthFixed, 130.0f);
    ImGui::TableHeadersRow();

    for (size_t i = 0; i < m_globalParams.size(); ++i) {
      auto& gp = m_globalParams[i];
      ImGui::PushID((int)i);
      ImGui::TableNextRow();

      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s", gp.paramPath.c_str());

      ImGui::TableSetColumnIndex(1);
      bool editable = gp.isEditable;
      const char* typeStr = (gp.type == 0) ? m_locTypeUser.c_str() : m_locTypeAuto.c_str();
      ImGui::TextColored((gp.type == 0) ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", typeStr);

      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%.2f .. %.2f", gp.minimum, gp.maximum);

      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%.2f", gp.defaultvalue);

      ImGui::TableSetColumnIndex(4);
      if (i < m_globalParamValues.size()) {
        if (editable) {
          m_soundService.GetGlobalParamValue(gp.paramPath, m_globalParamValues[i]);
          float val = m_globalParamValues[i];
          ImGui::SetNextItemWidth(-FLT_MIN);
          if (ImGui::SliderFloat("##val", &val, gp.minimum, gp.maximum, "%.3f")) {
            if (m_soundService.SetGlobalParamValue(gp.paramPath, val)) {
              m_globalParamValues[i] = val;
            }
          }
        } else {
          m_soundService.GetGlobalParamValue(gp.paramPath, m_globalParamValues[i]);
          ImGui::Text("%.3f", m_globalParamValues[i]);
        }
      }

      ImGui::PopID();
    }

    ImGui::EndTable();
  }
}

void SoundWindow::RenderTabListener() {
  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s", m_locListener.c_str());
  ImGui::Spacing();

  int numListeners = m_soundService.GetNumListeners();
  ImGui::Text("%s: %d", m_locNumListeners.c_str(), numListeners);

  for (int i = 0; i < numListeners; ++i) {
    float px = 0, py = 0, pz = 0;
    float vx = 0, vy = 0, vz = 0;
    float fx = 0, fy = 0, fz = 0;
    float ux = 0, uy = 0, uz = 0;
    m_soundService.GetListenerAttributes(i, px, py, pz, vx, vy, vz, fx, fy, fz, ux, uy, uz);

    char label[64];
    snprintf(label, sizeof(label), m_locListenerLabel.c_str(), i);
    if (ImGui::TreeNode(label)) {
      ImGui::Text("%s:", m_locPosition.c_str());
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gp_x", &px, 0.0f);
      ImGui::SameLine(120);
      ImGui::Text("X");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gp_y", &py, 0.0f);
      ImGui::SameLine(240);
      ImGui::Text("Y");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gp_z", &pz, 0.0f);
      ImGui::SameLine(360);
      ImGui::Text("Z");
      ImGui::Spacing();

      ImGui::Text("%s:", m_locVelocity.c_str());
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gv_x", &vx, 0.0f);
      ImGui::SameLine(120);
      ImGui::Text("X");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gv_y", &vy, 0.0f);
      ImGui::SameLine(240);
      ImGui::Text("Y");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gv_z", &vz, 0.0f);
      ImGui::SameLine(360);
      ImGui::Text("Z");
      ImGui::Spacing();

      ImGui::Text("%s:", m_locForward.c_str());
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gf_x", &fx, 0.0f);
      ImGui::SameLine(120);
      ImGui::Text("X");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gf_y", &fy, 0.0f);
      ImGui::SameLine(240);
      ImGui::Text("Y");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gf_z", &fz, 0.0f);
      ImGui::SameLine(360);
      ImGui::Text("Z");
      ImGui::Spacing();

      ImGui::Text("%s:", m_locUp.c_str());
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gu_x", &ux, 0.0f);
      ImGui::SameLine(120);
      ImGui::Text("X");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gu_y", &uy, 0.0f);
      ImGui::SameLine(240);
      ImGui::Text("Y");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::DragFloat("##gu_z", &uz, 0.0f);
      ImGui::SameLine(360);
      ImGui::Text("Z");
      ImGui::TreePop();
    }
  }
}

}  // namespace SPF::UI
