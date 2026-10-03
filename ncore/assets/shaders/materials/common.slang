public struct SceneData
{
    public uint32_t RenderWidth;
    public uint32_t RenderHeight;
    // The current camera's transform.
    public float4x4 CameraMatrix;
    // The current camera's inverse matrix pre-multiplied with the projection matrix.
    public float4x4 ViewProjMatrix;
    // Elapsed time in seconds.
    public float Time;
    // Elapsed time between frames.
    public float DeltaTime;
    // Near/far clipping planes of the current camera.
    public float ZNear;
    public float ZFar;
};

[[vk::binding(0, 2)]] public ParameterBlock<SceneData> g_Scene;

#define TIME g_Scene.Time
#define DELTA_TIME g_Scene.DeltaTime

#define CAMERA_MATRIX g_Scene.CameraMatrix
#define CAMERA_POSITION g_Scene.CameraMatrix._14_24_34
#define Z_CLIP_FAR g_Scene.ZFar
#define Z_CLIP_NEAR g_Scene.ZNear
