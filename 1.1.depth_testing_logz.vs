#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out float flogz;  // 传递给片段着色器的对数深度值

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float Fcoef;  // = 1.0 / log2(farPlane + 1.0)

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
    // 公式: depth = log2(z + 1) / log2(far + 1)
    // 当 z = 0 时, depth = 0
    // 当 z = far 时, depth = 1
    // ========================================================================
    
    // viewPos.z 是负数（相机看向 -Z），取正
    float viewZ = -viewPos.z;
    
    // 直接计算 [0, 1] 范围的深度值
    flogz = log2(max(1e-6, viewZ + 1.0)) * Fcoef;
}
