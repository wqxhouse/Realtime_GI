#include <PCH.h>
#include <TwHelper.h>
#include "AppSettings.h"

using namespace SampleFramework11;

static const char* MSAAModesLabels[4] =
{
    "None",
    "2x",
    "4x",
    "8x",
};

static const char* FilterTypesLabels[10] =
{
    "Box",
    "Triangle",
    "Gaussian",
    "BlackmanHarris",
    "Smoothstep",
    "BSpline",
    "CatmullRom",
    "Mitchell",
    "GeneralizedCubic",
    "Sinc",
};

static const char* JitterModesLabels[3] =
{
    "None",
    "Uniform2x",
    "Hammersly16",
};

static const char* ScenesLabels[2] =
{
    "CornellBox",
    "Sponza",
};

static const char* ShadingTechLabels[2] =
{
    "Forward",
    "Clustered_Deferred",
};

namespace AppSettings
{
    MSAAModesSetting MSAAMode;
    FilterTypesSetting FilterType;
    FloatSetting FilterSize;
    FloatSetting GaussianSigma;
    FloatSetting CubicB;
    FloatSetting CubicC;
    BoolSetting UseStandardResolve;
    BoolSetting InverseLuminanceFiltering;
    BoolSetting UseExposureFiltering;
    FloatSetting ExposureFilterOffset;
    BoolSetting UseGradientMipLevel;
    BoolSetting CentroidSampling;
    BoolSetting EnableTemporalAA;
    FloatSetting TemporalAABlendFactor;
    BoolSetting UseTemporalColorWeighting;
    BoolSetting ClampPrevColor;
    JitterModesSetting JitterMode;
    FloatSetting LowFreqWeight;
    FloatSetting HiFreqWeight;
    FloatSetting SharpeningAmount;
    ScenesSetting CurrentScene;
    ShadingTechSetting CurrentShadingTech;
    DirectionSetting LightDirection;
    ColorSetting LightColor;
    BoolSetting EnableSSR;
    BoolSetting PauseSceneScript;
    BoolSetting EnableDirectLighting;
    BoolSetting EnableIndirectDiffuseLighting;
    BoolSetting EnableIndirectSpecularLighting;
    BoolSetting RenderBackground;
    BoolSetting RenderSceneObjectBBox;
    BoolSetting RenderIrradianceVolumeProbes;
    BoolSetting EnableShadows;
    BoolSetting EnableNormalMaps;
    BoolSetting EnableRealtimeCubemap;
    IntSetting DiffuseGIBounces;
    FloatSetting DiffuseGI_Intensity;
    FloatSetting NormalMapIntensity;
    FloatSetting DiffuseIntensity;
    FloatSetting Roughness;
    FloatSetting SpecularIntensity;
    FloatSetting EmissiveIntensity;
    FloatSetting ProbeIndex;
    FloatSetting ProbeX;
    FloatSetting ProbeY;
    FloatSetting ProbeZ;
    FloatSetting BoxSizeX;
    FloatSetting BoxSizeY;
    FloatSetting BoxSizeZ;
    OrientationSetting SceneOrientation;
    FloatSetting ModelRotationSpeed;
    BoolSetting DoubleSyncInterval;
    FloatSetting ExposureScale;
    FloatSetting BloomExposure;
    FloatSetting BloomMagnitude;
    FloatSetting BloomBlurSigma;
    FloatSetting ManualExposure;

    ConstantBuffer<AppSettingsCBuffer> CBuffer;

    void Initialize(ID3D11Device* device)
    {
        TwBar* tweakBar = Settings.TweakBar();

        MSAAMode.Initialize(tweakBar, "MSAAMode", "Anti Aliasing", "MSAAMode", "", MSAAModes::MSAA4x, 4, MSAAModesLabels);
        Settings.AddSetting(&MSAAMode);

        FilterType.Initialize(tweakBar, "FilterType", "Anti Aliasing", "Filter Type", "", FilterTypes::Smoothstep, 10, FilterTypesLabels);
        Settings.AddSetting(&FilterType);

        FilterSize.Initialize(tweakBar, "FilterSize", "Anti Aliasing", "Filter Size", "", 2.0000f, 0.0000f, 6.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&FilterSize);

        GaussianSigma.Initialize(tweakBar, "GaussianSigma", "Anti Aliasing", "Gaussian Sigma", "", 0.5000f, 0.0100f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&GaussianSigma);

        CubicB.Initialize(tweakBar, "CubicB", "Anti Aliasing", "Cubic B", "", 0.3300f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&CubicB);

        CubicC.Initialize(tweakBar, "CubicC", "Anti Aliasing", "Cubic C", "", 0.3300f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&CubicC);

        UseStandardResolve.Initialize(tweakBar, "UseStandardResolve", "Anti Aliasing", "Use Standard Resolve", "", false);
        Settings.AddSetting(&UseStandardResolve);

        InverseLuminanceFiltering.Initialize(tweakBar, "InverseLuminanceFiltering", "Anti Aliasing", "Inverse Luminance Filtering", "", true);
        Settings.AddSetting(&InverseLuminanceFiltering);

        UseExposureFiltering.Initialize(tweakBar, "UseExposureFiltering", "Anti Aliasing", "Use Exposure Filtering", "", true);
        Settings.AddSetting(&UseExposureFiltering);

        ExposureFilterOffset.Initialize(tweakBar, "ExposureFilterOffset", "Anti Aliasing", "Exposure Filter Offset", "", 2.0000f, -16.0000f, 16.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ExposureFilterOffset);

        UseGradientMipLevel.Initialize(tweakBar, "UseGradientMipLevel", "Anti Aliasing", "Use Gradient Mip Level", "", false);
        Settings.AddSetting(&UseGradientMipLevel);

        CentroidSampling.Initialize(tweakBar, "CentroidSampling", "Anti Aliasing", "Centroid Sampling", "", true);
        Settings.AddSetting(&CentroidSampling);

        EnableTemporalAA.Initialize(tweakBar, "EnableTemporalAA", "Anti Aliasing", "Enable Temporal AA", "", true);
        Settings.AddSetting(&EnableTemporalAA);

        TemporalAABlendFactor.Initialize(tweakBar, "TemporalAABlendFactor", "Anti Aliasing", "Temporal AABlend Factor", "", 0.5000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&TemporalAABlendFactor);

        UseTemporalColorWeighting.Initialize(tweakBar, "UseTemporalColorWeighting", "Anti Aliasing", "Use Temporal Color Weighting", "", true);
        Settings.AddSetting(&UseTemporalColorWeighting);

        ClampPrevColor.Initialize(tweakBar, "ClampPrevColor", "Anti Aliasing", "Clamp Prev Color", "", true);
        Settings.AddSetting(&ClampPrevColor);

        JitterMode.Initialize(tweakBar, "JitterMode", "Anti Aliasing", "Jitter Mode", "", JitterModes::Uniform2x, 3, JitterModesLabels);
        Settings.AddSetting(&JitterMode);

        LowFreqWeight.Initialize(tweakBar, "LowFreqWeight", "Anti Aliasing", "Low Freq Weight", "", 0.2500f, 0.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&LowFreqWeight);

        HiFreqWeight.Initialize(tweakBar, "HiFreqWeight", "Anti Aliasing", "Hi Freq Weight", "", 0.8500f, 0.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&HiFreqWeight);

        SharpeningAmount.Initialize(tweakBar, "SharpeningAmount", "Anti Aliasing", "Sharpening Amount", "", 0.0000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SharpeningAmount);

        CurrentScene.Initialize(tweakBar, "CurrentScene", "Scene Controls", "Current Scene", "", Scenes::CornellBox, 2, ScenesLabels);
        Settings.AddSetting(&CurrentScene);

        CurrentShadingTech.Initialize(tweakBar, "CurrentShadingTech", "Scene Controls", "Current Shading Tech", "", ShadingTech::Clustered_Deferred, 2, ShadingTechLabels);
        Settings.AddSetting(&CurrentShadingTech);

        LightDirection.Initialize(tweakBar, "LightDirection", "Scene Controls", "Light Direction", "The direction of the light", Float3(-0.7500f, 0.9770f, -0.4000f));
        Settings.AddSetting(&LightDirection);

        LightColor.Initialize(tweakBar, "LightColor", "Scene Controls", "Light Color", "The color of the light", Float3(20.0000f, 16.0000f, 10.0000f), true, 0.0000f, 20.0000f, 0.1000f, ColorUnit::None);
        Settings.AddSetting(&LightColor);

        EnableSSR.Initialize(tweakBar, "EnableSSR", "Scene Controls", "Enable SSR", "", true);
        Settings.AddSetting(&EnableSSR);

        PauseSceneScript.Initialize(tweakBar, "PauseSceneScript", "Scene Controls", "Pause Scene Script", "", false);
        Settings.AddSetting(&PauseSceneScript);

        EnableDirectLighting.Initialize(tweakBar, "EnableDirectLighting", "Scene Controls", "Enable Direct Lighting", "", true);
        Settings.AddSetting(&EnableDirectLighting);

        EnableIndirectDiffuseLighting.Initialize(tweakBar, "EnableIndirectDiffuseLighting", "Scene Controls", "Enable Indirect Diffuse Lighting", "", true);
        Settings.AddSetting(&EnableIndirectDiffuseLighting);

        EnableIndirectSpecularLighting.Initialize(tweakBar, "EnableIndirectSpecularLighting", "Scene Controls", "Enable Indirect Specular Lighting", "", true);
        Settings.AddSetting(&EnableIndirectSpecularLighting);

        RenderBackground.Initialize(tweakBar, "RenderBackground", "Scene Controls", "Render Background", "", true);
        Settings.AddSetting(&RenderBackground);

        RenderSceneObjectBBox.Initialize(tweakBar, "RenderSceneObjectBBox", "Scene Controls", "Render Scene Object BBox", "", false);
        Settings.AddSetting(&RenderSceneObjectBBox);

        RenderIrradianceVolumeProbes.Initialize(tweakBar, "RenderIrradianceVolumeProbes", "Scene Controls", "Render Irradiance Volume Probes", "", false);
        Settings.AddSetting(&RenderIrradianceVolumeProbes);

        EnableShadows.Initialize(tweakBar, "EnableShadows", "Scene Controls", "Enable Shadows", "", true);
        Settings.AddSetting(&EnableShadows);

        EnableNormalMaps.Initialize(tweakBar, "EnableNormalMaps", "Scene Controls", "Enable Normal Maps", "", true);
        Settings.AddSetting(&EnableNormalMaps);

        EnableRealtimeCubemap.Initialize(tweakBar, "EnableRealtimeCubemap", "Scene Controls", "Enable Realtime Cubemap", "", false);
        Settings.AddSetting(&EnableRealtimeCubemap);

        DiffuseGIBounces.Initialize(tweakBar, "DiffuseGIBounces", "Scene Controls", "Diffuse GI Bounces", "The bounces of Indirect diffuse from GI", 1, 1, 10);
        Settings.AddSetting(&DiffuseGIBounces);

        DiffuseGI_Intensity.Initialize(tweakBar, "DiffuseGI_Intensity", "Scene Controls", "GI Intensity", "The intensity of Indirect diffuse from GI", 1.0000f, 0.0000f, 10.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&DiffuseGI_Intensity);

        NormalMapIntensity.Initialize(tweakBar, "NormalMapIntensity", "Scene Controls", "Normal Map Intensity", "", 1.0000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&NormalMapIntensity);

        DiffuseIntensity.Initialize(tweakBar, "DiffuseIntensity", "Scene Controls", "Diffuse Intensity", "Diffuse albedo intensity parameter for the material", 0.5000f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&DiffuseIntensity);

        Roughness.Initialize(tweakBar, "Roughness", "Scene Controls", "Roughness", "Specular roughness parameter for the material", 0.1000f, 0.0010f, 1.0000f, 0.0010f, ConversionMode::Square, 1.0000f);
        Settings.AddSetting(&Roughness);

        SpecularIntensity.Initialize(tweakBar, "SpecularIntensity", "Scene Controls", "Specular Intensity", "Specular intensity parameter for the material", 0.0500f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SpecularIntensity);

        EmissiveIntensity.Initialize(tweakBar, "EmissiveIntensity", "Scene Controls", "Emissive Intensity", "Emissive parameter for the material", 0.0000f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&EmissiveIntensity);

        ProbeIndex.Initialize(tweakBar, "ProbeIndex", "Scene Controls", "Probe Index", "", 0.0000f, 0.0000f, 50.0000f, 1.0000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ProbeIndex);

        ProbeX.Initialize(tweakBar, "ProbeX", "Scene Controls", "ProbeX", "", 0.0000f, -100.0000f, 100.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ProbeX);

        ProbeY.Initialize(tweakBar, "ProbeY", "Scene Controls", "ProbeY", "", 0.0000f, -100.0000f, 100.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ProbeY);

        ProbeZ.Initialize(tweakBar, "ProbeZ", "Scene Controls", "ProbeZ", "", 0.0000f, -100.0000f, 100.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ProbeZ);

        BoxSizeX.Initialize(tweakBar, "BoxSizeX", "Scene Controls", "BoxSizeX", "", 1.0000f, -100.0000f, 100.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BoxSizeX);

        BoxSizeY.Initialize(tweakBar, "BoxSizeY", "Scene Controls", "BoxSizeY", "", 1.0000f, -100.0000f, 100.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BoxSizeY);

        BoxSizeZ.Initialize(tweakBar, "BoxSizeZ", "Scene Controls", "BoxSizeZ", "", 1.0000f, -100.0000f, 100.0000f, 0.1000f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BoxSizeZ);

        SceneOrientation.Initialize(tweakBar, "SceneOrientation", "Scene Controls", "Scene Orientation", "", Quaternion(0.0000f, 0.0000f, 0.0000f, 1.0000f));
        Settings.AddSetting(&SceneOrientation);

        ModelRotationSpeed.Initialize(tweakBar, "ModelRotationSpeed", "Scene Controls", "Model Rotation Speed", "", 0.0000f, 0.0000f, 10.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ModelRotationSpeed);

        DoubleSyncInterval.Initialize(tweakBar, "DoubleSyncInterval", "Scene Controls", "Double Sync Interval", "", false);
        Settings.AddSetting(&DoubleSyncInterval);

        ExposureScale.Initialize(tweakBar, "ExposureScale", "Scene Controls", "Exposure Scale", "", 0.0000f, -16.0000f, 16.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ExposureScale);

        BloomExposure.Initialize(tweakBar, "BloomExposure", "Post Processing", "Bloom Exposure Offset", "Exposure offset applied to generate the input of the bloom pass", -4.0000f, -10.0000f, 0.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomExposure);

        BloomMagnitude.Initialize(tweakBar, "BloomMagnitude", "Post Processing", "Bloom Magnitude", "Scale factor applied to the bloom results when combined with tone-mapped result", 1.0000f, 0.0000f, 2.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomMagnitude);

        BloomBlurSigma.Initialize(tweakBar, "BloomBlurSigma", "Post Processing", "Bloom Blur Sigma", "Sigma parameter of the Gaussian filter used in the bloom pass", 5.0000f, 0.5000f, 5.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomBlurSigma);

        ManualExposure.Initialize(tweakBar, "ManualExposure", "Post Processing", "Manual Exposure", "Manual exposure value when auto-exposure is disabled", -2.5000f, -10.0000f, 10.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ManualExposure);

        TwHelper::SetOpened(tweakBar, "Anti Aliasing", true);

        TwHelper::SetOpened(tweakBar, "Scene Controls", true);

        TwHelper::SetOpened(tweakBar, "Post Processing", true);

        CBuffer.Initialize(device);
    }

    void Update()
    {
    }

    void UpdateCBuffer(ID3D11DeviceContext* context)
    {
        CBuffer.Data.MSAAMode = MSAAMode;
        CBuffer.Data.FilterType = FilterType;
        CBuffer.Data.FilterSize = FilterSize;
        CBuffer.Data.GaussianSigma = GaussianSigma;
        CBuffer.Data.CubicB = CubicB;
        CBuffer.Data.CubicC = CubicC;
        CBuffer.Data.UseStandardResolve = UseStandardResolve;
        CBuffer.Data.InverseLuminanceFiltering = InverseLuminanceFiltering;
        CBuffer.Data.UseExposureFiltering = UseExposureFiltering;
        CBuffer.Data.ExposureFilterOffset = ExposureFilterOffset;
        CBuffer.Data.UseGradientMipLevel = UseGradientMipLevel;
        CBuffer.Data.EnableTemporalAA = EnableTemporalAA;
        CBuffer.Data.TemporalAABlendFactor = TemporalAABlendFactor;
        CBuffer.Data.UseTemporalColorWeighting = UseTemporalColorWeighting;
        CBuffer.Data.ClampPrevColor = ClampPrevColor;
        CBuffer.Data.LowFreqWeight = LowFreqWeight;
        CBuffer.Data.HiFreqWeight = HiFreqWeight;
        CBuffer.Data.SharpeningAmount = SharpeningAmount;
        CBuffer.Data.CurrentScene = CurrentScene;
        CBuffer.Data.CurrentShadingTech = CurrentShadingTech;
        CBuffer.Data.LightDirection = LightDirection;
        CBuffer.Data.LightColor = LightColor;
        CBuffer.Data.EnableSSR = EnableSSR;
        CBuffer.Data.PauseSceneScript = PauseSceneScript;
        CBuffer.Data.EnableDirectLighting = EnableDirectLighting;
        CBuffer.Data.EnableIndirectDiffuseLighting = EnableIndirectDiffuseLighting;
        CBuffer.Data.EnableIndirectSpecularLighting = EnableIndirectSpecularLighting;
        CBuffer.Data.RenderBackground = RenderBackground;
        CBuffer.Data.RenderSceneObjectBBox = RenderSceneObjectBBox;
        CBuffer.Data.RenderIrradianceVolumeProbes = RenderIrradianceVolumeProbes;
        CBuffer.Data.EnableShadows = EnableShadows;
        CBuffer.Data.EnableNormalMaps = EnableNormalMaps;
        CBuffer.Data.EnableRealtimeCubemap = EnableRealtimeCubemap;
        CBuffer.Data.DiffuseGIBounces = DiffuseGIBounces;
        CBuffer.Data.DiffuseGI_Intensity = DiffuseGI_Intensity;
        CBuffer.Data.NormalMapIntensity = NormalMapIntensity;
        CBuffer.Data.DiffuseIntensity = DiffuseIntensity;
        CBuffer.Data.Roughness = Roughness;
        CBuffer.Data.SpecularIntensity = SpecularIntensity;
        CBuffer.Data.EmissiveIntensity = EmissiveIntensity;
        CBuffer.Data.ProbeIndex = ProbeIndex;
        CBuffer.Data.ProbeX = ProbeX;
        CBuffer.Data.ProbeY = ProbeY;
        CBuffer.Data.ProbeZ = ProbeZ;
        CBuffer.Data.BoxSizeX = BoxSizeX;
        CBuffer.Data.BoxSizeY = BoxSizeY;
        CBuffer.Data.BoxSizeZ = BoxSizeZ;
        CBuffer.Data.SceneOrientation = SceneOrientation;
        CBuffer.Data.ModelRotationSpeed = ModelRotationSpeed;
        CBuffer.Data.DoubleSyncInterval = DoubleSyncInterval;
        CBuffer.Data.ExposureScale = ExposureScale;
        CBuffer.Data.BloomExposure = BloomExposure;
        CBuffer.Data.BloomMagnitude = BloomMagnitude;
        CBuffer.Data.BloomBlurSigma = BloomBlurSigma;
        CBuffer.Data.ManualExposure = ManualExposure;

        CBuffer.ApplyChanges(context);
        CBuffer.SetVS(context, 7);
        CBuffer.SetHS(context, 7);
        CBuffer.SetDS(context, 7);
        CBuffer.SetGS(context, 7);
        CBuffer.SetPS(context, 7);
        CBuffer.SetCS(context, 7);
    }
}

// ================================================================================================

namespace AppSettings
{
    void UpdateUI()
    {
        ExposureFilterOffset.SetEditable(UseExposureFiltering.Value() ? true : false);

        bool enableTemporal = EnableTemporalAA.Value() ? true : false;
        JitterMode.SetEditable(enableTemporal);
        UseTemporalColorWeighting.SetEditable(enableTemporal);
        ClampPrevColor.SetEditable(enableTemporal);
        LowFreqWeight.SetEditable(enableTemporal);
        HiFreqWeight.SetEditable(enableTemporal);

        bool generalCubic = FilterType == FilterTypes::GeneralizedCubic;
        CubicB.SetEditable(generalCubic);
        CubicC.SetEditable(generalCubic);
        GaussianSigma.SetEditable(FilterType == FilterTypes::Gaussian);
    }
}
