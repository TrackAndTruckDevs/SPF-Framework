/**
 * @file ClimateWindow.hpp
 * @brief Header for the UI window for Climate and Weather manipulation.
 */

#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/UI/BaseWindow.hpp"

#include <string>


SPF_NS_BEGIN

namespace Data::GameData {
class ClimateService;
}

namespace UI {

/**
 * @class ClimateWindow
 * @brief A window that allows players to control weather, climate, and sun profiles.
 */
class ClimateWindow : public BaseWindow {
 public:
  ClimateWindow(const std::string& componentName, const std::string& windowId, Data::GameData::ClimateService& climateService);
  virtual ~ClimateWindow() = default;

  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  Data::GameData::ClimateService& m_climateService;

  // Localization keys
  std::string m_locNotReady;
  std::string m_locSubtitle;
  std::string m_locSelectClimate;
  std::string m_locRefreshList;
  std::string m_locActiveClimate;
  std::string m_locTimeAutostep;
  std::string m_locBtnStopMinus;
  std::string m_locBtnMinus1Min;
  std::string m_locBtnStopPlus;
  std::string m_locBtnPlus1Min;
  std::string m_locSunProfilesCount;
  std::string m_locActiveSunProfile;
  std::string m_locNextSunProfile;
  std::string m_locTransitionProgress;
  std::string m_locActiveElev;
  std::string m_locNextElev;
  std::string m_locSunAngle;
  std::string m_locNice;
  std::string m_locBad;
  std::string m_locNextWeather;
  std::string m_locInterpolatedChange;
  std::string m_locInterpolatedTooltip;
  std::string m_locWeatherMixing;
  std::string m_locNoTransition;
  std::string m_locDuration;
  std::string m_locBadWeatherFactor;
  std::string m_locFactor;
  std::string m_locSet;
  std::string m_locBadWeatherMode;
  std::string m_locBadWeatherActive;
  std::string m_locBadWeatherInactive;
  std::string m_locRemainingTime;
  std::string m_locVariationStatus;
  std::string m_locSetActiveVariation;
  std::string m_locSetNextVariation;
  std::string m_locActiveTag;
  std::string m_locBlendLabel;
  std::string m_locNextVarFmt;
  std::string m_locSectionEnvProfile;
  std::string m_locSectionSun;
  std::string m_locSectionMoonStars;
  std::string m_locSectionSkyTextures;
  std::string m_locSectionCloudShadows;
  std::string m_locSectionTemperature;
  std::string m_locSectionRainLightning;
  std::string m_locSectionSnow;
  std::string m_locSectionFog;
  std::string m_locSectionAmbientEnv;
  std::string m_locSectionPostProcess;
  std::string m_locSectionWindBlending;
  std::string m_locSunLowElevation;
  std::string m_locSunHighElevation;
  std::string m_locSunDirection;
  std::string m_locSunDirectionForward;
  std::string m_locSunDirectionZenith;
  std::string m_locSunDirectionBackward;
  std::string m_locSunColor;
  std::string m_locSunOpacity;
  std::string m_locSunHaloColor;
  std::string m_locSunShadowStrength;
  std::string m_locSunshaftColor;
  std::string m_locSunshaftSize;
  std::string m_locMoonColor;
  std::string m_locMoonHaloColor;
  std::string m_locMoonHaloScale;
  std::string m_locStarmapColor;
  std::string m_locStarsColor;
  std::string m_locStarsTexture;
  std::string m_locSkyColor;
  std::string m_locSkyBottomColor;
  std::string m_locSkyboxTexture;
  std::string m_locSkycloudMaskTexture;
  std::string m_locMirrorSkyTexture;
  std::string m_locCloudShadowWeight;
  std::string m_locCloudShadowTexture;
  std::string m_locCloudShadowAreaSize;
  std::string m_locCloudShadowSpeed;
  std::string m_locTemperature;
  std::string m_locRainIntensity;
  std::string m_locLightningIntensity;
  std::string m_locLightningMask;
  std::string m_locRainMaxWetness;
  std::string m_locRainAdditionalAmbient;
  std::string m_locSnowIntensity;
  std::string m_locSnowFlakeSizeRange;
  std::string m_locSnowAdditionalAmbient;
  std::string m_locSnowChaosRate;
  std::string m_locSnowChaosWeight;
  std::string m_locFogColor;
  std::string m_locFogColor2;
  std::string m_locFogVgradient;
  std::string m_locFogOffset;
  std::string m_locFogDensity;
  std::string m_locAmbient;
  std::string m_locDiffuse;
  std::string m_locSpecular;
  std::string m_locEnv;
  std::string m_locEnvStaticMod;
  std::string m_locToneMapping;
  std::string m_locContrast;
  std::string m_locShoulderLength;
  std::string m_locColorGrading;
  std::string m_locColorBalance;
  std::string m_locColorSaturation;
  std::string m_locBloom;
  std::string m_locBloomThreshold;
  std::string m_locBloomLimit;
  std::string m_locBloomIntensity;
  std::string m_locBloomStandardDeviation;
  std::string m_locDepthOfField;
  std::string m_locDofStart;
  std::string m_locDofTransition;
  std::string m_locDofFilterSize;
  std::string m_locEyeAdaptation;
  std::string m_locLowIntensityMin;
  std::string m_locLowIntensityMax;
  std::string m_locLowIntensityColor;
  std::string m_locDarkAdaptationSpeed;
  std::string m_locBrightAdaptationSpeed;
  std::string m_locTargetGray;
  std::string m_locExposureScale;
  std::string m_locMinScale;
  std::string m_locMaxScale;
  std::string m_locScaleOverride;
  std::string m_locWindType;
  std::string m_locSpeedCoef;
  std::string m_locStability;
  std::string m_locBlendWeight;
  std::string m_locLampsOnElevation;
  std::string m_locDayInYear;
  std::string m_locSummerTime;
  std::string m_locThunderstormProbability;
};

}  // namespace UI
SPF_NS_END
