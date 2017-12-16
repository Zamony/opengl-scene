#version 330 core
in vec3 vFragPosition;
in vec2 vTexCoords;
in vec3 vNormal;

out vec4 color;

uniform sampler2D ourTexture1;
uniform int draw_normals;

void main()
{
  vec3 lightDir = vec3(1.0f, 1.0f, 0.0f); 

  vec3 col = vec3(0.0f, 0.9f, 0.75f);

  float kd = max(dot(vNormal, lightDir), 0.0);

  if (draw_normals == 0){
    color = vec4(kd * texture(ourTexture1, vTexCoords));
  } else {
    vec3 vNormal2 = normalize(vNormal);
    color  = vec4( (vNormal2.x+1.0f)*0.5f, (vNormal2.y+1.0f)*0.5f, (vNormal2.z+1.0f)*0.5f, 1.0f  );
  }
}
