#version 150
in vec2 out_TexCoord;
in vec3 out_Normal_CC;
in vec4 out_Position_CC;
uniform sampler2D colorTex;
uniform int showNormals;
out vec4 fragColor;

void main(){
    vec3 normal = normalize(out_Normal_CC);

    if(showNormals == 1){
        //Normal components go from -1 -> 1 so you just move them to 0 -> 1
        fragColor = vec4(normal * 0.5 + 0.5, 1.0);
    } 
    else{
        vec3 lightDir = normalize(-out_Position_CC.xyz);
        float diffuse = max(dot(normal, lightDir), 0.0);
        float light = 0.35 + 0.65 * diffuse;
        vec3 color = texture(colorTex, out_TexCoord).rgb;
        fragColor = vec4(color * light, 1.0);
    }
}
