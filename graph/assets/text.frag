#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform sampler2D fontTexture;
uniform vec3 textColor;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(fontTexture, fragTexCoord);
    
    // Utiliza o canal alpha da textura para o texto
    // Isso assume que é um texto em branco em um fundo transparente na textura
    fragColor = vec4(textColor, texColor.a);
}