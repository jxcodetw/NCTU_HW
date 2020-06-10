#version 400

uniform sampler2D Tex;
uniform sampler2D BumpTex;
in vec3 position_v;
in vec3 normal_v;
in vec3 light_pos;
in vec3 eye_pos;
in vec2 texcoord;
in vec3 tangent;
in vec3 bitangent;

in vec4 ambient_L;
in vec4 diffuse_L;
in vec4 specular_L;

in float Shiness;
in float enablebump;

out vec4 outColor;
//there should be a out vec4 in fragment shader defining the output color of fragment shader(variable name can be arbitrary)
void main() {
	//outColor = gl_Color;
	vec3 red = vec3(1, 0, 0);
	vec3 L = normalize(light_pos - position_v);
	vec3 V = normalize(eye_pos - position_v);
	vec3 d = normalize(position_v - light_pos);
	vec4 tex = texture2D(Tex, texcoord);
	mat3 TBN = mat3(tangent, bitangent, normal_v);

	vec3 bump_normal = normal_v;
	if (enablebump > 0) {
		bump_normal = normalize(normalize(TBN * (2 * texture2D(BumpTex, texcoord).rgb - 1)) + normal_v);
	}
	
	vec3 I = vec3(0, 0, 0);
	I += ambient_L.xyz;
	float dot1 = dot(bump_normal, L);
	if (dot1 > 0) {
		I += diffuse_L.rgb * clamp(dot1, 0, 1);
	}


	I = I * vec3(tex);

	vec3 R = normalize(-L + 2 * dot(L, bump_normal) * bump_normal);
	float dot2 = dot(R, V);
	if (dot2 > 0) {
		I += specular_L.rgb * pow(dot2, Shiness);
	}
	
	outColor = vec4(I, 1);
	//outColor = vec4(bump_normal, 1);
}