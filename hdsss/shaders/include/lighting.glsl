struct ShaderLight {
    // spot, point
    vec4 position;
    // spot, directional
    vec4 direction;
    // all
    vec4 color;
    float intensity;
    // point, spot
    // negative value stands for INF
    float range;
    // spot
    float spotAngle;
    int type;
};

struct SurfaceParams {
    vec3 normal;
    vec3 viewDir;
    vec3 lightDir;

    vec4 albedo;
    float shininess;
};
const int LIGHT_TYPE_SPOT = 0, LIGHT_TYPE_POINT = 1, LIGHT_TYPE_DIRECTIONAL = 2;

vec3 computeBlinnPhongLocalLighting(in SurfaceParams surfaceParams,
                                    in ShaderLight light, float intensity) {
    vec3 N = normalize(surfaceParams.normal),
         V = normalize(surfaceParams.viewDir),
         L = normalize(surfaceParams.lightDir);
    vec4 albedo = surfaceParams.albedo;

    vec3 H = normalize(V + L);

    vec3 Ld = albedo.rgb * intensity * max(dot(L, N), 0.0);
    vec3 Ls =
        max(vec3(0.0), vec3(albedo.a) * intensity *
                           pow(max(0.0, dot(H, N)), surfaceParams.shininess));
    return light.color.rgb * (Ld);
}
