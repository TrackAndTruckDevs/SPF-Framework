#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/GameCamera/GameCameraType.hpp"
#include <string>

SPF_NS_BEGIN

// Forward-declare the service it depends on
namespace GameCamera {
class GameCameraManager;
}

namespace UI {
/**
 * @class CameraWindow
 * @brief A UI window for testing and demonstrating the GameCamera service.
 *        Follows the pattern of TelemetryWindow by taking its service
 *        dependency via the constructor.
 */
class CameraWindow : public BaseWindow {
 public:
  CameraWindow(GameCamera::GameCameraManager& gameCameraService, const std::string& owner, const std::string& name);

 protected:
    const char* GetWindowTitle() const override;
    void RenderContent() override;

 private:
  GameCamera::GameCameraManager& m_gameCameraService;

  // Tab switching state
  bool m_needsTabSwitch = false;
  GameCamera::GameCameraType m_activeTabType = GameCamera::GameCameraType::Unknown;

  // Localization keys
    std::string m_locCurrentCamera;
    std::string m_locCameraWorldCoordinates;
    std::string m_locCameraWorldCoordinatesNotFound;
    std::string m_locSelectCamera;
    std::string m_locInterior;
    std::string m_locBehind;
    std::string m_locTop;
    std::string m_locCabin;
    std::string m_locWindow;
    std::string m_locBumper;
    std::string m_locWheel;
    std::string m_locTV;
    std::string m_locDeveloperFreeCamera;

    std::string m_locTabInteriorCamera;
    std::string m_locTabBehindCamera;
    std::string m_locTabTopCamera;
    std::string m_locTabCabinCamera;
    std::string m_locTabWindowCamera;
    std::string m_locTabBumperCamera;
    std::string m_locTabWheelCamera;
    std::string m_locTabTVCamera;
    std::string m_locTabFreeCamera;
    std::string m_locTabDebug;

    // Interior Camera
    std::string m_locFovZoom;
    std::string m_locBaseFov;
    std::string m_locBaseFovNotFound;
    std::string m_locFinalHFov;
    std::string m_locFinalVFov;
    std::string m_locFinalFovNotFound;
    std::string m_locSeatPosition;
    std::string m_locSeatLr;
    std::string m_locSeatUd;
    std::string m_locSeatFb;
    std::string m_locSeatPositionNotFound;
    std::string m_locHeadRotation;
    std::string m_locYawLr;
    std::string m_locPitchUd;
    std::string m_locHeadRotationNotFound;
    std::string m_locMouseRotationLimits;
    std::string m_locLeftLimit;
    std::string m_locRightLimit;
    std::string m_locUpLimit;
    std::string m_locDownLimit;
    std::string m_locRotationLimitsNotFound;
    std::string m_locRotationDefaults;
    std::string m_locDefaultLr;
    std::string m_locDefaultUd;
    std::string m_locRotationDefaultsNotFound;
    std::string m_locResetToDefaults;
    std::string m_locInteriorCameraNotAvailable;

    // Behind Camera
    std::string m_locLiveState;
    std::string m_locLivePitch;
    std::string m_locLiveYaw;
    std::string m_locLiveZoom;
    std::string m_locLiveStateNotFound;
    std::string m_locDistanceZoomSettings;
    std::string m_locMinDistance;
    std::string m_locMaxDistance;
    std::string m_locTrailerMaxOffset;
    std::string m_locDefaultDistance;
    std::string m_locTrailerDefaultDist;
    std::string m_locZoomSpeed;
    std::string m_locDistanceLaziness;
    std::string m_locDistanceZoomSettingsNotFound;
    std::string m_locElevationPitchSettings;
    std::string m_locAzimuthLaziness;
    std::string m_locMinElevation;
    std::string m_locMaxElevation;
    std::string m_locDefaultElevation;
    std::string m_locTrailerDefaultElev;
    std::string m_locHeightLimit;
    std::string m_locElevationPitchSettingsNotFound;
    std::string m_locPivotOffset;
    std::string m_locPivotX;
    std::string m_locPivotY;
    std::string m_locPivotZ;
    std::string m_locPivotOffsetNotFound;
    std::string m_locDynamicOffset;
    std::string m_locMaxDynamicOffset;
    std::string m_locDynOffsetSpeedMin;
    std::string m_locDynOffsetSpeedMax;
    std::string m_locDynOffsetLaziness;
    std::string m_locDynamicOffsetNotFound;
    std::string m_locBaseFovBehind;
    std::string m_locResetToDefaultsBehind;
    std::string m_locBehindCameraNotAvailable;

    // Top Camera
    std::string m_locHeightZoom;
    std::string m_locMinimumHeight;
    std::string m_locMaximumHeight;
    std::string m_locHeightZoomNotFound;
    std::string m_locMovement;
    std::string m_locMovementSpeed;
    std::string m_locMovementNotFound;
    std::string m_locDynamicOffsetTop;
    std::string m_locForwardOffset;
    std::string m_locBackwardOffset;
    std::string m_locDynamicOffsetNotFoundTop;
    std::string m_locBaseFovTop;
    std::string m_locResetToDefaultsTop;
    std::string m_locTopCameraNotAvailable;

    // Cabin Camera
    std::string m_locBaseFovCabin;
    std::string m_locResetToDefaultsCabin;
    std::string m_locCabinCameraNotAvailable;

    // Window Camera
    std::string m_locHeadOffset;
    std::string m_locHeadXWindow;
    std::string m_locHeadYWindow;
    std::string m_locHeadZWindow;
    std::string m_locHeadOffsetNotFound;
    std::string m_locLiveRotation;
    std::string m_locLiveYawWindow;
    std::string m_locLivePitchWindow;
    std::string m_locLiveRotationNotFound;
    std::string m_locMouseRotationLimitsDefaults;
    std::string m_locLeftLimitWindow;
    std::string m_locRightLimitWindow;
    std::string m_locUpLimitWindow;
    std::string m_locDownLimitWindow;
    std::string m_locRotationLimitsNotFoundWindow;
    std::string m_locDefaultLrWindow;
    std::string m_locDefaultUdWindow;
    std::string m_locRotationDefaultsNotFoundWindow;
    std::string m_locBaseFovWindow;
    std::string m_locResetToDefaultsWindow;
    std::string m_locWindowCameraNotAvailable;

    // Bumper Camera
    std::string m_locOffsetBumper;
    std::string m_locOffsetXBumper;
    std::string m_locOffsetYBumper;
    std::string m_locOffsetZBumper;
    std::string m_locOffsetNotFoundBumper;
    std::string m_locBaseFovBumper;
    std::string m_locResetToDefaultsBumper;
    std::string m_locBumperCameraNotAvailable;

    // Wheel Camera
    std::string m_locOffsetWheel;
    std::string m_locOffsetXWheel;
    std::string m_locOffsetYWheel;
    std::string m_locOffsetZWheel;
    std::string m_locOffsetNotFoundWheel;
    std::string m_locBaseFovWheel;
    std::string m_locResetToDefaultsWheel;
    std::string m_locWheelCameraNotAvailable;

    // TV Camera
    std::string m_locDistanceTV;
    std::string m_locMaxDistanceTV;
    std::string m_locDistanceNotFoundTV;
    std::string m_locPrefabUpliftTV;
    std::string m_locPrefabUpliftXTV;
    std::string m_locPrefabUpliftYTV;
    std::string m_locPrefabUpliftZTV;
    std::string m_locPrefabUpliftNotFoundTV;
    std::string m_locRoadUpliftTV;
    std::string m_locRoadUpliftXTV;
    std::string m_locRoadUpliftYTV;
    std::string m_locRoadUpliftZTV;
    std::string m_locRoadUpliftNotFoundTV;
    std::string m_locBaseFovTV;
    std::string m_locResetToDefaultsTV;
    std::string m_locTVCameraNotAvailable;

    // Free Camera
    std::string m_locPositionFreeCam;
    std::string m_locPositionXFreeCam;
    std::string m_locPositionYFreeCam;
    std::string m_locPositionZFreeCam;
    std::string m_locPositionNotFoundFreeCam;
    std::string m_locOrientationFreeCam;
    std::string m_locMouseHorizontalFreeCam;
    std::string m_locMouseVerticalFreeCam;
    std::string m_locRollFreeCam;
    std::string m_locOrientationNotFoundFreeCam;
    std::string m_locQuaternionFreeCam;
    std::string m_locQuaternionXFreeCam;
    std::string m_locQuaternionYFreeCam;
    std::string m_locQuaternionZFreeCam;
    std::string m_locQuaternionWFreeCam;
    std::string m_locQuaternionNotFoundFreeCam;
    std::string m_locBaseFovFreeCam;
    std::string m_locMovementSpeedFreeCam;
    std::string m_locSpeedFreeCam;
    std::string m_locMovementSpeedNotFoundFreeCam;
    std::string m_locResetToDefaultsFreeCam;
    std::string m_locFreeCameraNotAvailable;

    // Debug Tab
    std::string m_locCurrentModeDebug;
    std::string m_locCurrentModeNADebug;
    std::string m_locEnableDebugCamera;
    std::string m_locEnableDebugCameraNotFound;
    std::string m_locCleanUI;
    std::string m_locCleanUINotFound;
    std::string m_locShowDebugHUD;
    std::string m_locShowDebugHUDNotFound;
    std::string m_locEnableDebugCameraToSelectMode;
    std::string m_locSimpleDebug;
    std::string m_locBasicDebugCameraMode;
    std::string m_locVideoDebug;
    std::string m_locHUDPositionDebug;
    std::string m_locTopLeftDebug;
    std::string m_locBottomLeftDebug;
    std::string m_locTopRightDebug;
    std::string m_locBottomRightDebug;
    std::string m_locCurrentDebug;
    std::string m_locCurrentNADebug;
    std::string m_locTrafficDebug;
    std::string m_locCameraFocusesTraffic;
    std::string m_locCinematicDebug;
    std::string m_locCinematicCameraMode;
    std::string m_locAnimatedDebug;
    std::string m_locCreatePlayAnimations;
    std::string m_locActivateGameAnimatedMode;
    std::string m_locCustomAnimationControls;
    std::string m_locPlayingStatus;
    std::string m_locPauseButton;
    std::string m_locPausedStatus;
    std::string m_locStoppedStatus;
    std::string m_locPlayButton;
    std::string m_locStopButton;
    std::string m_locStatusLabel;
    std::string m_locReversePlayback;
    std::string m_locTimelineLabel;
    std::string m_locStateCameraDebug;
    std::string m_locCreateStateCamera;
    std::string m_locSaveKeyframe;
    std::string m_locReloadFromFile;
    std::string m_locClearAllMemory;
    std::string m_locAnimationControls;
    std::string m_locAddEditState;
    std::string m_locPositionXYZ;
    std::string m_locMysteryFloat;
    std::string m_locQuaternionXYZW;
    std::string m_locFOVLabel;
    std::string m_locAddStateMemory;
    std::string m_locUpdateStateMemory;
    std::string m_locDeleteStateMemory;
    std::string m_locPreviousState;
    std::string m_locNextState;
    std::string m_locActiveStateLabel;
    std::string m_locPosLabel;
    std::string m_locMysteryLabel;
    std::string m_locQuatLabel;
    std::string m_locFOVValueLabel;
    std::string m_locActiveStateNone;
    std::string m_locSavedStatesLabel;
    std::string m_locStatesComboLabel;
    std::string m_locStateItemLabel;
    std::string m_locNoStatesSaved;
    std::string m_locOversizeDebug;
    std::string m_locCameraOversizedTrailers;
    std::string m_locDebugCameraNotAvailable;
};
}  // namespace UI
SPF_NS_END
