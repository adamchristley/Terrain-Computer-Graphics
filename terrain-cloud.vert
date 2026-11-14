#version 150
in vec3 in_Position;
uniform mat4 Projection;
uniform mat4 ModelView;
out vec2 out_TexCoord;

void main(){
    out_TexCoord = vec2(in_Position.x, in_Position.z);
    gl_Position = Projection * ModelView * vec4(in_Position, 1.0);
}
