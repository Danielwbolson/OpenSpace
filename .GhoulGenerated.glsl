
#version 460  core

    in vec2 texCoords;
    in vec2 outlineTexCoords;
    in float depth;
    in vec3 vsPosition;

    out vec4 FragColor;
    out vec4 gPosition;
    out vec4 gNormal;

    uniform sampler2D tex;
    uniform vec4 baseColor;
    uniform vec4 outlineColor;
    uniform bool hasOutline;
    uniform bool enableFalseDepth;
    uniform bool disableTransmittance;

    void main() {
        if (hasOutline) {
            float inside = texture(tex, texCoords).r;
            float outline = texture(tex, outlineTexCoords).r;
            vec4 blend = mix(outlineColor, baseColor, inside);
            FragColor = blend * vec4(1.0, 1.0, 1.0, max(inside, outline));
        }
        else {
            FragColor = vec4(baseColor.rgb, baseColor.a * texture(tex, texCoords).r);
        }
        if (FragColor.a < 0.1) {
            discard;
        }
        if (enableFalseDepth) {
            gl_FragDepth = 0.0;
        }
        else {
            if (depth > 1.0) {
                gl_FragDepth = depth / pow(10, 30);
            }
            else {
                gl_FragDepth = depth - 1.0;
            }
        }
        if (disableTransmittance) {
            gPosition = vec4(0.0, 0.0, -1.0, 1.0);
        }
        else {
            gPosition = vec4(vsPosition, 1.0);
        }
        // 4th coord of the gNormal is the water reflectance
        gNormal = vec4(0.0, 0.0, 1.0, 0.0);
    }
