#version 150
in vec3 in_Position;
uniform mat4 Projection;
uniform mat4 ModelView;
uniform sampler2D heightTex;
out vec2 out_TexCoord;
out vec3 out_Normal_CC;
out vec4 out_Position_CC;

void main(){
    vec2 uv = vec2(in_Position.x, in_Position.z);
    out_TexCoord = uv;
    float heightScale = 0.22;
    float height = texture(heightTex, uv).r * heightScale;
    vec3 pos = vec3(uv.x, height, uv.y);

    // Find the 2 directions on the surface via sampling the nearby heights
    vec2 texel = 1.0 / vec2(textureSize(heightTex, 0));
    float leftH = texture(heightTex, uv - vec2(texel.x, 0)).r * heightScale;
    float rightH = texture(heightTex, uv + vec2(texel.x, 0)).r * heightScale;
    float downH = texture(heightTex, uv - vec2(0, texel.y)).r * heightScale;
    float upH = texture(heightTex, uv + vec2(0, texel.y)).r * heightScale;

    vec3 across = vec3(2.0 * texel.x, rightH - leftH, 0);
    vec3 forward = vec3(0, upH - downH, 2.0 * texel.y);
    vec3 normal = normalize(cross(forward, across));
    mat3 normalMatrix = transpose(inverse(mat3(ModelView)));
    out_Normal_CC = normalize(normalMatrix * normal);
    out_Position_CC = ModelView * vec4(pos, 1.0);
    gl_Position = Projection * out_Position_CC;
}
