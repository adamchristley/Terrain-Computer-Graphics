#version 150
in vec2 out_TexCoord;
uniform sampler2D cloudTex;
out vec4 fragColor;

void main(){
    float opacity = texture(cloudTex, out_TexCoord).r;
    fragColor = vec4(1.0, 1.0, 1.0, opacity);
}
