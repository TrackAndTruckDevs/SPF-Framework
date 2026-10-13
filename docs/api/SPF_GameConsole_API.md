# SPF Game Console API

The SPF Game Console API provides a simple, one-way interface for your plugin to execute commands in the game's built-in developer console. This allows you to programmatically change game variables, trigger events, or perform any other action that is possible through console commands.

## Prerequisites: Requesting the Hook

To ensure the framework can safely hook into the game's console, your plugin **must** declare its dependency. In your plugin's manifest builder function, you must add the "GameConsole" hook.

**Example Manifest Snippet:**
```c
void BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
    // ...
    api->Policy_AddRequiredHook(h, "GameConsole");
}
```
If you do not request this hook, the framework will not provide a pointer to the Game Console API, and it will be `NULL`.

## Getting the API

The Game Console API is provided as part of the main `SPF_Core_API` struct that your plugin receives in its `OnActivated` lifecycle event.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_GameConsole_API.h"

// Global pointer to the Core API
const SPF_Core_API* s_coreAPI = NULL;

void MyPlugin_OnActivated(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;
    
    // You can now access s_coreAPI->console anywhere in your plugin
}
```

## Function Reference

The API is accessed via the `console` member of your `SPF_Core_API` pointer.

---
### `void GCon_ExecuteCommand(const char* command)`

Executes a command string in the in-game console as if a user had typed it.

*   **Parameters:**
    *   `command`: The full command string to execute (e.g., `"g_traffic 1.0"`, `"g_set_time 14 30"`).

**Important Note:** This API is for **executing** existing game commands only. It does not provide a way for plugins to **register** new custom commands with the console.

---
### `int GCon_GetCommandCount(void)`

Returns the total number of console commands the game supports.

*   **Returns:** Number of available commands, or `0` if discovery failed.
*   **Details:** The command list is discovered once (a one-time scan of the game's three command sources: console commands, script commands, and input constants) and cached. Subsequent calls return the cached count without rescanning.

---
### `int GCon_GetCommandNameByIndex(int index, char* out_buffer, int buffer_size)`

Copies the command name at `index` into `out_buffer`. Use together with `GCon_GetCommandCount` to iterate the full command list (e.g., to build an autocomplete UI in your plugin).

*   **Parameters:**
    *   `index`: Zero-based command index (`0 .. GCon_GetCommandCount() - 1`).
    *   `out_buffer`: Destination buffer for the null-terminated command name.
    *   `buffer_size`: Size of `out_buffer` in bytes. The name is truncated if it does not fit.
*   **Returns:** `1` on success, `0` on failure (NULL buffer, `buffer_size <= 0`, or index out of range).

**Enumerating all commands:**
```c
int count = s_coreAPI->console->GCon_GetCommandCount();
for (int i = 0; i < count; i++) {
    char name[128];
    if (s_coreAPI->console->GCon_GetCommandNameByIndex(i, name, sizeof(name))) {
        printf("command: %s\n", name);
    }
}
```

---
### `int GCon_IsCheatsEnabled(void)`

Returns whether developer cheats are currently enabled (the game's `g_developer` cvar is greater than 0). The value is cached for up to 1000 ms.

*   **Returns:** `1` if cheat commands will be accepted, `0` otherwise.

Use this to grey out cheat-related UI in your plugin before sending commands.

---
### `void GCon_EnableCheats(void)`

Enables developer cheats by setting `g_developer 1` through the game's console queue (equivalent to a user typing `g_developer 1` in the console). After the queue processes it, `GCon_IsCheatsEnabled` returns `1`.

---
### `int GCon_ExecuteCommandEx(const char* command, char* out_error, int error_size)`

Same routing as `GCon_ExecuteCommand`, but reports success and the rejection reason — useful when your plugin needs to show *why* a command did not run.

*   **Parameters:**
    *   `command`: The full command string (e.g. `"cheat money 10000"`, `"g_police 1"`).
    *   `out_error`: Optional destination for the error text; pass `NULL` to ignore it.
    *   `error_size`: Size of `out_error` in bytes; ignored when `out_error` is `NULL`.
*   **Returns:** `1` if the command was dispatched/accepted, `0` if it was rejected.

**Rejection reasons** (written into `out_error`):
*   `"game world is not loaded — cheat commands are unavailable. Load a save first."` — command sent before a save was loaded.
*   `"cheat commands are disabled."` — a `cheat ...` command while `g_developer` is off (use `GCon_EnableCheats`).
*   other discovery/gate errors — the console or cheat subsystem failed to initialize.

```c
char err[256];
if (!s_coreAPI->console->GCon_ExecuteCommandEx("cheat money 10000", err, sizeof(err))) {
    printf("rejected: %s\n", err);
}
```

## Complete Example

This example shows how you might create a button in your plugin's UI to toggle the in-game police presence.

```c
// Assumes s_coreAPI is initialized as shown above.
// Assumes 'ui' is a pointer to the SPF_UI_API.

static bool police_enabled = true;

void RenderMyUI(const SPF_UI_API* ui) {
    if (s_coreAPI && s_coreAPI->console && ui->Button("Toggle Police", 0, 0)) {
        police_enabled = !police_enabled;
        
        const char* cmd = police_enabled ? "g_police 1" : "g_police 0";
        s_coreAPI->console->GCon_ExecuteCommand(cmd);
    }
}
```


## Command Dump — Game version 1.61

```text
Cached 769 console commands (list1=88, list2=535, input=80, cheat=66).
```

> [!IMPORTANT]
> Don't hesitate to fill in the command table below if you know what a command does and what arguments it takes. Make a Pull Request — you will help other players and become a Contributor of this project.

## Command Table

| Command | Description | Arguments |
|:---:|:---:|:---:|
| `exec` | | |
| `echo` | | |
| `uset` | | |
| `flush_neg_cache` | | |
| `mm_dump` | | |
| `mm_flush` | | |
| `mm_flush_repeatedly` | | |
| `mm_check` | | |
| `mm_defrag` | | |
| `check` | | |
| `start_recording` | | |
| `stop_recording` | | |
| `reload_cargo_icons` | | |
| `hq_screenshot` | | |
| `g_set_time` | | |
| `rt_export` | | |
| `quit` | | |
| `pause` | | |
| `warp` | | |
| `g_mode` | | |
| `adapt_gfx` | | |
| `g_ui_recache` | | |
| `screenshot` | | |
| `goto` | | |
| `spawn` | | |
| `sdk` | | |
| `multimon` | | |
| `flush_unit_cache` | | |
| `deferred_debug` | | |
| `dispatch` | | |
| `game` | | |
| `load_game` | | |
| `preview` | | |
| `atest` | | |
| `validate` | | |
| `demo` | | |
| `academy` | | |
| `exit` | | |
| `load_error` | | |
| `edit` | | |
| `save` | | |
| `server_cfg` | | |
| `connect_lobby` | | |
| `export_server_packages` | | |
| `online` | | |
| `key` | | |
| `export_mirror_rotations` | | |
| `route` | | |
| `mark_country` | | |
| `mark_slow_time` | | |
| `mark_inside_city` | | |
| `reset_achievement` | | |
| `imgui_show` | | |
| `imgui_hide` | | |
| `show_texture` | | |
| `reset_desktop_vehicle_mode_tut` | | |
| `set_generic_cubemap` | | |
| `g_dump` | | |
| `g_game_dump` | | |
| `g_dump_signs` | | |
| `g_sel_dist` | | |
| `g_sel_type` | | |
| `g_sel_item` | | |
| `g_sel_uid` | | |
| `g_item_count` | | |
| `g_find_prefab` | | |
| `g_replace_prefab` | | |
| `g_delete_uid` | | |
| `g_delete_idx` | | |
| `edit_save` | | |
| `edit_save_text` | | |
| `edit_autocomp_dismiss` | | |
| `edit_build` | | |
| `edit_build_spawns` | | |
| `edit_mark` | | |
| `edit_import` | | |
| `g_next_border_node` | | |
| `g_save_curve_as_text` | | |
| `g_edit_performance_output` | | |
| `edit_free_camera_uplift` | | |
| `edit_album_city_data` | | |
| `edit_comp_check` | | |
| `g_cubemap_face_view` | | |
| `edit_check_teleports` | | |
| `edit_generate_portals` | | |
| `g_info` | | |
| `g_video_mode_camera` | | |
| `ui` | | |
| `s_reverb_world` | | |
| `s_reverb_cabin` | | |
| `s_noise_aero` | | |
| `s_noise_windows` | | |
| `s_rumble_enabled` | | |
| `s_reverse_enabled` | | |
| `s_cb_radio_beep_type` | | |
| `s_cb_radio_beep_own` | | |
| `s_cb_radio_beep_enabled` | | |
| `s_cb_radio_noise_enabled` | | |
| `s_cbeffects_mute` | | |
| `s_voiceover_mute` | | |
| `s_intro_music_mute` | | |
| `s_ui_music_mute` | | |
| `s_ui_mute` | | |
| `s_navigation_mute` | | |
| `s_music_mute` | | |
| `s_radio_mute` | | |
| `s_interior_mute` | | |
| `s_ambient_mute` | | |
| `s_world_mute` | | |
| `s_traffic_mute` | | |
| `s_trailer_mute` | | |
| `s_truck_noise_mute` | | |
| `s_truck_effects_mute` | | |
| `s_truck_turbo_mute` | | |
| `s_truck_exhaust_mute` | | |
| `s_truck_engine_mute` | | |
| `s_master_mute` | | |
| `s_cbeffects_volume` | | |
| `s_voiceover_volume` | | |
| `s_intro_music_volume` | | |
| `s_ui_music_volume` | | |
| `s_ui_volume` | | |
| `s_navigation_volume` | | |
| `s_music_volume` | | |
| `s_radio_volume` | | |
| `s_interior_volume` | | |
| `s_ambient_volume` | | |
| `s_world_volume` | | |
| `s_traffic_volume` | | |
| `s_trailer_volume` | | |
| `s_truck_noise_volume` | | |
| `s_truck_effects_volume` | | |
| `s_truck_turbo_volume` | | |
| `s_truck_exhaust_volume` | | |
| `s_truck_engine_volume` | | |
| `s_master_volume` | | |
| `r_hdr_display_gray_offset` | | |
| `r_hdr_display_white` | | |
| `r_hdr_display_black` | | |
| `r_sdr_display_gray_offset` | | |
| `r_sdr_display_white` | | |
| `r_sdr_display_black` | | |
| `r_far_shadow_disable` | | |
| `r_show_sun_cascades` | | |
| `r_adaptation_freeze` | | |
| `r_tonemap_debug` | | |
| `r_taa_modulated_drr_strength` | | |
| `r_drr_strength` | | |
| `r_use_depth_bounds` | | |
| `r_helpers_class` | | |
| `r_hide_helpers` | | |
| `shd_dbg5` | | |
| `shd_dbg4` | | |
| `shd_dbg3` | | |
| `shd_dbg2` | | |
| `shd_dbg1` | | |
| `shd_dbg0` | | |
| `o_openxr_right_bottom_override` | | |
| `o_openxr_right_top_override` | | |
| `o_openxr_right_right_override` | | |
| `o_openxr_right_left_override` | | |
| `o_openxr_left_bottom_override` | | |
| `o_openxr_left_top_override` | | |
| `o_openxr_left_right_override` | | |
| `o_openxr_left_left_override` | | |
| `o_openxr_max_vert_fov_override` | | |
| `o_openxr_max_hor_fov_override` | | |
| `o_openxr_init_duration_ms` | | |
| `o_openxr_one_period_ahead` | | |
| `o_openxr_acquire_after_begin` | | |
| `o_openxr_async_wait` | | |
| `o_openxr_external_loader` | | |
| `o_openxr_force_symmetrical_fov` | | |
| `o_openxr_log_changes` | | |
| `o_openxr_debug_level` | | |
| `o_openvr_any_adapter` | | |
| `o_openvr_interleaved_reproject` | | |
| `o_oculus_any_adapter` | | |
| `s_sound_debug` | | |
| `s_init_intro_music_mute` | | |
| `s_init_intro_music_volume` | | |
| `s_init_ui_music_mute` | | |
| `s_init_ui_music_volume` | | |
| `s_init_master_mute` | | |
| `s_init_master_volume` | | |
| `s_output_driver` | | |
| `s_live_update` | | |
| `s_suspend_sound` | | |
| `g_mm_help_shown` | | |
| `g_frame_image_fmt` | | |
| `g_frames_path` | | |
| `g_tire_smoke` | | |
| `g_show_returning_player_prompt` | | |
| `g_last_played_hours` | | |
| `g_show_tutorial_hints` | | |
| `g_convoy_allow_load` | | |
| `g_max_convoy_size` | | |
| `g_last_hidden_dlc_widget_name` | | |
| `g_last_seen_dlc_release_date` | | |
| `g_last_seen_patch_notes_hash` | | |
| `test_cvar_2` | | |
| `test_cvar` | | |
| `g_mp_name_tags` | | |
| `g_chat_fade_out_timer` | | |
| `g_chat_partial_fade_out` | | |
| `g_chat_activate` | | |
| `g_chat_close_automatic` | | |
| `g_light_randomization` | | |
| `g_cut_plane_ignore` | | |
| `g_camera_performance_record` | | |
| `g_debug_replay` | | |
| `g_academy_book_count` | | |
| `g_bus_passengers` | | |
| `g_anti_slip` | | |
| `g_abs` | | |
| `g_bumps` | | |
| `g_editor_zoom_speed` | | |
| `g_map_note_user_id` | | |
| `g_disable_beacons` | | |
| `g_color_feedback` | | |
| `g_disable_hud_activation` | | |
| `g_show_game_blockers` | | |
| `g_show_game_elements` | | |
| `g_hmd_no_3d_background` | | |
| `g_hmd_no_special_menu` | | |
| `g_hmd_reduced_cabin_movement` | | |
| `g_hmd_no_artifical_movement` | | |
| `r_interior_raindrops` | | |
| `r_interior` | | |
| `r_steering_wheel` | | |
| `r_debug_visualization_mode` | | |
| `g_interior_camera_zero_pitch` | | |
| `r_multimon_exterior_in_aux` | | |
| `r_multimon_interior_in_main` | | |
| `r_multimon_rotation_aux` | | |
| `r_multimon_rotation_right` | | |
| `r_multimon_rotation_left` | | |
| `r_multimon_rotation_center` | | |
| `r_multimon_vert_offset_right` | | |
| `r_multimon_vert_offset_left` | | |
| `r_multimon_border_fov_right` | | |
| `r_multimon_border_fov_left` | | |
| `r_multimon_fov_vertical` | | |
| `r_multimon_fov_horizontal` | | |
| `r_multimon_mode` | | |
| `r_buffer_page_size` | | |
| `r_deferred_debug` | | |
| `r_mirror_scale_y` | | |
| `r_mirror_scale_x` | | |
| `r_scale_y` | | |
| `r_scale_x` | | |
| `r_normal_maps` | | |
| `r_sunshafts` | | |
| `r_color_saturation` | | |
| `r_color_yellow_blue` | | |
| `r_color_magenta_green` | | |
| `r_color_cyan_red` | | |
| `r_color_correction` | | |
| `x_combined_lights` | | |
| `r_light_flares` | | |
| `r_cloud_shadows` | | |
| `r_dof_filter_size` | | |
| `r_dof_transition` | | |
| `r_dof_start` | | |
| `r_dof` | | |
| `r_ssao` | | |
| `r_taa_luma_sharpen` | | |
| `r_taa_tuning` | | |
| `r_aa` | | |
| `r_interior_shadow` | | |
| `r_fake_shadows` | | |
| `r_sun_debug` | | |
| `r_sun_shadow_cascade_drop` | | |
| `r_sun_shadow_texture_size` | | |
| `r_sun_shadow_quality` | | |
| `r_mirror_group` | | |
| `r_mirror_view_distance` | | |
| `r_deferred_mirrors` | | |
| `r_deferred` | | |
| `r_nowmi` | | |
| `r_startup_progress` | | |
| `r_setup_done` | | |
| `r_peak_brightness` | | |
| `r_hmd_controller_shadows` | | |
| `r_hmd_draw_controllers` | | |
| `r_hmd_water_pixels_per_deg` | | |
| `r_ipd_scale` | | |
| `r_manual_stereo_mirror_mode` | | |
| `r_manual_stereo_ui_lod_bias` | | |
| `r_manual_stereo_ui_mipmaps` | | |
| `r_manual_stereo_ui_buffer_scale` | | |
| `r_manual_stereo_buffer_scale` | | |
| `r_manual_stereo_ui_pitch_game` | | |
| `r_manual_stereo_ui_yaw_game` | | |
| `r_manual_stereo_ui_pitch` | | |
| `r_manual_stereo_ui_yaw` | | |
| `r_manual_stereo_ui_y_game` | | |
| `r_manual_stereo_ui_x_game` | | |
| `r_manual_stereo_ui_y` | | |
| `r_manual_stereo_ui_x` | | |
| `r_manual_stereo_ui_static_fov` | | |
| `r_manual_stereo_ui_fov_game` | | |
| `r_manual_stereo_ui_fov` | | |
| `r_manual_stereo_ui_radius_game` | | |
| `r_manual_stereo_ui_radius` | | |
| `r_manual_stereo_ui_dist_game` | | |
| `r_manual_stereo_ui_dist` | | |
| `r_manual_stereo_in_menu` | | |
| `r_minimal_unfinished_frames` | | |
| `r_no_frame_tracking` | | |
| `r_mode_refresh` | | |
| `r_mode_height` | | |
| `r_mode_width` | | |
| `r_mode` | | |
| `r_path` | | |
| `r_device` | | |
| `i_cursor_semantic_force_spd_mlt` | | |
| `i_cursor_force_spd_mlt` | | |
| `i_controller_cursor_speed` | | |
| `g_stream_exts` | | |
| `g_album_image` | | |
| `g_eye_preset_5` | | |
| `g_eye_preset_4` | | |
| `g_eye_preset_3` | | |
| `g_eye_preset_2` | | |
| `g_eye_preset_1` | | |
| `g_eye_pause_gaze` | | |
| `g_eye_pause` | | |
| `g_eye_enable` | | |
| `g_thrustmaster` | | |
| `g_tobii` | | |
| `g_trackir` | | |
| `g_lod_factor_pedestrian` | | |
| `g_pedestrian` | | |
| `g_light_span_factor` | | |
| `g_light_distance_factor` | | |
| `g_vehicle_flare_lights` | | |
| `g_auto_traffic_headlights` | | |
| `g_lod_factor_parked` | | |
| `g_lod_factor_traffic` | | |
| `g_traffic` | | |
| `g_kdop_preview` | | |
| `g_screenshot_on_bug_quality` | | |
| `g_screenshot_on_bug` | | |
| `g_debug_show_all_hookups` | | |
| `g_debug_shoulder_block` | | |
| `g_debug_road_roll` | | |
| `g_debug_road_slope` | | |
| `g_debug_road` | | |
| `g_debug_pins` | | |
| `g_debug_dealer` | | |
| `g_debug_height` | | |
| `g_debug_trailer` | | |
| `g_debug_bus_schedule` | | |
| `g_bus_gradual_schedule` | | |
| `g_debug_slow_time` | | |
| `g_debug_time_zone` | | |
| `g_debug_sectors` | | |
| `g_debug_climate` | | |
| `g_debug_country` | | |
| `g_debug_dlc` | | |
| `g_debug_vehicle_types` | | |
| `g_debug_map_limits` | | |
| `g_city_name_move` | | |
| `g_assert_dump` | | |
| `g_developer` | | |
| `g_semantical_pause_inactivity` | | |
| `g_pause_on_disconnect` | | |
| `g_semantical_ff_inactivity` | | |
| `g_steam_steamdeck_mode` | | |
| `g_steam_rich_presence` | | |
| `g_steam_browser` | | |
| `g_steam_screenshots` | | |
| `g_artist_id` | | |
| `g_save_format` | | |
| `g_physics_sim_velocity_derived` | | |
| `g_physics_sim_merge_explicit` | | |
| `g_use_convex_casts` | | |
| `g_atest` | | |
| `g_cam_parking` | | |
| `g_cam_interior_zoom` | | |
| `g_cam_window_block` | | |
| `g_simple_patch_physics` | | |
| `g_toy_physics` | | |
| `g_cam_blinker` | | |
| `g_cam_auto_move_interior_mult` | | |
| `g_cam_auto_move_speed` | | |
| `g_cam_auto_center_delay` | | |
| `g_cam_fov_speed` | | |
| `g_camera_horizon_lock` | | |
| `g_cam_physics_value` | | |
| `g_cam_physics` | | |
| `g_cam_steering_reverse` | | |
| `g_cam_steering_value` | | |
| `g_cam_steering` | | |
| `g_blind_spot` | | |
| `g_throttle_double_tap` | | |
| `g_lane_assistant` | | |
| `g_emergency_brake_mode` | | |
| `g_emergency_brake` | | |
| `g_acc` | | |
| `g_auto_diff_lock` | | |
| `g_automatic_high_beams` | | |
| `g_automatic_headlights` | | |
| `g_blinker_auto_off` | | |
| `g_cruise_control_smart` | | |
| `g_cruise_control_grid` | | |
| `g_transmission_crawl_ignore` | | |
| `g_adaptive_shift` | | |
| `g_pedal_clutch_range` | | |
| `g_fuel_simulation` | | |
| `g_hardcore_simulation` | | |
| `g_reg_setting` | | |
| `g_brake_light_all` | | |
| `g_use_speed_limiter` | | |
| `g_hud_speed_warning` | | |
| `g_hud_speed_limit` | | |
| `g_axle_drop_auto` | | |
| `g_engine_start_auto` | | |
| `g_hill_brake_assist` | | |
| `g_motor_brake_auto` | | |
| `g_retarder_auto` | | |
| `g_trailer_cables_mode` | | |
| `g_cabin_suspension_stiffness` | | |
| `g_suspension_stiffness` | | |
| `g_trailer_stability` | | |
| `g_driveshaft_torque` | | |
| `g_truck_stability` | | |
| `g_brake_intensity` | | |
| `g_steer_anim_range` | | |
| `g_steer_autocenter` | | |
| `g_mpg` | | |
| `g_gallon` | | |
| `g_psi` | | |
| `g_fahrenheit` | | |
| `g_pounds` | | |
| `g_mph` | | |
| `g_clutch_brake` | | |
| `g_intelligent_transmission` | | |
| `g_hshifter_synchronized` | | |
| `g_hshifter_split` | | |
| `g_hshifter_layout_shifting` | | |
| `g_hshifter_layout` | | |
| `g_trans` | | |
| `g_debug_replay_editor_origin` | | |
| `g_debug_cutscene` | | |
| `g_debug_dyn_cargo_compartment` | | |
| `g_debug_dynamic_cargo` | | |
| `g_debug_gate` | | |
| `g_debug_trigger` | | |
| `g_debug_lod_depot_trailer` | | |
| `g_debug_parking` | | |
| `g_spec_trans_refill_tank` | | |
| `g_ignore_oversize_check` | | |
| `g_road_events` | | |
| `g_debug_detours` | | |
| `g_detours` | | |
| `g_debug_camera` | | |
| `g_debug_fov` | | |
| `g_flyspeed` | | |
| `g_online_billboards` | | |
| `g_online_loading_screens` | | |
| `g_news` | | |
| `g_name_localization_secondary` | | |
| `g_name_localization` | | |
| `g_lang_set` | | |
| `g_lang_init` | | |
| `g_lang` | | |
| `g_col_offset_factor` | | |
| `g_col_fill_backface` | | |
| `g_col_fill_alpha` | | |
| `g_col_toy` | | |
| `g_colbox` | | |
| `g_radio_saved_state` | | |
| `g_radio_stream_safe` | | |
| `g_radio_mode` | | |
| `g_cutscenes` | | |
| `g_phys_mirrors` | | |
| `g_baked_vehicle` | | |
| `g_ignore_low_fps` | | |
| `g_truck_light_specular` | | |
| `g_gfx_all_scales` | | |
| `g_gfx_advanced` | | |
| `g_gfx_quality` | | |
| `g_hq_3d_scale` | | |
| `g_hq_3d_screenshot` | | |
| `g_menu_aa_limit` | | |
| `g_additional_water_fov` | | |
| `g_water_reflect_actor` | | |
| `g_rain_sensor` | | |
| `g_rain_reflect_hookups` | | |
| `g_rain_reflect_actor` | | |
| `g_rain_reflect_traffic` | | |
| `g_rain_reflect_cache` | | |
| `g_rain_reflection` | | |
| `g_reflection_scale` | | |
| `g_reflection` | | |
| `g_grass_density` | | |
| `g_veg_detail` | | |
| `g_world_map_zoom` | | |
| `g_cargo_load_require_park_brake` | | |
| `g_cargo_load_require_engine_off` | | |
| `g_relocate_truck_and_driver_tog` | | |
| `g_desktop_tour` | | |
| `g_event_desktop_bcg_change` | | |
| `g_event_desktop_bcg_restore` | | |
| `g_last_event_desktop_bcg_change` | | |
| `g_show_always_wotr_event` | | |
| `g_last_wotr_event_start` | | |
| `g_save_indicator` | | |
| `g_game_version` | | |
| `g_force_online_lscrs` | | |
| `g_hw_info` | | |
| `g_sns` | | |
| `g_pause_show_additional_buttons` | | |
| `g_glass_debugging_level` | | |
| `g_line_sort` | | |
| `g_cargo_sort` | | |
| `g_moon_orbit_count` | | |
| `g_debug_environment` | | |
| `g_bloom_standard_deviation` | | |
| `g_bloom_override` | | |
| `g_bloom` | | |
| `g_remove_missing_accessories` | | |
| `g_force_economy_reset` | | |
| `g_multiplayer_backup_index` | | |
| `g_global_force_load_selector` | | |
| `g_autoload_ignore_autosave` | | |
| `g_start_in_truck` | | |
| `g_input_configured` | | |
| `g_park_brake_init` | | |
| `g_trailer_advanced_coupling` | | |
| `g_parking_difficulty` | | |
| `g_simple_parking_doubles` | | |
| `g_speed_warning` | | |
| `g_voice_navigation_pack` | | |
| `g_voice_navigation` | | |
| `g_job_distance` | | |
| `g_job_distance_limit` | | |
| `g_income_factor` | | |
| `g_currency` | | |
| `g_bad_weather_factor` | | |
| `g_truck_analysis_fold` | | |
| `g_desktop_rnd` | | |
| `g_desktop_bcg` | | |
| `g_desktop_fadeout` | | |
| `g_proximity_exploration` | | |
| `g_ui_map_align` | | |
| `g_gps_debug_graph` | | |
| `g_gps_debug_mark_nodes` | | |
| `g_gps_debug_next_search` | | |
| `g_gps_debug_checkpoints` | | |
| `g_gps_routing_mode` | | |
| `g_gps_navigation` | | |
| `g_heavy_cargo_popup` | | |
| `g_hud_notifications_keep_hidden` | | |
| `g_mirrors_keep_hidden` | | |
| `g_mirrors_visibility_setup` | | |
| `g_mirrors` | | |
| `g_adviser_eta` | | |
| `g_adviser_radio_widget_visibili` | | |
| `g_adviser_widget_tourist_board` | | |
| `g_adviser_widget_message` | | |
| `g_adviser_auto_parking` | | |
| `g_adviser_keep_hidden` | | |
| `g_adviser_alpha` | | |
| `g_adviser_widget_bus_gmeter` | | |
| `g_adviser_widget_job_info` | | |
| `g_adviser_widget_damage` | | |
| `g_adviser_widget_finances` | | |
| `g_adviser_widget_chat` | | |
| `g_adviser_widget_indicators` | | |
| `g_adviser_widget_tachometer` | | |
| `g_adviser_widget_minimap` | | |
| `g_desktop_tutorial` | | |
| `g_mandatory_break` | | |
| `g_fatigue` | | |
| `g_time_zones` | | |
| `g_police` | | |
| `g_subtitles` | | |
| `g_item_check_speed` | | |
| `g_debug_allocators` | | |
| `g_suspension_auto_reset` | | |
| `g_upshift_coef` | | |
| `g_downshift_coef` | | |
| `g_clock_24` | | |
| `inst_flags` | | |
| `g_minicon` | | |
| `g_console_state` | | |
| `g_console` | | |
| `g_fps` | | |
| `ui_gamepad_mode` | | |
| `ui_tooltip_delay` | | |
| `ui_gamepad_glyphs` | | |
| `p_profile_draws` | | |
| `rg_dump` | | |
| `r_imgui_scale` | | |
| `r_null_renderer` | | |
| `r_vulkan_runtime` | | |
| `r_view_instancing` | | |
| `r_wireframe` | | |
| `r_windowed_borderless` | | |
| `r_fullscreen_borderless` | | |
| `r_fullscreen` | | |
| `r_output` | | |
| `r_adapter` | | |
| `r_vsync` | | |
| `r_anisotropy_factor` | | |
| `r_texture_detail` | | |
| `cf_beacon_light` | | |
| `cf_asus_aura` | | |
| `cf_razer_chroma` | | |
| `v_bug_break` | | |
| `t_ignore_hmd_timing` | | |
| `t_limit_fps_inactive` | | |
| `t_limit_fps` | | |
| `t_locked_fps` | | |
| `t_averaging_window_duration` | | |
| `t_averaging_window_length` | | |
| `vid_ver` | | |
| `vid_mem` | | |
| `vid_name` | | |
| `sys_info` | | |
| `c_lr_dz` | | |
| `c_ud_dz` | | |
| `c_stm_simple` | | |
| `c_steer_func` | | |
| `c_steer_dz` | | |
| `c_throt_dz` | | |
| `c_brake_dz` | | |
| `c_clutch_dz` | | |
| `c_msens` | | |
| `c_minvert` | | |
| `c_jzthrottle` | | |
| `c_jithrottle` | | |
| `c_jzbrake` | | |
| `c_jibrake` | | |
| `c_jzclutch` | | |
| `c_jiclutch` | | |
| `c_jisteer` | | |
| `c_jilookud` | | |
| `c_jilooklr` | | |
| `c_rsteersens` | | |
| `c_asteersens` | | |
| `c_mousesteer` | | |
| `c_mousepedal` | | |
| `c_mped_dz` | | |
| `c_relatsteer` | | |
| `c_ff_enabled` | | |
| `c_ff_gain` | | |
| `c_ff_hi_spd` | | |
| `c_ff_lo_spd` | | |
| `c_ff_fricton` | | |
| `c_ff_engine` | | |
| `c_ff_surface` | | |
| `c_ff_bumps` | | |
| `c_ff_collis` | | |
| `c_ff_gearbox` | | |
| `c_ff_undrstr` | | |
| `c_ff_assist` | | |
| `c_vff_surfac` | | |
| `c_vff_bumps` | | |
| `c_vff_collis` | | |
| `c_atr_enable` | | |
| `c_atr_abs` | | |
| `c_atr_fail` | | |
| `c_atr_start` | | |
| `c_atr_wear` | | |
| `c_atr_brake` | | |
| `c_atr_clutch` | | |
| `c_krspeed` | | |
| `c_jrspeed` | | |
| `c_ht_on` | | |
| `c_ht_roll` | | |
| `c_et_head` | | |
| `c_et_pos` | | |
| `c_et_yawsc` | | |
| `c_et_pitchsc` | | |
| `c_et_rollsc` | | |
| `c_etwm_yawsc` | | |
| `c_etwm_pitsc` | | |
| `c_et_xscale` | | |
| `c_et_yscale` | | |
| `c_et_zscale` | | |
| `c_et_gv_res` | | |
| `c_et_hv_res` | | |
| `c_et_gv_ext` | | |
| `c_et_hv_dscy` | | |
| `c_et_hv_dscp` | | |
| `c_et_hv_dscr` | | |
| `c_et_hv_wscy` | | |
| `c_et_hv_wscp` | | |
| `c_et_hv_wscr` | | |
| `c_et_gv_exp` | | |
| `c_et_gv_inf` | | |
| `c_et_gv_sta` | | |
| `c_et_gv_end` | | |
| `c_et_hv_exp` | | |
| `c_et_hv_inf` | | |
| `c_et_hv_sta` | | |
| `c_et_hv_end` | | |
| `c_et_hv_ac` | | |
| `c_et_hv_mode` | | |
| `cheat driver` | | |
| `cheat fire_drivers` | | |
| `cheat delivery` | | |
| `cheat bus_job` | | |
| `cheat money` | | |
| `cheat repair` | | |
| `cheat integrity_odometer` | | |
| `cheat fix_cargo` | | |
| `cheat unlock_screens` | | |
| `cheat lock_screen` | | |
| `cheat unlock_screen` | | |
| `cheat level` | | |
| `cheat xp` | | |
| `cheat bus_level` | | |
| `cheat generate_drivers` | | |
| `cheat jobs` | | |
| `cheat print_jobs` | | |
| `cheat job` | | |
| `cheat fuel` | | |
| `cheat fatigue` | | |
| `cheat mandatory_break` | | |
| `cheat on_break_time` | | |
| `cheat sleep_time` | | |
| `cheat trailer` | | |
| `cheat cargo` | | |
| `cheat cargo_mass` | | |
| `cheat export_truck` | | |
| `cheat load_truck` | | |
| `cheat export_trailer` | | |
| `cheat load_trailer` | | |
| `cheat export_car` | | |
| `cheat load_car` | | |
| `cheat export_bus` | | |
| `cheat load_bus` | | |
| `cheat truck` | | |
| `cheat car` | | |
| `cheat bus` | | |
| `cheat week` | | |
| `cheat time` | | |
| `cheat truck_dealers` | | |
| `cheat recruitment` | | |
| `cheat city` | | |
| `cheat passengers` | | |
| `cheat satisfaction` | | |
| `cheat oversize_gj` | | |
| `cheat service` | | |
| `cheat sell_trailer` | | |
| `cheat reload_upgrades` | | |
| `cheat reset_hints` | | |
| `cheat full_economy` | | |
| `cheat tbs` | | |
| `cheat switch_company` | | |
| `cheat police.report_offence` | | |
| `cheat get_job` | | |
| `cheat teleport` | | |
| `cheat explore` | | |
| `cheat used_vehicles` | | |
| `cheat closest_portal` | | |
| `cheat next_portal` | | |
| `cheat company_portal` | | |
| `cheat dyn_cargo_particles` | | |
| `cheat garage` | | |
| `cheat hud_animation` | | |
| `cheat iddqd` | | |
| `cheat idkfa` | | |
| `cheat idspispopd` | | |