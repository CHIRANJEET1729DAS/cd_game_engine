#version 330 core
out vec4 FragColor;

in vec3 FragPos;
uniform vec3 viewPos;

void main()
{
    float distance = length(FragPos - viewPos);
    float alpha = 1.0 - smoothstep(20.0, 100.0, distance);
    
    if (alpha <= 0.0)
        discard;
        
    FragColor = vec4(0.0, 1.0, 0.0, alpha); // Neon Green with fade
}
