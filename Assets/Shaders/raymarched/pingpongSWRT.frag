#version 460 core

out vec4 FragColor;
in highp vec2 texCoord;

layout(location = 0) out vec4 oindirect;
layout(location = 2) out vec4 oemission;

uniform sampler2D dIndirect;
uniform sampler2D dEmission;

void main(){
    oindirect = texture(dIndirect, texCoord);
    oemission = texture(dEmission, texCoord);
}