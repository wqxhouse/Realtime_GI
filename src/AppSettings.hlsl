cbuffer AppSettings : register(b7)
{
    int MSAAMode;
    int FilterType;
    float FilterSize;
    float GaussianSigma;
    float CubicB;
    float CubicC;
    bool UseStandardResolve;
    bool InverseLuminanceFiltering;
    bool UseExposureFiltering;
    float ExposureFilterOffset;
    bool UseGradientMipLevel;
    bool EnableTemporalAA;
    float TemporalAABlendFactor;
    bool UseTemporalColorWeighting;
    bool ClampPrevColor;
    float LowFreqWeight;
    float HiFreqWeight;
    float SharpeningAmount;
    int CurrentScene;
    int CurrentShadingTech;
    float3 LightDirection;
    float3 LightColor;
    float3 SkyColor;
    bool EnableSSR;
    bool PauseSceneScript;
    bool EnableDirectLighting;
    bool EnableIndirectDiffuseLighting;
    bool EnableIndirectSpecularLighting;
    bool RenderBackground;
    bool RenderSceneObjectBBox;
    bool RenderProbeBBox;
    bool RenderIrradianceVolumeProbes;
    bool EnableShadows;
    bool EnableNormalMaps;
    bool EnableRealtimeCubemap;
    int DiffuseGIBounces;
    float DiffuseGI_Intensity;
    float NormalMapIntensity;
    float DiffuseIntensity;
    float Roughness;
    float SpecularIntensity;
    float EmissiveIntensity;
    float ProbeIndex;
    float ProbeX;
    float ProbeY;
    float ProbeZ;
    float BoxSizeX;
    float BoxSizeY;
    float BoxSizeZ;
    float4 SceneOrientation;
    float ModelRotationSpeed;
    bool DoubleSyncInterval;
    float ExposureScale;
    float BloomExposure;
    float BloomMagnitude;
    float BloomBlurSigma;
    float ManualExposure;
}

static const int MSAAModes_MSAANone = 0;
static const int MSAAModes_MSAA2x = 1;
static const int MSAAModes_MSAA4x = 2;
static const int MSAAModes_MSAA8x = 3;

static const int FilterTypes_Box = 0;
static const int FilterTypes_Triangle = 1;
static const int FilterTypes_Gaussian = 2;
static const int FilterTypes_BlackmanHarris = 3;
static const int FilterTypes_Smoothstep = 4;
static const int FilterTypes_BSpline = 5;
static const int FilterTypes_CatmullRom = 6;
static const int FilterTypes_Mitchell = 7;
static const int FilterTypes_GeneralizedCubic = 8;
static const int FilterTypes_Sinc = 9;

static const int JitterModes_None = 0;
static const int JitterModes_Uniform2x = 1;
static const int JitterModes_Hammersly16 = 2;

static const int Scenes_CornellBox = 0;
static const int Scenes_DropBoxes = 1;
static const int Scenes_Sponza = 2;

static const int ShadingTech_Forward = 0;
static const int ShadingTech_Clustered_Deferred = 1;

static const bool EnableAutoExposure = false;
static const float KeyValue = 0.1150f;
static const float AdaptationRate = 0.5000f;
