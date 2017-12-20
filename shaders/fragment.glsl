#version 330 core

#define FOG_DENSITY 0.08

in vec3 vFragPosition;
in vec2 vTexCoords;
in vec3 vNormal;

out vec4 color;

uniform sampler2D ourTexture1;
uniform int draw_normals;
uniform int is_grass;


float fog_exp2(
  const float dist,
  const float density
) {
  const float LOG2 = -1.442695;
  float d = density * dist;
  return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

void main()
{
  vec3 lightDir = vec3(1.0f, 1.0f, 0.0f); 

  vec3 col = vec3(0.0f, 0.9f, 0.75f);

  float kd = max(dot(vNormal, lightDir), 0.0);

  if (draw_normals == 0){
    float fogDistance = gl_FragCoord.z / gl_FragCoord.w;
    float fogAmount = fog_exp2(fogDistance, FOG_DENSITY);
    vec4 fogColor = vec4(0.92, 0.92, 0.92, 1.0);
    if ( is_grass == 1 ){
      kd = kd - 0.1;
    }
    vec4 color2 = vec4(kd * texture(ourTexture1, vTexCoords));
    gl_FragColor = mix(color2, fogColor, fogAmount);
  } else {
    vec3 vNormal2 = normalize(vNormal);
    gl_FragColor  = vec4( (vNormal2.x+1.0f)*0.5f, (vNormal2.y+1.0f)*0.5f, (vNormal2.z+1.0f)*0.5f, 1.0f  );
  }

}
