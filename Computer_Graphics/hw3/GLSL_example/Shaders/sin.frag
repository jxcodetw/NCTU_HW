#version 400

in vec2 position_v;

out vec4 outColor;
//there should be a out vec4 in fragment shader defining the output color of fragment shader(variable name can be arbitrary)
void main() {
	outColor = vec4(1.0, position_v, 1.0);
}