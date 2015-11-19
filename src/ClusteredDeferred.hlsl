//=================================================================================================
// Includes
//=================================================================================================
#include "SharedConstants.h"
#include "AppSettings.hlsl"

float4 ClusteredDeferredVS(in uint VertexID : SV_VertexID) : SV_Position
{
    float4 output = 0.0f;

    if(VertexID == 0)
        output = float4(-1.0f, 1.0f, 1.0f, 1.0f);
    else if(VertexID == 1)
        output = float4(3.0f, 1.0f, 1.0f, 1.0f);
    else
        output = float4(-1.0f, -3.0f, 1.0f, 1.0f);

    return output;
}

float4 ClusteredDeferredPS(in float4 PositionSS : SV_Position) : SV_Target0
{
    return float4(0.0, 0.3, 0.7, 1.0);
}