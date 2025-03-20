#version 330 core
out vec4 FragColor;
uniform vec3 forceColor;

void main() {
    FragColor = vec4(forceColor, 1.0);
}
