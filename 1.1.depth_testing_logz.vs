#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out float flogz;  // 传递给片段着色器的对数深度值

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float Fcoef;  // = 2.0 / log2(farPlane + 1.0)

void main()
{
    TexCoords = aTexCoords;
    
    // 计算视图空间位置
    vec4 viewPos = view * model * vec4(aPos, 1.0);
    
    // 裁剪空间位置
    gl_Position = projection * viewPos;
    
    // ========================================================================
    // 对数深度缓冲 (Logarithmic Depth Buffer)
    // ========================================================================
    // 使用视图空间的 Z 值（取正，因为相机看向 -Z）
    // 计算对数深度，范围 [0, 1]
    // ========================================================================
    
    // viewPos.z 是负数（相机看向 -Z），取正
    float viewZ = -viewPos.z;
    flogz = log2(max(1e-6, viewZ + 1.0)) * Fcoef;
}
