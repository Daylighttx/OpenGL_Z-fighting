#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in float flogz;  // 从顶点着色器传来的对数深度，范围 [0, 1]

uniform sampler2D texture1;

void main()
{    
    FragColor = texture(texture1, TexCoords);
    
    // 直接写入对数深度（已经是 [0, 1] 范围）
    gl_FragDepth = clamp(flogz, 0.0, 1.0);
}
