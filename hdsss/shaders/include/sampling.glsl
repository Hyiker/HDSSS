#ifndef HDSSS_SHADERS_INCLUDE_SAMPLING_GLSL
#define HDSSS_SHADERS_INCLUDE_SAMPLING_GLSL

const int SAMPLING_CATROM = 0, SAMPLING_MITCHELL = 1;
// from https://github.com/runelite/runelite/blob/master/runelite-client/src/main/resources/net/runelite/client/plugins/gpu/scale/bicubic.glsl
// Cubic filter with Catmull-Rom parameters
float catmull_rom(float x) {
    /*
   * Generally favorable results in image upscaling are given by a cubic filter with the values b = 0 and c = 0.5.
   * This is known as the Catmull-Rom filter, and it closely approximates Jinc upscaling with Lanczos input values.
   * Placing these values into the piecewise equation gives us a more compact representation of:
   *  y = 1.5 * abs(x)^3 - 2.5 * abs(x)^2 + 1                 // abs(x) < 1
   *  y = -0.5 * abs(x)^3 + 2.5 * abs(x)^2 - 4 * abs(x) + 2   // 1 <= abs(x) < 2
   */

    float t = abs(x);
    float t2 = t * t;
    float t3 = t * t * t;

    if (t < 1)
        return 1.5 * t3 - 2.5 * t2 + 1.0;
    else if (t < 2)
        return -0.5 * t3 + 2.5 * t2 - 4.0 * t + 2.0;
    else
        return 0.0;
}

float mitchell(float x) {
    /*
   * This is another cubic filter with less aggressive sharpening than Catmull-Rom, which some users may prefer.
   * B = 1/3, C = 1/3.
   */

    float t = abs(x);
    float t2 = t * t;
    float t3 = t * t * t;

    if (t < 1)
        return 7.0 / 6.0 * t3 + -2.0 * t2 + 8.0 / 9.0;
    else if (t < 2)
        return -7.0 / 18.0 * t3 + 2.0 * t2 - 10.0 / 3.0 * t + 16.0 / 9.0;
    else
        return 0.0;
}

#define CR_AR_STRENGTH 0.9

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38

// Calculates the distance between two points
float d(vec2 pt1, vec2 pt2) {
    vec2 v = pt2 - pt1;
    return sqrt(dot(v, v));
}

// Samples a texture using a 4x4 kernel.
vec4 textureCubic(sampler2D sampler, vec2 texCoords, int mode) {
    vec2 texSize = textureSize(sampler, 0);
    vec2 texelSize = 1.0 / texSize;
    vec2 texelFCoords = texCoords * texSize;
    texelFCoords -= 0.5;

    vec4 nSum = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 nDenom = vec4(0.0, 0.0, 0.0, 0.0);

    vec2 coordFract = fract(texelFCoords);
    texCoords -= coordFract * texelSize;

    vec4 c;

    if (mode == SAMPLING_CATROM) {
        // catrom benefits from anti-ringing, which requires knowledge of the minimum and maximum samples in the kernel
        vec4 min_sample = vec4(FLT_MAX);
        vec4 max_sample = vec4(FLT_MIN);
        for (int m = -1; m <= 2; m++) {
            for (int n = -1; n <= 2; n++) {
                // this would use texelFetch, but that would require manual implementation of texture wrapping
                vec4 vecData =
                    texture(sampler, texCoords + vec2(m, n) * texelSize);

                // update min and max as we go
                min_sample = min(min_sample, vecData);
                max_sample = max(max_sample, vecData);

                // calculate weight based on distance of the current texel offset from the sub-texel position of the sampling location
                float w = catmull_rom(d(vec2(m, n), coordFract));

                // build the weighted average
                nSum += vecData * w;
                nDenom += w;
            }
        }
        // calculate weighted average
        c = nSum / nDenom;

        // store value before anti-ringing
        vec4 aux = c;
        // anti-ringing: clamp the color value so that it cannot exceed values already present in the kernel area
        c = clamp(c, min_sample, max_sample);
        // mix according to anti-ringing strength
        c = mix(aux, c, CR_AR_STRENGTH);
    } else if (mode == SAMPLING_MITCHELL) {
        for (int m = -1; m <= 2; m++) {
            for (int n = -1; n <= 2; n++) {
                // this would use texelFetch, but that would require manual implementation of texture wrapping
                vec4 vecData =
                    texture(sampler, texCoords + vec2(m, n) * texelSize);

                // calculate weight based on distance of the current texel offset from the sub-texel position of the sampling location
                float w = mitchell(d(vec2(m, n), coordFract));

                // build the weighted average
                nSum += vecData * w;
                nDenom += w;
            }
        }
        // calculate weighted average
        c = nSum / nDenom;
    }

    // return the weighted average
    return c;
}
#endif /* HDSSS_SHADERS_INCLUDE_SAMPLING_GLSL */
