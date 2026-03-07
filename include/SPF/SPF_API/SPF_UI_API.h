/**                                                                                               
* @file SPF_UI_API.h                                                                          
* @brief C-style API for creating and managing plugin UI windows.
*                                                                                                 
* @details This API provides a stable C interface to the framework's underlying 
*          ImGui-based rendering engine. It uses an immediate-mode paradigm where 
*          the UI is defined and processed every frame inside a draw callback.
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **Immediate Mode**: UI elements are not persistent objects. They are "drawn" 
*    every frame within the draw callback. State must be managed by the plugin.
*                                                                                                 
* 2. **Draw Callbacks**: The framework calls a plugin-provided function to render 
*    each window. This function receives a pointer to the 'SPF_UI_API' table.
*                                                                                                 
* 3. **Window Lifecycle**: Windows are declared in the manifest, registered in 
*    'OnRegisterUI', and drawn during the frame rendering phase.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* // 1. Define your rendering logic
* void MyPlugin_RenderWindow(SPF_UI_API* ui, void* user_data) {
*     ui->UI_Text("Welcome to my Plugin!");
*     
*     static bool my_bool = false;
*     if (ui->UI_Checkbox("Enable Feature", &my_bool)) {
*         // React to change...
*     }
* }
*
* // 2. Register the callback during initialization
* void OnRegisterUI(SPF_UI_API* api) {
*     api->UI_RegisterDrawCallback("MyPlugin", "MainWindow", MyPlugin_RenderWindow, nullptr);
* }
* @endcode                                                                                        
*/ 
#pragma once

#include <stdbool.h>
#include <stdint.h>

// Note: This is a C-style header for ABI stability.

// Forward-declare handle type
typedef struct SPF_Window_Handle SPF_Window_Handle;

// Forward-declare SPF_TextStyle_Handle
typedef struct SPF_TextStyle_Handle_t* SPF_TextStyle_Handle;

// Forward-declare SPF_DrawList_Handle for custom drawing
typedef struct SPF_DrawList_Handle_t* SPF_DrawList_Handle;

// Forward-declare SPF_Font_Handle for dynamic font management
typedef struct ImFont* SPF_Font_Handle;

// Forward-declare SPF_Style_Handle for accessing global style variables
typedef struct ImGuiStyle* SPF_Style_Handle;

// Forward-declare SPF_Payload_Handle for Drag and Drop operations
typedef struct ImGuiPayload* SPF_Payload_Handle;

/**
 * @enum SPF_StyleVar
 * @brief Enumeration of style variables that can be temporarily pushed and popped.
 * @details Use these with UI_PushStyleVarFloat() or UI_PushStyleVarVec2() to modify the look of widgets.
 *          Every 'Push' call MUST be paired with a corresponding 'PopStyleVar' call.
 */
typedef enum
{
    /** Overall alpha transparency applied to all windows and widgets (float, 0.0f to 1.0f). */
    SPF_STYLE_VAR_ALPHA,
    /** Alpha transparency applied to disabled widgets (float, 0.0f to 1.0f). */
    SPF_STYLE_VAR_DISABLED_ALPHA,
    /** Padding within a window: space between the window edges and the content (Vec2). */
    SPF_STYLE_VAR_WINDOW_PADDING,
    /** Radius of window corners (float). 0.0f for sharp corners. */
    SPF_STYLE_VAR_WINDOW_ROUNDING,
    /** Thickness of the border around windows (float). */
    SPF_STYLE_VAR_WINDOW_BORDERSIZE,
    /** Minimum allowed size for a window (Vec2). Prevents windows from becoming too small. */
    SPF_STYLE_VAR_WINDOW_MIN_SIZE,
    /** Alignment for the title bar text (Vec2: 0.0 is left, 0.5 is centered, 1.0 is right). */
    SPF_STYLE_VAR_WINDOW_TITLE_ALIGN,
    /** Radius of child window corners (float). */
    SPF_STYLE_VAR_CHILD_ROUNDING,
    /** Thickness of the border around child windows (float). */
    SPF_STYLE_VAR_CHILD_BORDERSIZE,
    /** Radius of popup/menu window corners (float). */
    SPF_STYLE_VAR_POPUP_ROUNDING,
    /** Thickness of the border around popup/menu windows (float). */
    SPF_STYLE_VAR_POPUP_BORDERSIZE,
    /** Internal padding within widgets like buttons or input fields (Vec2). */
    SPF_STYLE_VAR_FRAME_PADDING,
    /** Radius of widget corners (buttons, input boxes, etc.) (float). */
    SPF_STYLE_VAR_FRAME_ROUNDING,
    /** Thickness of the border around widgets (float). */
    SPF_STYLE_VAR_FRAME_BORDERSIZE,
    /** Horizontal and vertical spacing between widgets (Vec2). */
    SPF_STYLE_VAR_ITEM_SPACING,
    /** Spacing between elements inside a complex widget (e.g., between text and a checkbox) (Vec2). */
    SPF_STYLE_VAR_ITEM_INNER_SPACING,
    /** Horizontal indentation when using trees or manual indents (float). */
    SPF_STYLE_VAR_INDENT_SPACING,
    /** Internal padding within table cells (Vec2). */
    SPF_STYLE_VAR_CELL_PADDING,
    /** Width (for vertical) or height (for horizontal) of scrollbars (float). */
    SPF_STYLE_VAR_SCROLLBAR_SIZE,
    /** Radius of scrollbar corners (float). */
    SPF_STYLE_VAR_SCROLLBAR_ROUNDING,
    /** Padding between the scrollbar track and the grab handle (float). */
    SPF_STYLE_VAR_SCROLLBAR_PADDING,
    /** Minimum size of the grab handle in sliders or scrollbars (float). */
    SPF_STYLE_VAR_GRAB_MINSIZE,
    /** Radius of the grab handle corners (float). */
    SPF_STYLE_VAR_GRAB_ROUNDING,
    /** Radius of image corners when using rounded image functions (float). */
    SPF_STYLE_VAR_IMAGE_ROUNDING,
    /** Thickness of the border around images (float). */
    SPF_STYLE_VAR_IMAGE_BORDERSIZE,
    /** Radius of tab corners (float). */
    SPF_STYLE_VAR_TAB_ROUNDING,
    /** Thickness of the border around tabs (float). */
    SPF_STYLE_VAR_TAB_BORDERSIZE,
    /** Minimum width of a tab (float). */
    SPF_STYLE_VAR_TAB_MIN_WIDTH_BASE,
    /** Amount by which tabs can shrink when space is limited (float). */
    SPF_STYLE_VAR_TAB_MIN_WIDTH_SHRINK,
    /** Thickness of the bottom border of a tab bar (float). */
    SPF_STYLE_VAR_TAB_BAR_BORDERSIZE,
    /** Thickness of the accent line (overline) on the active tab (float). */
    SPF_STYLE_VAR_TAB_BAR_OVERLINE_SIZE,
    /** Angle of slanted table headers (float, typically in radians). */
    SPF_STYLE_VAR_TABLE_ANGLED_HEADERS_ANGLE,
    /** Alignment of text within slanted table headers (Vec2). */
    SPF_STYLE_VAR_TABLE_ANGLED_HEADERS_TEXT_ALIGN,
    /** Size of the lines drawn in trees to show hierarchy (float). */
    SPF_STYLE_VAR_TREE_LINES_SIZE,
    /** Rounding of the hierarchy lines in trees (float). */
    SPF_STYLE_VAR_TREE_LINES_ROUNDING,
    /** Alignment of text within buttons (Vec2: 0.0 left, 0.5 centered). */
    SPF_STYLE_VAR_BUTTON_TEXT_ALIGN,
    /** Alignment of text within selectable items (Vec2). */
    SPF_STYLE_VAR_SELECTABLE_TEXT_ALIGN,
    /** Thickness of the line in a labeled separator (float). */
    SPF_STYLE_VAR_SEPARATOR_TEXT_BORDERSIZE,
    /** Alignment of the label text within a separator (Vec2). */
    SPF_STYLE_VAR_SEPARATOR_TEXT_ALIGN,
    /** Padding between the separator line and the text label (Vec2). */
    SPF_STYLE_VAR_SEPARATOR_TEXT_PADDING,
    /** Size of the separator line between docked windows (float). */
    SPF_STYLE_VAR_DOCKING_SEPARATOR_SIZE,
    /** Total count of style variables. */
    SPF_STYLE_VAR_COUNT
} SPF_StyleVar;

/**
 * @enum SPF_StyleColor
 * @brief Identifiers for every color used by the UI system.
 * @details Use these with UI_PushStyleColor() to customize colors of specific UI elements.
 */
typedef enum
{
    /** Default text color. */
    SPF_COLOR_TEXT,
    /** Color for disabled/inactive text. */
    SPF_COLOR_TEXT_DISABLED,
    /** Background color of a normal window. */
    SPF_COLOR_WINDOW_BG,
    /** Background color of child windows. */
    SPF_COLOR_CHILD_BG,
    /** Background color of popups, menus, and tooltips. */
    SPF_COLOR_POPUP_BG,
    /** Color of borders around windows and widgets. */
    SPF_COLOR_BORDER,
    /** Color of the shadow cast by borders. */
    SPF_COLOR_BORDER_SHADOW,
    /** Background color for input widgets (checkbox, radio, input text, slider). */
    SPF_COLOR_FRAME_BG,
    /** Background color of input widgets when hovered. */
    SPF_COLOR_FRAME_BG_HOVERED,
    /** Background color of input widgets when being interacted with (clicked). */
    SPF_COLOR_FRAME_BG_ACTIVE,
    /** Background color of the window title bar. */
    SPF_COLOR_TITLE_BG,
    /** Background color of the title bar when the window is active. */
    SPF_COLOR_TITLE_BG_ACTIVE,
    /** Background color of the title bar when the window is collapsed. */
    SPF_COLOR_TITLE_BG_COLLAPSED,
    /** Background color of the menu bar at the top of a window. */
    SPF_COLOR_MENU_BAR_BG,
    /** Background color of the scrollbar track. */
    SPF_COLOR_SCROLLBAR_BG,
    /** Color of the scrollbar grab handle. */
    SPF_COLOR_SCROLLBAR_GRAB,
    /** Color of the scrollbar grab handle when hovered. */
    SPF_COLOR_SCROLLBAR_GRAB_HOVERED,
    /** Color of the scrollbar grab handle when being dragged. */
    SPF_COLOR_SCROLLBAR_GRAB_ACTIVE,
    /** Color of the checkmark in checkboxes. */
    SPF_COLOR_CHECK_MARK,
    /** Color of the grab handle in sliders/drags. */
    SPF_COLOR_SLIDER_GRAB,
    /** Color of the grab handle in sliders/drags when active. */
    SPF_COLOR_SLIDER_GRAB_ACTIVE,
    /** Default color for buttons. */
    SPF_COLOR_BUTTON,
    /** Color of a button when hovered. */
    SPF_COLOR_BUTTON_HOVERED,
    /** Color of a button when pressed. */
    SPF_COLOR_BUTTON_ACTIVE,
    /** Color used for tree headers, selectables, and menu items. */
    SPF_COLOR_HEADER,
    /** Color of headers when hovered. */
    SPF_COLOR_HEADER_HOVERED,
    /** Color of headers when active/clicked. */
    SPF_COLOR_HEADER_ACTIVE,
    /** Color of separator lines. */
    SPF_COLOR_SEPARATOR,
    /** Color of separators when hovered. */
    SPF_COLOR_SEPARATOR_HOVERED,
    /** Color of separators when active. */
    SPF_COLOR_SEPARATOR_ACTIVE,
    /** Color of the resize grip in the bottom-right of windows. */
    SPF_COLOR_RESIZE_GRIP,
    /** Color of the resize grip when hovered. */
    SPF_COLOR_RESIZE_GRIP_HOVERED,
    /** Color of the resize grip when active. */
    SPF_COLOR_RESIZE_GRIP_ACTIVE,
    /** Color of an inactive tab. */
    SPF_COLOR_TAB,
    /** Color of a tab when hovered. */
    SPF_COLOR_TAB_HOVERED,
    /** Color of the currently active/visible tab. */
    SPF_COLOR_TAB_ACTIVE,
    /** Color of a tab in an unfocused window. */
    SPF_COLOR_TAB_UNFOCUSED,
    /** Color of the active tab in an unfocused window. */
    SPF_COLOR_TAB_UNFOCUSED_ACTIVE,
    /** Color shown as a preview when docking a window. */
    SPF_COLOR_DOCKING_PREVIEW,
    /** Background color of an empty docking node (no windows). */
    SPF_COLOR_DOCKING_EMPTY_BG,
    /** Color of lines in a plot widget. */
    SPF_COLOR_PLOT_LINES,
    /** Color of plot lines when hovered. */
    SPF_COLOR_PLOT_LINES_HOVERED,
    /** Color of bars in a histogram. */
    SPF_COLOR_PLOT_HISTOGRAM,
    /** Color of histogram bars when hovered. */
    SPF_COLOR_PLOT_HISTOGRAM_HOVERED,
    /** Background color for table headers. */
    SPF_COLOR_TABLE_HEADER_BG,
    /** Color for strong/outer table borders. */
    SPF_COLOR_TABLE_BORDER_STRONG,
    /** Color for light/inner table borders. */
    SPF_COLOR_TABLE_BORDER_LIGHT,
    /** Background color for even table rows. */
    SPF_COLOR_TABLE_ROW_BG,
    /** Background color for odd table rows (alternating colors). */
    SPF_COLOR_TABLE_ROW_BG_ALT,
    /** Background color for selected text. */
    SPF_COLOR_TEXT_SELECTED_BG,
    /** Color highlighting a valid drop target during drag-and-drop. */
    SPF_COLOR_DRAG_DROP_TARGET,
    /** Color used for the keyboard/gamepad navigation highlight. */
    SPF_COLOR_NAV_HIGHLIGHT,
    /** Color used for the windowing list highlight (Ctrl+Tab). */
    SPF_COLOR_NAV_WINDOWING_HIGHLIGHT,
    /** Dimming color applied to the screen behind the windowing list. */
    SPF_COLOR_NAV_WINDOWING_DIM_BG,
    /** Dimming color applied to the screen behind a modal window. */
    SPF_COLOR_MODAL_WINDOW_DIM_BG,
    /** Total count of style colors. */
    SPF_COLOR_COUNT
} SPF_StyleColor;

/**
 * @enum SPF_ConfigFlags
 * @brief Global configuration flags for the UI system (UI_GetIO_ConfigFlags).
 */
typedef enum
{
    SPF_CONFIG_NONE = 0,
    /** Enable keyboard navigation. */
    SPF_CONFIG_NAV_ENABLE_KEYBOARD = 1 << 0,
    /** Enable gamepad navigation. */
    SPF_CONFIG_NAV_ENABLE_GAMEPAD = 1 << 1,
    /** Enable docking features. */
    SPF_CONFIG_DOCKING_ENABLE = 1 << 10,
    /** Enable multi-viewport (multiple OS windows) features. */
    SPF_CONFIG_VIEWPORTS_ENABLE = 1 << 11
} SPF_ConfigFlags;

/**
 * @enum SPF_BackendFlags
 * @brief Capabilities of the current rendering backend (UI_GetIO_BackendFlags).
 */
typedef enum
{
    SPF_BACKEND_NONE = 0,
    /** Backend supports mouse cursors. */
    SPF_BACKEND_HAS_MOUSE_CURSORS = 1 << 0,
    /** Backend supports mouse hover (e.g., not a touch-only screen). */
    SPF_BACKEND_HAS_MOUSE_HOVERED = 1 << 1,
    /** Backend supports multiple viewports. */
    SPF_BACKEND_PLATFORM_HAS_VIEWPORTS = 1 << 10,
    /** Backend supports rendering into multiple viewports. */
    SPF_BACKEND_RENDERER_HAS_VIEWPORTS = 1 << 11
} SPF_BackendFlags;

/**
 * @enum SPF_ItemFlags
 * @brief Flags for fine-tuning widget behavior (UI_PushItemFlag).
 */
typedef enum
{
    SPF_ITEM_FLAG_NONE = 0,
    /** Widget does not accept focus via Tab. */
    SPF_ITEM_FLAG_NO_TAB_STOP = 1 << 0,
    /** Disable navigation for this item. */
    SPF_ITEM_FLAG_NO_NAV = 1 << 1,
    /** Item is for display only (read-only). */
    SPF_ITEM_FLAG_READ_ONLY = 1 << 5,
    /** Item is completely disabled (grayed out and non-interactive). */
    SPF_ITEM_FLAG_DISABLED = 1 << 10
} SPF_ItemFlags;

/**
 * @enum SPF_KeyMod
 * @brief Modifier keys for shortcuts and input queries.
 * @details Can be combined using bitwise OR (e.g., SPF_MOD_CTRL | SPF_MOD_SHIFT).
 */
typedef enum
{
    SPF_MOD_NONE = 0,
    SPF_MOD_CTRL = 1 << 12,
    SPF_MOD_SHIFT = 1 << 13,
    SPF_MOD_ALT = 1 << 14,
    SPF_MOD_SUPER = 1 << 15  /** Windows or Command key. */
} SPF_KeyMod;

/**
 * @enum SPF_Key
 * @brief Standard keyboard key identifiers.
 * @details This list contains the most commonly used keys. 
 *          Other codes match the standard ImGuiKey enumeration.
 */
typedef enum
{
    SPF_KEY_NONE = 0,
    SPF_KEY_TAB = 512,
    SPF_KEY_LEFT_ARROW,
    SPF_KEY_RIGHT_ARROW,
    SPF_KEY_UP_ARROW,
    SPF_KEY_DOWN_ARROW,
    SPF_KEY_PAGE_UP,
    SPF_KEY_PAGE_DOWN,
    SPF_KEY_HOME,
    SPF_KEY_END,
    SPF_KEY_INSERT,
    SPF_KEY_DELETE,
    SPF_KEY_BACKSPACE,
    SPF_KEY_SPACE,
    SPF_KEY_ENTER,
    SPF_KEY_ESCAPE,
    SPF_KEY_LEFT_CTRL, SPF_KEY_LEFT_SHIFT, SPF_KEY_LEFT_ALT, SPF_KEY_LEFT_SUPER,
    SPF_KEY_RIGHT_CTRL, SPF_KEY_RIGHT_SHIFT, SPF_KEY_RIGHT_ALT, SPF_KEY_RIGHT_SUPER,
    SPF_KEY_0 = 542, SPF_KEY_1, SPF_KEY_2, SPF_KEY_3, SPF_KEY_4, SPF_KEY_5, SPF_KEY_6, SPF_KEY_7, SPF_KEY_8, SPF_KEY_9,
    SPF_KEY_A = 552, SPF_KEY_B, SPF_KEY_C, SPF_KEY_D, SPF_KEY_E, SPF_KEY_F, SPF_KEY_G, SPF_KEY_H, SPF_KEY_I, SPF_KEY_J,
    SPF_KEY_K, SPF_KEY_L, SPF_KEY_M, SPF_KEY_N, SPF_KEY_O, SPF_KEY_P, SPF_KEY_Q, SPF_KEY_R, SPF_KEY_S, SPF_KEY_T,
    SPF_KEY_U, SPF_KEY_V, SPF_KEY_W, SPF_KEY_X, SPF_KEY_Y, SPF_KEY_Z,
    SPF_KEY_F1 = 578, SPF_KEY_F2, SPF_KEY_F3, SPF_KEY_F4, SPF_KEY_F5, SPF_KEY_F6, SPF_KEY_F7, SPF_KEY_F8, SPF_KEY_F9, SPF_KEY_F10, SPF_KEY_F11, SPF_KEY_F12
} SPF_Key;

/**
 * @enum SPF_Cond
 * @brief Conditions used to determine when a setting (like window position or size) should be applied.
 * @details These flags control the persistence and frequency of state changes for UI elements.
 */
typedef enum
{
    /** No condition. The setting will not be applied. */
    SPF_COND_NONE = 0,
    /** Always apply the setting every frame. Useful for programmatic animations. */
    SPF_COND_ALWAYS = 1 << 0,
    /** Apply the setting only once per runtime session. Subsequent calls will be ignored. */
    SPF_COND_ONCE = 1 << 1,
    /** Apply the setting if the object has no existing data in the .ini configuration file. */
    SPF_COND_FIRST_USE_EVER = 1 << 2,
    /** Apply the setting when the object appears (e.g., switches from hidden to visible state). */
    SPF_COND_APPEARING = 1 << 3
} SPF_Cond;

/**
 * @enum SPF_WindowFlags
 * @brief Configuration flags for window behavior and appearance (Begin, BeginChild).
 * @details These flags allow for deep customization of how a window interacts with the user and the layout engine.
 */
typedef enum
{
    /** Default window behavior: has title bar, can be resized, moved, and scrolled. */
    SPF_WINDOW_FLAG_NONE = 0,
    /** Completely hides the title bar (the top area containing the name and close button). */
    SPF_WINDOW_FLAG_NO_TITLE_BAR = 1 << 0,
    /** Disables the user's ability to resize the window by dragging its edges or corner. */
    SPF_WINDOW_FLAG_NO_RESIZE = 1 << 1,
    /** Disables the user's ability to move the window by dragging the title bar. */
    SPF_WINDOW_FLAG_NO_MOVE = 1 << 2,
    /** Disables and hides all scrollbars, even if content exceeds window dimensions. */
    SPF_WINDOW_FLAG_NO_SCROLLBAR = 1 << 3,
    /** Disables scrolling via the mouse wheel. User must use scrollbars manually if present. */
    SPF_WINDOW_FLAG_NO_SCROLL_WITH_MOUSE = 1 << 4,
    /** Disables the ability to collapse/minimize the window (usually via double-clicking the title bar). */
    SPF_WINDOW_FLAG_NO_COLLAPSE = 1 << 5,
    /** The window will automatically shrink or grow to perfectly fit its content every frame. */
    SPF_WINDOW_FLAG_ALWAYS_AUTO_RESIZE = 1 << 6,
    /** Disables the background drawing. The window will be transparent, showing only its widgets. */
    SPF_WINDOW_FLAG_NO_BACKGROUND = 1 << 7,
    /** The framework will not save/load the window's position and size to the .ini configuration file. */
    SPF_WINDOW_FLAG_NO_SAVED_SETTINGS = 1 << 8,
    /** The window will ignore all mouse interactions. Clicks will pass through to elements behind it. */
    SPF_WINDOW_FLAG_NO_MOUSE_INPUTS = 1 << 9,
    /** Adds a menu bar area at the top of the window (below the title bar). Use with UI_BeginMenuBar. */
    SPF_WINDOW_FLAG_MENU_BAR = 1 << 10,
    /** Enables horizontal scrolling. By default, only vertical scrolling is allowed. */
    SPF_WINDOW_FLAG_HORIZONTAL_SCROLLBAR = 1 << 11,
    /** The window will not take focus automatically when it first appears. */
    SPF_WINDOW_FLAG_NO_FOCUS_ON_APPEARING = 1 << 12,
    /** Clicking on the window will not bring it to the front of the display stack. */
    SPF_WINDOW_FLAG_NO_BRING_TO_FRONT_ON_FOCUS = 1 << 13,
    /** Always show the vertical scrollbar, even if the content fits within the window. */
    SPF_WINDOW_FLAG_ALWAYS_VERTICAL_SCROLLBAR = 1 << 14,
    /** Always show the horizontal scrollbar. */
    SPF_WINDOW_FLAG_ALWAYS_HORIZONTAL_SCROLLBAR = 1 << 15,
    /** Disables keyboard and gamepad navigation within this window. */
    SPF_WINDOW_FLAG_NO_NAV_INPUTS = 1 << 16,
    /** Disables the ability to focus on elements within this window using the navigation system (e.g., Tab key). */
    SPF_WINDOW_FLAG_NO_NAV_FOCUS = 1 << 17,
    /** Displays an asterisk (*) next to the window name to indicate unsaved changes. */
    SPF_WINDOW_FLAG_UNSAVED_DOCUMENT = 1 << 18,
    /** Disables the ability to dock this window into other nodes or containers. */
    SPF_WINDOW_FLAG_NO_DOCKING = 1 << 19,

    /* Internal flags - usually managed by the framework */
    /** Indicates the window is a child of another window. */
    SPF_WINDOW_FLAG_CHILD_WINDOW = 1 << 24,
    /** Indicates the window is a floating tooltip. */
    SPF_WINDOW_FLAG_TOOLTIP = 1 << 25,
    /** Indicates the window is a popup or context menu. */
    SPF_WINDOW_FLAG_POPUP = 1 << 26,
    /** Indicates the window is a modal (blocks interaction with other windows). */
    SPF_WINDOW_FLAG_MODAL = 1 << 27,
    /** Indicates the window is a sub-menu. */
    SPF_WINDOW_FLAG_CHILD_MENU = 1 << 28
} SPF_WindowFlags;


/**
 * @enum SPF_InputTextFlags
 * @brief Configuration flags for text input widgets (UI_InputText, UI_InputTextMultiline).
 * @details These flags control character filtering, keyboard behavior, and callback triggers.
 */
typedef enum
{
    /** Default behavior. */
    SPF_INPUT_TEXT_FLAG_NONE = 0,
    /** Allow only digits (0-9) and numeric symbols (+, -, ., etc.). */
    SPF_INPUT_TEXT_FLAG_CHARS_DECIMAL = 1 << 0,
    /** Allow only hexadecimal characters (0-9, A-F, a-f). */
    SPF_INPUT_TEXT_FLAG_CHARS_HEXADECIMAL = 1 << 1,
    /** Automatically convert all entered lowercase letters to uppercase. */
    SPF_INPUT_TEXT_FLAG_CHARS_UPPERCASE = 1 << 2,
    /** Disallow the entry of space characters. */
    SPF_INPUT_TEXT_FLAG_CHARS_NO_BLANK = 1 << 3,
    /** Automatically select the entire text when the field gains focus. */
    SPF_INPUT_TEXT_FLAG_AUTO_SELECT_ALL = 1 << 4,
    /** The input function returns true only when the Enter key is pressed, rather than on every change. */
    SPF_INPUT_TEXT_FLAG_ENTER_RETURNS_TRUE = 1 << 5,
    /** Enable a callback for text completion (typically triggered by the Tab key). */
    SPF_INPUT_TEXT_FLAG_CALLBACK_COMPLETION = 1 << 6,
    /** Enable a callback for navigating text history (typically triggered by Up/Down arrows). */
    SPF_INPUT_TEXT_FLAG_CALLBACK_HISTORY = 1 << 7,
    /** Trigger the callback on every frame while the widget is active. */
    SPF_INPUT_TEXT_FLAG_CALLBACK_ALWAYS = 1 << 8,
    /** Trigger a callback for every character entered to allow for custom filtering. */
    SPF_INPUT_TEXT_FLAG_CALLBACK_CHAR_FILTER = 1 << 9,
    /** Allow entering the Tab character into the buffer instead of switching focus. */
    SPF_INPUT_TEXT_FLAG_ALLOW_TAB_INPUT = 1 << 10,
    /** Ctrl+Enter inserts a newline, while a plain Enter finishes the input. */
    SPF_INPUT_TEXT_FLAG_CTRL_ENTER_FOR_NEW_LINE = 1 << 11,
    /** Disable horizontal scrolling. Text will either be clipped or wrapped. */
    SPF_INPUT_TEXT_FLAG_NO_HORIZONTAL_SCROLL = 1 << 12,
    /** New characters will overwrite existing ones (Insert key mode). */
    SPF_INPUT_TEXT_FLAG_ALWAYS_OVERWRITE = 1 << 13,
    /** Set the field to read-only mode. No user input is allowed. */
    SPF_INPUT_TEXT_FLAG_READ_ONLY = 1 << 14,
    /** Password mode: display asterisks instead of the actual characters. */
    SPF_INPUT_TEXT_FLAG_PASSWORD = 1 << 15,
    /** Disable the built-in undo/redo system for this field. */
    SPF_INPUT_TEXT_FLAG_NO_UNDO_REDO = 1 << 16,
    /** Allow characters used in scientific notation (e.g., 'e', 'E'). */
    SPF_INPUT_TEXT_FLAG_CHARS_SCIENTIFIC = 1 << 17,
    /** Trigger a callback when the internal buffer needs to be resized. */
    SPF_INPUT_TEXT_FLAG_CALLBACK_RESIZE = 1 << 18,
    /** Trigger a callback on any user edit. */
    SPF_INPUT_TEXT_FLAG_CALLBACK_EDIT = 1 << 19
} SPF_InputTextFlags;

/**
 * @enum SPF_TreeNodeFlags
 * @brief Flags for tree nodes (UI_TreeNode) and hierarchical headers (UI_CollapsingHeader).
 * @details Controls visual cues, interaction logic, and hierarchy behavior.
 */
typedef enum
{
    /** Default behavior. */
    SPF_TREE_NODE_FLAG_NONE = 0,
    /** The node is rendered in a "selected" state (usually a different background color). */
    SPF_TREE_NODE_FLAG_SELECTED = 1 << 0,
    /** Draw a frame around the node title (used by CollapsingHeader). */
    SPF_TREE_NODE_FLAG_FRAMED = 1 << 1,
    /** Allow other UI elements to overlap the node's hit area. */
    SPF_TREE_NODE_FLAG_ALLOW_OVERLAP = 1 << 2,
    /** Opening the node will not automatically trigger a 'TreePush' (manual management required). */
    SPF_TREE_NODE_FLAG_NO_TREE_PUSH_ON_OPEN = 1 << 3,
    /** Prevent the node from automatically opening when text logging occurs. */
    SPF_TREE_NODE_FLAG_NO_AUTO_OPEN_ON_LOG = 1 << 4,
    /** The node will be open by default when first encountered. */
    SPF_TREE_NODE_FLAG_DEFAULT_OPEN = 1 << 5,
    /** The node only toggles its open state on a double-click. */
    SPF_TREE_NODE_FLAG_OPEN_ON_DOUBLE_CLICK = 1 << 6,
    /** The node only toggles its open state when the arrow icon is clicked, not the label. */
    SPF_TREE_NODE_FLAG_OPEN_ON_ARROW = 1 << 7,
    /** Indicates the node is a "leaf" (has no children). Disables the expansion arrow. */
    SPF_TREE_NODE_FLAG_LEAF = 1 << 8,
    /** Display a bullet point instead of an expansion arrow. */
    SPF_TREE_NODE_FLAG_BULLET = 1 << 9,
    /** Use frame padding for the node's background and hit area. */
    SPF_TREE_NODE_FLAG_FRAME_PADDING = 1 << 10,
    /** The node's highlight area spans the full available width of the window. */
    SPF_TREE_NODE_FLAG_SPAN_AVAIL_WIDTH = 1 << 11,
    /** The node's highlight area spans the full width of the window, ignoring padding. */
    SPF_TREE_NODE_FLAG_SPAN_FULL_WIDTH = 1 << 12,
    /** In a table, the node's background highlight will span across all columns. */
    SPF_TREE_NODE_FLAG_SPAN_ALL_COLUMNS = 1 << 13,
    /** Navigation: moving left from a child node returns focus to this parent node. */
    SPF_TREE_NODE_FLAG_NAV_LEFT_JUMPS_BACK_HERE = 1 << 14,
    /** Preset for creating a header that collapses (Framed + NoTreePush + NoAutoOpen). */
    SPF_TREE_NODE_FLAG_COLLAPSING_HEADER = SPF_TREE_NODE_FLAG_FRAMED | SPF_TREE_NODE_FLAG_NO_TREE_PUSH_ON_OPEN | SPF_TREE_NODE_FLAG_NO_AUTO_OPEN_ON_LOG
} SPF_TreeNodeFlags;

/**
 * @enum SPF_TableFlags
 * @brief Configuration flags for high-level tables (UI_BeginTable).
 * @details Tables provide powerful multi-column layouts with optional sorting, resizing, and borders.
 */
typedef enum
{
    /** Basic table with no special features. */
    SPF_TABLE_FLAG_NONE = 0,
    /** Allow the user to manually resize columns by dragging their borders. */
    SPF_TABLE_FLAG_RESIZABLE = 1 << 0,
    /** Allow the user to change the order of columns by dragging their headers. */
    SPF_TABLE_FLAG_REORDERABLE = 1 << 1,
    /** Allow the user to hide/show columns via a context menu on the headers. */
    SPF_TABLE_FLAG_HIDEABLE = 1 << 2,
    /** Enable sorting. User can click headers to cycle through sort directions. */
    SPF_TABLE_FLAG_SORTABLE = 1 << 3,
    /** Disable saving and loading of table state (column widths, order) to the .ini file. */
    SPF_TABLE_FLAG_NO_SAVED_SETTINGS = 1 << 4,
    /** Show the column context menu when right-clicking anywhere in the table body. */
    SPF_TABLE_FLAG_CONTEXT_MENU_IN_BODY = 1 << 5,
    /** Enable alternating row background colors (zebra striping). */
    SPF_TABLE_FLAG_ROW_BG = 1 << 6,
    /** Draw horizontal lines between rows inside the table. */
    SPF_TABLE_FLAG_BORDERS_INNER_H = 1 << 7,
    /** Draw horizontal lines at the top and bottom of the table. */
    SPF_TABLE_FLAG_BORDERS_OUTER_H = 1 << 8,
    /** Draw vertical lines between columns inside the table. */
    SPF_TABLE_FLAG_BORDERS_INNER_V = 1 << 9,
    /** Draw vertical lines at the left and right edges of the table. */
    SPF_TABLE_FLAG_BORDERS_OUTER_V = 1 << 10,
    /** Draw all horizontal borders. */
    SPF_TABLE_FLAG_BORDERS_H = SPF_TABLE_FLAG_BORDERS_INNER_H | SPF_TABLE_FLAG_BORDERS_OUTER_H,
    /** Draw all vertical borders. */
    SPF_TABLE_FLAG_BORDERS_V = SPF_TABLE_FLAG_BORDERS_INNER_V | SPF_TABLE_FLAG_BORDERS_OUTER_V,
    /** Draw all internal borders (between cells). */
    SPF_TABLE_FLAG_BORDERS_INNER = SPF_TABLE_FLAG_BORDERS_INNER_V | SPF_TABLE_FLAG_BORDERS_INNER_H,
    /** Draw all external borders (the table frame). */
    SPF_TABLE_FLAG_BORDERS_OUTER = SPF_TABLE_FLAG_BORDERS_OUTER_V | SPF_TABLE_FLAG_BORDERS_OUTER_H,
    /** Draw all possible borders. */
    SPF_TABLE_FLAG_BORDERS = SPF_TABLE_FLAG_BORDERS_INNER | SPF_TABLE_FLAG_BORDERS_OUTER,
    /** Disable content clipping within cells (may improve performance but content may overlap). */
    SPF_TABLE_FLAG_NO_BORDERS_IN_BODY = 1 << 11,
    /** Borders inside the body are hidden until the user starts resizing a column. */
    SPF_TABLE_FLAG_NO_BORDERS_IN_BODY_UNTIL_RESIZE = 1 << 12,
    /** Sizing Policy: Columns take exactly as much space as their content needs. */
    SPF_TABLE_FLAG_SIZING_FIXED_FIT = 1 << 13,
    /** Sizing Policy: All columns have the same fixed width. */
    SPF_TABLE_FLAG_SIZING_FIXED_SAME = 2 << 13,
    /** Sizing Policy: Columns stretch proportionally to fill available space (Default). */
    SPF_TABLE_FLAG_SIZING_STRETCH_PROP = 3 << 13,
    /** Sizing Policy: All columns stretch equally to fill available space. */
    SPF_TABLE_FLAG_SIZING_STRETCH_SAME = 4 << 13,
    /** Disable the table's ability to automatically expand its host window horizontally. */
    SPF_TABLE_FLAG_NO_HOST_EXTEND_X = 1 << 16,
    /** Disable the table's ability to automatically expand its host window vertically. */
    SPF_TABLE_FLAG_NO_HOST_EXTEND_Y = 1 << 17,
    /** The table will attempt to keep all columns visible when shrinking, pushing others out if needed. */
    SPF_TABLE_FLAG_NO_KEEP_COLUMNS_VISIBLE = 1 << 18,
    /** Use high-precision width calculations (may cause slight jitter during resizing). */
    SPF_TABLE_FLAG_PRECISE_WIDTHS = 1 << 19,
    /** Disable clipping for the entire table. */
    SPF_TABLE_FLAG_NO_CLIP = 1 << 20,
    /** Add external padding on the left and right of the table. */
    SPF_TABLE_FLAG_PAD_OUTER_X = 1 << 21,
    /** Disable external padding. */
    SPF_TABLE_FLAG_NO_PAD_OUTER_X = 1 << 22,
    /** Disable internal padding between cells. */
    SPF_TABLE_FLAG_NO_PAD_INNER_X = 1 << 23,
    /** Enable horizontal scrolling within the table. */
    SPF_TABLE_FLAG_SCROLL_X = 1 << 24,
    /** Enable vertical scrolling within the table. */
    SPF_TABLE_FLAG_SCROLL_Y = 1 << 25,
    /** Allow sorting by multiple columns simultaneously (usually via Shift+Click). */
    SPF_TABLE_FLAG_SORT_MULTI = 1 << 26,
    /** Sorting cycle: Ascending -> Descending -> None (instead of just toggling). */
    SPF_TABLE_FLAG_SORT_TRISTATE = 1 << 27
} SPF_TableFlags;


/**
 * @enum SPF_TableColumnFlags
 * @brief Configuration flags for individual columns within a table (UI_TableSetupColumn).
 * @details Allows for granular control over each column's visibility, sizing, and sorting rules.
 */
typedef enum
{
    /** Default column behavior. */
    SPF_TABLE_COLUMN_FLAG_NONE = 0,
    /** Completely disables the column. It won't be drawn or take up any space. */
    SPF_TABLE_COLUMN_FLAG_DISABLED = 1 << 0,
    /** The column is hidden by default. User can enable it via the context menu. */
    SPF_TABLE_COLUMN_FLAG_DEFAULT_HIDE = 1 << 1,
    /** This column is used as the primary sort key when the table is first displayed. */
    SPF_TABLE_COLUMN_FLAG_DEFAULT_SORT = 1 << 2,
    /** The column will attempt to stretch to fill available space. */
    SPF_TABLE_COLUMN_FLAG_WIDTH_STRETCH = 1 << 3,
    /** The column has a fixed width and will not change when the table or window is resized. */
    SPF_TABLE_COLUMN_FLAG_WIDTH_FIXED = 1 << 4,
    /** Disables the user's ability to manually resize this specific column. */
    SPF_TABLE_COLUMN_FLAG_NO_RESIZE = 1 << 5,
    /** Disables the user's ability to drag and reorder this specific column. */
    SPF_TABLE_COLUMN_FLAG_NO_REORDER = 1 << 6,
    /** Disables the user's ability to hide this column via the context menu. */
    SPF_TABLE_COLUMN_FLAG_NO_HIDE = 1 << 7,
    /** Disables content clipping for this specific column. */
    SPF_TABLE_COLUMN_FLAG_NO_CLIP = 1 << 8,
    /** Disables the ability to use this column as a sort key. */
    SPF_TABLE_COLUMN_FLAG_NO_SORT = 1 << 9,
    /** Only allows sorting in ascending order for this column. */
    SPF_TABLE_COLUMN_FLAG_NO_SORT_ASCENDING = 1 << 10,
    /** Only allows sorting in descending order. */
    SPF_TABLE_COLUMN_FLAG_NO_SORT_DESCENDING = 1 << 11,
    /** Hides the text label in the column header. */
    SPF_TABLE_COLUMN_FLAG_NO_HEADER_LABEL = 1 << 12,
    /** Ignore the width of the header label when calculating automatic column width. */
    SPF_TABLE_COLUMN_FLAG_NO_HEADER_WIDTH = 1 << 13,
    /** First click on header will sort in ascending order. */
    SPF_TABLE_COLUMN_FLAG_PREFER_SORT_ASCENDING = 1 << 14,
    /** First click on header will sort in descending order. */
    SPF_TABLE_COLUMN_FLAG_PREFER_SORT_DESCENDING = 1 << 15,
    /** Enable indentation for content within this column. */
    SPF_TABLE_COLUMN_FLAG_INDENT_ENABLE = 1 << 16,
    /** Disable indentation for this column. */
    SPF_TABLE_COLUMN_FLAG_INDENT_DISABLE = 1 << 17,
    /** The header text for this column will be rendered at an angle. */
    SPF_TABLE_COLUMN_FLAG_ANGLED_HEADER = 1 << 18,

    /* Status flags (Output only) */
    /** [Output] True if the column is currently visible to the user. */
    SPF_TABLE_COLUMN_FLAG_IS_VISIBLE = 1 << 24,
    /** [Output] True if this column is currently part of the active sort specs. */
    SPF_TABLE_COLUMN_FLAG_IS_SORTED = 1 << 25,
    /** [Output] True if the mouse is currently hovering over this column's cells. */
    SPF_TABLE_COLUMN_FLAG_IS_HOVERED = 1 << 26
} SPF_TableColumnFlags;

/**
 * @enum SPF_TableRowFlags
 * @brief Configuration flags for table rows (UI_TableNextRow).
 * @details Primarily used to identify special rows like headers.
 */
typedef enum
{
    /** A standard data row. */
    SPF_TABLE_ROW_FLAG_NONE = 0,
    /** The row is a header row. It will use header styling and can be frozen/pinned if supported. */
    SPF_TABLE_ROW_FLAG_HEADERS = 1 << 0
} SPF_TableRowFlags;

/**
 * @enum SPF_TabBarFlags
 * @brief Configuration flags for tab bars (UI_BeginTabBar).
 * @details Tab bars organize multiple windows or views into a single row of clickable tabs.
 *          These flags control sizing, ordering, and interaction behavior of the entire bar.
 */
typedef enum
{
    /** Default tab bar behavior. */
    SPF_TAB_BAR_FLAG_NONE = 0,
    /** Allow the user to reorder tabs by dragging them horizontally with the mouse. */
    SPF_TAB_BAR_FLAG_REORDERABLE = 1 << 0,
    /** Automatically select and switch to new tabs when they are added to the bar. */
    SPF_TAB_BAR_FLAG_AUTO_SELECT_NEW_TABS = 1 << 1,
    /** Show a button that opens a popup list of all tabs (useful when many tabs overflow the bar). */
    SPF_TAB_BAR_FLAG_TAB_LIST_POPUP_BUTTON = 1 << 2,
    /** Prevent the user from closing tabs using the middle mouse button (scroll wheel click). */
    SPF_TAB_BAR_FLAG_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON = 1 << 3,
    /** Disable the left/right scroll buttons that appear when tabs exceed the bar's width. */
    SPF_TAB_BAR_FLAG_NO_TAB_LIST_SCROLLING_BUTTONS = 1 << 4,
    /** Disable tooltips that show the full tab name when the mouse hovers over a tab. */
    SPF_TAB_BAR_FLAG_NO_TOOLTIP = 1 << 5,
    /** Sizing Policy: Shrink the width of all tabs equally to attempt to fit the bar. */
    SPF_TAB_BAR_FLAG_FITTING_POLICY_RESIZE_DOWN = 1 << 6,
    /** Sizing Policy: Maintain tab widths and allow the entire bar to scroll horizontally. */
    SPF_TAB_BAR_FLAG_FITTING_POLICY_SCROLL = 1 << 7,
    /** Mask for fitting policy bits. */
    SPF_TAB_BAR_FLAG_FITTING_POLICY_MASK_ = SPF_TAB_BAR_FLAG_FITTING_POLICY_RESIZE_DOWN | SPF_TAB_BAR_FLAG_FITTING_POLICY_SCROLL,
    /** Default fitting policy. */
    SPF_TAB_BAR_FLAG_FITTING_POLICY_DEFAULT_ = SPF_TAB_BAR_FLAG_FITTING_POLICY_RESIZE_DOWN
} SPF_TabBarFlags;

/**
 * @enum SPF_TabItemFlags
 * @brief Configuration flags for individual tab items (UI_BeginTabItem).
 * @details These flags control the visual state and local behavior of a single tab.
 */
typedef enum
{
    /** Default tab item behavior. */
    SPF_TAB_ITEM_FLAG_NONE = 0,
    /** Displays an asterisk (*) next to the tab name to indicate that the document has unsaved changes. */
    SPF_TAB_ITEM_FLAG_UNSAVED_DOCUMENT = 1 << 0,
    /** Programmatically select this tab in the current frame, making its content active. */
    SPF_TAB_ITEM_FLAG_SET_SELECTED = 1 << 1,
    /** Prevent closing this specific tab with the middle mouse button. */
    SPF_TAB_ITEM_FLAG_NO_CLOSE_WITH_MIDDLE_MOUSE_BUTTON = 1 << 2,
    /** Prevent the tab from using its label string as a unique ID in the global ID stack. */
    SPF_TAB_ITEM_FLAG_NO_PUSH_ID = 1 << 3,
    /** Disable the hover tooltip for this specific tab. */
    SPF_TAB_ITEM_FLAG_NO_TOOLTIP = 1 << 4,
    /** Disable the user's ability to manually reorder this specific tab by dragging. */
    SPF_TAB_ITEM_FLAG_NO_REORDER = 1 << 5,
    /** Force this tab to stay pinned on the far-left side of the tab bar. */
    SPF_TAB_ITEM_FLAG_LEADING = 1 << 6,
    /** Force this tab to stay pinned on the far-right side of the tab bar. */
    SPF_TAB_ITEM_FLAG_TRAILING = 1 << 7
} SPF_TabItemFlags;

/**
 * @enum SPF_MultiSelectFlags
 * @brief Configuration flags for the Multi-Select API (UI_BeginMultiSelect).
 * @details Controls how multiple items can be selected, ranged, and cleared.
 */
typedef enum {
    /** Default behavior. */
    SPF_MULTI_SELECT_FLAG_NONE = 0,
    /** Only one item can be selected at a time. */
    SPF_MULTI_SELECT_FLAG_SINGLE_SELECT = 1 << 0,
    /** Disable 'Select All' shortcut (Ctrl+A). */
    SPF_MULTI_SELECT_FLAG_NO_SELECT_ALL = 1 << 1,
    /** Disable range selection (Shift+Click). */
    SPF_MULTI_SELECT_FLAG_NO_RANGE_SELECT = 1 << 2,
    /** Disable automatic selection on navigation or click. */
    SPF_MULTI_SELECT_FLAG_NO_AUTO_SELECT = 1 << 3,
    /** Disable automatic clearing of selection. */
    SPF_MULTI_SELECT_FLAG_NO_AUTO_CLEAR = 1 << 4,
    /** Do not clear selection when clicking on empty space. */
    SPF_MULTI_SELECT_FLAG_NO_AUTO_CLEAR_ON_CLICK_OUTSIDE = 1 << 5,
    /** Enable keyboard navigation wrapping within the selection scope. */
    SPF_MULTI_SELECT_FLAG_NAV_WRAPPING = 1 << 6,
    /** Selection loops around when reaching boundaries. */
    SPF_MULTI_SELECT_FLAG_LOOP = 1 << 7,
    /** Enable 1D box-selection (marquee). */
    SPF_MULTI_SELECT_FLAG_BOX_SELECT_1D = 1 << 8,
    /** Enable 2D box-selection (marquee). */
    SPF_MULTI_SELECT_FLAG_BOX_SELECT_2D = 1 << 9,
    /** Disable scrolling during box-selection. */
    SPF_MULTI_SELECT_FLAG_BOX_SELECT_NO_SCROLL = 1 << 10,
    /** Clear selection when the Escape key is pressed. */
    SPF_MULTI_SELECT_FLAG_CLEAR_ON_ESCAPE = 1 << 11,
    /** Clear selection when clicking on the window background. */
    SPF_MULTI_SELECT_FLAG_CLEAR_ON_CLICK_VOID = 1 << 12,
    /** The selection scope is limited to the current window. */
    SPF_MULTI_SELECT_FLAG_SCOPE_WINDOW = 1 << 13,
    /** The selection scope is defined by a specific rectangle. */
    SPF_MULTI_SELECT_FLAG_SCOPE_RECT = 1 << 14,
    /** Select items immediately on mouse down. */
    SPF_MULTI_SELECT_FLAG_SELECT_ON_CLICK = 1 << 15,
    /** Use default interaction rules for selection. */
    SPF_MULTI_SELECT_FLAG_SELECT_ON_DEFAULT_PURPOSE = 1 << 16
} SPF_MultiSelectFlags;

/**
 * @enum SPF_InputFlags
 * @brief Flags for input routing and shortcut handling (UI_Shortcut).
 */
typedef enum {
    /** Default behavior. */
    SPF_INPUT_FLAG_NONE = 0,
    /** Enable key repeat for the shortcut. */
    SPF_INPUT_FLAG_REPEAT = 1 << 0,
    /** Route only if the item is active. */
    SPF_INPUT_FLAG_ROUTE_ACTIVE = 1 << 10,
    /** Route to the focused window stack (Default). */
    SPF_INPUT_FLAG_ROUTE_FOCUSED = 1 << 11,
    /** Global shortcut, processed regardless of focus. */
    SPF_INPUT_FLAG_ROUTE_GLOBAL = 1 << 12,
    /** Always process, do not participate in routing logic. */
    SPF_INPUT_FLAG_ROUTE_ALWAYS = 1 << 13,
    /** Overrides focused route even if not in the focus stack. */
    SPF_INPUT_FLAG_ROUTE_OVER_FOCUSED = 1 << 14,
    /** Overrides active item route. */
    SPF_INPUT_FLAG_ROUTE_OVER_ACTIVE = 1 << 15,
    /** Process only if no ImGui window is focused. */
    SPF_INPUT_FLAG_ROUTE_UNLESS_BG_FOCUSED = 1 << 16,
    /** Evaluate route from the root window's perspective. */
    SPF_INPUT_FLAG_ROUTE_FROM_ROOT_WINDOW = 1 << 17,
    /** Automatically display a tooltip for the shortcut. */
    SPF_INPUT_FLAG_TOOLTIP = 1 << 18
} SPF_InputFlags;

/**
 * @enum SPF_SelectionRequestType
 * @brief Defines the type of selection modification being requested by the system.
 */
typedef enum {
    /** No request. */
    SPF_SELECTION_REQUEST_NONE = 0,
    /** Request to set the selection state for all items in the scope. */
    SPF_SELECTION_REQUEST_SET_ALL,
    /** Request to set the selection state for a specific range of items. */
    SPF_SELECTION_REQUEST_SET_RANGE
} SPF_SelectionRequestType;

/**
 * @typedef SPF_Storage_Handle
 * @brief Opaque handle to the internal ImGuiStorage system.
 * @details Allows for direct manipulation of persisted UI states.
 */
typedef void* SPF_Storage_Handle;

/**
 * @typedef SPF_PlotGetter
 * @brief Function signature for dynamic data retrieval in plot widgets.
 * @param data User-provided data pointer.
 * @param idx Index of the value to retrieve.
 * @return float The value at the specified index.
 */
typedef float (*SPF_PlotGetter)(void* data, int idx);

/**
 * @struct SPF_SelectionRequest
 * @brief Represents a single request to update the selection state.
 */
typedef struct {
    /** The type of modification (SetAll or SetRange). */
    SPF_SelectionRequestType type;
    /** The target selection state (true = selected, false = unselected). */
    bool selected;
    /** Direction of the range selection (-1 or +1). */
    int64_t range_direction;
    /** User data ID of the first item in the range. */
    int64_t first_item_user_data;
    /** User data ID of the last item in the range. */
    int64_t last_item_user_data;
} SPF_SelectionRequest;

/**
 * @struct SPF_MultiSelectIO
 * @brief Data exchange structure for the Multi-Select API.
 * @details Contains the list of selection requests that the plugin must process.
 */
typedef struct {
    /** Array of selection requests to be executed. */
    SPF_SelectionRequest* requests;
    /** Number of valid requests in the array. */
    int requests_count;
    /** User data ID of the item where range selection started. */
    int64_t range_src_item_user_data;
    /** User data ID of the item where range selection ended. */
    int64_t range_dst_item_user_data;
} SPF_MultiSelectIO;

/**
 * @enum SPF_PopupFlags
 * @brief Configuration flags for popups and context menus (UI_BeginPopup, UI_OpenPopup).
 * @details Controls trigger conditions and layering behavior for floating windows.
 */
typedef enum
{
    /** Default popup behavior. */
    SPF_POPUP_FLAG_NONE = 0,
    /** Open the popup when the left mouse button is clicked. */
    SPF_POPUP_FLAG_MOUSE_BUTTON_LEFT = 0,
    /** Open the popup when the right mouse button is clicked. */
    SPF_POPUP_FLAG_MOUSE_BUTTON_RIGHT = 1,
    /** Open the popup when the middle mouse button is clicked. */
    SPF_POPUP_FLAG_MOUSE_BUTTON_MIDDLE = 2,
    /** Technical mask for mouse button bits (0-4). Used by the engine to extract button index. */
    SPF_POPUP_FLAG_MOUSE_BUTTON_MASK_ = 0x1F,
    /** Default mouse button used for popups (Right click). */
    SPF_POPUP_FLAG_MOUSE_BUTTON_DEFAULT_ = 1,
    /** Prevent opening the popup if another popup at the same level is already open. */
    SPF_POPUP_FLAG_NO_OPEN_OVER_EXISTING_POPUP = 1 << 5,
    /** Prevent opening the popup when clicking on existing UI items (only triggers on empty space). */
    SPF_POPUP_FLAG_NO_OPEN_OVER_ITEMS = 1 << 6,
    /** Allow the function to match any popup ID (global query across all IDs). */
    SPF_POPUP_FLAG_ANY_POPUP_ID = 1 << 7,
    /** Allow the function to query all levels of the popup hierarchy (recursive check). */
    SPF_POPUP_FLAG_ANY_POPUP_LEVEL = 1 << 8,
    /** Helper: match any popup regardless of its specific ID or depth level. */
    SPF_POPUP_FLAG_ANY_POPUP = SPF_POPUP_FLAG_ANY_POPUP_ID | SPF_POPUP_FLAG_ANY_POPUP_LEVEL
} SPF_PopupFlags;

/**
 * @enum SPF_SelectableFlags
 * @brief Flags for selectable items (UI_Selectable).
 * @details Controls interaction behavior for items that can be highlighted, chosen, or used as list elements.
 */
typedef enum
{
    /** Default behavior. */
    SPF_SELECTABLE_FLAG_NONE = 0,
    /** Clicking the item will not automatically close an open popup or menu. */
    SPF_SELECTABLE_FLAG_DONT_CLOSE_POPUPS = 1 << 0,
    /** In a table, the highlight/hover area will span the entire row (all columns). */
    SPF_SELECTABLE_FLAG_SPAN_ALL_COLUMNS = 1 << 1,
    /** The selectable only triggers (returns true) on a double-click. */
    SPF_SELECTABLE_FLAG_ALLOW_DOUBLE_CLICK = 1 << 2,
    /** Disables the item. It will be rendered grayed-out and ignore mouse interactions. */
    SPF_SELECTABLE_FLAG_DISABLED = 1 << 3,
    /** Allow other UI elements to overlap the selectable's hit-test area. */
    SPF_SELECTABLE_FLAG_ALLOW_OVERLAP = 1 << 4
} SPF_SelectableFlags;

/**
 * @enum SPF_ComboFlags
 * @brief Configuration flags for combo boxes (UI_BeginCombo).
 * @details Controls the appearance, sizing, and preview logic of the dropdown selection window.
 */
typedef enum
{
    /** Default combo box behavior. */
    SPF_COMBO_FLAG_NONE = 0,
    /** The dropdown list will have a small maximum height (approx. 4 items). */
    SPF_COMBO_FLAG_POPUP_MAX_HEIGHT_SMALL = 1 << 0,
    /** Standard maximum height (approx. 8 items). */
    SPF_COMBO_FLAG_POPUP_MAX_HEIGHT_REGULAR = 1 << 1,
    /** Large maximum height (approx. 20 items). */
    SPF_COMBO_FLAG_POPUP_MAX_HEIGHT_LARGE = 1 << 2,
    /** The dropdown height will be calculated to exactly match the number of items. */
    SPF_COMBO_FLAG_POPUP_MAX_HEIGHT_IN_ITEMS = 1 << 3,
    /** Hide the downward-pointing arrow icon on the right side of the combo box. */
    SPF_COMBO_FLAG_NO_ARROW_BUTTON = 1 << 4,
    /** Completely hide the preview area showing the currently selected value. */
    SPF_COMBO_FLAG_NO_PREVIEW = 1 << 5,
    /** Automatically adjust the width of the combo box to fit its current preview text. */
    SPF_COMBO_FLAG_WIDTH_FIT_PREVIEW = 1 << 6,
    /** Technical mask for height-related bits. Used internally for extraction. */
    SPF_COMBO_FLAG_HEIGHT_MASK_ = SPF_COMBO_FLAG_POPUP_MAX_HEIGHT_SMALL | SPF_COMBO_FLAG_POPUP_MAX_HEIGHT_REGULAR | SPF_COMBO_FLAG_POPUP_MAX_HEIGHT_LARGE | SPF_COMBO_FLAG_POPUP_MAX_HEIGHT_IN_ITEMS
} SPF_ComboFlags;

/**
 * @enum SPF_FocusedFlags
 * @brief Flags for querying the focus state of a window (UI_IsWindowFocused).
 * @details Allows for granular checks of the focus hierarchy, including child windows and docking nodes.
 */
typedef enum
{
    /** Check focus for the current window only. */
    SPF_FOCUSED_FLAG_NONE = 0,
    /** Returns true if any child window of the current window currently has focus. */
    SPF_FOCUSED_FLAG_CHILD_WINDOWS = 1 << 0,
    /** Returns true if the entire window hierarchy (starting from the root) is focused. */
    SPF_FOCUSED_FLAG_ROOT_WINDOW = 1 << 1,
    /** Returns true if ANY window managed by the framework currently has focus. */
    SPF_FOCUSED_FLAG_ANY_WINDOW = 1 << 2,
    /** Do not consider windows that are part of a popup or context menu hierarchy. */
    SPF_FOCUSED_FLAG_NO_POPUP_HIERARCHY = 1 << 3,
    /** Consider the docking hierarchy (adjacent tabs within the same docking node). */
    SPF_FOCUSED_FLAG_DOCK_HIERARCHY = 1 << 4,
    /** Combined flag for checking both the root window and all its children. */
    SPF_FOCUSED_FLAG_ROOT_AND_CHILD_WINDOWS = SPF_FOCUSED_FLAG_ROOT_WINDOW | SPF_FOCUSED_FLAG_CHILD_WINDOWS
} SPF_FocusedFlags;

/**
 * @enum SPF_HoveredFlags
 * @brief Flags for querying if the mouse is over an item or window (UI_IsItemHovered, UI_IsWindowHovered).
 * @details Allows for refining what "hovered" means, accounting for overlaps, popups, and specific timing delays.
 */
typedef enum
{
    /** Standard check: item must be visible and not blocked by any other window or popup. */
    SPF_HOVERED_FLAG_NONE = 0,
    /** Returns true if the mouse is over any child window of the current window. */
    SPF_HOVERED_FLAG_CHILD_WINDOWS = 1 << 0,
    /** Returns true if the mouse is over the entire window tree (starting from the root). */
    SPF_HOVERED_FLAG_ROOT_WINDOW = 1 << 1,
    /** Returns true if the mouse is over any window managed by the framework. */
    SPF_HOVERED_FLAG_ANY_WINDOW = 1 << 2,
    /** Do not consider windows that are part of a popup or context menu hierarchy. */
    SPF_HOVERED_FLAG_NO_POPUP_HIERARCHY = 1 << 3,
    /** Consider the docking hierarchy (adjacent tabs within the same docking node). */
    SPF_HOVERED_FLAG_DOCK_HIERARCHY = 1 << 4,
    /** Return true even if a popup window is blocking access to this item/window. */
    SPF_HOVERED_FLAG_ALLOW_WHEN_BLOCKED_BY_POPUP = 1 << 5,
    /** Return true even if an active item (e.g., during a drag) is blocking access. */
    SPF_HOVERED_FLAG_ALLOW_WHEN_BLOCKED_BY_ACTIVE_ITEM = 1 << 7,
    /** Return true even if the item is overlapped by another widget in the same window. */
    SPF_HOVERED_FLAG_ALLOW_WHEN_OVERLAPPED_BY_ITEM = 1 << 8,
    /** Return true even if the window is overlapped by another window. */
    SPF_HOVERED_FLAG_ALLOW_WHEN_OVERLAPPED_BY_WINDOW = 1 << 9,
    /** Only check for mouse position within the bounding box, ignoring visibility, focus, or layers. */
    SPF_HOVERED_FLAG_RECT_ONLY = 1 << 10,
    /** Combined flag for checking both the root window and all its children. */
    SPF_HOVERED_FLAG_ROOT_AND_CHILD_WINDOWS = SPF_FOCUSED_FLAG_ROOT_WINDOW | SPF_FOCUSED_FLAG_CHILD_WINDOWS,
    /** Use logic appropriate for triggering tooltips (respects global hover delay settings). */
    SPF_HOVERED_FLAG_FOR_TOOLTIP = 1 << 11,
    /** Returns true only if the mouse has remained stationary for a short period. */
    SPF_HOVERED_FLAG_STATIONARY = 1 << 12,
    /** Trigger immediately with no delay, ignoring Style.HoverDelay. */
    SPF_HOVERED_FLAG_DELAY_NONE = 1 << 13,
    /** Use a short delay before returning true (defined by Style.HoverDelayShort). */
    SPF_HOVERED_FLAG_DELAY_SHORT = 1 << 14,
    /** Use the standard delay before returning true (defined by Style.HoverDelayNormal). */
    SPF_HOVERED_FLAG_DELAY_NORMAL = 1 << 15,
    /** Disable the shared delay timer for this specific hovered check. */
    SPF_HOVERED_FLAG_NO_SHARED_DELAY = 1 << 16
} SPF_HoveredFlags;

/**
 * @enum SPF_DockNodeFlags
 * @brief Flags for configuring docking nodes (UI_DockSpace, DockBuilder).
 * @details Controls how windows can be attached to, split within, or removed from a specific node.
 */
typedef enum
{
    /** No restrictions. Standard docking behavior. */
    SPF_DOCK_NODE_FLAG_NONE = 0,
    /** The node will only exist as long as it is needed in the current frame (not saved to .ini). */
    SPF_DOCK_NODE_FLAG_KEEP_ALIVE_ONLY = 1 << 0,
    /** Prevent users from docking windows into the central area of this node. */
    SPF_DOCK_NODE_FLAG_NO_DOCKING_OVER_CENTRAL_NODE = 1 << 2,
    /** Allow the background or underlying render to pass through the central node area. */
    SPF_DOCK_NODE_FLAG_PASSTHRU_CENTRAL_NODE = 1 << 3,
    /** Prevent the user from splitting this node into two smaller nodes. */
    SPF_DOCK_NODE_FLAG_NO_DOCKING_SPLIT = 1 << 4,
    /** Prevent any window from being docked directly onto this specific node. */
    SPF_DOCK_NODE_FLAG_NO_DOCKING_OVER_ME = 1 << 5,
    /** Prevent docking windows into nodes that are immediate neighbors of this node. */
    SPF_DOCK_NODE_FLAG_NO_DOCKING_OVER_OTHER = 1 << 6,
    /** Disable the user's ability to resize the splitter/gap between windows in this node. */
    SPF_DOCK_NODE_FLAG_NO_RESIZE = 1 << 7,
    /** Automatically hide the tab bar if this node contains only a single window. */
    SPF_DOCK_NODE_FLAG_AUTO_HIDE_TAB_BAR = 1 << 8,
    /** Completely disable and hide the tab bar for this node. */
    SPF_DOCK_NODE_FLAG_NO_TAB_BAR = 1 << 9,
    /** Hide the tab bar UI elements like the close and menu buttons. */
    SPF_DOCK_NODE_FLAG_HIDDEN_TAB_BAR = 1 << 10,
    /** Hide the window menu button (the small triangle) in the corner of the node. */
    SPF_DOCK_NODE_FLAG_NO_WINDOW_MENU_BUTTON = 1 << 11,
    /** Hide the close button for all windows residing in this node. */
    SPF_DOCK_NODE_FLAG_NO_CLOSE_BUTTON = 1 << 12,
    /** Completely disable all docking functionality for this node and its children. */
    SPF_DOCK_NODE_FLAG_NO_DOCKING = 1 << 13
} SPF_DockNodeFlags;

/**
 * @enum SPF_DragDropFlags
 * @brief Flags for drag and drop operations (UI_BeginDragDropSource, UI_BeginDragDropTarget).
 * @details Controls how data is packaged, displayed, and accepted during a drag-and-drop interaction.
 */
typedef enum
{
    /** Default behavior. */
    SPF_DRAG_DROP_FLAG_NONE = 0,
    /** [Source] Do not show the default tooltip while dragging. Use if you provide a custom tooltip. */
    SPF_DRAG_DROP_FLAG_SOURCE_NO_PREVIEW_TOOLTIP = 1 << 0,
    /** [Source] Do not disable the hover state of the source item while dragging. */
    SPF_DRAG_DROP_FLAG_SOURCE_NO_DISABLE_HOVER = 1 << 1,
    /** [Source] Holding the mouse over a collapsed node will not automatically open it. */
    SPF_DRAG_DROP_FLAG_SOURCE_NO_HOLD_TO_OPEN_OTHERS = 1 << 2,
    /** [Source] Allow the payload to have a NULL ID. Useful for simple drag operations. */
    SPF_DRAG_DROP_FLAG_SOURCE_ALLOW_NULL_ID = 1 << 3,
    /** [Source] The drag operation started from outside the framework (e.g., from the OS). */
    SPF_DRAG_DROP_FLAG_SOURCE_EXTERN = 1 << 4,
    /** [Source] The payload automatically expires and is cleared after exactly one frame. */
    SPF_DRAG_DROP_FLAG_SOURCE_AUTO_EXPIRE_PAYLOAD = 1 << 5,
    /** [Target] Accept the payload even before the mouse button is released (during hover). */
    SPF_DRAG_DROP_FLAG_ACCEPT_BEFORE_DELIVERY = 1 << 10,
    /** [Target] Do not draw the default highlight rectangle around the target area. */
    SPF_DRAG_DROP_FLAG_ACCEPT_NO_DRAW_DEFAULT_RECT = 1 << 11,
    /** [Target] Do not show the source's tooltip when hovering over this specific target. */
    SPF_DRAG_DROP_FLAG_ACCEPT_NO_PREVIEW_TOOLTIP = 1 << 12,
    /** [Target] Only peek at the data without actually accepting or consuming the drop. */
    SPF_DRAG_DROP_FLAG_ACCEPT_PEEK_ONLY = SPF_DRAG_DROP_FLAG_ACCEPT_BEFORE_DELIVERY | SPF_DRAG_DROP_FLAG_ACCEPT_NO_DRAW_DEFAULT_RECT
} SPF_DragDropFlags;

/**
 * @enum SPF_DataType
 * @brief Fundamental data types for numeric widgets (UI_Drag, UI_Slider, UI_Input).
 * @details These types tell the API how to interpret raw 'void*' pointers and how to format the displayed values.
 */
typedef enum
{
    /** Signed 8-bit integer (signed char). */
    SPF_DATA_TYPE_S8,
    /** Unsigned 8-bit integer (unsigned char). */
    SPF_DATA_TYPE_U8,
    /** Signed 16-bit integer (short). */
    SPF_DATA_TYPE_S16,
    /** Unsigned 16-bit integer (unsigned short). */
    SPF_DATA_TYPE_U16,
    /** Signed 32-bit integer (int). */
    SPF_DATA_TYPE_S32,
    /** Unsigned 32-bit integer (unsigned int). */
    SPF_DATA_TYPE_U32,
    /** Signed 64-bit integer (long long / __int64). */
    SPF_DATA_TYPE_S64,
    /** Unsigned 64-bit integer (unsigned long long / unsigned __int64). */
    SPF_DATA_TYPE_U64,
    /** 32-bit floating point (float). */
    SPF_DATA_TYPE_FLOAT,
    /** 64-bit floating point (double). */
    SPF_DATA_TYPE_DOUBLE,
    /** Total number of data types. */
    SPF_DATA_TYPE_COUNT
} SPF_DataType;

/**
 * @enum SPF_Dir
 * @brief Simple direction enum used for arrow icons, window placement, and layout rules.
 */
typedef enum
{
    /** No direction specified or applicable. */
    SPF_DIR_NONE = -1,
    /** Leftward direction. */
    SPF_DIR_LEFT = 0,
    /** Rightward direction. */
    SPF_DIR_RIGHT = 1,
    /** Upward direction. */
    SPF_DIR_UP = 2,
    /** Downward direction. */
    SPF_DIR_DOWN = 3,
    /** Total count of directions. */
    SPF_DIR_COUNT
} SPF_Dir;

/**
 * @enum SPF_SortDirection
 * @brief Table sort direction (UI_TableGetSortSpecs).
 */
typedef enum
{
    /** No sorting applied to the column. */
    SPF_SORT_DIRECTION_NONE = 0,
    /** Sort the column in ascending order (A-Z, 0-9). */
    SPF_SORT_DIRECTION_ASCENDING = 1,
    /** Sort the column in descending order (Z-A, 9-0). */
    SPF_SORT_DIRECTION_DESCENDING = 2
} SPF_SortDirection;

/**
 * @enum SPF_ColorEditFlags
 * @brief Flags for customizing the behavior and layout of color editors and pickers (UI_ColorEdit, UI_ColorPicker).
 * @details Controls which components are shown (Alpha, RGB, HSV) and how the user interacts with them.
 */
typedef enum
{
    /** Default behavior. */
    SPF_COLOR_EDIT_FLAG_NONE = 0,
    /** Ignore/hide the alpha component (no transparency editing). */
    SPF_COLOR_EDIT_FLAG_NO_ALPHA = 1 << 1,
    /** Disable the popup picker when clicking on the color square. */
    SPF_COLOR_EDIT_FLAG_NO_PICKER = 1 << 2,
    /** Disable the context menu where users can change display options. */
    SPF_COLOR_EDIT_FLAG_NO_OPTIONS = 1 << 3,
    /** Hide the small color preview square next to the label. */
    SPF_COLOR_EDIT_FLAG_NO_SMALL_PREVIEW = 1 << 4,
    /** Hide the numeric input fields. Only the color square/picker remains. */
    SPF_COLOR_EDIT_FLAG_NO_INPUTS = 1 << 5,
    /** Disable the tooltip that shows color values when hovering over the square. */
    SPF_COLOR_EDIT_FLAG_NO_TOOLTIP = 1 << 6,
    /** Hide the text label associated with the widget. */
    SPF_COLOR_EDIT_FLAG_NO_LABEL = 1 << 7,
    /** Hide the vertical color bar shown on the side of the picker. */
    SPF_COLOR_EDIT_FLAG_NO_SIDE_PREVIEW = 1 << 8,
    /** Disable the ability to drag-and-drop colors from/to this widget. */
    SPF_COLOR_EDIT_FLAG_NO_DRAG_DROP = 1 << 9,
    /** Disable the border around the color square. */
    SPF_COLOR_EDIT_FLAG_NO_BORDER = 1 << 10,
    /** Show a vertical alpha bar in the picker. */
    SPF_COLOR_EDIT_FLAG_ALPHA_BAR = 1 << 16,
    /** Show alpha as a transparent checkerboard pattern in the square. */
    SPF_COLOR_EDIT_FLAG_ALPHA_PREVIEW = 1 << 17,
    /** Show alpha preview but only on half of the color square. */
    SPF_COLOR_EDIT_FLAG_ALPHA_PREVIEW_HALF = 1 << 18,
    /** Enable High Dynamic Range (HDR) color editing. */
    SPF_COLOR_EDIT_FLAG_HDR = 1 << 19,
    /** Display values in RGB format (Default). */
    SPF_COLOR_EDIT_FLAG_DISPLAY_RGB = 1 << 20,
    /** Display values in HSV format. */
    SPF_COLOR_EDIT_FLAG_DISPLAY_HSV = 1 << 21,
    /** Display values in Hexadecimal format (e.g., #RRGGBB). */
    SPF_COLOR_EDIT_FLAG_DISPLAY_HEX = 1 << 22,
    /** Use 0-255 range for numeric inputs (Default for RGB). */
    SPF_COLOR_EDIT_FLAG_UINT8 = 1 << 23,
    /** Use 0.0-1.0 range for numeric inputs. */
    SPF_COLOR_EDIT_FLAG_FLOAT = 1 << 24,
    /** [Picker] Use a vertical hue bar. */
    SPF_COLOR_EDIT_FLAG_PICKER_HUE_BAR = 1 << 25,
    /** [Picker] Use a hue wheel (circle). */
    SPF_COLOR_EDIT_FLAG_PICKER_HUE_WHEEL = 1 << 26,
    /** [Input] Force RGB numeric input. */
    SPF_COLOR_EDIT_FLAG_INPUT_RGB = 1 << 27,
    /** [Input] Force HSV numeric input. */
    SPF_COLOR_EDIT_FLAG_INPUT_HSV = 1 << 28
} SPF_ColorEditFlags;

/**
 * @enum SPF_SliderFlags
 * @brief Configuration flags for slider and drag widgets (UI_Slider..., UI_Drag...).
 * @details Controls how numeric values are constrained and displayed during interaction.
 */
typedef enum
{
    /** Default behavior. */
    SPF_SLIDER_FLAG_NONE = 0,
    /** Clamp value to min/max bounds when inputting manually via keyboard. */
    SPF_SLIDER_FLAG_ALWAYS_CLAMP = 1 << 4,
    /** Enable logarithmic scale for the slider (useful for frequency or volume). */
    SPF_SLIDER_FLAG_LOGARITHMIC = 1 << 5,
    /** Disable rounding of the displayed value to the nearest step. */
    SPF_SLIDER_FLAG_NO_ROUND_STEP = 1 << 6,
    /** Render the slider vertically instead of horizontally. */
    SPF_SLIDER_FLAG_VERTICAL = 1 << 7,
    /** The widget is visible but its value cannot be changed by the user. */
    SPF_SLIDER_FLAG_READ_ONLY = 1 << 21
} SPF_SliderFlags;

/**
 * @enum SPF_MouseButton
 * @brief Identifiers for physical mouse buttons.
 */
typedef enum
{
    /** The primary (usually left) mouse button. */
    SPF_MOUSE_BUTTON_LEFT = 0,
    /** The secondary (usually right) mouse button. */
    SPF_MOUSE_BUTTON_RIGHT = 1,
    /** The middle mouse button (scroll wheel click). */
    SPF_MOUSE_BUTTON_MIDDLE = 2,
    /** Total number of supported mouse buttons. */
    SPF_MOUSE_BUTTON_COUNT = 5
} SPF_MouseButton;

/**
 * @enum SPF_MouseCursor
 * @brief Enumeration of supported system mouse cursor shapes.
 * @details The actual visual representation depends on the host operating system.
 */
typedef enum
{
    /** Do not display any mouse cursor. */
    SPF_MOUSE_CURSOR_NONE = -1,
    /** Standard arrow pointer. */
    SPF_MOUSE_CURSOR_ARROW = 0,
    /** Text insertion beam (I-beam). */
    SPF_MOUSE_CURSOR_TEXT_INPUT,
    /** Resize crosshair pointing in four directions. */
    SPF_MOUSE_CURSOR_RESIZE_ALL,
    /** Vertical resize pointer (Up-Down). */
    SPF_MOUSE_CURSOR_RESIZE_NS,
    /** Horizontal resize pointer (Left-Right). */
    SPF_MOUSE_CURSOR_RESIZE_EW,
    /** Diagonal resize pointer (North-East / South-West). */
    SPF_MOUSE_CURSOR_RESIZE_NESW,
    /** Diagonal resize pointer (North-West / South-East). */
    SPF_MOUSE_CURSOR_RESIZE_NWSE,
    /** Hand pointer, typically used for clickable links. */
    SPF_MOUSE_CURSOR_HAND,
    /** "Not allowed" or "Operation blocked" pointer. */
    SPF_MOUSE_CURSOR_NOT_ALLOWED,
    /** Total number of defined cursor shapes. */
    SPF_MOUSE_CURSOR_COUNT
} SPF_MouseCursor;

/**
 * @enum SPF_DrawFlags
 * @brief Flags for fine-tuning low-level drawing primitives (UI_DrawList).
 * @details These flags control shape closure and which specific corners are rounded.
 */
typedef enum
{
    /** Default behavior. */
    SPF_DRAW_FLAG_NONE = 0,
    /** Path will be automatically closed (line drawn from last point to first). */
    SPF_DRAW_FLAG_CLOSED = 1 << 0,
    /** Only round the top-left corner. */
    SPF_DRAW_FLAG_ROUND_CORNERS_TOP_LEFT = 1 << 4,
    /** Only round the top-right corner. */
    SPF_DRAW_FLAG_ROUND_CORNERS_TOP_RIGHT = 1 << 5,
    /** Only round the bottom-left corner. */
    SPF_DRAW_FLAG_ROUND_CORNERS_BOTTOM_LEFT = 1 << 6,
    /** Only round the bottom-right corner. */
    SPF_DRAW_FLAG_ROUND_CORNERS_BOTTOM_RIGHT = 1 << 7,
    /** Do not apply any rounding (force sharp 90-degree corners). */
    SPF_DRAW_FLAG_ROUND_CORNERS_NONE = 1 << 8,
    /** Round both top corners. */
    SPF_DRAW_FLAG_ROUND_CORNERS_TOP = SPF_DRAW_FLAG_ROUND_CORNERS_TOP_LEFT | SPF_DRAW_FLAG_ROUND_CORNERS_TOP_RIGHT,
    /** Round both bottom corners. */
    SPF_DRAW_FLAG_ROUND_CORNERS_BOTTOM = SPF_DRAW_FLAG_ROUND_CORNERS_BOTTOM_LEFT | SPF_DRAW_FLAG_ROUND_CORNERS_BOTTOM_RIGHT,
    /** Round both left corners. */
    SPF_DRAW_FLAG_ROUND_CORNERS_LEFT = SPF_DRAW_FLAG_ROUND_CORNERS_TOP_LEFT | SPF_DRAW_FLAG_ROUND_CORNERS_BOTTOM_LEFT,
    /** Round both right corners. */
    SPF_DRAW_FLAG_ROUND_CORNERS_RIGHT = SPF_DRAW_FLAG_ROUND_CORNERS_TOP_RIGHT | SPF_DRAW_FLAG_ROUND_CORNERS_BOTTOM_RIGHT,
    /** Round all four corners. This is the default for most rounded rectangle functions. */
    SPF_DRAW_FLAG_ROUND_CORNERS_ALL = SPF_DRAW_FLAG_ROUND_CORNERS_TOP | SPF_DRAW_FLAG_ROUND_CORNERS_BOTTOM
} SPF_DrawFlags;

/**
 * @enum SPF_NotificationType
 * @brief Visual categories for framework-level notifications.
 * @details Each type determines the default background color and icon used in UI_ShowNotification.
 */
typedef enum {
    /** General information or neutral status update. Standard blue styling. */
    SPF_NOTIFICATION_INFO,
    /** Success confirmation. Standard green styling. */
    SPF_NOTIFICATION_SUCCESS,
    /** Important warning that does not halt the process. Standard yellow styling. */
    SPF_NOTIFICATION_WARNING,
    /** Error notification indicating a failed operation. Standard red styling. */
    SPF_NOTIFICATION_ERROR,
    /** Critical system failure requiring immediate attention. Deep red styling. */
    SPF_NOTIFICATION_CRITICAL,
    /** Helpful hint or suggestion for the user. Purple/Violet styling. */
    SPF_NOTIFICATION_HINT,
    /** Custom notification with no default icon or color. Requires custom parameters. */
    SPF_NOTIFICATION_CUSTOM
} SPF_NotificationType;

/**
 * @enum SPF_Notification_DisplayMode
 * @brief Defines the visual positioning and stacking behavior of framework notifications.
 */
typedef enum {
    /** Shows the notification at the top center of the screen. Newer notifications replace older ones in this slot. */
    SPF_NOTIF_MODE_TOP,
    /** Shows the notification at the bottom-right. Multiple notifications will stack upwards. */
    SPF_NOTIF_MODE_STACK,
    /** Appears near the current mouse cursor position and remains until manually dismissed or a timeout occurs. */
    SPF_NOTIF_MODE_STICKY
} SPF_Notification_DisplayMode;

/**
 * @typedef SPF_Notification_Handle
 * @brief An opaque handle to a specific notification instance.
 * @details Use this handle with UI_HideNotification() to programmatically close a notification.
 */
typedef void* SPF_Notification_Handle;

/**
 * @struct SPF_Notification_Params
 * @brief Extended parameters for framework notifications with control over styling and duration.
 * @details This structure allows for overriding default colors and icons, and defining 
 *          the precise lifetime behavior (automatic, programmatic, or timed).
 */
typedef struct {
    /** The notification type (influences default icon and color). Use SPF_NOTIFICATION_CUSTOM for a blank slate. */
    SPF_NotificationType type;
    /** The message text to display. Supports Markdown. */
    const char* message;
    /** Where to show the notification. */
    SPF_Notification_DisplayMode mode;
    /** 
     * Notification lifetime in seconds:
     * - Negative value (e.g. -1.0f): Automatic duration based on framework settings (Recommended/Default).
     * - Zero (0.0f): Programmatic (infinite duration until manually hidden by the plugin).
     * - Positive value: Exact time in seconds to show the notification.
     */
    float duration;
    /** 
     * Custom background color (RGBA). Set all components to 0.0f to use the default for 'type'.
     * Components are in 0.0f - 1.0f range.
     */
    float r, g, b, a;
    /** Custom FontAwesome icon string (e.g. FA_GEAR). Set to NULL to use the default for 'type'. */
    const char* custom_icon;
} SPF_Notification_Params;

/**
 * @enum SPF_TransitionType
 * @brief Enumeration of cinematic screen transition effects.
 * @details These are used by the framework to create professional-grade visual flow between different states or scenes.
 */
typedef enum {
    /** A simple linear opacity fade in or out. */
    SPF_TRANS_FADE,
    /** A cross-fade effect that transitions from 0% to 100% and back to 0% opacity automatically. */
    SPF_TRANS_CROSS,
    /** A bright 'flash' entry that slowly fades away, mimicking a camera flash or explosion. */
    SPF_TRANS_FLASH,
    /** Cinematic black bars (letterboxing) that slide in from the top and bottom. */
    SPF_TRANS_LETTERBOX,
    /** A horizontal wipe effect moving from the right edge to the left. */
    SPF_TRANS_WIPE_LEFT,
    /** A horizontal wipe effect moving from the left edge to the right. */
    SPF_TRANS_WIPE_RIGHT,
    /** A vertical wipe effect moving from the bottom edge to the top. */
    SPF_TRANS_WIPE_TOP,
    /** A vertical wipe effect moving from the top edge to the bottom. */
    SPF_TRANS_WIPE_BOTTOM,
    /** Two horizontal panels closing towards the center line like a camera shutter. */
    SPF_TRANS_SHUTTER_H,
    /** Two vertical panels closing towards the center line like a camera shutter. */
    SPF_TRANS_SHUTTER_V,
    /** A circular mask that expands from or shrinks towards the center of the screen. */
    SPF_TRANS_RADIAL
} SPF_TransitionType;

/**
 * @enum SPF_TransitionColor
 * @brief Defines preset color palettes for cinematic transitions.
 */
typedef enum {
    /** Opaque solid black. The default for most cinematic fades. */
    SPF_TRANS_COLOR_BLACK,
    /** Pure brilliant white. Ideal for 'flash' effects or dream sequences. */
    SPF_TRANS_COLOR_WHITE,
    /** A warm brownish-orange tint, mimicking old film stock or nostalgia. */
    SPF_TRANS_COLOR_SEPIA,
    /** A neutral, mid-tone gray. */
    SPF_TRANS_COLOR_GRAY
} SPF_TransitionColor;

/**
 * @enum SPF_Font
 * @brief Available pre-configured font styles provided by the framework.
 * @details Plugins should use these to maintain visual consistency with the host application.
 */
typedef enum {
    /** Standard proportional sans-serif font for UI text. */
    SPF_FONT_REGULAR,
    /** Bold version of the standard UI font. */
    SPF_FONT_BOLD,
    /** Italic version of the standard UI font. */
    SPF_FONT_ITALIC,
    /** Bold and italic version of the standard UI font. */
    SPF_FONT_BOLD_ITALIC,
    /** Slightly heavier weight font for sub-headers or emphasis. */
    SPF_FONT_MEDIUM,
    /** Italic version of the medium weight font. */
    SPF_FONT_MEDIUM_ITALIC,
    /** Fixed-width font, ideal for code, data tables, or terminal-like displays. */
    SPF_FONT_MONOSPACE,
    /** Large header font (e.g., Level 1 Header). */
    SPF_FONT_H1,
    /** Medium header font (e.g., Level 2 Header). */
    SPF_FONT_H2,
    /** Small header font (e.g., Level 3 Header). */
    SPF_FONT_H3
} SPF_Font;

/**
 * @enum SPF_TextAlign
 * @brief Horizontal text alignment options.
 */
typedef enum {
    /** Align text to the left boundary of the available region (Default). */
    SPF_TEXT_ALIGN_LEFT,
    /** Center the text horizontally within the available region. */
    SPF_TEXT_ALIGN_CENTER,
    /** Align text to the right boundary of the available region. */
    SPF_TEXT_ALIGN_RIGHT
} SPF_TextAlign;


/**
 * @struct SPF_TableColumnSortSpecs
 * @brief Sorting specification for a single column.
 */
typedef struct
{
    /** Unique ID of the column (defined in UI_TableSetupColumn). */
    uint32_t ColumnUserID;
    /** Index of the column within the table. */
    int16_t  ColumnIndex;
    /** Index within the total sort order (0 for primary, 1 for secondary, etc.). */
    int16_t  SortOrder;
    /** Desired sort direction (Ascending/Descending). */
    SPF_SortDirection SortDirection;
} SPF_TableColumnSortSpecs;

/**
 * @struct SPF_TableSortSpecs
 * @brief Full sorting specification for a table (can include multiple columns).
 */
typedef struct
{
    /** Array of column sort specifications. */
    const SPF_TableColumnSortSpecs* Specs;
    /** Number of columns involved in sorting. */
    int SpecsCount;
    /** True if the specifications have changed since the last query. */
    bool SpecsDirty;
} SPF_TableSortSpecs;

/**
 * @struct SPF_ListClipper
 * @brief Helper to restrict item drawing to the visible area only.
 * @details Essential for performance when rendering very large lists (thousands of items).
 */
typedef struct
{
    /** Number of items in the list. */
    int ItemsCount;
    /** Height of each item in pixels. Use -1.0f to auto-calculate from first item. */
    float ItemsHeight;
    /** [Output] Index of the first visible item. */
    int DisplayStart;
    /** [Output] Index of the last visible item (exclusive). */
    int DisplayEnd;
    /** [Internal] Cursor position at the time of Begin(). */
    float StartPosY;
    /** Internal state (managed by the framework). */
    void* InternalData;
} SPF_ListClipper;

/**
 * @brief A callback function that a plugin provides to draw the content of its window.
 * @param builder A pointer to the UI builder API, used to construct widgets.
 * @param user_data A pointer to user-defined data, passed during registration.
 */
typedef void (*SPF_DrawCallback)(struct SPF_UI_API* builder, void* user_data);

/**
 * @struct SPF_UI_API
 * @brief C-style API for creating and managing plugin UI windows using an immediate-mode paradigm.
 *
 * @details This API provides a stable C interface to the framework's underlying
 *          ImGui-based rendering engine. It allows plugins to register windows,
 *          define their content using simple widget calls, and control their behavior.
 *          The function pointers within this struct map directly to ImGui functions.
 * 
 *          The window title is handled automatically by the framework using the localization
 *          key `windowId.title` (e.g., "MainWindow.title"). If no translation is specified,
 *          the windowId itself will be used as the title.
 *
 * @section Workflow
 * 1.  **Declare in Manifest**: First, declare all your windows in the `ui` section
 *     of your plugin's manifest (`GetManifestData`). This tells the framework
 *     what windows your plugin has, their default visibility, size, etc.
 * 2.  **Implement a Draw Callback**: For each window, create a C-style function in your
 *     plugin that will be responsible for drawing its content. This function must
 *     match the `SPF_DrawCallback` signature.
      * 3.  **Register the Callback**: Implement the `OnRegisterUI` lifecycle function in your
      *     plugin. The framework will call this function once. Inside it, call
      *     `UI_RegisterDrawCallback` for each window, linking the window ID from the
      *     manifest to your corresponding draw callback function.
      * 4.  **Draw Widgets**: Inside your draw callback, use the provided `SPF_UI_API*`
      *      pointer to call widget functions (`Text`, `Button`, etc.) to build your UI.
 *     This is done every frame the window is visible.
 */
typedef struct SPF_UI_API {
    
    // ============================================================================================
    // I. PLUGIN REGISTRATION & WINDOW LIFECYCLE (Framework Specific)
    // ============================================================================================

    /**
     * @brief Registers a draw callback for a window declared in the plugin's manifest.
     * @details This function links a window ID (defined in manifest) to a render function.
     *          Should be called during the 'OnRegisterUI' lifecycle event.
     * @param pluginName Name of the plugin owning this window.
     * @param windowId Unique identifier for the window from the manifest.
     * @param drawCallback Function pointer called every frame to render content.
     * @param user_data Optional user pointer passed back to the callback.
     */
    void (*UI_RegisterDrawCallback)(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data);

    /**
     * @brief Advanced version of RegisterDrawCallback with initial configuration flags.
     * @details Allows setting persistent behavior flags (e.g., NoTitleBar) during registration.
     * @param flags Bitmask of SPF_WindowFlags.
     */
    void (*UI_RegisterDrawCallbackWithFlags)(const char* pluginName, const char* windowId, SPF_DrawCallback drawCallback, void* user_data, SPF_WindowFlags flags);

    /**
     * @brief Retrieves a handle to a registered window for programmatic control.
     * @return Opaque window handle, or NULL if the window is not found or registered.
     */
    SPF_Window_Handle* (*UI_GetWindowHandle)(const char* pluginName, const char* windowId);

    /**
     * @brief Sets the visibility of a framework-managed window.
     * @param handle Window handle obtained from UI_GetWindowHandle.
     * @param isVisible True to show, false to hide.
     */
    void (*UI_SetVisibility)(SPF_Window_Handle* handle, bool isVisible);

    /**
     * @brief Gets the current visibility state of a window.
     * @return True if the window is currently visible.
     */
    bool (*UI_IsVisible)(SPF_Window_Handle* handle);

    /**
     * @brief Focuses a specific framework-managed window.
     * @param handle Window handle.
     */
    void (*UI_SetFocus)(SPF_Window_Handle* handle);




    // ============================================================================================
    // II. MAIN LOOP, CONTEXT & IO
    // ============================================================================================

    /**
     * @brief Retrieves the current ImGui context pointer.
     * @details Useful for plugins that need to share the context with external libraries or backends.
     * @return void* Pointer to the internal ImGuiContext.
     */
    void* (*UI_GetCurrentContext)();

    /**
     * @brief Sets the current active ImGui context.
     * @param ctx Pointer to the context to activate.
     */
    void (*UI_SetCurrentContext)(void* ctx);

    /**
     * @brief Configures custom memory allocation functions for the UI system.
     * @param alloc_func Function pointer for allocation (like malloc).
     * @param free_func Function pointer for deallocation (like free).
     * @param user_data Optional pointer passed to the allocation functions.
     */
    void (*UI_SetAllocatorFunctions)(void* (*alloc_func)(size_t sz, void* user_data), void (*free_func)(void* ptr, void* user_data), void* user_data);

    /**
     * @brief Allocates a block of memory using the UI system's active allocator.
     * @param sz Size in bytes.
     * @return void* Pointer to the allocated memory.
     */
    void* (*UI_MemAlloc)(size_t sz);

    /**
     * @brief Frees memory previously allocated via UI_MemAlloc.
     * @param ptr Pointer to the memory block.
     */
    void (*UI_MemFree)(void* ptr);

    /**
     * @brief Gets the time elapsed since the last frame in seconds.
     * @return Delta time value (e.g., 0.016f for 60 FPS).
     */
    float (*UI_GetDeltaTime)();

    /**
     * @brief Gets the total time elapsed since the start of the application in seconds.
     * @return Running time in seconds.
     */
    double (*UI_GetTime)();

    /**
     * @brief Gets the current frame count since the start of the application.
     */
    int (*UI_GetFrameCount)();

    /**
     * @brief Gets the average frame rate (FPS) over the last few seconds.
     */
    float (*UI_GetFramerate)();

    /**
     * @brief Retrieves the current configuration flags from the IO system.
     * @details Bitmask containing settings like ConfigFlags_DockingEnable.
     */
    int (*UI_GetIO_ConfigFlags)();

    /**
     * @brief Retrieves the backend capability flags.
     */
    int (*UI_GetIO_BackendFlags)();

    // --- Mouse Input ---

    /**
     * @brief Gets the current position of the mouse cursor in screen coordinates.
     * @param[out] out_x Pointer to store the x-coordinate.
     * @param[out] out_y Pointer to store the y-coordinate.
     */
    void (*UI_GetMousePos)(float* out_x, float* out_y);

    /**
     * @brief Checks if a specific mouse button is currently held down.
     * @param button The button to check (from SPF_MouseButton).
     * @return True if the button is held down.
     */
    bool (*UI_IsMouseDown)(SPF_MouseButton button);

    /**
     * @brief Checks if a mouse button was clicked (pressed and released) in the current frame.
     * @param button The button index.
     * @return True if the button was clicked this frame.
     */
    bool (*UI_IsMouseClicked)(SPF_MouseButton button);

    /**
     * @brief Checks if a mouse button was released in the current frame.
     */
    bool (*UI_IsMouseReleased)(SPF_MouseButton button);

    /**
     * @brief Checks if a mouse button was double-clicked.
     */
    bool (*UI_IsMouseDoubleClicked)(SPF_MouseButton button);

    /**
     * @brief Checks if the mouse is currently hovering over a specific rectangular area.
     * @details This function is independent of the current window context unless 'clip' is enabled.
     * @param min_x, min_y Top-left corner of the target rectangle in screen coordinates.
     * @param max_x, max_y Bottom-right corner.
     * @param clip If true, the hit-test is clipped by the current window's boundaries.
     * @return bool True if the mouse cursor is within the specified area.
     */
    bool (*UI_IsMouseHoveringRect)(float min_x, float min_y, float max_x, float max_y, bool clip);

    /**
     * @brief Checks if the current mouse position data is valid.
     * @details Returns false if the mouse is outside the application window or the platform backend hasn't provided coordinates yet.
     */
    bool (*UI_IsMousePosValid)();

    /**
     * @brief Retrieves the mouse position at the exact moment the current popup was opened.
     * @param[out] out_x, out_y Pointers to store the coordinates.
     */
    void (*UI_GetMousePosOnOpeningCurrentPopup)(float* out_x, float* out_y);

    /**
     * @brief Gets the vertical scroll amount of the mouse wheel for the current frame.
     * @return Positive for up, negative for down, 0 if no scroll.
     */
    float (*UI_GetMouseWheel)();

    /**
     * @brief Gets the horizontal scroll amount of the mouse wheel.
     */
    float (*UI_GetMouseWheelH)();

    // --- Keyboard & Shortcut System ---

    /**
     * @brief Checks if a specific key or key chord (key + modifiers) is pressed using the global shortcut system.
     * @details This is the modern way to handle hotkeys (e.g., Ctrl+S). 
     *          Participates in the priority-based routing system.
     * @param key_chord Combined key and modifier flags (e.g., SPF_MOD_CTRL | SPF_KEY_S).
     * @param flags Configuration flags from SPF_InputFlags.
     * @return bool True if the shortcut was triggered this frame.
     */
    bool (*UI_Shortcut)(int key_chord, SPF_InputFlags flags);

    /**
     * @brief Assigns a shortcut to the next item to be created (e.g., a menu item or button).
     * @details The shortcut will be displayed next to the item label and automatically handled by the system.
     * @param key_chord The key combination (e.g., SPF_MOD_CTRL | SPF_KEY_N).
     * @param flags Configuration flags from SPF_InputFlags.
     */
    void (*UI_SetNextItemShortcut)(int key_chord, SPF_InputFlags flags);

    /**
     * @brief Sets the key owner to the last item ID if it is hovered or active.
     * @details Allows a widget to exclusively "own" a key, preventing other systems from processing it.
     * @param key The key to take ownership of (from SPF_Key).
     */
    void (*UI_SetItemKeyOwner)(SPF_Key key);

    /**
     * @brief Checks if the user is currently dragging the mouse with a button held down.
     * @param button The button index (from SPF_MouseButton).
     * @return bool True if dragging is in progress.
     */
    bool (*UI_IsMouseDragging)(SPF_MouseButton button);

    /**
     * @brief Gets the displacement of the mouse since the drag operation started.
     * @param button The mouse button index.
     * @param[out] out_dx Pointer to store horizontal displacement.
     * @param[out] out_dy Pointer to store vertical displacement.
     */
    void (*UI_GetMouseDragDelta)(SPF_MouseButton button, float* out_dx, float* out_dy);

    /**
     * @brief Resets the mouse drag delta for a specific button.
     * @param button The button index to reset.
     */
    void (*UI_ResetMouseDragDelta)(SPF_MouseButton button);

    /**
     * @brief Sets the system mouse cursor shape.
     * @param cursor The desired cursor shape from SPF_MouseCursor.
     */
    void (*UI_SetMouseCursor)(SPF_MouseCursor cursor);

    /**
     * @brief Gets the current system mouse cursor shape.
     * @return SPF_MouseCursor The active cursor type.
     */
    SPF_MouseCursor (*UI_GetMouseCursor)();

    // --- Keyboard Input ---

    /**
     * @brief Checks if a specific key is currently held down.
     * @param key_index The virtual key code (matching SPF_Key constants).
     * @return bool True if the key is held down.
     */
    bool (*UI_IsKeyDown)(int key_index);

    /**
     * @brief Checks if a specific key was pressed in the current frame.
     * @param key_index The virtual key code.
     * @return bool True if pressed.
     */
    bool (*UI_IsKeyPressed)(int key_index);

    /**
     * @brief Checks if a specific key was released in the current frame.
     * @param key_index The virtual key code.
     * @return bool True if released.
     */
    bool (*UI_IsKeyReleased)(int key_index);

    /**
     * @brief Returns the number of times a key has been pressed this frame (including repeats).
     * @param key_index The virtual key code.
     * @param repeat_delay Delay before the first repeat trigger (in seconds).
     * @param rate Frequency of repeats (in seconds).
     * @return int Number of trigger events.
     */
    int (*UI_GetKeyPressedAmount)(int key_index, float repeat_delay, float rate);

    /**
     * @brief Checks if a mouse button was released with a specific delay.
     * @details Useful for handling long clicks (like renaming in Windows Explorer).
     * @param button Mouse button index.
     * @param delay Delay threshold in seconds.
     * @return bool True if released after the delay.
     */
    bool (*UI_IsMouseReleasedWithDelay)(SPF_MouseButton button, float delay);

    /**
     * @brief Gets the human-readable name of a key.
     * @param key_index The virtual key code.
     * @return const char* Pointer to a static string containing the key name.
     */
    const char* (*UI_GetKeyName)(int key_index);

    // --- Clipboard ---

    /**
     * @brief Retrieves the current content of the system clipboard.
     * @return Read-only string containing clipboard text.
     */
    const char* (*UI_GetClipboardText)();

    /**
     * @brief Sets the system clipboard text.
     * @param text The string to copy to the clipboard.
     */
    void (*UI_SetClipboardText)(const char* text);


    // ============================================================================================
    // III. WINDOWS, LAYOUT & POSITIONING
    // ============================================================================================

    // --- Window State Queries ---

    /**
     * @brief Checks if the current window is appearing for the first time.
     */
    bool (*UI_IsWindowAppearing)();

    /**
     * @brief Checks if the current window is collapsed (minimized).
     */
    bool (*UI_IsWindowCollapsed)();

    /**
     * @brief Checks if the current window is focused.
     * @param flags Optional focus flags (from SPF_FocusedFlags).
     */
    bool (*UI_IsWindowFocused)(SPF_FocusedFlags flags);

    /**
     * @brief Checks if the current window is hovered by the mouse.
     * @param flags Optional hover flags (from SPF_HoveredFlags).
     */
    bool (*UI_IsWindowHovered)(SPF_HoveredFlags flags);

    /**
     * @brief Retrieves the internal state storage for the current window.
     * @return SPF_Storage_Handle Handle to the storage object.
     */
    SPF_Storage_Handle (*UI_GetStateStorage)();

    /**
     * @brief Replaces the internal state storage for the current window.
     * @param storage Handle to the new storage object.
     */
    void (*UI_SetStateStorage)(SPF_Storage_Handle storage);

    /**
     * @brief Retrieves the viewport currently containing this window.
     * @return void* Pointer to the internal ImGuiViewport structure.
     */
    void* (*UI_GetWindowViewport)();

    /**
     * @brief Gets the handle to the low-level draw list for the current window.
     * @details Used for custom vector drawing (lines, circles, etc.).
     */
    SPF_DrawList_Handle (*UI_GetWindowDrawList)();

    /**
     * @brief Gets the draw list for the entire screen background.
     * @details Items drawn here appear behind all other windows.
     */
    SPF_DrawList_Handle (*UI_GetBackgroundDrawList)();

    /**
     * @brief Gets the draw list for the entire screen foreground.
     * @details Items drawn here appear on top of all windows (e.g., custom cursors).
     */
    SPF_DrawList_Handle (*UI_GetForegroundDrawList)();

    // --- Window Manipulation ---

    /**
     * @brief Gets the current window's screen position.
     */
    void (*UI_GetWindowPos)(float* out_x, float* out_y);

    /**
     * @brief Gets the current window's size (including title bar and scrollbars).
     */
    void (*UI_GetWindowSize)(float* out_x, float* out_y);

    /**
     * @brief Gets the width of the current window.
     * @return float Total width in pixels.
     */
    float (*UI_GetWindowWidth)();

    /**
     * @brief Gets the height of the current window.
     * @return float Total height in pixels.
     */
    float (*UI_GetWindowHeight)();

    /**
     * @brief Gets the DPI scale of the current window.
     * @return float Scale factor (e.g., 1.0f for 96 DPI, 2.0f for 192 DPI).
     */
    float (*UI_GetWindowDpiScale)();

    /**
     * @brief Sets the position of the current window.
     * @param x, y New coordinates.
     * @param cond Execution condition (from SPF_Cond).
     */
    void (*UI_SetWindowPos)(float x, float y, SPF_Cond cond);

    /**
     * @brief Sets the size of the current window.
     * @param x, y New dimensions.
     * @param cond Execution condition.
     */
    void (*UI_SetWindowSize)(float x, float y, SPF_Cond cond);

    /**
     * @brief Sets the position for the next window to be created.
     * @param x, y The new screen coordinates.
     * @param cond Condition (from SPF_Cond) when this should be applied.
     * @param pivot_x, pivot_y Pivot point (0.0f = top-left, 0.5f = center, 1.0f = bottom-right).
     */
    void (*UI_SetNextWindowPos)(float x, float y, SPF_Cond cond, float pivot_x, float pivot_y);

    /**
     * @brief Sets the size for the next window to be created.
     * @param x, y The new dimensions in pixels.
     * @param cond Condition (from SPF_Cond) when this should be applied.
     */
    void (*UI_SetNextWindowSize)(float x, float y, SPF_Cond cond);

    /**
     * @brief Sets the viewport the next window should belong to (for multi-viewport support).
     * @param viewport_id The ID of the target viewport.
     */
    void (*UI_SetNextWindowViewport)(uint32_t viewport_id);

    /**
     * @brief Programmatically scrolls the next window to a specific position.
     * @param scroll_x Horizontal scroll position.
     * @param scroll_y Vertical scroll position.
     */
    void (*UI_SetNextWindowScroll)(float scroll_x, float scroll_y);

    /**
     * @brief Gets the current horizontal scroll position of the window.
     * @return float Scroll position in pixels.
     */
    float (*UI_GetScrollX)();

    /**
     * @brief Gets the current vertical scroll position of the window.
     * @return float Scroll position in pixels.
     */
    float (*UI_GetScrollY)();

    /**
     * @brief Gets the maximum horizontal scroll position (width of content minus window width).
     */
    float (*UI_GetScrollMaxX)();

    /**
     * @brief Gets the maximum vertical scroll position.
     */
    float (*UI_GetScrollMaxY)();

    /**
     * @brief Sets the horizontal scroll position of the current window.
     */
    void (*UI_SetScrollX)(float scroll_x);

    /**
     * @brief Sets the vertical scroll position of the current window.
     */
    void (*UI_SetScrollY)(float scroll_y);

    /**
     * @brief Adjusts scroll position to make the current cursor position visible.
     * @param center_y_ratio 0.0f = top, 0.5f = center, 1.0f = bottom.
     */
    void (*UI_SetScrollHereY)(float center_y_ratio);

    /**
     * @brief Forces focus onto the next window to be created.
     */
    void (*UI_SetNextWindowFocus)();

    /**
     * @brief Programmatically collapses or expands the next window.
     */
    void (*UI_SetNextWindowCollapsed)(bool collapsed, SPF_Cond cond);

    /**
     * @brief Sets the collapsed state of the current window.
     */
    void (*UI_SetWindowCollapsed)(bool collapsed, SPF_Cond cond);

    /**
     * @brief Sets the focus to the current window.
     */
    void (*UI_SetWindowFocus)();

    /**
     * @brief Low-level: Sets the focus to a specific window by its internal ImGui context.
     * @param window Pointer to the internal ImGuiWindow structure (cast to void*).
     */
    void (*UI_FocusWindow)(void* window);

    /**
     * @brief Brings the specified window to the front of the focus stack and the display stack.
     */
    void (*UI_BringWindowToFocusFront)(void* window);

    /**
     * @brief Sets the font scale for the current window.
     * @param scale The new font scale. 1.0f is default, >1.0f makes text larger.
     */
    void (*UI_SetWindowFontScale)(float scale);

    /**
     * @brief Sets size constraints for the next window to be created.
     * @details Use this to enforce minimum and maximum dimensions for a window. 
     *          Usually called before UI_Begin() or similar framework-managed window logic.
     * @param min_x Minimum allowed width.
     * @param min_y Minimum allowed height.
     * @param max_x Maximum allowed width.
     * @param max_y Maximum allowed height.
     */
    void (*UI_SetNextWindowSizeConstraints)(float min_x, float min_y, float max_x, float max_y);

    /**
     * @brief Sets the background transparency for the next window.
     * @param alpha Transparency value from 0.0f (fully transparent) to 1.0f (fully opaque).
     */
    void (*UI_SetNextWindowBgAlpha)(float alpha);

    /**
     * @brief Sets the content region size for the next window.
     * @details This allows you to specify the size of the inner area where widgets are placed, 
     *          excluding window decorations like title bars or scrollbars.
     * @param size_x Desired content width.
     * @param size_y Desired content height.
     */
    void (*UI_SetNextWindowContentSize)(float size_x, float size_y);

    /**
     * @brief Brings the current window to the front of the display stack.
     * @details Ensures the window is rendered on top of all other windows in its layer.
     */
    void (*UI_BringWindowToDisplayFront)();

    /**
     * @brief Moves the current window to the back of the display stack.
     * @details Useful for background-style windows or dashboards.
     */
    void (*UI_BringWindowToDisplayBack)();

    /**
     * @brief Retrieves the absolute screen position of the main viewport.
     * @param[out] out_x Pointer to store the x-coordinate of the viewport's top-left corner.
     * @param[out] out_y Pointer to store the y-coordinate.
     */
    void (*UI_GetMainViewportPos)(float* out_x, float* out_y);

    /**
     * @brief Retrieves the total size of the main viewport in screen coordinates.
     * @param[out] out_x Pointer to store the width of the viewport.
     * @param[out] out_y Pointer to store the height of the viewport.
     */
    void (*UI_GetMainViewportSize)(float* out_x, float* out_y);

    // --- Child Windows ---

    /**
     * @brief Begins a self-contained scrolling region within the current window.
     * @details Must be matched with UI_EndChild().
     * @param str_id Unique ID for the child region.
     * @param size_x, size_y Size of the region. Use 0.0f to fill available space.
     * @param border If true, draws a visible border around the child.
     * @param flags Bitmask of SPF_WindowFlags.
     * @return True if the child region is visible and content should be drawn.
     */
    bool (*UI_BeginChild)(const char* str_id, float size_x, float size_y, bool border, SPF_WindowFlags flags);

    /**
     * @brief Ends the current child window.
     */
    void (*UI_EndChild)();

    // --- Content Regions & Cursor ---

    /**
     * @brief Gets the remaining available space in the current content region.
     * @details Measured from current cursor position to bottom-right edge of the region.
     */
    void (*UI_GetContentRegionAvail)(float* out_x, float* out_y);

    /**
     * @brief Gets the maximum boundaries of the current content region.
     */
    void (*UI_GetContentRegionMax)(float* out_x, float* out_y);

    /**
     * @brief Gets the top-left coordinate of the content region relative to window origin.
     */
    void (*UI_GetWindowContentRegionMin)(float* out_x, float* out_y);

    /**
     * @brief Gets the bottom-right coordinate of the content region.
     */
    void (*UI_GetWindowContentRegionMax)(float* out_x, float* out_y);

    /**
     * @brief Gets the current cursor position relative to the window origin.
     */
    void (*UI_GetCursorPos)(float* out_x, float* out_y);

    /** @brief Gets the current cursor X position relative to the window. */
    float (*UI_GetCursorPosX)();
    /** @brief Gets the current cursor Y position relative to the window. */
    float (*UI_GetCursorPosY)();

    /**
     * @brief Sets the cursor position relative to the window origin.
     */
    void (*UI_SetCursorPos)(float x, float y);

    /** @brief Sets the cursor X position relative to the window. */
    void (*UI_SetCursorPosX)(float x);
    /** @brief Sets the cursor Y position relative to the window. */
    void (*UI_SetCursorPosY)(float y);

    /**
     * @brief Gets the current cursor position in absolute screen coordinates.
     */
    void (*UI_GetCursorScreenPos)(float* out_x, float* out_y);

    /**
     * @brief Sets the cursor position in absolute screen coordinates.
     */
    void (*UI_SetCursorScreenPos)(float x, float y);

    /**
     * @brief Gets the start position of the content region (after window padding).
     */
    void (*UI_GetCursorStartPos)(float* out_x, float* out_y);

    // --- Layout Helpers ---

    /**
     * @brief Adds a horizontal line to separate content.
     */
    void (*UI_Separator)();

    /**
     * @brief Adds a horizontal line with a text label in the middle.
     * @details Automatically manages spacing around the separator.
     * @param label Text to display inside the separator.
     */
    void (*UI_SeparatorText)(const char* label);

    /**
     * @brief Aligns the text baseline of the next widget to the standard frame padding.
     * @details Useful for vertical alignment of text with adjacent buttons or inputs.
     */
    void (*UI_AlignTextToFramePadding)();

    /**
     * @brief Places the next widget on the same line as the previous one.
     * @param offset_from_start_x Optional offset from left edge.
     * @param spacing Optional spacing between items (-1.0f for default).
     */
    void (*UI_SameLine)(float offset_from_start_x, float spacing);

    /**
     * @brief Forces the next widget to start on a new line.
     */
    void (*UI_NewLine)();

    /**
     * @brief Adds a blank vertical space of Style.ItemSpacing.y height.
     */
    void (*UI_Spacing)();

    /**
     * @brief Adds an empty non-interactive element of a specific size.
     */
    void (*UI_Dummy)(float size_x, float size_y);

    /**
     * @brief Indents subsequent content to the right.
     * @param indent_w Width of indentation (0.0f for default).
     */
    void (*UI_Indent)(float indent_w);

    /**
     * @brief Cancels indentation, moving content back to the left.
     */
    void (*UI_Unindent)(float indent_w);

    /**
     * @brief Captures multiple widgets into a single logical group.
     * @details Groups are treated as a single item for layout and IsItemHovered checks.
     */
    void (*UI_BeginGroup)();

    /**
     * @brief Ends the current logical group.
     */
    void (*UI_EndGroup)();

    // --- ID Stack ---

    /**
     * @brief Pushes a string ID onto the stack to differentiate widgets in a loop.
     */
    void (*UI_PushID_Str)(const char* str_id);

    /**
     * @brief Pushes an integer ID onto the stack.
     */
    void (*UI_PushID_Int)(int int_id);

    /**
     * @brief Pushes a pointer ID onto the stack.
     */
    void (*UI_PushID_Ptr)(const void* ptr_id);

    /**
     * @brief Pops the top ID from the stack.
     */
    void (*UI_PopID)();

    /**
     * @brief Pushes a specialized item flag onto the stack.
     * @details Item flags control low-level widget behavior (e.g., ImGuiItemFlags_Disabled).
     * @param flags Flag from ImGuiItemFlags_ (internal).
     * @param enabled True to enable the flag, false to disable.
     */
    void (*UI_PushItemFlag)(int flags, bool enabled);

    /**
     * @brief Pops an item flag from the stack.
     */
    void (*UI_PopItemFlag)();

    /**
     * @brief Begins a disabled block where all widgets are non-interactive and grayed out.
     * @param disabled If true, the subsequent widgets will be disabled.
     */
    void (*UI_BeginDisabled)(bool disabled);

    /**
     * @brief Ends the disabled block.
     */
    void (*UI_EndDisabled)();

    /**
     * @brief Gets a unique ID for a string without pushing it onto the ID stack.
     * @param str_id The string to hash into a unique ID.
     * @return uint32_t The generated unique identifier.
     */
    uint32_t (*UI_GetID_Str)(const char* str_id);

    // --- Layout Metrics & Item Sizing ---

    /**
     * @brief Sets the width of the next and subsequent items in the layout.
     * @details Use this to force a specific width for widgets like sliders or input fields. 
     *          Every call MUST be matched with UI_PopItemWidth().
     * @param item_width The desired width in pixels. Use -1.0f to revert to default behavior.
     */
    void (*UI_PushItemWidth)(float item_width);

    /**
     * @brief Restores the previous item width from the stack.
     */
    void (*UI_PopItemWidth)();

    /**
     * @brief Sets the width of ONLY the next item in the layout.
     * @details This is a convenience function that doesn't require a 'Pop' call.
     * @param item_width The desired width in pixels.
     */
    void (*UI_SetNextItemWidth)(float item_width);

    /**
     * @brief Calculates the width of the next item based on current layout rules.
     * @return float The calculated width in pixels.
     */
    float (*UI_CalcItemWidth)();

    /**
     * @brief Sets the wrapping position for text relative to the window origin.
     * @details Every call MUST be matched with UI_PopTextWrapPos().
     * @param wrap_local_pos_x Position in pixels. 0.0f wraps at the window boundary.
     */
    void (*UI_PushTextWrapPos)(float wrap_local_pos_x);

    /**
     * @brief Restores the previous text wrapping position from the stack.
     */
    void (*UI_PopTextWrapPos)();

    /**
     * @brief Retrieves the height of a single line of text with the current font.
     * @return float Height in pixels.
     */
    float (*UI_GetTextLineHeight)();

    /**
     * @brief Retrieves the text line height plus the standard vertical spacing.
     * @return float Height including spacing in pixels.
     */
    float (*UI_GetTextLineHeightWithSpacing)();

    /**
     * @brief Retrieves the standard height of a frame (e.g., a button) in the current style.
     * @return float Height in pixels.
     */
    float (*UI_GetFrameHeight)();

    /**
     * @brief Retrieves the frame height plus the standard vertical spacing.
     * @return float Total height including spacing.
     */
    float (*UI_GetFrameHeightWithSpacing)();


    // ============================================================================================
    // IV. BASIC WIDGETS
    // ============================================================================================

    // --- Text ---

    /**
     * @brief Displays simple text.
     * @details Does not support printf formatting. Use string buffers for dynamic text.
     * @param text The string to display.
     */
    void (*UI_Text)(const char* text);

    /**
     * @brief Displays raw text without any formatting check.
     * @details Fastest way to render text as it ignores '%' characters and doesn't process formatting.
     * @param text The string to display.
     */
    void (*UI_TextUnformatted)(const char* text);

    /**
     * @brief Displays a clickable text link.
     * @param label Text to display.
     * @return bool True if clicked.
     */
    bool (*UI_TextLink)(const char* label);

    /**
     * @brief Displays a link that opens a URL in the system browser when clicked.
     * @param label Text label.
     * @param url The target URL (e.g., "https://google.com").
     */
    void (*UI_TextLinkOpenURL)(const char* label, const char* url);

    /**
     * @brief Displays colored text.
     * @details Does not support printf formatting.
     * @param r, g, b, a Text color components (0.0f - 1.0f).
     * @param text The string to display.
     */
    void (*UI_TextColored)(float r, float g, float b, float a, const char* text);

    /**
     * @brief Displays disabled (grayed out) text.
     * @details Useful for non-interactive or unavailable information.
     * @param text The string to display.
     */
    void (*UI_TextDisabled)(const char* text);

    /**
     * @brief Displays text that automatically wraps at the end of the line.
     * @param text The string to display.
     */
    void (*UI_TextWrapped)(const char* text);

    /**
     * @brief Displays a label followed by a value text.
     * @details Typically used for "Key: Value" displays.
     * @param label The label string (displayed on the left).
     * @param text The value string (displayed on the right).
     */
    void (*UI_LabelText)(const char* label, const char* text);

    /**
     * @brief Displays text with a bullet point.
     * @param text The string to display.
     */
    void (*UI_BulletText)(const char* text);

    // --- Buttons & Interaction ---

    /**
     * @brief Displays a standard button.
     * @param label Text to display on the button.
     * @param width Width of the button (0.0f = auto-size).
     * @param height Height of the button (0.0f = auto-size).
     * @return True if clicked.
     */
    bool (*UI_Button)(const char* label, float width, float height);

    /**
     * @brief Renders a unified framework button with advanced features.
     * 
     * @details This button handles state-dependent text/icon colors automatically:
     *          - Idle:   Uses style color (or framework WHITE default).
     *          - Hover:  Uses style hoverColor (or framework GOLD default).
     *          - Active: Uses style activeColor (or framework DARK background color default).
     * 
     * @param label   Text or icon to display on the button.
     * @param width   Width of the button. Set to 0 for auto-sizing to label.
     * @param height  Height of the button. Set to 0 for auto-sizing to label.
     * @param tooltip Optional tooltip text shown on hover. Set to NULL to disable.
     * @param style   Optional handle to a custom TextStyle. 
     *                - If NULL: Uses DefaultButton() (White/Gold/Dark logic).
     *                - If provided: You can override specific states (e.g., .Color(RED) for a red delete button).
     * @return bool   True if the button was clicked this frame.
     */
    bool (*UI_ButtonEx)(const char* label, float width, float height, const char* tooltip, SPF_TextStyle_Handle style);

    /**
     * @brief Displays a small button.
     * @details Has smaller padding than a regular button. Good for embedding in text.
     * @param label Text to display.
     * @return True if clicked.
     */
    bool (*UI_SmallButton)(const char* label);

    /**
     * @brief Creates an invisible button.
     * @details Useful for creating custom click areas or drag sources without rendering anything.
     * @param str_id Unique ID for the button.
     * @param width Width of the hit area.
     * @param height Height of the hit area.
     * @return True if clicked.
     */
    bool (*UI_InvisibleButton)(const char* str_id, float width, float height);

    /**
     * @brief Displays an arrow button (e.g., for navigation).
     * @param str_id Unique ID.
     * @param dir Direction of the arrow (from SPF_Dir).
     * @return True if clicked.
     */
    bool (*UI_ArrowButton)(const char* str_id, SPF_Dir dir);

    /**
     * @brief Displays a checkbox.
     * @param label Text label next to the checkbox.
     * @param v Pointer to the boolean state.
     * @return True if the value changed.
     */
    bool (*UI_Checkbox)(const char* label, bool* v);

    /**
     * @brief Displays a checkbox that manipulates specific bits within an integer flags variable.
     * @details This is a shortcut for toggling bitmasks.
     * @param label Text label.
     * @param flags Pointer to the integer containing bit flags.
     * @param flags_value The specific bit(s) to toggle (e.g., 1 << 2).
     * @return bool True if the value was modified.
     */
    bool (*UI_CheckboxFlags)(const char* label, int* flags, int flags_value);

    /**
     * @brief Displays a radio button.
     * @details Activates when clicked, ensuring only one in a group is active.
     * @param label Text label.
     * @param active True if currently selected.
     * @return True if clicked.
     */
    bool (*UI_RadioButton)(const char* label, bool active);

    /**
     * @brief Displays a radio button that automatically manages an integer state.
     * @param label Text label.
     * @param v Pointer to the integer state variable.
     * @param v_button The specific value this button represents.
     * @return bool True if selection changed.
     */
    bool (*UI_RadioButtonFlags)(const char* label, int* v, int v_button);

    /**
     * @brief Displays a progress bar.
     * @param fraction Progress from 0.0f to 1.0f.
     * @param width Width of the bar (0.0f = auto).
     * @param height Height of the bar (0.0f = auto).
     * @param overlay Optional text to display over the bar.
     */
    void (*UI_ProgressBar)(float fraction, float width, float height, const char* overlay);

    /**
     * @brief Draws a bullet point (small circle).
     */
    void (*UI_Bullet)();

    // --- Images ---

    /**
     * @brief Displays an image.
     * @details Requires a texture ID from the rendering backend.
     * @param user_texture_id The texture identifier (cast to void*).
     * @param width Display width.
     * @param height Display height.
     */
    void (*UI_Image)(void* user_texture_id, float width, float height);

    /**
     * @brief Displays an image with a specific background color and tint.
     * @details Useful for rendering transparent icons with a custom background.
     * @param user_texture_id The texture identifier.
     * @param width, height Dimensions.
     * @param bg_col Background color (RGBA, 0.0f-1.0f).
     * @param tint_col Tint color applied to the image pixels.
     */
    void (*UI_ImageWithBg)(void* user_texture_id, float width, float height, float bg_col[4], float tint_col[4]);

    /**
     * @brief Displays a clickable image button.
     * @param str_id Unique ID for the button.
     * @param user_texture_id Texture identifier.
     * @param width Button width.
     * @param height Button height.
     * @return True if clicked.
     */
    bool (*UI_ImageButton)(const char* str_id, void* user_texture_id, float width, float height);


    // ============================================================================================
    // V. ADVANCED INPUTS (Combos, Drags, Sliders, Inputs)
    // ============================================================================================

    // --- Combo Box (Dropdowns) ---

    /**
     * @brief Begins a combo box (dropdown selection list).
     * @details If this function returns true, the combo is open and you must draw items 
     *          (usually via UI_Selectable) and call UI_EndCombo() at the end.
     * @param label Text label displayed next to the combo box.
     * @param preview_value The text currently shown inside the combo box (usually the selected item).
     * @param flags Configuration flags from SPF_ComboFlags.
     * @return bool True if the combo box is open and content should be rendered.
     */
    bool (*UI_BeginCombo)(const char* label, const char* preview_value, SPF_ComboFlags flags);

    /**
     * @brief Ends the currently open combo box. 
     * @details Must be called if UI_BeginCombo() returned true.
     */
    void (*UI_EndCombo)();

    /**
     * @brief Simplified combo box for selecting a string from a fixed array.
     * @param label Text label.
     * @param current_item Pointer to the index of the selected item in the array. Will be updated on selection.
     * @param items Pointer to an array of C-strings representing the selectable options.
     * @param items_count The number of strings in the items array.
     * @return bool True if the selection was changed by the user.
     */
    bool (*UI_Combo)(const char* label, int* current_item, const char* const items[], int items_count);

    // --- Drags (Click and drag horizontally to change numeric values) ---

    /**
     * @brief Draggable float widget.
     * @param label Text label displayed next to the widget.
     * @param v Pointer to the float value to be modified.
     * @param v_speed Speed of value change per pixel of mouse movement. Use 1.0f for default speed.
     * @param v_min Lower bound for the value. If v_min == v_max, clamping is disabled.
     * @param v_max Upper bound for the value.
     * @param format Printf-style format string for value display (e.g., "%.3f").
     * @param flags Behavior customization flags from SPF_SliderFlags.
     * @return bool True if the value was modified by the user.
     */
    bool (*UI_DragFloat)(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags);

    /**
     * @brief Draggable 2-component float vector.
     * @param v Pointer to an array of 2 float values.
     */
    bool (*UI_DragFloat2)(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags);
    
    /**
     * @brief Draggable 3-component float vector.
     * @param v Pointer to an array of 3 float values.
     */
    bool (*UI_DragFloat3)(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags);
    
    /**
     * @brief Draggable 4-component float vector.
     * @param v Pointer to an array of 4 float values.
     */
    bool (*UI_DragFloat4)(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, SPF_SliderFlags flags);

    /**
     * @brief Draggable range of two float values (Min and Max).
     * @param v_current_min Pointer to the starting boundary of the range.
     * @param v_current_max Pointer to the ending boundary of the range.
     * @param v_speed Speed of adjustment.
     * @param v_min, v_max Global bounds for both values.
     * @param format Display format for the min value.
     * @param format_max Display format for the max value.
     * @param flags Additional flags.
     * @return bool True if either value was modified.
     */
    bool (*UI_DragFloatRange2)(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, SPF_SliderFlags flags);

    /**
     * @brief Draggable integer widget.
     * @param label Text label.
     * @param v Pointer to the integer value.
     * @param v_speed Adjustment speed.
     * @param v_min, v_max Bounds.
     * @param format Format string (e.g., "%d").
     * @param flags Slider flags.
     * @return bool True if modified.
     */
    bool (*UI_DragInt)(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Draggable 2-component integer vector. */
    bool (*UI_DragInt2)(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags);
    
    /** @brief Draggable 3-component integer vector. */
    bool (*UI_DragInt3)(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags);
    
    /** @brief Draggable 4-component integer vector. */
    bool (*UI_DragInt4)(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Draggable range of two integer values. */
    bool (*UI_DragIntRange2)(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max, SPF_SliderFlags flags);

    /**
     * @brief Generic draggable widget for any numeric data type.
     * @param label Text label.
     * @param data_type The underlying numeric type (from SPF_DataType).
     * @param p_data Pointer to the raw value(s).
     * @param v_speed Speed of change.
     * @param p_min Optional pointer to the minimum value (matching data_type).
     * @param p_max Optional pointer to the maximum value.
     * @param format Formatting string.
     * @param flags Customization flags.
     * @return bool True if modified.
     */
    bool (*UI_DragScalar)(const char* label, SPF_DataType data_type, void* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);

    /**
     * @brief Generic draggable widget for an array of numeric values.
     * @param components Number of components in the p_data array.
     */
    bool (*UI_DragScalarN)(const char* label, SPF_DataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);

    // --- Sliders (Visual range selection) ---

    /**
     * @brief Standard horizontal slider for a single float value.
     * @param label Text label.
     * @param v Pointer to the float value.
     * @param v_min Lower bound.
     * @param v_max Upper bound.
     * @param format Display format string (e.g., "%.3f").
     * @param flags Flags from SPF_SliderFlags.
     * @return bool True if the value was modified.
     */
    bool (*UI_SliderFloat)(const char* label, float* v, float v_min, float v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Horizontal slider for a 2-component float vector. */
    bool (*UI_SliderFloat2)(const char* label, float v[2], float v_min, float v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Horizontal slider for a 3-component float vector. */
    bool (*UI_SliderFloat3)(const char* label, float v[3], float v_min, float v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Horizontal slider for a 4-component float vector. */
    bool (*UI_SliderFloat4)(const char* label, float v[4], float v_min, float v_max, const char* format, SPF_SliderFlags flags);

    /**
     * @brief Specialized slider for editing angles.
     * @details The internally stored 'v_rad' value is in radians, but it is displayed to the user in degrees.
     * @param label Text label.
     * @param v_rad Pointer to the angle value in radians.
     * @param v_degrees_min Minimum display angle in degrees.
     * @param v_degrees_max Maximum display angle in degrees.
     * @param format Format string.
     * @param flags Slider flags.
     * @return bool True if modified.
     */
    bool (*UI_SliderAngle)(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format, SPF_SliderFlags flags);

    /**
     * @brief Standard horizontal slider for a single integer value.
     * @param v Pointer to the integer value.
     * @param v_min, v_max Integer bounds.
     */
    bool (*UI_SliderInt)(const char* label, int* v, int v_min, int v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Horizontal slider for a 2-component integer vector. */
    bool (*UI_SliderInt2)(const char* label, int v[2], int v_min, int v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Horizontal slider for a 3-component integer vector. */
    bool (*UI_SliderInt3)(const char* label, int v[3], int v_min, int v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Horizontal slider for a 4-component integer vector. */
    bool (*UI_SliderInt4)(const char* label, int v[4], int v_min, int v_max, const char* format, SPF_SliderFlags flags);

    /**
     * @brief Generic slider for any numeric data type.
     * @param data_type The numeric type (from SPF_DataType).
     * @param p_data Pointer to the value(s).
     * @param p_min, p_max Pointers to min/max bounds (matching data_type).
     */
    bool (*UI_SliderScalar)(const char* label, SPF_DataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);
    
    /** @brief Generic slider for an array of numeric values. */
    bool (*UI_SliderScalarN)(const char* label, SPF_DataType data_type, void* p_data, int components, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);

    /**
     * @brief Vertical slider for a single float value.
     * @param label Text label.
     * @param size_x, size_y Dimensions of the slider widget.
     * @param v Pointer to the value.
     * @param v_min, v_max Bounds.
     * @param format Formatting string.
     * @param flags Slider flags.
     * @return bool True if modified.
     */
    bool (*UI_VSliderFloat)(const char* label, float size_x, float size_y, float* v, float v_min, float v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Vertical slider for a single integer value. */
    bool (*UI_VSliderInt)(const char* label, float size_x, float size_y, int* v, int v_min, int v_max, const char* format, SPF_SliderFlags flags);

    /** @brief Vertical slider for any numeric data type. */
    bool (*UI_VSliderScalar)(const char* label, float size_x, float size_y, SPF_DataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);

    // --- Direct Inputs (Keyboard-based entry) ---

    /**
     * @brief Standard single-line text input field.
     * @param label Text label.
     * @param buf Pointer to the character buffer where text will be stored.
     * @param buf_size Total size of the buffer in bytes.
     * @param flags Customization flags from SPF_InputTextFlags.
     * @return bool True if the buffer was modified.
     */
    bool (*UI_InputText)(const char* label, char* buf, size_t buf_size, SPF_InputTextFlags flags);

    /**
     * @brief Multi-line text input field (text area).
     * @param size_x, size_y Dimensions of the input area.
     */
    bool (*UI_InputTextMultiline)(const char* label, char* buf, size_t buf_size, float size_x, float size_y, SPF_InputTextFlags flags);

    /**
     * @brief Single-line text input with a background hint.
     * @param hint The text displayed when the buffer is empty (placeholder).
     */
    bool (*UI_InputTextWithHint)(const char* label, const char* hint, char* buf, size_t buf_size, SPF_InputTextFlags flags);

    /**
     * @brief Keyboard-based input for a single float value.
     * @param v Pointer to the value.
     * @param step Amount to increment/decrement when using [+] and [-] buttons.
     * @param step_fast Accelerated step amount when holding Shift.
     * @param format Formatting string.
     * @param flags Input text flags.
     * @return bool True if modified.
     */
    bool (*UI_InputFloat)(const char* label, float* v, float step, float step_fast, const char* format, SPF_InputTextFlags flags);

    /** @brief Keyboard input for a 2-component float vector. */
    bool (*UI_InputFloat2)(const char* label, float v[2], const char* format, SPF_InputTextFlags flags);
    
    /** @brief Keyboard input for a 3-component float vector. */
    bool (*UI_InputFloat3)(const char* label, float v[3], const char* format, SPF_InputTextFlags flags);
    
    /** @brief Keyboard input for a 4-component float vector. */
    bool (*UI_InputFloat4)(const char* label, float v[4], const char* format, SPF_InputTextFlags flags);

    /**
     * @brief Keyboard input for a single integer value.
     * @param v Pointer to value.
     * @param step, step_fast Incremental steps.
     */
    bool (*UI_InputInt)(const char* label, int* v, int step, int step_fast, SPF_InputTextFlags flags);

    /** @brief Keyboard input for a 2-component integer vector. */
    bool (*UI_InputInt2)(const char* label, int v[2], SPF_InputTextFlags flags);
    
    /** @brief Keyboard input for a 3-component integer vector. */
    bool (*UI_InputInt3)(const char* label, int v[3], SPF_InputTextFlags flags);
    
    /** @brief Keyboard input for a 4-component integer vector. */
    bool (*UI_InputInt4)(const char* label, int v[4], SPF_InputTextFlags flags);

    /**
     * @brief Keyboard input for a double-precision float value.
     * @param v Pointer to double.
     * @param step, step_fast Incremental steps.
     */
    bool (*UI_InputDouble)(const char* label, double* v, double step, double step_fast, const char* format, SPF_InputTextFlags flags);

    /**
     * @brief Generic keyboard input for any numeric data type.
     * @param data_type The numeric type.
     * @param p_data Pointer to the value(s).
     * @param p_step, p_step_fast Pointers to step values.
     */
    bool (*UI_InputScalar)(const char* label, SPF_DataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, SPF_InputTextFlags flags);
    
    /** @brief Generic keyboard input for an array of numeric values. */
    bool (*UI_InputScalarN)(const char* label, SPF_DataType data_type, void* p_data, int components, const void* p_step, const void* p_step_fast, const char* format, SPF_InputTextFlags flags);

    // --- Color Editor & Picker ---

    /**
     * @brief Compact color editor with a color square and numeric inputs.
     * @param label Text label.
     * @param col Array of 3 floats (RGB, range 0.0f to 1.0f).
     * @param flags Formatting and behavior flags from SPF_ColorEditFlags.
     * @return bool True if the color was modified.
     */
    bool (*UI_ColorEdit3)(const char* label, float col[3], SPF_ColorEditFlags flags);

    /**
     * @brief Compact color editor for 4-component color (RGBA).
     * @param col Array of 4 floats (RGBA).
     */
    bool (*UI_ColorEdit4)(const char* label, float col[4], SPF_ColorEditFlags flags);

    /**
     * @brief Full-featured color picker widget with a hue wheel or bar.
     * @param label Text label.
     * @param col Array of 3 floats (RGB).
     * @param flags Formatting flags.
     * @return bool True if modified.
     */
    bool (*UI_ColorPicker3)(const char* label, float col[3], SPF_ColorEditFlags flags);

    /** @brief Full-featured color picker for 4-component color (RGBA). */
    bool (*UI_ColorPicker4)(const char* label, float col[4], SPF_ColorEditFlags flags);

    /**
     * @brief Displays a small color square button that can trigger a picker.
     * @param desc_id Unique identifier or description for the button.
     * @param r, g, b, a Color components (0.0f to 1.0f).
     * @param flags Customization flags.
     * @param size_x, size_y Dimensions of the square. Use 0.0f for default.
     * @return bool True if clicked.
     */
    bool (*UI_ColorButton)(const char* desc_id, float r, float g, float b, float a, SPF_ColorEditFlags flags, float size_x, float size_y);

    /**
     * @brief Sets global options for all subsequent color widgets in the current session.
     * @param flags Bitmask of SPF_ColorEditFlags.
     */
    void (*UI_SetColorEditOptions)(SPF_ColorEditFlags flags);


    // ============================================================================================
    // VI. ADVANCED WIDGETS (Trees, Selectables, Lists, Plots)
    // ============================================================================================

    // --- Trees & Hierarchies ---

    /**
     * @brief Begins a collapsible tree node.
     * @details If this returns true, the node is open and you must eventually call UI_TreePop().
     * @param label Text label for the node.
     * @return bool True if the node is open and its children should be rendered.
     */
    bool (*UI_TreeNode)(const char* label);

    /**
     * @brief Advanced version of TreeNode with flags.
     * @param str_id Unique identifier for the node.
     * @param label Text label.
     * @param flags Configuration flags from SPF_TreeNodeFlags.
     * @return bool True if the node is open.
     */
    bool (*UI_TreeNodeEx)(const char* str_id, const char* label, SPF_TreeNodeFlags flags);

    /**
     * @brief Manually pushes a string ID onto the tree stack.
     * @details Used for creating custom tree behaviors. Must be matched with UI_TreePop().
     * @param str_id Identifier to push.
     */
    void (*UI_TreePush)(const char* str_id);

    /**
     * @brief Pops a level from the tree hierarchy stack.
     */
    void (*UI_TreePop)();

    /**
     * @brief Manually sets the storage ID for the next item.
     * @details This allows you to force a specific ID for storing the open/closed state of tree nodes or collapsing headers.
     * @param storage_id Unique ID to use for state persistence.
     */
    void (*UI_SetNextItemStorageID)(uint32_t storage_id);

    /**
     * @brief Gets the horizontal distance between the tree arrow and its label.
     * @return float Spacing value in pixels.
     */
    float (*UI_GetTreeNodeToLabelSpacing)();

    /**
     * @brief Displays a full-width header that can be collapsed.
     * @param label Text label.
     * @param flags Configuration flags (usually SPF_TREE_NODE_FLAG_COLLAPSING_HEADER).
     * @return bool True if the header is open.
     */
    bool (*UI_CollapsingHeader)(const char* label, SPF_TreeNodeFlags flags);

    /**
     * @brief Programmatically sets the open/closed state of the next tree node or collapsing header.
     * @param is_open Desired state.
     * @param cond Condition for applying the state.
     */
    void (*UI_SetNextItemOpen)(bool is_open, SPF_Cond cond);

    /**
     * @brief Returns whether the next tree node is open.
     * @param storage_id ID of the node in storage.
     * @return bool True if open.
     */
    bool (*UI_TreeNodeGetOpen)(uint32_t storage_id);

    // --- Selectables & Lists ---

    /**
     * @brief Displays an item that can be clicked and highlighted as selected.
     * @param label Text label.
     * @param selected Current selection state.
     * @param flags Configuration flags from SPF_SelectableFlags.
     * @param size_x, size_y Dimensions. Use 0.0f for auto-width and default height.
     * @return bool True if the item was clicked.
     */
    bool (*UI_Selectable)(const char* label, bool selected, SPF_SelectableFlags flags, float size_x, float size_y);

    /**
     * @brief Begins a scrolling list box container.
     * @details If true, you must render items inside and call UI_EndListBox().
     * @param label Text label.
     * @param size_x, size_y Total dimensions of the list box.
     * @return bool True if visible.
     */
    bool (*UI_BeginListBox)(const char* label, float size_x, float size_y);

    /**
     * @brief Ends the currently open list box.
     */
    void (*UI_EndListBox)();

    /**
     * @brief Simplified list box for a fixed array of strings.
     * @param label Text label.
     * @param current_item Pointer to the index of the selected item.
     * @param items Array of C-strings.
     * @param items_count Number of items.
     * @param height_in_items Number of items visible without scrolling (-1 for auto).
     * @return bool True if selection changed.
     */
    bool (*UI_ListBox)(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items);

    // --- Multi-Select (Complex list selection) ---

    /**
     * @brief Begins a multi-selection scope for the following items (Selectable, Checkbox, etc.).
     * @details Returns a structure containing selection requests that the plugin must handle.
     * @param flags Behavior flags from SPF_MultiSelectFlags.
     * @param selection_size Current number of selected items (-1 if unknown).
     * @param items_count Total number of items in the list (-1 if unknown).
     * @return SPF_MultiSelectIO* Pointer to the interaction object. Valid until UI_EndMultiSelect.
     */
    SPF_MultiSelectIO* (*UI_BeginMultiSelect)(SPF_MultiSelectFlags flags, int selection_size, int items_count);

    /**
     * @brief Ends the multi-selection scope.
     * @return SPF_MultiSelectIO* Final state of the selection interaction.
     */
    SPF_MultiSelectIO* (*UI_EndMultiSelect)();

    /**
     * @brief Links a unique ID to the next item for use with the Multi-Select API.
     * @param selection_user_data A 64-bit identifier for the item (usually an index or pointer).
     */
    void (*UI_SetNextItemSelectionUserData)(int64_t selection_user_data);

    /**
     * @brief Returns true if the last item's selection state was toggled by the Multi-Select system.
     */
    bool (*UI_IsItemToggledSelection)();

    // --- Data Visualization (Plots) ---

    /**
     * @brief Displays a line graph.
     * @param label Text label.
     * @param values Pointer to the data array.
     * @param values_count Number of points in the array.
     * @param values_offset Offset into the array (for circular buffers).
     * @param overlay_text Optional text rendered on top of the graph.
     * @param scale_min, scale_max Minimum and maximum Y-axis values. Use FLT_MAX for auto-scale.
     * @param graph_size Total dimensions of the widget.
     * @param stride Byte offset between values (usually sizeof(float)).
     */
    void (*UI_PlotLines)(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y, int stride);

    /**
     * @brief Displays a bar graph (histogram).
     */
    void (*UI_PlotHistogram)(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y, int stride);

    /**
     * @brief Displays a line graph using a data-retrieval callback instead of an array.
     * @details This allows for dynamic data plotting without copying large arrays every frame.
     * @param label Text label for the graph.
     * @param values_getter Function pointer to the data retriever (SPF_PlotGetter).
     * @param data User-provided data pointer passed to the getter.
     * @param values_count Total number of points to plot.
     * @param values_offset Optional offset into the data.
     * @param overlay_text Optional text rendered on top of the graph.
     * @param scale_min, scale_max Y-axis boundaries. Use FLT_MAX for auto-scaling.
     * @param graph_size_x, graph_size_y Total dimensions of the widget.
     */
    void (*UI_PlotLinesCallback)(const char* label, SPF_PlotGetter values_getter, void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y);

    /**
     * @brief Displays a bar graph using a data-retrieval callback.
     * @param label Text label.
     * @param values_getter Callback function.
     * @param data User data.
     * @param values_count Total points.
     * @param values_offset Optional offset.
     * @param overlay_text Optional overlay string.
     * @param scale_min, scale_max Y-axis limits.
     * @param graph_size_x, graph_size_y Widget dimensions.
     */
    void (*UI_PlotHistogramCallback)(const char* label, SPF_PlotGetter values_getter, void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, float graph_size_x, float graph_size_y);

    // --- Simple Value Display ---

    /** @brief Displays a boolean as "Label: true/false". */
    void (*UI_Value_Bool)(const char* prefix, bool b);
    /** @brief Displays an integer as "Label: value". */
    void (*UI_Value_Int)(const char* prefix, int v);
    /** @brief Displays an unsigned integer. */
    void (*UI_Value_UInt)(const char* prefix, unsigned int v);
    /** @brief Displays a float with formatted decimals. */
    void (*UI_Value_Float)(const char* prefix, float v, const char* float_format);

    // --- List Clipper (Performance for large lists) ---

    /**
     * @brief Initializes a list clipper for efficient rendering of large data sets.
     * @param items_count Total number of items in the list.
     * @param items_height Height of each item (use -1.0f for auto).
     * @return SPF_ListClipper Initialized clipper object.
     */
    SPF_ListClipper (*UI_ListClipper_Begin)(int items_count, float items_height);

    /**
     * @brief Processes the next batch of visible items.
     * @details Call this in a loop: while(ui->UI_ListClipper_Step(&clipper)) { ... }
     * @return bool True if there are more batches to render.
     */
    bool (*UI_ListClipper_Step)(SPF_ListClipper* clipper);

    /**
     * @brief Cleans up the clipper state.
     */
    void (*UI_ListClipper_End)(SPF_ListClipper* clipper);


    // ============================================================================================
    // VII. MENUS
    // ============================================================================================

    // --- Window Menu Bar ---

    /**
     * @brief Begins a menu bar at the top of the current window.
     * @details Requires the SPF_WINDOW_FLAG_MENU_BAR flag to be set during window creation.
     * @return bool True if the menu bar is visible and items can be added.
     */
    bool (*UI_BeginMenuBar)();

    /**
     * @brief Ends the current window's menu bar.
     */
    void (*UI_EndMenuBar)();

    // --- Main Menu Bar (Global) ---

    /**
     * @brief Begins the main menu bar at the top of the entire screen.
     * @details Items added here will appear in the global application header.
     * @return bool True if the bar is visible and items can be added.
     */
    bool (*UI_BeginMainMenuBar)();

    /**
     * @brief Ends the global main menu bar.
     */
    void (*UI_EndMainMenuBar)();

    // --- Menu Items ---

    /**
     * @brief Begins a sub-menu within a menu bar or another menu.
     * @param label Text label for the menu.
     * @param enabled If false, the menu is grayed out and cannot be opened.
     * @return bool True if the menu is open and sub-items should be rendered.
     */
    bool (*UI_BeginMenu)(const char* label, bool enabled);

    /**
     * @brief Ends the current sub-menu level.
     */
    void (*UI_EndMenu)();

    /**
     * @brief Displays a clickable menu item.
     * @param label Text label for the item.
     * @param shortcut Optional keyboard shortcut text (e.g., "Ctrl+S"). Does not implement logic.
     * @param selected Pointer to a boolean reflecting the item's checked state (optional).
     * @param enabled If false, the item is non-interactive.
     * @return bool True if the item was clicked.
     */
    bool (*UI_MenuItem)(const char* label, const char* shortcut, bool selected, bool enabled);

    // ============================================================================================
    // VIII. TABLES
    // ============================================================================================

    /**
     * @brief Begins a multi-column table.
     * @details Tables are powerful layout tools that support resizing, reordering, and sorting.
     *          Must be matched with UI_EndTable().
     * @param str_id Unique identifier for the table.
     * @param columns_count Number of columns in the table.
     * @param flags Configuration flags from SPF_TableFlags.
     * @param outer_size Total size of the table (0.0f for auto).
     * @param inner_width Inner width for horizontal scrolling (0.0f for auto).
     * @return bool True if the table is visible and rows should be rendered.
     */
    bool (*UI_BeginTable)(const char* str_id, int columns_count, SPF_TableFlags flags, float outer_size_x, float outer_size_y, float inner_width);

    /**
     * @brief Ends the current table.
     */
    void (*UI_EndTable)();

    /**
     * @brief Moves the cursor to the next row in the table.
     * @param row_flags Flags for the row (e.g., SPF_TABLE_ROW_FLAG_HEADERS).
     * @param min_row_height Minimum height for the row.
     */
    void (*UI_TableNextRow)(SPF_TableRowFlags row_flags, float min_row_height);

    /**
     * @brief Moves the cursor to the next column in the current row.
     * @return bool True if the column is visible and should be rendered.
     */
    bool (*UI_TableNextColumn)();

    /**
     * @brief Directly sets the current column index.
     * @param column_n Index of the column (0 to columns_count - 1).
     * @return bool True if visible.
     */
    bool (*UI_TableSetColumnIndex)(int column_n);

    // --- Table Settings ---

    /**
     * @brief Defines properties for a specific column.
     * @details Should be called before UI_TableHeadersRow().
     * @param label Text label for the column header.
     * @param flags Column-specific flags from SPF_TableColumnFlags.
     * @param init_width_or_weight Initial width (fixed) or weight (stretch).
     * @param user_id Optional unique ID for the column.
     */
    void (*UI_TableSetupColumn)(const char* label, SPF_TableColumnFlags flags, float init_width_or_weight, uint32_t user_id);

    /**
     * @brief Locks a specific number of rows or columns from scrolling.
     * @param cols_count Number of columns to freeze (starting from left).
     * @param rows_count Number of rows to freeze (starting from top).
     */
    void (*UI_TableSetupScrollFreeze)(int cols_count, int rows_count);

    /**
     * @brief Renders the entire header row using labels defined in UI_TableSetupColumn.
     */
    void (*UI_TableHeadersRow)();

    /**
     * @brief Renders a row of angled (slanted) headers to save horizontal space.
     */
    void (*UI_TableAngledHeadersRow)();

    /**
     * @brief Renders a header for a single column.
     * @param label Header text.
     */
    void (*UI_TableHeader)(const char* label);

    // --- Table State & Sorting ---

    /**
     * @brief Retrieves the current sorting specifications for the table.
     * @details Returns NULL if the table doesn't have the 'Sortable' flag or no sorting is active.
     *          The returned pointer is valid until the next table-related call.
     * @return const SPF_TableSortSpecs* Pointer to sorting specifications.
     */
    const SPF_TableSortSpecs* (*UI_TableGetSortSpecs)();

    /**
     * @brief Enables or disables a specific column programmatically.
     * @param column_n Index of the column.
     * @param enabled True to show, false to hide.
     */
    void (*UI_TableSetColumnEnabled)(int column_n, bool enabled);

    /**
     * @brief Checks if the mouse is currently hovering over a specific column.
     * @param column_n Column index.
     * @return bool True if hovered.
     */
    bool (*UI_TableGetHoveredColumn)(int column_n);

    // --- Table Metadata ---

    /**
     * @brief Gets the number of columns in the current table.
     */
    int (*UI_TableGetColumnCount)();

    /**
     * @brief Gets the current column index.
     */
    int (*UI_TableGetColumnIndex)();

    /**
     * @brief Gets the current row index.
     */
    int (*UI_TableGetRowIndex)();

    /**
     * @brief Gets the human-readable name of a column.
     * @param column_n Index of the column.
     * @return const char* Column name.
     */
    const char* (*UI_TableGetColumnName)(int column_n);

    /**
     * @brief Gets the configuration flags for a specific column.
     */
    SPF_TableColumnFlags (*UI_TableGetColumnFlags)(int column_n);

    /**
     * @brief Sets the background color for a row, column, or specific cell.
     * @param target Target type (usually SPF_TableBgTarget_).
     * @param color Packed 32-bit color.
     * @param column_n Column index (if target is a cell).
     */
    void (*UI_TableSetBgColor)(int target, uint32_t color, int column_n);

    // ============================================================================================
    // IX. POPUPS & TOOLTIPS
    // ============================================================================================

    // --- Popups ---

    /**
     * @brief Begins a general-purpose popup window.
     * @details If this returns true, the popup is open and you must draw content and call UI_EndPopup().
     * @param str_id Unique identifier for the popup.
     * @param flags Configuration flags from SPF_WindowFlags.
     * @return bool True if the popup is open.
     */
    bool (*UI_BeginPopup)(const char* str_id, SPF_WindowFlags flags);

    /**
     * @brief Begins a modal popup window.
     * @details Modals block interaction with other windows until closed.
     * @param name Text title for the modal window.
     * @param p_open Optional pointer to a boolean to control the 'X' close button.
     * @param flags Configuration flags.
     * @return bool True if the modal is open.
     */
    bool (*UI_BeginPopupModal)(const char* name, bool* p_open, SPF_WindowFlags flags);

    /**
     * @brief Ends the currently open popup or modal.
     */
    void (*UI_EndPopup)();

    /**
     * @brief Marks a popup as open.
     * @details Call this to trigger a popup defined by UI_BeginPopup.
     * @param str_id ID of the popup to open.
     * @param flags Trigger flags from SPF_PopupFlags.
     */
    void (*UI_OpenPopup)(const char* str_id, SPF_PopupFlags flags);

    /**
     * @brief Opens a popup when the last item is clicked.
     * @param str_id ID of the popup.
     * @param flags Trigger flags (e.g., specific mouse button).
     */
    void (*UI_OpenPopupOnItemClick)(const char* str_id, SPF_PopupFlags flags);

    /**
     * @brief Programmatically closes the currently active popup.
     */
    void (*UI_CloseCurrentPopup)();

    /**
     * @brief Opens and begins a context menu when the last item is right-clicked.
     * @param str_id Optional ID. If NULL, uses the last item's ID.
     * @param flags Trigger flags.
     * @return bool True if the context menu is open.
     */
    bool (*UI_BeginPopupContextItem)(const char* str_id, SPF_PopupFlags flags);

    /**
     * @brief Opens and begins a context menu when the current window is right-clicked.
     * @param str_id Optional ID. If NULL, uses the last item's ID.
     * @param flags Trigger flags.
     * @return bool True if open.
     */
    bool (*UI_BeginPopupContextWindow)(const char* str_id, SPF_PopupFlags flags);

    /**
     * @brief Opens and begins a context menu when the user right-clicks on the empty background area.
     * @details This triggers only if no other window or item is under the mouse.
     * @param str_id Unique identifier for the context menu.
     * @param flags Trigger conditions.
     * @return bool True if the context menu is open and should be rendered.
     */
    bool (*UI_BeginPopupContextVoid)(const char* str_id, SPF_PopupFlags flags);


    /**
     * @brief Checks if a specific popup is currently open.
     * @param str_id ID of the popup to check.
     * @param flags Flags to refine the search.
     * @return bool True if open.
     */
    bool (*UI_IsPopupOpen)(const char* str_id, SPF_PopupFlags flags);

    // --- Tooltips ---

    /**
     * @brief Begins a custom floating tooltip.
     * @details Items drawn between UI_BeginTooltip and UI_EndTooltip will follow the mouse.
     */
    void (*UI_BeginTooltip)();

    /**
     * @brief Ends the current tooltip.
     */
    void (*UI_EndTooltip)();

    /**
     * @brief Sets a simple text tooltip for the last item.
     * @param text String to display.
     */
    void (*UI_SetTooltip)(const char* text);

    /**
     * @brief Begins a tooltip for the last item only when it is hovered.
     * @return bool True if the item is hovered and the tooltip should be rendered.
     */
    bool (*UI_BeginItemTooltip)();

    /**
     * @brief Sets a simple text tooltip for the last item, only if it's hovered.
     * @param text String to display.
     */
    void (*UI_SetItemTooltip)(const char* text);

    // ============================================================================================
    // X. DRAG & DROP
    // ============================================================================================

    /**
     * @brief Begins a drag and drop source.
     * @details Call this immediately after an item (e.g., Button, Selectable) to make it draggable.
     *          If this returns true, you must call UI_SetDragDropPayload() and then UI_EndDragDropSource().
     * @param flags Configuration flags from SPF_DragDropFlags.
     * @return bool True if the user is currently dragging this item.
     */
    bool (*UI_BeginDragDropSource)(SPF_DragDropFlags flags);

    /**
     * @brief Sets the data payload for the current drag and drop operation.
     * @details This function MUST be called within a UI_BeginDragDropSource / UI_EndDragDropSource block.
     * @param type A unique string identifier for the data type (e.g., "INV_ITEM_ID").
     * @param data Pointer to the raw data buffer to be transferred.
     * @param size The size of the data buffer in bytes.
     * @param cond Condition for applying the payload (usually SPF_COND_NONE).
     * @return bool True if the payload was set successfully.
     */
    bool (*UI_SetDragDropPayload)(const char* type, const void* data, size_t size, SPF_Cond cond);

    /**
     * @brief Ends the current drag and drop source block.
     */
    void (*UI_EndDragDropSource)();

    /**
     * @brief Begins a drag and drop target area.
     * @details Call this immediately after an item to make it a valid drop zone. 
     *          If this returns true, you can call UI_AcceptDragDropPayload() and MUST call UI_EndDragDropTarget().
     * @return bool True if a draggable item is currently hovering over this target.
     */
    bool (*UI_BeginDragDropTarget)();

    /**
     * @brief Accepts a drag and drop payload of a specific type.
     * @details This function is called within a UI_BeginDragDropTarget block. 
     *          It checks if the hovering payload matches the type and accepts the drop if released.
     * @param type The unique string identifier for the payload type to accept.
     * @param flags Configuration flags for the acceptance behavior.
     * @return const SPF_Payload_Handle* Pointer to the accepted payload metadata, or NULL if not accepted.
     */
    const SPF_Payload_Handle* (*UI_AcceptDragDropPayload)(const char* type, SPF_DragDropFlags flags);

    /**
     * @brief Ends the current drag and drop target block.
     */
    void (*UI_EndDragDropTarget)();

    /**
     * @brief Retrieves information about the current active drag and drop payload without accepting it.
     * @return const SPF_Payload_Handle* Pointer to the active payload, or NULL if no drag is in progress.
     */
    const SPF_Payload_Handle* (*UI_GetDragDropPayload)();

    // ============================================================================================
    // XI. STYLE & TYPOGRAPHY
    // ============================================================================================

    // --- Fonts ---

    /**
     * @brief Retrieves a handle to a pre-configured font by its identifier.
     * @param font_key String ID of the font (e.g., "bold", "h1", "regular").
     * @return SPF_Font_Handle* Handle to the font, or NULL if not found.
     */
    SPF_Font_Handle* (*UI_GetFont)(const char* font_key);

    /**
     * @brief Pushes a font onto the stack, making it active for all subsequent text rendering.
     * @details Every UI_PushFont() must be matched with a UI_PopFont().
     * @param font_handle Handle obtained from UI_GetFont().
     */
    void (*UI_PushFont)(SPF_Font_Handle* font_handle);

    /**
     * @brief Restores the previous font from the stack.
     */
    void (*UI_PopFont)();

    // --- Style Stack (Colors & Variables) ---

    /**
     * @brief Pushes a temporary color change for a specific UI element.
     * @param idx The element color to change (from SPF_StyleColor).
     * @param r, g, b, a New color components (0.0f to 1.0f).
     */
    void (*UI_PushStyleColor)(SPF_StyleColor idx, float r, float g, float b, float a);

    /**
     * @brief Restores 'count' previous colors from the style stack.
     * @param count Number of colors to pop.
     */
    void (*UI_PopStyleColor)(int count);

    /**
     * @brief Pushes a temporary change for a float-based style variable.
     * @param idx The variable to change (from SPF_StyleVar).
     * @param val New float value.
     */
    void (*UI_PushStyleVarFloat)(SPF_StyleVar idx, float val);

    /**
     * @brief Pushes a temporary change for a Vec2-based style variable.
     * @param idx The variable to change.
     * @param val_x, val_y New vector components.
     */
    void (*UI_PushStyleVarVec2)(SPF_StyleVar idx, float val_x, float val_y);

    /**
     * @brief Restores 'count' previous variables from the style stack.
     * @param count Number of variables to pop.
     */
    void (*UI_PopStyleVar)(int count);

    /**
     * @brief Retrieves a handle to the global UI style object for read-only queries.
     * @return SPF_Style_Handle* Handle to the current global style.
     */
    SPF_Style_Handle* (*UI_GetStyle)();

    /**
     * @brief Retrieves the window padding from a style handle.
     * @param style_handle Handle to the style object.
     * @param[out] out_x Pointer to store horizontal padding.
     * @param[out] out_y Pointer to store vertical padding.
     */
    void (*UI_Style_GetWindowPadding)(SPF_Style_Handle* style_handle, float* out_x, float* out_y);

    /**
     * @brief Retrieves the item spacing from a style handle.
     * @param style_handle Handle to the style object.
     * @param[out] out_x Pointer to store horizontal spacing.
     * @param[out] out_y Pointer to store vertical spacing.
     */
    void (*UI_Style_GetItemSpacing)(SPF_Style_Handle* style_handle, float* out_x, float* out_y);

    /**
     * @brief Retrieves the frame padding from a style handle.
     * @param style_handle Handle to the style object.
     * @param[out] out_x Pointer to store horizontal padding.
     * @param[out] out_y Pointer to store vertical padding.
     */
    void (*UI_Style_GetFramePadding)(SPF_Style_Handle* style_handle, float* out_x, float* out_y);

    /**
     * @brief Applies the standard ImGui dark theme.
     */
    void (*UI_StyleColorsDark)();

    /**
     * @brief Applies the standard ImGui light theme.
     */
    void (*UI_StyleColorsLight)();

    /**
     * @brief Applies the classic ImGui theme (retro look).
     */
    void (*UI_StyleColorsClassic)();

    /**
     * @brief Retrieves a specific color from the current global style.
     * @param idx Color identifier from SPF_StyleColor.
     * @param[out] out_r, out_g, out_b, out_a Pointers to store the RGBA components.
     */
    void (*UI_GetStyleColor)(SPF_StyleColor idx, float* out_r, float* out_g, float* out_b, float* out_a);

    // --- Text Utilities ---

    /**
     * @brief Calculates the bounding box size of a text string using the current font.
     * @param text String to measure.
     * @param[out] out_w, out_h Pointers to store calculated width and height.
     */
    void (*UI_CalcTextSize)(const char* text, float* out_w, float* out_h);

    /**
     * @brief Advanced text measurement with a specific font and size.
     * @param font Font style to use.
     * @param font_size Size in pixels.
     * @param text String to measure.
     * @param[out] out_w, out_h Pointers to store results.
     */
    void (*UI_CalcTextSizeWithFont)(SPF_Font font, float font_size, const char* text, float* out_w, float* out_h);

    /**
     * @brief Packs four float color components into a 32-bit integer (0xAABBGGRR).
     * @return uint32_t Packed color value.
     */
    uint32_t (*UI_ColorConvertFloat4ToU32)(float r, float g, float b, float a);

    /**
     * @brief Unpacks a 32-bit integer color into four float components.
     */
    void (*UI_ColorConvertU32ToFloat4)(uint32_t in, float* out_r, float* out_g, float* out_b, float* out_a);

    /**
     * @brief Converts an RGB color to HSV (Hue, Saturation, Value).
     * @param r, g, b Input components (0.0f to 1.0f).
     * @param[out] out_h, out_s, out_v Pointers to store results.
     */
    void (*UI_ColorConvertRGBtoHSV)(float r, float g, float b, float* out_h, float* out_s, float* out_v);

    /**
     * @brief Converts an HSV color back to RGB.
     */
    void (*UI_ColorConvertHSVtoRGB)(float h, float s, float v, float* out_r, float* out_g, float* out_b);

    // --- SPF Text Styling API (Advanced Typography) ---

    /**
     * @brief Creates a new text style object.
     * @details Styles allow for complex formatting (alignment, wrapping, decorations) 
     *          to be applied atomically. Must be destroyed with UI_Style_Destroy().
     * @return SPF_TextStyle_Handle New style handle.
     */
    SPF_TextStyle_Handle (*UI_Style_Create)();

    /**
     * @brief Releases memory associated with a text style object.
     * @param handle Style handle to destroy.
     */
    void (*UI_Style_Destroy)(SPF_TextStyle_Handle handle);

    /** @brief Sets the font for this style. */
    void (*UI_Style_SetFont)(SPF_TextStyle_Handle handle, SPF_Font font);
    /** @brief Sets the text color for this style. */
    void (*UI_Style_SetColor)(SPF_TextStyle_Handle handle, float r, float g, float b, float a);
    /** @brief Sets the hover color for this style. */
    void (*UI_Style_SetHoverColor)(SPF_TextStyle_Handle handle, float r, float g, float b, float a);
    /** @brief Sets the active/pressed color for this style. */
    void (*UI_Style_SetActiveColor)(SPF_TextStyle_Handle handle, float r, float g, float b, float a);
    /** @brief Sets horizontal alignment (Left, Center, Right). */
    void (*UI_Style_SetAlign)(SPF_TextStyle_Handle handle, SPF_TextAlign align);
    /** @brief Enables or disables text wrapping. */
    void (*UI_Style_SetWrap)(SPF_TextStyle_Handle handle, bool wrap);
    /** @brief Sets internal padding for the text block. */
    void (*UI_Style_SetPadding)(SPF_TextStyle_Handle handle, float pad_x, float pad_y);
    /** @brief Forces text to render as a separator line with a label. */
    void (*UI_Style_SetSeparator)(SPF_TextStyle_Handle handle, bool is_separator);
    /** @brief Enables underline decoration. */
    void (*UI_Style_SetUnderline)(SPF_TextStyle_Handle handle, bool is_underline);
    /** @brief Enables strikethrough decoration. */
    void (*UI_Style_SetStrikethrough)(SPF_TextStyle_Handle handle, bool is_strikethrough);

    /**
     * @brief Renders formatted text using a style object.
     * @param handle Style handle (or NULL for default).
     * @param fmt Printf-style format string.
     */
    void (*UI_TextStyled)(SPF_TextStyle_Handle handle, const char* fmt, ...);

    /**
     * @brief Renders a block of Markdown text.
     * @details Supports headers, bold, italic, lists, and code blocks.
     *          Also supports custom text color highlighting using <#RRGGBB>text</> tags.
     *          Example: ui->UI_RenderMarkdown("This is <#ff0000>red</> and <#00ff00>green</> text.", NULL);
     * @param markdown_text Raw markdown string.
     * @param base_style_handle Optional base style for overall layout.
     */
    void (*UI_RenderMarkdown)(const char* markdown_text, SPF_TextStyle_Handle base_style_handle);

    // ============================================================================================
    // XII. DRAWLIST API (Low-level Vector Drawing)
    // ============================================================================================

    // --- Clipping ---

    /**
     * @brief Pushes a clipping rectangle onto the draw list stack.
     * @details Subsequent drawing commands will be clipped to this area.
     * @param dl Handle to the draw list.
     * @param p_min_x, p_min_y Top-left corner of the clip rectangle.
     * @param p_max_x, p_max_y Bottom-right corner of the clip rectangle.
     * @param intersect_with_current_clip_rect If true, the new region is the intersection of the new and current area.
     */
    void (*UI_DrawList_PushClipRect)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, bool intersect_with_current_clip_rect);

    /**
     * @brief Pushes a clipping rectangle that covers the entire screen viewport.
     */
    void (*UI_DrawList_PushClipRectFullScreen)(SPF_DrawList_Handle dl);

    /**
     * @brief Restores the previous clipping rectangle from the stack.
     */
    void (*UI_DrawList_PopClipRect)(SPF_DrawList_Handle dl);

    // --- Basic Shapes ---

    /**
     * @brief Adds a single line segment.
     * @param p1_x, p1_y Starting point.
     * @param p2_x, p2_y Ending point.
     * @param col Packed 32-bit color.
     * @param thickness Line width in pixels.
     */
    void (*UI_DrawList_AddLine)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, uint32_t col, float thickness);

    /**
     * @brief Adds an outlined rectangle.
     * @param p_min_x, p_min_y Top-left corner.
     * @param p_max_x, p_max_y Bottom-right corner.
     * @param col Outline color.
     * @param rounding Corner radius.
     * @param flags Rounding flags from SPF_DrawFlags.
     * @param thickness Border width.
     */
    void (*UI_DrawList_AddRect)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, SPF_DrawFlags flags, float thickness);

    /** @brief Adds a filled rectangle. */
    void (*UI_DrawList_AddRectFilled)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col, float rounding, SPF_DrawFlags flags);

    /** @brief Adds a multi-color filled rectangle (gradient).
     * @param col_upr_left, col_upr_right, col_bot_right, col_bot_left Colors for each corner.
     */
    void (*UI_DrawList_AddRectFilledMultiColor)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, uint32_t col_upr_left, uint32_t col_upr_right, uint32_t col_bot_right, uint32_t col_bot_left);

    /** @brief Adds an outlined quad (4-point polygon). */
    void (*UI_DrawList_AddQuad)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col, float thickness);

    /** @brief Adds a filled quad (4-point polygon). */
    void (*UI_DrawList_AddQuadFilled)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, uint32_t col);

    /** @brief Adds an outlined triangle. */
    void (*UI_DrawList_AddTriangle)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col, float thickness);

    /** @brief Adds a filled triangle. */
    void (*UI_DrawList_AddTriangleFilled)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col);

    /**
     * @brief Adds a multi-color filled triangle (linear gradient) to the draw list.
     * @param dl The draw list handle.
     * @param p1_x, p1_y, p2_x, p2_y, p3_x, p3_y The three corner points of the triangle.
     * @param col1, col2, col3 Packed colors at each vertex.
     */
    void (*UI_DrawList_AddTriangleFilledMultiColor)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, uint32_t col1, uint32_t col2, uint32_t col3);

    /**
     * @brief Adds an outlined circle.
     * @param center_x, center_y Origin of the circle.
     * @param radius Circle radius.
     * @param num_segments Number of segments to approximate the curve (0 for auto).
     */
    void (*UI_DrawList_AddCircle)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness);

    /** @brief Adds a filled circle. */
    void (*UI_DrawList_AddCircleFilled)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments);

    /**
     * @brief Adds a multi-color filled circle (radial gradient) to the draw list.
     * @param dl The draw list handle.
     * @param center_x, center_y Center position.
     * @param radius Circle radius.
     * @param col_inner Color at the center.
     * @param col_outer Color at the edge.
     * @param num_segments Number of segments.
     */
    void (*UI_DrawList_AddCircleFilledMultiColor)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col_inner, uint32_t col_outer, int num_segments);

    /**
     * @brief Adds a filled rectangle to the current window's draw list (Helper).
     * @param x1, y1 Top-left corner.
     * @param x2, y2 Bottom-right corner.
     * @param r, g, b, a Color components (0.0f - 1.0f).
     */
    void (*UI_AddRectFilled)(float x1, float y1, float x2, float y2, float r, float g, float b, float a);

    /** @brief Adds an outlined N-gon (regular polygon). */
    void (*UI_DrawList_AddNgon)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness);

    /** @brief Adds a filled N-gon. */
    void (*UI_DrawList_AddNgonFilled)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments);

    /**
     * @brief Adds an outlined N-gon using a specific path-building logic for contours.
     */
    void (*UI_DrawList_AddNgonContour)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, uint32_t col, int num_segments, float thickness);

    /**
     * @brief Adds an outlined ellipse.
     * @param center_x, center_y Center of the ellipse.
     * @param radius_x Horizontal radius.
     * @param radius_y Vertical radius.
     * @param col Packed color.
     * @param rot Rotation angle in radians.
     * @param num_segments Quality of the curve (0 for auto).
     * @param thickness Line width.
     */
    void (*UI_DrawList_AddEllipse)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius_x, float radius_y, uint32_t col, float rot, int num_segments, float thickness);

    /** @brief Adds a filled ellipse. */
    void (*UI_DrawList_AddEllipseFilled)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius_x, float radius_y, uint32_t col, float rot, int num_segments);

    // --- Complex Curves & Polylines ---

    /**
     * @brief Adds a cubic Bezier curve.
     * @param p1 Starting point.
     * @param cp1 First control point.
     * @param cp2 Second control point.
     * @param p2 Ending point.
     */
    void (*UI_DrawList_AddBezierCubic)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float cp1_x, float cp1_y, float cp2_x, float cp2_y, float p2_x, float p2_y, uint32_t col, float thickness, int num_segments);

    /**
     * @brief Adds a quadratic Bezier curve (single control point).
     */
    void (*UI_DrawList_AddBezierQuadratic)(SPF_DrawList_Handle dl, float p1_x, float p1_y, float cp_x, float cp_y, float p2_x, float p2_y, uint32_t col, float thickness, int num_segments);

    /**
     * @brief Adds a line strip connecting multiple points.
     * @param points_x, points_y Arrays of coordinates.
     * @param num_points Size of the arrays.
     * @param closed If true, connects the last point back to the first.
     */
    void (*UI_DrawList_AddPolyline)(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col, SPF_DrawFlags flags, float thickness);

    /** @brief Adds a filled convex polygon from points. */
    void (*UI_DrawList_AddConvexPolyFilled)(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col);

    /**
     * @brief Adds a filled concave polygon.
     * @details Unlike ConvexPolyFilled, this supports complex shapes with self-intersections or holes.
     */
    void (*UI_DrawList_AddConcavePolyFilled)(SPF_DrawList_Handle dl, const float* points_x, const float* points_y, int num_points, uint32_t col);

    // --- Images ---

    /**
     * @brief Draws a texture at a specific screen location.
     */
    void (*UI_DrawList_AddImage)(SPF_DrawList_Handle dl, void* user_texture_id, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col);

    /**
     * @brief Draws a texture stretched across a quadrilateral.
     */
    void (*UI_DrawList_AddImageQuad)(SPF_DrawList_Handle dl, void* user_texture_id, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, float p4_x, float p4_y, float uv1_x, float uv1_y, float uv2_x, float uv2_y, float uv3_x, float uv3_y, float uv4_x, float uv4_y, uint32_t col);

    /**
     * @brief Draws a texture with rounded corners.
     */
    void (*UI_DrawList_AddImageRounded)(SPF_DrawList_Handle dl, void* user_texture_id, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col, float rounding, SPF_DrawFlags flags);

    /**
     * @brief Injects a custom rendering callback into the draw stream.
     * @details This allows plugins to execute raw DirectX/OpenGL/Vulkan commands 
     *          at a specific depth within the UI draw order.
     * @param callback Function pointer to the custom renderer.
     * @param user_data Data passed to the callback.
     */
    void (*UI_DrawList_AddCallback)(SPF_DrawList_Handle dl, void (*callback)(const void* parent_list, const void* cmd), void* user_data);

    // --- Text ---

    /** @brief Adds text at an absolute screen position. */
    void (*UI_DrawList_AddText)(SPF_DrawList_Handle dl, float pos_x, float pos_y, uint32_t col, const char* text);

    /** @brief Adds text using a specific font handle and size. */
    void (*UI_DrawList_AddTextWithFont)(SPF_DrawList_Handle dl, SPF_Font_Handle* font, float font_size, float pos_x, float pos_y, uint32_t col, const char* text, float wrap_width);

    // --- Path API (Stateful shape building) ---

    /** @brief Clears the current path state. */
    void (*UI_DrawList_PathClear)(SPF_DrawList_Handle dl);
    /** @brief Moves the path to a new point. */
    void (*UI_DrawList_PathLineTo)(SPF_DrawList_Handle dl, float pos_x, float pos_y);
    /** @brief Draws the outline of the current path. */
    void (*UI_DrawList_PathStroke)(SPF_DrawList_Handle dl, uint32_t col, SPF_DrawFlags flags, float thickness);
    /** @brief Fills the interior of the current convex path. */
    void (*UI_DrawList_PathFillConvex)(SPF_DrawList_Handle dl, uint32_t col);
    /** @brief Adds an arc to the path. */
    void (*UI_DrawList_PathArcTo)(SPF_DrawList_Handle dl, float center_x, float center_y, float radius, float a_min, float a_max, int num_segments);
    /** @brief Adds a rectangle to the path. */
    void (*UI_DrawList_PathRect)(SPF_DrawList_Handle dl, float rect_min_x, float rect_min_y, float rect_max_x, float rect_max_y, float rounding, SPF_DrawFlags flags);

    // --- Channels (Layered Drawing) ---

    /** @brief Splits the draw list into multiple virtual layers. */
    void (*UI_DrawList_ChannelsSplit)(SPF_DrawList_Handle dl, int count);
    /** @brief Merges all channels back into the main draw list. */
    void (*UI_DrawList_ChannelsMerge)(SPF_DrawList_Handle dl);
    /** @brief Sets the active channel for subsequent commands. */
    void (*UI_DrawList_ChannelsSetCurrent)(SPF_DrawList_Handle dl, int n);

    // --- Low-level Buffer API (Direct Mesh Access) ---

    /**
     * @brief Reserves space in the vertex and index buffers for custom geometry.
     * @param dl Handle to the draw list.
     * @param idx_count Number of indices to reserve.
     * @param vtx_count Number of vertices to reserve.
     */
    void (*UI_DrawList_PrimReserve)(SPF_DrawList_Handle dl, int idx_count, int vtx_count);

    /**
     * @brief Adds a rectangle with custom UV coordinates directly to the vertex buffer.
     */
    void (*UI_DrawList_PrimRectUV)(SPF_DrawList_Handle dl, float p_min_x, float p_min_y, float p_max_x, float p_max_y, float uv_min_x, float uv_min_y, float uv_max_x, float uv_max_y, uint32_t col);

    /**
     * @brief Manually pushes a single vertex into the buffer.
     * @param x, y Vertex position.
     * @param u, v Texture coordinates.
     * @param col Vertex color.
     */
    void (*UI_DrawList_PrimVtx)(SPF_DrawList_Handle dl, float x, float y, float u, float v, uint32_t col);

    /**
     * @brief Manually pushes a single index into the buffer.
     */
    void (*UI_DrawList_PrimIdx)(SPF_DrawList_Handle dl, uint16_t idx);

    // ============================================================================================
    // XIII. ITEM QUERIES & STATE
    // ============================================================================================

    /**
     * @brief Checks if the last submitted item is hovered by the mouse.
     * @param flags Optional configuration flags from SPF_HoveredFlags.
     * @return bool True if hovered.
     */
    bool (*UI_IsItemHovered)(SPF_HoveredFlags flags);

    /** @brief Checks if the last item is active (e.g., being clicked or dragged). */
    bool (*UI_IsItemActive)();

    /** @brief Checks if the last item is focused by the keyboard or navigation system. */
    bool (*UI_IsItemFocused)();

    /**
     * @brief Checks if the last item was clicked by a specific mouse button.
     * @param mouse_button Button index from SPF_MouseButton.
     * @return bool True if clicked.
     */
    bool (*UI_IsItemClicked)(SPF_MouseButton mouse_button);

    /** @brief Checks if the last item is visible within the current clipping region. */
    bool (*UI_IsItemVisible)();

    /** @brief Checks if the last item's value was modified in the current frame. */
    bool (*UI_IsItemEdited)();

    /** @brief Checks if the last item just became active in this frame. */
    bool (*UI_IsItemActivated)();

    /** @brief Checks if the last item just stopped being active. */
    bool (*UI_IsItemDeactivated)();

    /** @brief Checks if the last item stopped being active specifically after an edit. */
    bool (*UI_IsItemDeactivatedAfterEdit)();

    /** @brief Checks if the last tree node or collapsing header was toggled open/closed. */
    bool (*UI_IsItemToggledOpen)();

    /** @brief Checks if ANY item in the entire UI is currently hovered. */
    bool (*UI_IsAnyItemHovered)();

    /** @brief Checks if ANY item is currently active. */
    bool (*UI_IsAnyItemActive)();

    /** @brief Checks if ANY item is currently focused. */
    bool (*UI_IsAnyItemFocused)();

    /**
     * @brief Retrieves the internal unique ID of the last submitted item.
     * @return uint32_t Item identifier.
     */
    uint32_t (*UI_GetItemID)();

    /**
     * @brief Gets the bounding box of the last drawn item in screen coordinates.
     * @param[out] out_x, out_y Pointers to store the coordinates.
     */
    void (*UI_GetItemRectMin)(float* out_x, float* out_y);
    void (*UI_GetItemRectMax)(float* out_x, float* out_y);
    void (*UI_GetItemRectSize)(float* out_x, float* out_y);

    // ============================================================================================
    // XIV. TABS & TAB BARS
    // ============================================================================================

    /**
     * @brief Begins a tab bar container.
     * @details If this returns true, you must render UI_BeginTabItem() elements inside and call UI_EndTabBar().
     * @param str_id Unique identifier for the tab bar.
     * @param flags Configuration flags from SPF_TabBarFlags.
     * @return bool True if the tab bar is visible.
     */
    bool (*UI_BeginTabBar)(const char* str_id, SPF_TabBarFlags flags);

    /** @brief Ends the current tab bar. Must be matched with UI_BeginTabBar(). */
    void (*UI_EndTabBar)();

    /**
     * @brief Begins a single tab item within a tab bar.
     * @param label Text displayed on the tab.
     * @param p_open Optional pointer to a boolean to control the 'X' close button.
     * @param flags Tab-specific flags from SPF_TabItemFlags.
     * @return bool True if the tab is active and its content should be rendered.
     */
    bool (*UI_BeginTabItem)(const char* label, bool* p_open, SPF_TabItemFlags flags);

    /** @brief Ends the current tab item content. */
    void (*UI_EndTabItem)();

    /**
     * @brief Renders a button that sits directly on the tab bar.
     * @return bool True if the button was clicked.
     */
    bool (*UI_TabItemButton)(const char* label, SPF_TabItemFlags flags);

    /**
     * @brief Programmatically closes a specific tab.
     * @param tab_or_docked_window_label The label of the tab to close.
     */
    void (*UI_SetTabItemClosed)(const char* tab_or_docked_window_label);

    // ============================================================================================
    // XV. DOCKING & VIEWPORTS
    // ============================================================================================

    /**
     * @brief Creates a docking node that fills the current window's content region.
     * @param id Unique ID for the dockspace.
     * @param size_x, size_y Dimensions of the dockspace. Use 0.0f to fill available space.
     * @param flags Configuration flags from SPF_DockNodeFlags.
     * @return uint32_t The ID of the created dock node.
     */
    uint32_t (*UI_DockSpace)(uint32_t id, float size_x, float size_y, SPF_DockNodeFlags flags);

    /**
     * @brief Creates a docking node that fills the entire background viewport.
     * @param flags Configuration flags.
     * @return uint32_t The ID of the created dock node.
     */
    uint32_t (*UI_DockSpaceOverViewport)(SPF_DockNodeFlags flags);

    /**
     * @brief Sets the target dock node for the next window to be created.
     * @param dock_id ID of the dock node obtained from UI_DockSpace.
     * @param cond Condition for applying the dock state.
     */
    void (*UI_SetNextWindowDockID)(uint32_t dock_id, SPF_Cond cond);

    /** @brief Checks if the current window is currently docked into a node. */
    bool (*UI_IsWindowDocked)();

    // --- DockBuilder API (Programmatic Layout Setup) ---

    /**
     * @brief Begins a DockBuilder transaction for a specific dockspace ID.
     */
    void (*UI_DockBuilderDockWindow)(const char* window_name, uint32_t node_id);

    /**
     * @brief Retrieves a handle to a specific docking node.
     */
    void* (*UI_DockBuilderGetNode)(uint32_t node_id);

    /**
     * @brief Adds a new, empty docking node.
     */
    uint32_t (*UI_DockBuilderAddNode)(uint32_t node_id, SPF_DockNodeFlags flags);

    /**
     * @brief Removes a docking node and all its children.
     */
    void (*UI_DockBuilderRemoveNode)(uint32_t node_id);

    /**
     * @brief Removes all docked windows from a node.
     */
    void (*UI_DockBuilderRemoveNodeDockedWindows)(uint32_t node_id);

    /**
     * @brief Sets the screen position of a docking node.
     */
    void (*UI_DockBuilderSetNodePos)(uint32_t node_id, float pos_x, float pos_y);

    /**
     * @brief Sets the size of a docking node.
     */
    void (*UI_DockBuilderSetNodeSize)(uint32_t node_id, float size_x, float size_y);

    /**
     * @brief Splits a docking node into two sub-nodes.
     * @param node_id The ID of the node to split.
     * @param split_dir The direction of the split (SPF_DIR_LEFT, SPF_DIR_UP, etc.).
     * @param size_ratio The ratio of the first node (0.0f to 1.0f).
     * @param[out] out_id_at_dir ID of the newly created node in the specified direction.
     * @param[out] out_id_at_opposite ID of the node in the opposite direction.
     * @return uint32_t The ID of the parent node.
     */
    uint32_t (*UI_DockBuilderSplitNode)(uint32_t node_id, SPF_Dir split_dir, float size_ratio, uint32_t* out_id_at_dir, uint32_t* out_id_at_opposite);

    /**
     * @brief Completes the DockBuilder configuration and applies changes.
     */
    void (*UI_DockBuilderFinish)(uint32_t node_id);

    /**
     * @brief Retrieves the ID of the central node within a dockspace.
     */
    uint32_t (*UI_DockBuilderGetCentralNode)(uint32_t node_id);

    // --- Platform Viewports ---

    /**
     * @brief Synchronizes and renders windows that have been dragged out of the main host window.
     */
    void (*UI_UpdatePlatformWindows)();

    /**
     * @brief Finalizes and presents all platform windows.
     */
    void (*UI_RenderPlatformWindowsDefault)();

    /**
     * @brief Cleanly destroys all auxiliary platform windows.
     */
    void (*UI_DestroyPlatformWindows)();

    // ============================================================================================
    // XVI. INTERNAL & CUSTOM WIDGET UTILITIES (Low-Level API)
    // ============================================================================================

    /**
     * @brief Low-level: Reserves space for a custom widget in the current window's layout.
     * @details Updates the window's cursor position and content bounding box.
     * @param size_x, size_y Total dimensions to reserve for the widget.
     * @param text_baseline_y Vertical offset used to align the widget's text with surrounding elements.
     */
    void (*UI_ItemSize)(float size_x, float size_y, float text_baseline_y);

    /**
     * @brief Low-level: Adds a custom item to the window's internal registry for interaction.
     * @details Performs a collision check against the mouse and registers the item's ID 
     *          for hover/active state tracking. Required for custom widgets to support 'IsItemHovered'.
     * @param min_x, min_y Top-left corner of the item's hit area.
     * @param max_x, max_y Bottom-right corner.
     * @param id Unique identifier for the item.
     * @param flags Configuration flags from SPF_WindowFlags.
     * @return bool True if the item is visible and within the current clipping region.
     */
    bool (*UI_ItemAdd)(float min_x, float min_y, float max_x, float max_y, uint32_t id, SPF_WindowFlags flags);

    /**
     * @brief Low-level: Manually sets metadata for the last submitted item.
     * @details Advanced tool to override the internal state (ID, flags, status) of a widget.
     */
    void (*UI_SetLastItemData)(uint32_t item_id, SPF_InputTextFlags flags, SPF_HoveredFlags status_flags, float min_x, float min_y, float max_x, float max_y);

    // --- Custom Widget Behaviors ---

    /**
     * @brief Implements standard button interaction logic (hovering, clicking, holding).
     * @param min_x, min_y Bounding box start.
     * @param max_x, max_y Bounding box end.
     * @param id Unique ID for the interaction.
     * @param[out] out_hovered True if the item is currently hovered by the mouse.
     * @param[out] out_held True if the mouse button is being held down on the item.
     * @param flags Interaction flags.
     * @return bool True if the button was clicked (released after press).
     */
    bool (*UI_ButtonBehavior)(float min_x, float min_y, float max_x, float max_y, uint32_t id, bool* out_hovered, bool* out_held, SPF_WindowFlags flags);
    
    /**
     * @brief Implements logical behavior for draggable numeric widgets.
     * @param id Unique interaction ID.
     * @param data_type Numeric data type.
     * @param p_v Pointer to the value.
     * @param v_speed Change speed per pixel.
     * @param p_min, p_max Optional bounds.
     * @param format Display format string.
     * @param flags Customization flags.
     * @return bool True if the value was modified.
     */
    bool (*UI_DragBehavior)(uint32_t id, SPF_DataType data_type, void* p_v, float v_speed, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);
    
    /**
     * @brief Implements logical behavior for slider widgets.
     */
    bool (*UI_SliderBehavior)(float min_x, float min_y, float max_x, float max_y, uint32_t id, SPF_DataType data_type, void* p_v, const void* p_min, const void* p_max, const char* format, SPF_SliderFlags flags);

    /**
     * @brief Implements logical behavior for a draggable splitter (e.g., between two panes).
     * @param bb_min_x, bb_min_y Bounding box start.
     * @param bb_max_x, bb_max_y Bounding box end.
     * @param id Unique ID.
     * @param axis Split direction (0 for X-axis, 1 for Y-axis).
     * @param[in,out] size1 Pointer to the first item's size.
     * @param[in,out] size2 Pointer to the second item's size.
     * @param min_size1 Minimum size for the first item.
     * @param min_size2 Minimum size for the second item.
     * @return bool True if the splitter is currently being dragged.
     */
    bool (*UI_SplitterBehavior)(float bb_min_x, float bb_min_y, float bb_max_x, float bb_max_y, uint32_t id, int axis, float* size1, float* size2, float min_size1, float min_size2);

    // --- Low-Level Rendering Helpers ---

    /**
     * @brief Directly renders a frame background (box) into the current draw list.
     * @param col Packed 32-bit color.
     * @param border If true, draws a 1-pixel border around the frame.
     * @param rounding Corner radius for the frame.
     */
    void (*UI_RenderFrame)(float min_x, float min_y, float max_x, float max_y, uint32_t col, bool border, float rounding);
    
    /**
     * @brief Directly renders an outlined frame border.
     */
    void (*UI_RenderFrameBorder)(float min_x, float min_y, float max_x, float max_y, float rounding);

    /**
     * @brief Directly renders text with alignment and optional clipping.
     * @param x, y Baseline position for the text.
     * @param text String to render.
     * @param hide_text_after_hash If true, does not render characters after a '##' marker.
     */
    void (*UI_RenderText)(float x, float y, const char* text, bool hide_text_after_hash);

    /**
     * @brief Renders text clipped within a specific rectangle.
     */
    void (*UI_RenderTextClipped)(float min_x, float min_y, float max_x, float max_y, const char* text, const char* text_end, float* out_text_size, float align_x, float align_y);

    /**
     * @brief Renders text with an ellipsis (...) if it exceeds the specified width.
     */
    void (*UI_RenderTextEllipsis)(float x, float y, float max_width, const char* text);
    
    /**
     * @brief Directly renders a small arrow icon pointing in a specific direction.
     * @param x, y Center position.
     * @param col Packed color.
     * @param dir Direction (from SPF_Dir).
     * @param scale Overall size multiplier.
     */
    void (*UI_RenderArrow)(float x, float y, uint32_t col, SPF_Dir dir, float scale);

    /**
     * @brief Directly renders a standard checkmark icon.
     * @param x, y Baseline position.
     * @param col Packed color.
     * @param sz Size of the checkmark.
     */
    void (*UI_RenderCheckMark)(float x, float y, uint32_t col, float sz);

    /**
     * @brief Directly renders a small bullet point.
     * @param dl Handle to the draw list.
     * @param x, y Position.
     * @param col Packed color.
     */
    void (*UI_RenderBullet)(SPF_DrawList_Handle dl, float x, float y, uint32_t col);


    // ============================================================================================
    // XVII. FRAMEWORK UTILITIES (Non-ImGui Features)
    // ============================================================================================

    // --- Notifications ---

    /**
     * @brief Displays a notification popup on the screen using extended parameters.
     * @details Triggers a message that can be automatic, timed, or programmatically managed.
     *          Supports Markdown syntax and custom styling.
     * @param params Pointer to the notification configuration structure.
     * @return SPF_Notification_Handle A handle to the created notification for future control.
     */
    SPF_Notification_Handle (*UI_ShowNotification)(const SPF_Notification_Params* params);

    /**
     * @brief Programmatically hides/closes a notification by its handle.
     * @param handle The notification handle returned by UI_ShowNotification.
     */
    void (*UI_HideNotification)(SPF_Notification_Handle handle);

    // --- Transitions ---

    /**
     * @brief Plays a cinematic screen transition effect.
     * @details Useful for hiding scene loading or creating dramatic entries.
     * @param type The visual effect to use (Fade, Wipe, Shutter, etc.).
     * @param duration Total duration of the effect in seconds.
     * @param reverse If true, plays the effect backwards (e.g., Fade-In instead of Fade-Out).
     * @param color Preset color palette for the transition.
     */
    void (*UI_PlayTransition)(SPF_TransitionType type, float duration, bool reverse, SPF_TransitionColor color);

    /**
     * @brief Checks if a cinematic transition is currently active on screen.
     * @return bool True if a transition is playing.
     */
    bool (*UI_IsTransitionActive)();

    // --- Input Blocking & Overrides ---

    /**
     * @brief Programmatically blocks physical mouse input from reaching the game engine.
     * @details Allows a plugin to temporarily "capture" mouse axes or buttons to prevent 
     *          them from affecting the game camera/controls during specific interactions.
     * @param blockAxes If true, mouse movement (X/Y) is blocked from the game.
     * @param blockButtons If true, mouse clicks are blocked.
     * @param blockWheel If true, the mouse wheel is blocked.
     */
    void (*UI_SetMouseBlockState)(bool blockAxes, bool blockButtons, bool blockWheel);

    /**
     * @brief Manually overrides the internal mouse capture logic.
     * @details If set to true, it can force mouse control back to the game or UI 
     *          regardless of the current auto-detection state.
     * @param overridden True to enable manual override.
     */
    void (*UI_SetMouseOverride)(bool overridden);

    /**
     * @brief Checks if the mouse control logic is currently in override mode.
     * @return bool True if overridden.
     */
    bool (*UI_IsMouseOverridden)();

} SPF_UI_API;
