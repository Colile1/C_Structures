#version 330 core
out vec4 FragColor;

uniform vec3 forceColor;
uniform float forceIntensity;

void main() {
    // Calculate the final color based on force intensity
    vec3 finalColor = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), forceIntensity);
    
    // Apply the force color
    finalColor *= forceColor;
    
    FragColor = vec4(finalColor, 1.0);
}
