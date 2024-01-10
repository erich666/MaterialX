#version 400

struct BSDF { vec3 response; vec3 throughput; float thickness; float ior; };
#define EDF vec3
struct surfaceshader { vec3 color; vec3 transparency; };
struct volumeshader { vec3 color; vec3 transparency; };
struct displacementshader { vec3 offset; float scale; };
struct lightshader { vec3 intensity; vec3 direction; };
#define material surfaceshader

// Uniform block: PublicUniforms
uniform displacementshader displacementshader1;
uniform int geomprop_UV0_index = 0;
uniform sampler2D mtlximage21_file;
uniform int mtlximage21_layer = 0;
uniform float mtlximage21_default = 0.000000;
uniform int mtlximage21_uaddressmode = 2;
uniform int mtlximage21_vaddressmode = 2;
uniform int mtlximage21_filtertype = 1;
uniform int mtlximage21_framerange = 0;
uniform int mtlximage21_frameoffset = 0;
uniform int mtlximage21_frameendaction = 0;
uniform vec2 mtlximage21_uv_scale = vec2(1.000000, 1.000000);
uniform vec2 mtlximage21_uv_offset = vec2(0.000000, 0.000000);
uniform sampler2D mtlximage20_file;
uniform int mtlximage20_layer = 0;
uniform vec3 mtlximage20_default = vec3(0.000000, 0.000000, 0.000000);
uniform int mtlximage20_uaddressmode = 2;
uniform int mtlximage20_vaddressmode = 2;
uniform int mtlximage20_filtertype = 1;
uniform int mtlximage20_framerange = 0;
uniform int mtlximage20_frameoffset = 0;
uniform int mtlximage20_frameendaction = 0;
uniform vec2 mtlximage20_uv_scale = vec2(1.000000, 1.000000);
uniform vec2 mtlximage20_uv_offset = vec2(0.000000, 0.000000);
uniform int mtlxnormalmap15_space = 0;
uniform vec2 mtlxnormalmap15_scale = vec2(1.000000, 1.000000);
uniform float Pawn_Top_W_base = 1.000000;
uniform vec3 Pawn_Top_W_base_color = vec3(1.000000, 1.000000, 1.000000);
uniform float Pawn_Top_W_diffuse_roughness = 0.000000;
uniform float Pawn_Top_W_metalness = 0.000000;
uniform float Pawn_Top_W_specular = 1.000000;
uniform vec3 Pawn_Top_W_specular_color = vec3(1.000000, 1.000000, 1.000000);
uniform float Pawn_Top_W_specular_IOR = 1.500000;
uniform float Pawn_Top_W_specular_anisotropy = 0.000000;
uniform float Pawn_Top_W_specular_rotation = 0.000000;
uniform float Pawn_Top_W_transmission = 1.000000;
uniform vec3 Pawn_Top_W_transmission_color = vec3(1.000000, 1.000000, 0.828000);
uniform float Pawn_Top_W_transmission_depth = 0.000000;
uniform vec3 Pawn_Top_W_transmission_scatter = vec3(0.000000, 0.000000, 0.000000);
uniform float Pawn_Top_W_transmission_scatter_anisotropy = 0.000000;
uniform float Pawn_Top_W_transmission_dispersion = 0.000000;
uniform float Pawn_Top_W_transmission_extra_roughness = 0.000000;
uniform float Pawn_Top_W_subsurface = 0.000000;
uniform vec3 Pawn_Top_W_subsurface_color = vec3(1.000000, 1.000000, 1.000000);
uniform vec3 Pawn_Top_W_subsurface_radius = vec3(1.000000, 1.000000, 1.000000);
uniform float Pawn_Top_W_subsurface_scale = 0.003000;
uniform float Pawn_Top_W_subsurface_anisotropy = 0.000000;
uniform float Pawn_Top_W_sheen = 0.000000;
uniform vec3 Pawn_Top_W_sheen_color = vec3(1.000000, 1.000000, 1.000000);
uniform float Pawn_Top_W_sheen_roughness = 0.300000;
uniform float Pawn_Top_W_coat = 0.000000;
uniform vec3 Pawn_Top_W_coat_color = vec3(1.000000, 1.000000, 1.000000);
uniform float Pawn_Top_W_coat_roughness = 0.100000;
uniform float Pawn_Top_W_coat_anisotropy = 0.000000;
uniform float Pawn_Top_W_coat_rotation = 0.000000;
uniform float Pawn_Top_W_coat_IOR = 1.500000;
uniform float Pawn_Top_W_coat_affect_color = 0.000000;
uniform float Pawn_Top_W_coat_affect_roughness = 0.000000;
uniform float Pawn_Top_W_thin_film_thickness = 0.000000;
uniform float Pawn_Top_W_thin_film_IOR = 1.500000;
uniform float Pawn_Top_W_emission = 0.000000;
uniform vec3 Pawn_Top_W_emission_color = vec3(1.000000, 1.000000, 1.000000);
uniform vec3 Pawn_Top_W_opacity = vec3(1.000000, 1.000000, 1.000000);
uniform bool Pawn_Top_W_thin_walled = false;

// Uniform block: PrivateUniforms
uniform mat4 u_envMatrix = mat4(-1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, -1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000);
uniform sampler2D u_envRadiance;
uniform int u_envRadianceMips = 1;
uniform int u_envRadianceSamples = 16;
uniform sampler2D u_envIrradiance;
uniform bool u_refractionTwoSided = false;
uniform vec3 u_viewPosition = vec3(0.0);
uniform int u_numActiveLightSources = 0;

in VertexData
{
    vec3 normalWorld;
    vec3 tangentWorld;
    vec2 texcoord_0;
    vec3 positionWorld;
} vd;

// Pixel shader outputs
out vec4 out1;

#define M_FLOAT_EPS 1e-8

float mx_square(float x)
{
    return x*x;
}

vec2 mx_square(vec2 x)
{
    return x*x;
}

vec3 mx_square(vec3 x)
{
    return x*x;
}

#define DIRECTIONAL_ALBEDO_METHOD 0

#define MAX_LIGHT_SOURCES 3
#define M_PI 3.1415926535897932
#define M_PI_INV (1.0 / M_PI)

float mx_pow5(float x)
{
    return mx_square(mx_square(x)) * x;
}

// Standard Schlick Fresnel
float mx_fresnel_schlick(float cosTheta, float F0)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    float x5 = mx_pow5(x);
    return F0 + (1.0 - F0) * x5;
}
vec3 mx_fresnel_schlick(float cosTheta, vec3 F0)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    float x5 = mx_pow5(x);
    return F0 + (1.0 - F0) * x5;
}

// Generalized Schlick Fresnel
float mx_fresnel_schlick(float cosTheta, float F0, float F90)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    float x5 = mx_pow5(x);
    return mix(F0, F90, x5);
}
vec3 mx_fresnel_schlick(float cosTheta, vec3 F0, vec3 F90)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    float x5 = mx_pow5(x);
    return mix(F0, F90, x5);
}

// Generalized Schlick Fresnel with a variable exponent
float mx_fresnel_schlick(float cosTheta, float F0, float F90, float exponent)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    return mix(F0, F90, pow(x, exponent));
}
vec3 mx_fresnel_schlick(float cosTheta, vec3 F0, vec3 F90, float exponent)
{
    float x = clamp(1.0 - cosTheta, 0.0, 1.0);
    return mix(F0, F90, pow(x, exponent));
}

// Enforce that the given normal is forward-facing from the specified view direction.
vec3 mx_forward_facing_normal(vec3 N, vec3 V)
{
    return (dot(N, V) < 0.0) ? -N : N;
}

// https://www.graphics.rwth-aachen.de/publication/2/jgt.pdf
float mx_golden_ratio_sequence(int i)
{
    const float GOLDEN_RATIO = 1.6180339887498948;
    return fract((float(i) + 1.0) * GOLDEN_RATIO);
}

// https://people.irisa.fr/Ricardo.Marques/articles/2013/SF_CGF.pdf
vec2 mx_spherical_fibonacci(int i, int numSamples)
{
    return vec2((float(i) + 0.5) / float(numSamples), mx_golden_ratio_sequence(i));
}

// Generate a uniform-weighted sample in the unit hemisphere.
vec3 mx_uniform_sample_hemisphere(vec2 Xi)
{
    float phi = 2.0 * M_PI * Xi.x;
    float cosTheta = 1.0 - Xi.y;
    float sinTheta = sqrt(1.0 - mx_square(cosTheta));
    return vec3(cos(phi) * sinTheta,
                sin(phi) * sinTheta,
                cosTheta);
}

// Fresnel model options.
const int FRESNEL_MODEL_DIELECTRIC = 0;
const int FRESNEL_MODEL_CONDUCTOR = 1;
const int FRESNEL_MODEL_SCHLICK = 2;
const int FRESNEL_MODEL_AIRY = 3;
const int FRESNEL_MODEL_SCHLICK_AIRY = 4;

// XYZ to CIE 1931 RGB color space (using neutral E illuminant)
const mat3 XYZ_TO_RGB = mat3(2.3706743, -0.5138850, 0.0052982, -0.9000405, 1.4253036, -0.0146949, -0.4706338, 0.0885814, 1.0093968);

// Parameters for Fresnel calculations.
struct FresnelData
{
    int model;

    // Physical Fresnel
    vec3 ior;
    vec3 extinction;

    // Generalized Schlick Fresnel
    vec3 F0;
    vec3 F90;
    float exponent;

    // Thin film
    float tf_thickness;
    float tf_ior;

    // Refraction
    bool refraction;

#ifdef __METAL__ 
FresnelData(int   _model        = 0, 
            vec3  _ior          = vec3(0.0f),
            vec3  _extinction   = vec3(0.0f),
            vec3  _F0           = vec3(0.0f),
            vec3  _F90          = vec3(0.0f),
            float _exponent     = 0.0f,
            float _tf_thickness = 0.0f,
            float _tf_ior       = 0.0f,
            bool  _refraction   = false) : 
                model(_model),
                ior(_ior),
                extinction(_extinction),
                F0(_F0), F90(_F90), exponent(_exponent),
                tf_thickness(_tf_thickness),
                tf_ior(_tf_ior),
                refraction(_refraction) {}
#endif

};

// https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf
// Appendix B.2 Equation 13
float mx_ggx_NDF(vec3 H, vec2 alpha)
{
    vec2 He = H.xy / alpha;
    float denom = dot(He, He) + mx_square(H.z);
    return 1.0 / (M_PI * alpha.x * alpha.y * mx_square(denom));
}

// https://ggx-research.github.io/publication/2023/06/09/publication-ggx.html
vec3 mx_ggx_importance_sample_VNDF(vec2 Xi, vec3 V, vec2 alpha)
{
    // Transform the view direction to the hemisphere configuration.
    V = normalize(vec3(V.xy * alpha, V.z));

    // Sample a spherical cap in (-V.z, 1].
    float phi = 2.0 * M_PI * Xi.x;
    float z = (1.0 - Xi.y) * (1.0 + V.z) - V.z;
    float sinTheta = sqrt(clamp(1.0 - z * z, 0.0, 1.0));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    vec3 c = vec3(x, y, z);

    // Compute the microfacet normal.
    vec3 H = c + V;

    // Transform the microfacet normal back to the ellipsoid configuration.
    H = normalize(vec3(H.xy * alpha, max(H.z, 0.0)));

    return H;
}

// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
// Equation 34
float mx_ggx_smith_G1(float cosTheta, float alpha)
{
    float cosTheta2 = mx_square(cosTheta);
    float tanTheta2 = (1.0 - cosTheta2) / cosTheta2;
    return 2.0 / (1.0 + sqrt(1.0 + mx_square(alpha) * tanTheta2));
}

// Height-correlated Smith masking-shadowing
// http://jcgt.org/published/0003/02/03/paper.pdf
// Equations 72 and 99
float mx_ggx_smith_G2(float NdotL, float NdotV, float alpha)
{
    float alpha2 = mx_square(alpha);
    float lambdaL = sqrt(alpha2 + (1.0 - alpha2) * mx_square(NdotL));
    float lambdaV = sqrt(alpha2 + (1.0 - alpha2) * mx_square(NdotV));
    return 2.0 / (lambdaL / NdotL + lambdaV / NdotV);
}

// Rational quadratic fit to Monte Carlo data for GGX directional albedo.
vec3 mx_ggx_dir_albedo_analytic(float NdotV, float alpha, vec3 F0, vec3 F90)
{
    float x = NdotV;
    float y = alpha;
    float x2 = mx_square(x);
    float y2 = mx_square(y);
    vec4 r = vec4(0.1003, 0.9345, 1.0, 1.0) +
             vec4(-0.6303, -2.323, -1.765, 0.2281) * x +
             vec4(9.748, 2.229, 8.263, 15.94) * y +
             vec4(-2.038, -3.748, 11.53, -55.83) * x * y +
             vec4(29.34, 1.424, 28.96, 13.08) * x2 +
             vec4(-8.245, -0.7684, -7.507, 41.26) * y2 +
             vec4(-26.44, 1.436, -36.11, 54.9) * x2 * y +
             vec4(19.99, 0.2913, 15.86, 300.2) * x * y2 +
             vec4(-5.448, 0.6286, 33.37, -285.1) * x2 * y2;
    vec2 AB = clamp(r.xy / r.zw, 0.0, 1.0);
    return F0 * AB.x + F90 * AB.y;
}

vec3 mx_ggx_dir_albedo_table_lookup(float NdotV, float alpha, vec3 F0, vec3 F90)
{
#if DIRECTIONAL_ALBEDO_METHOD == 1
    if (textureSize(u_albedoTable, 0).x > 1)
    {
        vec2 AB = texture(u_albedoTable, vec2(NdotV, alpha)).rg;
        return F0 * AB.x + F90 * AB.y;
    }
#endif
    return vec3(0.0);
}

// https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
vec3 mx_ggx_dir_albedo_monte_carlo(float NdotV, float alpha, vec3 F0, vec3 F90)
{
    NdotV = clamp(NdotV, M_FLOAT_EPS, 1.0);
    vec3 V = vec3(sqrt(1.0 - mx_square(NdotV)), 0, NdotV);

    vec2 AB = vec2(0.0);
    const int SAMPLE_COUNT = 64;
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        vec2 Xi = mx_spherical_fibonacci(i, SAMPLE_COUNT);

        // Compute the half vector and incoming light direction.
        vec3 H = mx_ggx_importance_sample_VNDF(Xi, V, vec2(alpha));
        vec3 L = -reflect(V, H);
        
        // Compute dot products for this sample.
        float NdotL = clamp(L.z, M_FLOAT_EPS, 1.0);
        float VdotH = clamp(dot(V, H), M_FLOAT_EPS, 1.0);

        // Compute the Fresnel term.
        float Fc = mx_fresnel_schlick(VdotH, 0.0, 1.0);

        // Compute the per-sample geometric term.
        // https://hal.inria.fr/hal-00996995v2/document, Algorithm 2
        float G2 = mx_ggx_smith_G2(NdotL, NdotV, alpha);
        
        // Add the contribution of this sample.
        AB += vec2(G2 * (1.0 - Fc), G2 * Fc);
    }

    // Apply the global component of the geometric term and normalize.
    AB /= mx_ggx_smith_G1(NdotV, alpha) * float(SAMPLE_COUNT);

    // Return the final directional albedo.
    return F0 * AB.x + F90 * AB.y;
}

vec3 mx_ggx_dir_albedo(float NdotV, float alpha, vec3 F0, vec3 F90)
{
#if DIRECTIONAL_ALBEDO_METHOD == 0
    return mx_ggx_dir_albedo_analytic(NdotV, alpha, F0, F90);
#elif DIRECTIONAL_ALBEDO_METHOD == 1
    return mx_ggx_dir_albedo_table_lookup(NdotV, alpha, F0, F90);
#else
    return mx_ggx_dir_albedo_monte_carlo(NdotV, alpha, F0, F90);
#endif
}

float mx_ggx_dir_albedo(float NdotV, float alpha, float F0, float F90)
{
    return mx_ggx_dir_albedo(NdotV, alpha, vec3(F0), vec3(F90)).x;
}

// https://blog.selfshadow.com/publications/turquin/ms_comp_final.pdf
// Equations 14 and 16
vec3 mx_ggx_energy_compensation(float NdotV, float alpha, vec3 Fss)
{
    float Ess = mx_ggx_dir_albedo(NdotV, alpha, 1.0, 1.0);
    return 1.0 + Fss * (1.0 - Ess) / Ess;
}

float mx_ggx_energy_compensation(float NdotV, float alpha, float Fss)
{
    return mx_ggx_energy_compensation(NdotV, alpha, vec3(Fss)).x;
}

// Compute the average of an anisotropic alpha pair.
float mx_average_alpha(vec2 alpha)
{
    return sqrt(alpha.x * alpha.y);
}

// Convert a real-valued index of refraction to normal-incidence reflectivity.
float mx_ior_to_f0(float ior)
{
    return mx_square((ior - 1.0) / (ior + 1.0));
}

// Convert normal-incidence reflectivity to real-valued index of refraction.
float mx_f0_to_ior(float F0)
{
    float sqrtF0 = sqrt(clamp(F0, 0.01, 0.99));
    return (1.0 + sqrtF0) / (1.0 - sqrtF0);
}

vec3 mx_f0_to_ior_colored(vec3 F0)
{
    vec3 sqrtF0 = sqrt(clamp(F0, 0.01, 0.99));
    return (vec3(1.0) + sqrtF0) / (vec3(1.0) - sqrtF0);
}

// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
float mx_fresnel_dielectric(float cosTheta, float ior)
{
    if (cosTheta < 0.0)
        return 1.0;

    float g =  ior*ior + cosTheta*cosTheta - 1.0;
    // Check for total internal reflection
    if (g < 0.0)
        return 1.0;

    g = sqrt(g);
    float gmc = g - cosTheta;
    float gpc = g + cosTheta;
    float x = gmc / gpc;
    float y = (gpc * cosTheta - 1.0) / (gmc * cosTheta + 1.0);
    return 0.5 * x * x * (1.0 + y * y);
}

void mx_fresnel_dielectric_polarized(float cosTheta, float n, out float Rp, out float Rs)
{
    if (cosTheta < 0.0) {
        Rp = 1.0;
        Rs = 1.0;
        return;
    }

    float cosTheta2 = cosTheta * cosTheta;
    float sinTheta2 = 1.0 - cosTheta2;
    float n2 = n * n;

    float t0 = n2 - sinTheta2;
    float a2plusb2 = sqrt(t0 * t0);
    float t1 = a2plusb2 + cosTheta2;
    float a = sqrt(max(0.5 * (a2plusb2 + t0), 0.0));
    float t2 = 2.0 * a * cosTheta;
    Rs = (t1 - t2) / (t1 + t2);

    float t3 = cosTheta2 * a2plusb2 + sinTheta2 * sinTheta2;
    float t4 = t2 * sinTheta2;
    Rp = Rs * (t3 - t4) / (t3 + t4);
}

void mx_fresnel_dielectric_polarized(float cosTheta, float eta1, float eta2, out float Rp, out float Rs)
{
    float n = eta2 / eta1;
    mx_fresnel_dielectric_polarized(cosTheta, n, Rp, Rs);
}

void mx_fresnel_conductor_polarized(float cosTheta, vec3 n, vec3 k, out vec3 Rp, out vec3 Rs)
{
    cosTheta = clamp(cosTheta, 0.0, 1.0);
    float cosTheta2 = cosTheta * cosTheta;
    float sinTheta2 = 1.0 - cosTheta2;
    vec3 n2 = n * n;
    vec3 k2 = k * k;

    vec3 t0 = n2 - k2 - vec3(sinTheta2);
    vec3 a2plusb2 = sqrt(t0 * t0 + 4.0 * n2 * k2);
    vec3 t1 = a2plusb2 + vec3(cosTheta2);
    vec3 a = sqrt(max(0.5 * (a2plusb2 + t0), 0.0));
    vec3 t2 = 2.0 * a * cosTheta;
    Rs = (t1 - t2) / (t1 + t2);

    vec3 t3 = cosTheta2 * a2plusb2 + vec3(sinTheta2 * sinTheta2);
    vec3 t4 = t2 * sinTheta2;
    Rp = Rs * (t3 - t4) / (t3 + t4);
}

void mx_fresnel_conductor_polarized(float cosTheta, float eta1, vec3 eta2, vec3 kappa2, out vec3 Rp, out vec3 Rs)
{
    vec3 n = eta2 / eta1;
    vec3 k = kappa2 / eta1;
    mx_fresnel_conductor_polarized(cosTheta, n, k, Rp, Rs);
}

vec3 mx_fresnel_conductor(float cosTheta, vec3 n, vec3 k)
{
    vec3 Rp, Rs;
    mx_fresnel_conductor_polarized(cosTheta, n, k, Rp, Rs);
    return 0.5 * (Rp  + Rs);
}

// Phase shift due to a dielectric material
void mx_fresnel_dielectric_phase_polarized(float cosTheta, float eta1, float eta2, out float phiP, out float phiS)
{
    float cosB = cos(atan(eta2 / eta1));    // Brewster's angle
    if (eta2 > eta1) {
        phiP = cosTheta < cosB ? M_PI : 0.0f;
        phiS = 0.0f;
    } else {
        phiP = cosTheta < cosB ? 0.0f : M_PI;
        phiS = M_PI;
    }
}

// Phase shift due to a conducting material
void mx_fresnel_conductor_phase_polarized(float cosTheta, float eta1, vec3 eta2, vec3 kappa2, out vec3 phiP, out vec3 phiS)
{
    if (dot(kappa2, kappa2) == 0.0 && eta2.x == eta2.y && eta2.y == eta2.z) {
        // Use dielectric formula to increase performance
        float phiPx, phiSx;
        mx_fresnel_dielectric_phase_polarized(cosTheta, eta1, eta2.x, phiPx, phiSx);
        phiP = vec3(phiPx, phiPx, phiPx);
        phiS = vec3(phiSx, phiSx, phiSx);
        return;
    }
    vec3 k2 = kappa2 / eta2;
    vec3 sinThetaSqr = vec3(1.0) - cosTheta * cosTheta;
    vec3 A = eta2*eta2*(vec3(1.0)-k2*k2) - eta1*eta1*sinThetaSqr;
    vec3 B = sqrt(A*A + mx_square(2.0*eta2*eta2*k2));
    vec3 U = sqrt((A+B)/2.0);
    vec3 V = max(vec3(0.0), sqrt((B-A)/2.0));

    phiS = atan(2.0*eta1*V*cosTheta, U*U + V*V - mx_square(eta1*cosTheta));
    phiP = atan(2.0*eta1*eta2*eta2*cosTheta * (2.0*k2*U - (vec3(1.0)-k2*k2) * V),
                mx_square(eta2*eta2*(vec3(1.0)+k2*k2)*cosTheta) - eta1*eta1*(U*U+V*V));
}

// Evaluation XYZ sensitivity curves in Fourier space
vec3 mx_eval_sensitivity(float opd, vec3 shift)
{
    // Use Gaussian fits, given by 3 parameters: val, pos and var
    float phase = 2.0*M_PI * opd;
    vec3 val = vec3(5.4856e-13, 4.4201e-13, 5.2481e-13);
    vec3 pos = vec3(1.6810e+06, 1.7953e+06, 2.2084e+06);
    vec3 var = vec3(4.3278e+09, 9.3046e+09, 6.6121e+09);
    vec3 xyz = val * sqrt(2.0*M_PI * var) * cos(pos * phase + shift) * exp(- var * phase*phase);
    xyz.x   += 9.7470e-14 * sqrt(2.0*M_PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift[0]) * exp(- 4.5282e+09 * phase*phase);
    return xyz / 1.0685e-7;
}

// A Practical Extension to Microfacet Theory for the Modeling of Varying Iridescence
// https://belcour.github.io/blog/research/publication/2017/05/01/brdf-thin-film.html
vec3 mx_fresnel_airy(float cosTheta, vec3 ior, vec3 extinction, float tf_thickness, float tf_ior,
                                     vec3 f0, vec3 f90, float exponent, bool use_schlick)
{
    // Convert nm -> m
    float d = tf_thickness * 1.0e-9;

    // Assume vacuum on the outside
    float eta1 = 1.0;
    float eta2 = max(tf_ior, eta1);
    vec3 eta3   = use_schlick ? mx_f0_to_ior_colored(f0) : ior;
    vec3 kappa3 = use_schlick ? vec3(0.0)                : extinction;

    // Compute the Spectral versions of the Fresnel reflectance and
    // transmitance for each interface.
    float R12p, T121p, R12s, T121s;
    vec3 R23p, R23s;
    
    // Reflected and transmitted parts in the thin film
    mx_fresnel_dielectric_polarized(cosTheta, eta1, eta2, R12p, R12s);

    // Reflected part by the base
    float scale = eta1 / eta2;
    float cosThetaTSqr = 1.0 - (1.0-cosTheta*cosTheta) * scale*scale;
    float cosTheta2 = sqrt(cosThetaTSqr);
    if (use_schlick)
    {
        vec3 f = mx_fresnel_schlick(cosTheta2, f0, f90, exponent);
        R23p = 0.5 * f;
        R23s = 0.5 * f;
    }
    else
    {
        mx_fresnel_conductor_polarized(cosTheta2, eta2, eta3, kappa3, R23p, R23s);
    }

    // Check for total internal reflection
    if (cosThetaTSqr <= 0.0f)
    {
        R12s = 1.0;
        R12p = 1.0;
    }

    // Compute the transmission coefficients
    T121p = 1.0 - R12p;
    T121s = 1.0 - R12s;

    // Optical path difference
    float D = 2.0 * eta2 * d * cosTheta2;

    float phi21p, phi21s;
    vec3 phi23p, phi23s, r123s, r123p;

    // Evaluate the phase shift
    mx_fresnel_dielectric_phase_polarized(cosTheta, eta1, eta2, phi21p, phi21s);
    if (use_schlick)
    {
        phi23p = vec3(
            (eta3[0] < eta2) ? M_PI : 0.0,
            (eta3[1] < eta2) ? M_PI : 0.0,
            (eta3[2] < eta2) ? M_PI : 0.0);
        phi23s = phi23p;
    }
    else
    {
        mx_fresnel_conductor_phase_polarized(cosTheta2, eta2, eta3, kappa3, phi23p, phi23s);
    }

    phi21p = M_PI - phi21p;
    phi21s = M_PI - phi21s;

    r123p = max(vec3(0.0), sqrt(R12p*R23p));
    r123s = max(vec3(0.0), sqrt(R12s*R23s));

    // Evaluate iridescence term
    vec3 I = vec3(0.0);
    vec3 C0, Cm, Sm;

    // Iridescence term using spectral antialiasing for Parallel polarization

    vec3 S0 = vec3(1.0);

    // Reflectance term for m=0 (DC term amplitude)
    vec3 Rs = (T121p*T121p*R23p) / (vec3(1.0) - R12p*R23p);
    C0 = R12p + Rs;
    I += C0 * S0;

    // Reflectance term for m>0 (pairs of diracs)
    Cm = Rs - T121p;
    for (int m=1; m<=2; ++m)
    {
        Cm *= r123p;
        Sm  = 2.0 * mx_eval_sensitivity(float(m)*D, float(m)*(phi23p+vec3(phi21p)));
        I  += Cm*Sm;
    }

    // Iridescence term using spectral antialiasing for Perpendicular polarization

    // Reflectance term for m=0 (DC term amplitude)
    vec3 Rp = (T121s*T121s*R23s) / (vec3(1.0) - R12s*R23s);
    C0 = R12s + Rp;
    I += C0 * S0;

    // Reflectance term for m>0 (pairs of diracs)
    Cm = Rp - T121s ;
    for (int m=1; m<=2; ++m)
    {
        Cm *= r123s;
        Sm  = 2.0 * mx_eval_sensitivity(float(m)*D, float(m)*(phi23s+vec3(phi21s)));
        I  += Cm*Sm;
    }

    // Average parallel and perpendicular polarization
    I *= 0.5;

    // Convert back to RGB reflectance
    I = clamp(XYZ_TO_RGB * I, vec3(0.0), vec3(1.0));

    return I;
}

FresnelData mx_init_fresnel_data(int model)
{
    return FresnelData(model, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), 0.0, 0.0, 0.0, false);
}

FresnelData mx_init_fresnel_dielectric(float ior)
{
    FresnelData fd = mx_init_fresnel_data(FRESNEL_MODEL_DIELECTRIC);
    fd.ior = vec3(ior);
    return fd;
}

FresnelData mx_init_fresnel_conductor(vec3 ior, vec3 extinction)
{
    FresnelData fd = mx_init_fresnel_data(FRESNEL_MODEL_CONDUCTOR);
    fd.ior = ior;
    fd.extinction = extinction;
    return fd;
}

FresnelData mx_init_fresnel_schlick(vec3 F0)
{
    FresnelData fd = mx_init_fresnel_data(FRESNEL_MODEL_SCHLICK);
    fd.F0 = F0;
    fd.F90 = vec3(1.0);
    fd.exponent = 5.0f;
    return fd;
}

FresnelData mx_init_fresnel_schlick(vec3 F0, vec3 F90, float exponent)
{
    FresnelData fd = mx_init_fresnel_data(FRESNEL_MODEL_SCHLICK);
    fd.F0 = F0;
    fd.F90 = F90;
    fd.exponent = exponent;
    return fd;
}

FresnelData mx_init_fresnel_schlick_airy(vec3 F0, vec3 F90, float exponent, float tf_thickness, float tf_ior)
{
    FresnelData fd = mx_init_fresnel_data(FRESNEL_MODEL_SCHLICK_AIRY);
    fd.F0 = F0;
    fd.F90 = F90;
    fd.exponent = exponent;
    fd.tf_thickness = tf_thickness;
    fd.tf_ior = tf_ior;
    return fd;
}

FresnelData mx_init_fresnel_dielectric_airy(float ior, float tf_thickness, float tf_ior)
{
    FresnelData fd = mx_init_fresnel_data(FRESNEL_MODEL_AIRY);
    fd.ior = vec3(ior);
    fd.tf_thickness = tf_thickness;
    fd.tf_ior = tf_ior;
    return fd;
}

FresnelData mx_init_fresnel_conductor_airy(vec3 ior, vec3 extinction, float tf_thickness, float tf_ior)
{
    FresnelData fd = mx_init_fresnel_data(FRESNEL_MODEL_AIRY);
    fd.ior = ior;
    fd.extinction = extinction;
    fd.tf_thickness = tf_thickness;
    fd.tf_ior = tf_ior;
    return fd;
}

vec3 mx_compute_fresnel(float cosTheta, FresnelData fd)
{
    if (fd.model == FRESNEL_MODEL_DIELECTRIC)
    {
        return vec3(mx_fresnel_dielectric(cosTheta, fd.ior.x));
    }
    else if (fd.model == FRESNEL_MODEL_CONDUCTOR)
    {
        return mx_fresnel_conductor(cosTheta, fd.ior, fd.extinction);
    }
    else if (fd.model == FRESNEL_MODEL_SCHLICK)
    {
        return mx_fresnel_schlick(cosTheta, fd.F0, fd.F90, fd.exponent);
    }
    else
    {
        return mx_fresnel_airy(cosTheta, fd.ior, fd.extinction, fd.tf_thickness, fd.tf_ior,
                                         fd.F0, fd.F90, fd.exponent,
                                         fd.model == FRESNEL_MODEL_SCHLICK_AIRY);
    }
}

// Compute the refraction of a ray through a solid sphere.
vec3 mx_refraction_solid_sphere(vec3 R, vec3 N, float ior)
{
    R = refract(R, N, 1.0 / ior);
    vec3 N1 = normalize(R * dot(R, N) - N * 0.5);
    return refract(R, N1, ior);
}

vec2 mx_latlong_projection(vec3 dir)
{
    float latitude = -asin(dir.y) * M_PI_INV + 0.5;
    float longitude = atan(dir.x, -dir.z) * M_PI_INV * 0.5 + 0.5;
    return vec2(longitude, latitude);
}

vec3 mx_latlong_map_lookup(vec3 dir, mat4 transform, float lod, sampler2D envSampler)
{
    vec3 envDir = normalize((transform * vec4(dir,0.0)).xyz);
    vec2 uv = mx_latlong_projection(envDir);
    return textureLod(envSampler, uv, lod).rgb;
}

// Return the mip level with the appropriate coverage for a filtered importance sample.
// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
// Section 20.4 Equation 13
float mx_latlong_compute_lod(vec3 dir, float pdf, float maxMipLevel, int envSamples)
{
    const float MIP_LEVEL_OFFSET = 1.5;
    float effectiveMaxMipLevel = maxMipLevel - MIP_LEVEL_OFFSET;
    float distortion = sqrt(1.0 - mx_square(dir.y));
    return max(effectiveMaxMipLevel - 0.5 * log2(float(envSamples) * pdf * distortion), 0.0);
}

// Return the mip level associated with the given alpha in a prefiltered environment.
float mx_latlong_alpha_to_lod(float alpha)
{
    float lodBias = (alpha < 0.25) ? sqrt(alpha) : 0.5 * alpha + 0.375;
    return lodBias * float(u_envRadianceMips - 1);
}

// Return the alpha associated with the given mip level in a prefiltered environment.
float mx_latlong_lod_to_alpha(float lod)
{
    float lodBias = lod / float(u_envRadianceMips - 1);
    return (lodBias < 0.5) ? mx_square(lodBias) : 2.0 * (lodBias - 0.375);
}

vec3 mx_environment_radiance(vec3 N, vec3 V, vec3 X, vec2 alpha, int distribution, FresnelData fd)
{
    // Generate tangent frame.
    X = normalize(X - dot(X, N) * N);
    vec3 Y = cross(N, X);
    mat3 tangentToWorld = mat3(X, Y, N);

    // Transform the view vector to tangent space.
    V = vec3(dot(V, X), dot(V, Y), dot(V, N));

    // Compute derived properties.
    float NdotV = clamp(V.z, M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(alpha);
    float G1V = mx_ggx_smith_G1(NdotV, avgAlpha);
    
    // Integrate outgoing radiance using filtered importance sampling.
    // http://cgg.mff.cuni.cz/~jaroslav/papers/2008-egsr-fis/2008-egsr-fis-final-embedded.pdf
    vec3 radiance = vec3(0.0);
    int envRadianceSamples = u_envRadianceSamples;
    for (int i = 0; i < envRadianceSamples; i++)
    {
        vec2 Xi = mx_spherical_fibonacci(i, envRadianceSamples);

        // Compute the half vector and incoming light direction.
        vec3 H = mx_ggx_importance_sample_VNDF(Xi, V, alpha);
        vec3 L = fd.refraction ? mx_refraction_solid_sphere(-V, H, fd.ior.x) : -reflect(V, H);
        
        // Compute dot products for this sample.
        float NdotL = clamp(L.z, M_FLOAT_EPS, 1.0);
        float VdotH = clamp(dot(V, H), M_FLOAT_EPS, 1.0);

        // Sample the environment light from the given direction.
        vec3 Lw = tangentToWorld * L;
        float pdf = mx_ggx_NDF(H, alpha) * G1V / (4.0 * NdotV);
        float lod = mx_latlong_compute_lod(Lw, pdf, float(u_envRadianceMips - 1), envRadianceSamples);
        vec3 sampleColor = mx_latlong_map_lookup(Lw, u_envMatrix, lod, u_envRadiance);

        // Compute the Fresnel term.
        vec3 F = mx_compute_fresnel(VdotH, fd);

        // Compute the geometric term.
        float G = mx_ggx_smith_G2(NdotL, NdotV, avgAlpha);

        // Compute the combined FG term, which is inverted for refraction.
        vec3 FG = fd.refraction ? vec3(1.0) - (F * G) : F * G;

        // Add the radiance contribution of this sample.
        // From https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
        //   incidentLight = sampleColor * NdotL
        //   microfacetSpecular = D * F * G / (4 * NdotL * NdotV)
        //   pdf = D * G1V / (4 * NdotV);
        //   radiance = incidentLight * microfacetSpecular / pdf
        radiance += sampleColor * FG;
    }

    // Apply the global component of the geometric term and normalize.
    radiance /= G1V * float(envRadianceSamples);

    // Return the final radiance.
    return radiance;
}

vec3 mx_environment_irradiance(vec3 N)
{
    return mx_latlong_map_lookup(N, u_envMatrix, 0.0, u_envIrradiance);
}


vec3 mx_surface_transmission(vec3 N, vec3 V, vec3 X, vec2 alpha, int distribution, FresnelData fd, vec3 tint)
{
    // Approximate the appearance of surface transmission as glossy
    // environment map refraction, ignoring any scene geometry that might
    // be visible through the surface.
    fd.refraction = true;
    if (u_refractionTwoSided)
    {
        tint = mx_square(tint);
    }
    return mx_environment_radiance(N, V, X, alpha, distribution, fd) * tint;
}

struct LightData
{
    int type;
};

uniform LightData u_lightData[MAX_LIGHT_SOURCES];

int numActiveLightSources()
{
    return min(u_numActiveLightSources, MAX_LIGHT_SOURCES) ;
}

void sampleLightSource(LightData light, vec3 position, out lightshader result)
{
    result.intensity = vec3(0.0);
    result.direction = vec3(0.0);
}

vec2 mx_transform_uv(vec2 uv, vec2 uv_scale, vec2 uv_offset)
{
    uv = uv * uv_scale + uv_offset;
    return uv;
}

void mx_image_float(sampler2D tex_sampler, int layer, float defaultval, vec2 texcoord, int uaddressmode, int vaddressmode, int filtertype, int framerange, int frameoffset, int frameendaction, vec2 uv_scale, vec2 uv_offset, out float result)
{
    vec2 uv = mx_transform_uv(texcoord, uv_scale, uv_offset);
    result = texture(tex_sampler, uv).r;
}


void mx_image_vector3(sampler2D tex_sampler, int layer, vec3 defaultval, vec2 texcoord, int uaddressmode, int vaddressmode, int filtertype, int framerange, int frameoffset, int frameendaction, vec2 uv_scale, vec2 uv_offset, out vec3 result)
{
    vec2 uv = mx_transform_uv(texcoord, uv_scale, uv_offset);
    result = texture(tex_sampler, uv).rgb;
}

void mx_normalmap_vector2(vec3 value, int map_space, vec2 normal_scale, vec3 N, vec3 T,  out vec3 result)
{
    // Decode the normal map.
    value = (value == vec3(0.0f)) ? vec3(0.0, 0.0, 1.0) : value * 2.0 - 1.0;

    // Transform from tangent space if needed.
    if (map_space == 0)
    {
        vec3 B = normalize(cross(N, T));
        value.xy *= normal_scale;
        value = T * value.x + B * value.y + N * value.z;
    }

    // Normalize the result.
    result = normalize(value);
}

void mx_normalmap_float(vec3 value, int map_space, float normal_scale, vec3 N, vec3 T,  out vec3 result)
{
    mx_normalmap_vector2(value, map_space, vec2(normal_scale), N, T, result);
}

void mx_roughness_anisotropy(float roughness, float anisotropy, out vec2 result)
{
    float roughness_sqr = clamp(roughness*roughness, M_FLOAT_EPS, 1.0);
    if (anisotropy > 0.0)
    {
        float aspect = sqrt(1.0 - clamp(anisotropy, 0.0, 0.98));
        result.x = min(roughness_sqr / aspect, 1.0);
        result.y = roughness_sqr * aspect;
    }
    else
    {
        result.x = roughness_sqr;
        result.y = roughness_sqr;
    }
}


// http://www.aconty.com/pdf/s2017_pbs_imageworks_sheen.pdf
// Equation 2
float mx_imageworks_sheen_NDF(float NdotH, float roughness)
{
    float invRoughness = 1.0 / max(roughness, 0.005);
    float cos2 = NdotH * NdotH;
    float sin2 = 1.0 - cos2;
    return (2.0 + invRoughness) * pow(sin2, invRoughness * 0.5) / (2.0 * M_PI);
}

float mx_imageworks_sheen_brdf(float NdotL, float NdotV, float NdotH, float roughness)
{
    // Microfacet distribution.
    float D = mx_imageworks_sheen_NDF(NdotH, roughness);

    // Fresnel and geometry terms are ignored.
    float F = 1.0;
    float G = 1.0;

    // We use a smoother denominator, as in:
    // https://blog.selfshadow.com/publications/s2013-shading-course/rad/s2013_pbs_rad_notes.pdf
    return D * F * G / (4.0 * (NdotL + NdotV - NdotL*NdotV));
}

// Rational quadratic fit to Monte Carlo data for Imageworks sheen directional albedo.
float mx_imageworks_sheen_dir_albedo_analytic(float NdotV, float roughness)
{
    vec2 r = vec2(13.67300, 1.0) +
             vec2(-68.78018, 61.57746) * NdotV +
             vec2(799.08825, 442.78211) * roughness +
             vec2(-905.00061, 2597.49308) * NdotV * roughness +
             vec2(60.28956, 121.81241) * mx_square(NdotV) +
             vec2(1086.96473, 3045.55075) * mx_square(roughness);
    return r.x / r.y;
}

float mx_imageworks_sheen_dir_albedo_table_lookup(float NdotV, float roughness)
{
#if DIRECTIONAL_ALBEDO_METHOD == 1
    if (textureSize(u_albedoTable, 0).x > 1)
    {
        return texture(u_albedoTable, vec2(NdotV, roughness)).b;
    }
#endif
    return 0.0;
}

float mx_imageworks_sheen_dir_albedo_monte_carlo(float NdotV, float roughness)
{
    NdotV = clamp(NdotV, M_FLOAT_EPS, 1.0);
    vec3 V = vec3(sqrt(1.0f - mx_square(NdotV)), 0, NdotV);

    float radiance = 0.0;
    const int SAMPLE_COUNT = 64;
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        vec2 Xi = mx_spherical_fibonacci(i, SAMPLE_COUNT);

        // Compute the incoming light direction and half vector.
        vec3 L = mx_uniform_sample_hemisphere(Xi);
        vec3 H = normalize(L + V);
        
        // Compute dot products for this sample.
        float NdotL = clamp(L.z, M_FLOAT_EPS, 1.0);
        float NdotH = clamp(H.z, M_FLOAT_EPS, 1.0);

        // Compute sheen reflectance.
        float reflectance = mx_imageworks_sheen_brdf(NdotL, NdotV, NdotH, roughness);

        // Add the radiance contribution of this sample.
        //   uniform_pdf = 1 / (2 * PI)
        //   radiance = reflectance * NdotL / uniform_pdf;
        radiance += reflectance * NdotL * 2.0 * M_PI;
    }

    // Return the final directional albedo.
    return radiance / float(SAMPLE_COUNT);
}

float mx_imageworks_sheen_dir_albedo(float NdotV, float roughness)
{
#if DIRECTIONAL_ALBEDO_METHOD == 0
    float dirAlbedo = mx_imageworks_sheen_dir_albedo_analytic(NdotV, roughness);
#elif DIRECTIONAL_ALBEDO_METHOD == 1
    float dirAlbedo = mx_imageworks_sheen_dir_albedo_table_lookup(NdotV, roughness);
#else
    float dirAlbedo = mx_imageworks_sheen_dir_albedo_monte_carlo(NdotV, roughness);
#endif
    return clamp(dirAlbedo, 0.0, 1.0);
}

void mx_sheen_bsdf_reflection(vec3 L, vec3 V, vec3 P, float occlusion, float weight, vec3 color, float roughness, vec3 N, inout BSDF bsdf)
{
    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    N = mx_forward_facing_normal(N, V);

    vec3 H = normalize(L + V);

    float NdotL = clamp(dot(N, L), M_FLOAT_EPS, 1.0);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    float NdotH = clamp(dot(N, H), M_FLOAT_EPS, 1.0);

    vec3 fr = color * mx_imageworks_sheen_brdf(NdotL, NdotV, NdotH, roughness);
    float dirAlbedo = mx_imageworks_sheen_dir_albedo(NdotV, roughness);
    bsdf.throughput = vec3(1.0 - dirAlbedo * weight);

    // We need to include NdotL from the light integral here
    // as in this case it's not cancelled out by the BRDF denominator.
    bsdf.response = fr * NdotL * occlusion * weight;
}

void mx_sheen_bsdf_indirect(vec3 V, float weight, vec3 color, float roughness, vec3 N, inout BSDF bsdf)
{
    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    N = mx_forward_facing_normal(N, V);

    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);

    float dirAlbedo = mx_imageworks_sheen_dir_albedo(NdotV, roughness);
    bsdf.throughput = vec3(1.0 - dirAlbedo * weight);

    vec3 Li = mx_environment_irradiance(N);
    bsdf.response = Li * color * dirAlbedo * weight;
}

void mx_luminance_color3(vec3 _in, vec3 lumacoeffs, out vec3 result)
{
    result = vec3(dot(_in, lumacoeffs));
}

mat4 mx_rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void mx_rotate_vector3(vec3 _in, float amount, vec3 axis, out vec3 result)
{
    float rotationRadians = radians(amount);
    mat4 m = mx_rotationMatrix(axis, rotationRadians);
    result = (m * vec4(_in, 1.0)).xyz;
}

void mx_artistic_ior(vec3 reflectivity, vec3 edge_color, out vec3 ior, out vec3 extinction)
{
    // "Artist Friendly Metallic Fresnel", Ole Gulbrandsen, 2014
    // http://jcgt.org/published/0003/04/03/paper.pdf

    vec3 r = clamp(reflectivity, 0.0, 0.99);
    vec3 r_sqrt = sqrt(r);
    vec3 n_min = (1.0 - r) / (1.0 + r);
    vec3 n_max = (1.0 + r_sqrt) / (1.0 - r_sqrt);
    ior = mix(n_max, n_min, edge_color);

    vec3 np1 = ior + 1.0;
    vec3 nm1 = ior - 1.0;
    vec3 k2 = (np1*np1 * r - nm1*nm1) / (1.0 - r);
    k2 = max(k2, 0.0);
    extinction = sqrt(k2);
}

void mx_uniform_edf(vec3 N, vec3 L, vec3 color, out EDF result)
{
    result = color;
}


void mx_dielectric_bsdf_reflection(vec3 L, vec3 V, vec3 P, float occlusion, float weight, vec3 tint, float ior, vec2 roughness, vec3 N, vec3 X, int distribution, int scatter_mode, inout BSDF bsdf)
{
    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    N = mx_forward_facing_normal(N, V);

    X = normalize(X - dot(X, N) * N);
    vec3 Y = cross(N, X);
    vec3 H = normalize(L + V);

    float NdotL = clamp(dot(N, L), M_FLOAT_EPS, 1.0);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    float VdotH = clamp(dot(V, H), M_FLOAT_EPS, 1.0);

    vec2 safeAlpha = clamp(roughness, M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(safeAlpha);
    vec3 Ht = vec3(dot(H, X), dot(H, Y), dot(H, N));

    FresnelData fd;
    if (bsdf.thickness > 0.0)
    { 
        fd = mx_init_fresnel_dielectric_airy(ior, bsdf.thickness, bsdf.ior);
    }
    else
    {
        fd = mx_init_fresnel_dielectric(ior);
    }
    vec3  F = mx_compute_fresnel(VdotH, fd);
    float D = mx_ggx_NDF(Ht, safeAlpha);
    float G = mx_ggx_smith_G2(NdotL, NdotV, avgAlpha);

    float F0 = mx_ior_to_f0(ior);
    vec3 comp = mx_ggx_energy_compensation(NdotV, avgAlpha, F);
    vec3 dirAlbedo = mx_ggx_dir_albedo(NdotV, avgAlpha, F0, 1.0) * comp;
    bsdf.throughput = 1.0 - dirAlbedo * weight;

    // Note: NdotL is cancelled out
    bsdf.response = D * F * G * comp * tint * occlusion * weight / (4.0 * NdotV);
}

void mx_dielectric_bsdf_transmission(vec3 V, float weight, vec3 tint, float ior, vec2 roughness, vec3 N, vec3 X, int distribution, int scatter_mode, inout BSDF bsdf)
{
    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    N = mx_forward_facing_normal(N, V);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);

    FresnelData fd;
    if (bsdf.thickness > 0.0)
    { 
        fd = mx_init_fresnel_dielectric_airy(ior, bsdf.thickness, bsdf.ior);
    }
    else
    {
        fd = mx_init_fresnel_dielectric(ior);
    }
    vec3 F = mx_compute_fresnel(NdotV, fd);

    vec2 safeAlpha = clamp(roughness, M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(safeAlpha);

    float F0 = mx_ior_to_f0(ior);
    vec3 comp = mx_ggx_energy_compensation(NdotV, avgAlpha, F);
    vec3 dirAlbedo = mx_ggx_dir_albedo(NdotV, avgAlpha, F0, 1.0) * comp;
    bsdf.throughput = 1.0 - dirAlbedo * weight;

    if (scatter_mode != 0)
    {
        bsdf.response = mx_surface_transmission(N, V, X, safeAlpha, distribution, fd, tint) * weight;
    }
}

void mx_dielectric_bsdf_indirect(vec3 V, float weight, vec3 tint, float ior, vec2 roughness, vec3 N, vec3 X, int distribution, int scatter_mode, inout BSDF bsdf)
{
    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    N = mx_forward_facing_normal(N, V);

    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);

    FresnelData fd;
    if (bsdf.thickness > 0.0)
    { 
        fd = mx_init_fresnel_dielectric_airy(ior, bsdf.thickness, bsdf.ior);
    }
    else
    {
        fd = mx_init_fresnel_dielectric(ior);
    }
    vec3 F = mx_compute_fresnel(NdotV, fd);

    vec2 safeAlpha = clamp(roughness, M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(safeAlpha);

    float F0 = mx_ior_to_f0(ior);
    vec3 comp = mx_ggx_energy_compensation(NdotV, avgAlpha, F);
    vec3 dirAlbedo = mx_ggx_dir_albedo(NdotV, avgAlpha, F0, 1.0) * comp;
    bsdf.throughput = 1.0 - dirAlbedo * weight;

    vec3 Li = mx_environment_radiance(N, V, X, safeAlpha, distribution, fd);
    bsdf.response = Li * tint * comp * weight;
}


void mx_conductor_bsdf_reflection(vec3 L, vec3 V, vec3 P, float occlusion, float weight, vec3 ior_n, vec3 ior_k, vec2 roughness, vec3 N, vec3 X, int distribution, inout BSDF bsdf)
{
    bsdf.throughput = vec3(0.0);

    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    N = mx_forward_facing_normal(N, V);

    X = normalize(X - dot(X, N) * N);
    vec3 Y = cross(N, X);
    vec3 H = normalize(L + V);

    float NdotL = clamp(dot(N, L), M_FLOAT_EPS, 1.0);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    float VdotH = clamp(dot(V, H), M_FLOAT_EPS, 1.0);

    vec2 safeAlpha = clamp(roughness, M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(safeAlpha);
    vec3 Ht = vec3(dot(H, X), dot(H, Y), dot(H, N));

    FresnelData fd;
    if (bsdf.thickness > 0.0)
        fd = mx_init_fresnel_conductor_airy(ior_n, ior_k, bsdf.thickness, bsdf.ior);
    else
        fd = mx_init_fresnel_conductor(ior_n, ior_k);

    vec3 F = mx_compute_fresnel(VdotH, fd);
    float D = mx_ggx_NDF(Ht, safeAlpha);
    float G = mx_ggx_smith_G2(NdotL, NdotV, avgAlpha);

    vec3 comp = mx_ggx_energy_compensation(NdotV, avgAlpha, F);

    // Note: NdotL is cancelled out
    bsdf.response = D * F * G * comp * occlusion * weight / (4.0 * NdotV);
}

void mx_conductor_bsdf_indirect(vec3 V, float weight, vec3 ior_n, vec3 ior_k, vec2 roughness, vec3 N, vec3 X, int distribution, inout BSDF bsdf)
{
    bsdf.throughput = vec3(0.0);

    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    N = mx_forward_facing_normal(N, V);

    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);

    FresnelData fd;
    if (bsdf.thickness > 0.0)
        fd = mx_init_fresnel_conductor_airy(ior_n, ior_k, bsdf.thickness, bsdf.ior);
    else
        fd = mx_init_fresnel_conductor(ior_n, ior_k);

    vec3 F = mx_compute_fresnel(NdotV, fd);

    vec2 safeAlpha = clamp(roughness, M_FLOAT_EPS, 1.0);
    float avgAlpha = mx_average_alpha(safeAlpha);
    vec3 comp = mx_ggx_energy_compensation(NdotV, avgAlpha, F);

    vec3 Li = mx_environment_radiance(N, V, X, safeAlpha, distribution, fd);

    bsdf.response = Li * comp * weight;
}

// We fake diffuse transmission by using diffuse reflection from the opposite side.
// So this BTDF is really a BRDF.
void mx_translucent_bsdf_reflection(vec3 L, vec3 V, vec3 P, float occlusion, float weight, vec3 color, vec3 normal, inout BSDF bsdf)
{
    bsdf.throughput = vec3(0.0);

    // Invert normal since we're transmitting light from the other side
    float NdotL = dot(L, -normal);
    if (NdotL <= 0.0 || weight < M_FLOAT_EPS)
    {
        return;
    }

    bsdf.response = color * weight * NdotL * M_PI_INV;
}

void mx_translucent_bsdf_indirect(vec3 V, float weight, vec3 color, vec3 normal, inout BSDF bsdf)
{
    bsdf.throughput = vec3(0.0);

    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    // Invert normal since we're transmitting light from the other side
    vec3 Li = mx_environment_irradiance(-normal);
    bsdf.response = Li * color * weight;
}


// Based on the OSL implementation of Oren-Nayar diffuse, which is in turn
// based on https://mimosa-pudica.net/improved-oren-nayar.html.
float mx_oren_nayar_diffuse(vec3 L, vec3 V, vec3 N, float NdotL, float roughness)
{
    float LdotV = clamp(dot(L, V), M_FLOAT_EPS, 1.0);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    float s = LdotV - NdotL * NdotV;
    float stinv = (s > 0.0f) ? s / max(NdotL, NdotV) : 0.0;

    float sigma2 = mx_square(roughness * M_PI);
    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * sigma2 / (sigma2 + 0.09);

    return A + B * stinv;
}

// https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf
// Section 5.3
float mx_burley_diffuse(vec3 L, vec3 V, vec3 N, float NdotL, float roughness)
{
    vec3 H = normalize(L + V);
    float LdotH = clamp(dot(L, H), M_FLOAT_EPS, 1.0);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);

    float F90 = 0.5 + (2.0 * roughness * mx_square(LdotH));
    float refL = mx_fresnel_schlick(NdotL, 1.0, F90);
    float refV = mx_fresnel_schlick(NdotV, 1.0, F90);
    return refL * refV;
}

// Compute the directional albedo component of Burley diffuse for the given
// view angle and roughness.  Curve fit provided by Stephen Hill.
float mx_burley_diffuse_dir_albedo(float NdotV, float roughness)
{
    float x = NdotV;
    float fit0 = 0.97619 - 0.488095 * mx_pow5(1.0 - x);
    float fit1 = 1.55754 + (-2.02221 + (2.56283 - 1.06244 * x) * x) * x;
    return mix(fit0, fit1, roughness);
}

// Evaluate the Burley diffusion profile for the given distance and diffusion shape.
// Based on https://graphics.pixar.com/library/ApproxBSSRDF/
vec3 mx_burley_diffusion_profile(float dist, vec3 shape)
{
    vec3 num1 = exp(-shape * dist);
    vec3 num2 = exp(-shape * dist / 3.0);
    float denom = max(dist, M_FLOAT_EPS);
    return (num1 + num2) / denom;
}

// Integrate the Burley diffusion profile over a sphere of the given radius.
// Inspired by Eric Penner's presentation in http://advances.realtimerendering.com/s2011/
vec3 mx_integrate_burley_diffusion(vec3 N, vec3 L, float radius, vec3 mfp)
{
    float theta = acos(dot(N, L));

    // Estimate the Burley diffusion shape from mean free path.
    vec3 shape = vec3(1.0) / max(mfp, 0.1);

    // Integrate the profile over the sphere.
    vec3 sumD = vec3(0.0);
    vec3 sumR = vec3(0.0);
    const int SAMPLE_COUNT = 32;
    const float SAMPLE_WIDTH = (2.0 * M_PI) / float(SAMPLE_COUNT);
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        float x = -M_PI + (float(i) + 0.5) * SAMPLE_WIDTH;
        float dist = radius * abs(2.0 * sin(x * 0.5));
        vec3 R = mx_burley_diffusion_profile(dist, shape);
        sumD += R * max(cos(theta + x), 0.0);
        sumR += R;
    }

    return sumD / sumR;
}

vec3 mx_subsurface_scattering_approx(vec3 N, vec3 L, vec3 P, vec3 albedo, vec3 mfp)
{
    float curvature = length(fwidth(N)) / length(fwidth(P));
    float radius = 1.0 / max(curvature, 0.01);
    return albedo * mx_integrate_burley_diffusion(N, L, radius, mfp) / vec3(M_PI);
}

void mx_subsurface_bsdf_reflection(vec3 L, vec3 V, vec3 P, float occlusion, float weight, vec3 color, vec3 radius, float anisotropy, vec3 normal, inout BSDF bsdf)
{
    bsdf.throughput = vec3(0.0);

    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    normal = mx_forward_facing_normal(normal, V);

    vec3 sss = mx_subsurface_scattering_approx(normal, L, P, color, radius);
    float NdotL = clamp(dot(normal, L), M_FLOAT_EPS, 1.0);
    float visibleOcclusion = 1.0 - NdotL * (1.0 - occlusion);
    bsdf.response = sss * visibleOcclusion * weight;
}

void mx_subsurface_bsdf_indirect(vec3 V, float weight, vec3 color, vec3 radius, float anisotropy, vec3 normal, inout BSDF bsdf)
{
    bsdf.throughput = vec3(0.0);

    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    normal = mx_forward_facing_normal(normal, V);

    // For now, we render indirect subsurface as simple indirect diffuse.
    vec3 Li = mx_environment_irradiance(normal);
    bsdf.response = Li * color * weight;
}


void mx_oren_nayar_diffuse_bsdf_reflection(vec3 L, vec3 V, vec3 P, float occlusion, float weight, vec3 color, float roughness, vec3 normal, inout BSDF bsdf)
{
    bsdf.throughput = vec3(0.0);

    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    normal = mx_forward_facing_normal(normal, V);

    float NdotL = clamp(dot(normal, L), M_FLOAT_EPS, 1.0);

    bsdf.response = color * occlusion * weight * NdotL * M_PI_INV;
    if (roughness > 0.0)
    {
        bsdf.response *= mx_oren_nayar_diffuse(L, V, normal, NdotL, roughness);
    }
}

void mx_oren_nayar_diffuse_bsdf_indirect(vec3 V, float weight, vec3 color, float roughness, vec3 normal, inout BSDF bsdf)
{
    bsdf.throughput = vec3(0.0);

    if (weight < M_FLOAT_EPS)
    {
        return;
    }

    normal = mx_forward_facing_normal(normal, V);

    vec3 Li = mx_environment_irradiance(normal);
    bsdf.response = Li * color * weight;
}


void mx_generalized_schlick_edf(vec3 N, vec3 V, vec3 color0, vec3 color90, float exponent, EDF base, out EDF result)
{
    N = mx_forward_facing_normal(N, V);
    float NdotV = clamp(dot(N, V), M_FLOAT_EPS, 1.0);
    vec3 f = mx_fresnel_schlick(NdotV, color0, color90, exponent);
    result = base * f;
}

void NG_standard_surface_surfaceshader_100(float base, vec3 base_color, float diffuse_roughness, float metalness, float specular, vec3 specular_color, float specular_roughness, float specular_IOR, float specular_anisotropy, float specular_rotation, float transmission, vec3 transmission_color, float transmission_depth, vec3 transmission_scatter, float transmission_scatter_anisotropy, float transmission_dispersion, float transmission_extra_roughness, float subsurface, vec3 subsurface_color, vec3 subsurface_radius, float subsurface_scale, float subsurface_anisotropy, float sheen, vec3 sheen_color, float sheen_roughness, float coat, vec3 coat_color, float coat_roughness, float coat_anisotropy, float coat_rotation, float coat_IOR, vec3 coat_normal, float coat_affect_color, float coat_affect_roughness, float thin_film_thickness, float thin_film_IOR, float emission, vec3 emission_color, vec3 opacity, bool thin_walled, vec3 normal, vec3 tangent, out surfaceshader out1)
{
    vec2 coat_roughness_vector_out = vec2(0.0);
    mx_roughness_anisotropy(coat_roughness, coat_anisotropy, coat_roughness_vector_out);
    const float coat_tangent_rotate_degree_in2_tmp = 360.000000;
    float coat_tangent_rotate_degree_out = coat_rotation * coat_tangent_rotate_degree_in2_tmp;
    vec3 metal_reflectivity_out = base_color * base;
    vec3 metal_edgecolor_out = specular_color * specular;
    float coat_affect_roughness_multiply1_out = coat_affect_roughness * coat;
    const float tangent_rotate_degree_in2_tmp = 360.000000;
    float tangent_rotate_degree_out = specular_rotation * tangent_rotate_degree_in2_tmp;
    float transmission_roughness_add_out = specular_roughness + transmission_extra_roughness;
    const float subsurface_color_nonnegative_in2_tmp = 0.000000;
    vec3 subsurface_color_nonnegative_out = max(subsurface_color, subsurface_color_nonnegative_in2_tmp);
    const float coat_clamped_low_tmp = 0.000000;
    const float coat_clamped_high_tmp = 1.000000;
    float coat_clamped_out = clamp(coat, coat_clamped_low_tmp, coat_clamped_high_tmp);
    vec3 subsurface_radius_vector_out = vec3(subsurface_radius.x, subsurface_radius.y, subsurface_radius.z);
    float subsurface_selector_out = float(thin_walled);
    const float base_color_nonnegative_in2_tmp = 0.000000;
    vec3 base_color_nonnegative_out = max(base_color, base_color_nonnegative_in2_tmp);
    const vec3 coat_attenuation_bg_tmp = vec3(1.000000, 1.000000, 1.000000);
    vec3 coat_attenuation_out = mix(coat_attenuation_bg_tmp, coat_color, coat);
    const float one_minus_coat_ior_in1_tmp = 1.000000;
    float one_minus_coat_ior_out = one_minus_coat_ior_in1_tmp - coat_IOR;
    const float one_plus_coat_ior_in1_tmp = 1.000000;
    float one_plus_coat_ior_out = one_plus_coat_ior_in1_tmp + coat_IOR;
    vec3 emission_weight_out = emission_color * emission;
    vec3 opacity_luminance_out = vec3(0.0);
    mx_luminance_color3(opacity, vec3(0.272229, 0.674082, 0.053689), opacity_luminance_out);
    vec3 coat_tangent_rotate_out = vec3(0.0);
    mx_rotate_vector3(tangent, coat_tangent_rotate_degree_out, coat_normal, coat_tangent_rotate_out);
    vec3 artistic_ior_ior = vec3(0.0);
    vec3 artistic_ior_extinction = vec3(0.0);
    mx_artistic_ior(metal_reflectivity_out, metal_edgecolor_out, artistic_ior_ior, artistic_ior_extinction);
    float coat_affect_roughness_multiply2_out = coat_affect_roughness_multiply1_out * coat_roughness;
    vec3 tangent_rotate_out = vec3(0.0);
    mx_rotate_vector3(tangent, tangent_rotate_degree_out, normal, tangent_rotate_out);
    const float transmission_roughness_clamped_low_tmp = 0.000000;
    const float transmission_roughness_clamped_high_tmp = 1.000000;
    float transmission_roughness_clamped_out = clamp(transmission_roughness_add_out, transmission_roughness_clamped_low_tmp, transmission_roughness_clamped_high_tmp);
    float coat_gamma_multiply_out = coat_clamped_out * coat_affect_color;
    vec3 subsurface_radius_scaled_out = subsurface_radius_vector_out * subsurface_scale;
    float coat_ior_to_F0_sqrt_out = one_minus_coat_ior_out / one_plus_coat_ior_out;
    vec3 coat_tangent_rotate_normalize_out = normalize(coat_tangent_rotate_out);
    const float coat_affected_roughness_fg_tmp = 1.000000;
    float coat_affected_roughness_out = mix(specular_roughness, coat_affected_roughness_fg_tmp, coat_affect_roughness_multiply2_out);
    vec3 tangent_rotate_normalize_out = normalize(tangent_rotate_out);
    const float coat_affected_transmission_roughness_fg_tmp = 1.000000;
    float coat_affected_transmission_roughness_out = mix(transmission_roughness_clamped_out, coat_affected_transmission_roughness_fg_tmp, coat_affect_roughness_multiply2_out);
    const float coat_gamma_in2_tmp = 1.000000;
    float coat_gamma_out = coat_gamma_multiply_out + coat_gamma_in2_tmp;
    float coat_ior_to_F0_out = coat_ior_to_F0_sqrt_out * coat_ior_to_F0_sqrt_out;
    const float coat_tangent_value2_tmp = 0.000000;
    vec3 coat_tangent_out = (coat_anisotropy > coat_tangent_value2_tmp) ? coat_tangent_rotate_normalize_out : tangent;
    vec2 main_roughness_out = vec2(0.0);
    mx_roughness_anisotropy(coat_affected_roughness_out, specular_anisotropy, main_roughness_out);
    const float main_tangent_value2_tmp = 0.000000;
    vec3 main_tangent_out = (specular_anisotropy > main_tangent_value2_tmp) ? tangent_rotate_normalize_out : tangent;
    vec2 transmission_roughness_out = vec2(0.0);
    mx_roughness_anisotropy(coat_affected_transmission_roughness_out, specular_anisotropy, transmission_roughness_out);
    vec3 coat_affected_subsurface_color_out = pow(subsurface_color_nonnegative_out, vec3(coat_gamma_out));
    vec3 coat_affected_diffuse_color_out = pow(base_color_nonnegative_out, vec3(coat_gamma_out));
    const float one_minus_coat_ior_to_F0_in1_tmp = 1.000000;
    float one_minus_coat_ior_to_F0_out = one_minus_coat_ior_to_F0_in1_tmp - coat_ior_to_F0_out;
    surfaceshader shader_constructor_out = surfaceshader(vec3(0.0),vec3(0.0));
    {
        vec3 N = normalize(vd.normalWorld);
        vec3 V = normalize(u_viewPosition - vd.positionWorld);
        vec3 P = vd.positionWorld;

        float surfaceOpacity = opacity_luminance_out.x;

        // Shadow occlusion
        float occlusion = 1.0;

        // Light loop
        int numLights = numActiveLightSources();
        lightshader lightShader;
        for (int activeLightIndex = 0; activeLightIndex < numLights; ++activeLightIndex)
        {
            sampleLightSource(u_lightData[activeLightIndex], vd.positionWorld, lightShader);
            vec3 L = lightShader.direction;

            // Calculate the BSDF response for this light source
            BSDF coat_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_dielectric_bsdf_reflection(L, V, P, occlusion, coat, vec3(1.000000, 1.000000, 1.000000), coat_IOR, coat_roughness_vector_out, coat_normal, coat_tangent_out, 0, 0, coat_bsdf_out);
            BSDF metal_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            metal_bsdf_out.ior = thin_film_IOR;
            metal_bsdf_out.thickness = thin_film_thickness;
            mx_conductor_bsdf_reflection(L, V, P, occlusion, 1.000000, artistic_ior_ior, artistic_ior_extinction, main_roughness_out, normal, main_tangent_out, 0, metal_bsdf_out);
            BSDF specular_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            specular_bsdf_out.ior = thin_film_IOR;
            specular_bsdf_out.thickness = thin_film_thickness;
            mx_dielectric_bsdf_reflection(L, V, P, occlusion, specular, specular_color, specular_IOR, main_roughness_out, normal, main_tangent_out, 0, 0, specular_bsdf_out);
            BSDF transmission_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            BSDF sheen_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_sheen_bsdf_reflection(L, V, P, occlusion, sheen, sheen_color, sheen_roughness, normal, sheen_bsdf_out);
            BSDF translucent_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_translucent_bsdf_reflection(L, V, P, occlusion, 1.000000, coat_affected_subsurface_color_out, normal, translucent_bsdf_out);
            BSDF subsurface_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_subsurface_bsdf_reflection(L, V, P, occlusion, 1.000000, coat_affected_subsurface_color_out, subsurface_radius_scaled_out, subsurface_anisotropy, normal, subsurface_bsdf_out);
            BSDF selected_subsurface_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            selected_subsurface_bsdf_out.response = mix(subsurface_bsdf_out.response, translucent_bsdf_out.response, subsurface_selector_out);
            selected_subsurface_bsdf_out.throughput = mix(subsurface_bsdf_out.throughput, translucent_bsdf_out.throughput, subsurface_selector_out);
            BSDF diffuse_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_oren_nayar_diffuse_bsdf_reflection(L, V, P, occlusion, base, coat_affected_diffuse_color_out, diffuse_roughness, normal, diffuse_bsdf_out);
            BSDF subsurface_mix_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            subsurface_mix_out.response = mix(diffuse_bsdf_out.response, selected_subsurface_bsdf_out.response, subsurface);
            subsurface_mix_out.throughput = mix(diffuse_bsdf_out.throughput, selected_subsurface_bsdf_out.throughput, subsurface);
            BSDF sheen_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            sheen_layer_out.response = sheen_bsdf_out.response + subsurface_mix_out.response * sheen_bsdf_out.throughput;
            sheen_layer_out.throughput = sheen_bsdf_out.throughput * subsurface_mix_out.throughput;
            BSDF transmission_mix_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            transmission_mix_out.response = mix(sheen_layer_out.response, transmission_bsdf_out.response, transmission);
            transmission_mix_out.throughput = mix(sheen_layer_out.throughput, transmission_bsdf_out.throughput, transmission);
            BSDF specular_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            specular_layer_out.response = specular_bsdf_out.response + transmission_mix_out.response * specular_bsdf_out.throughput;
            specular_layer_out.throughput = specular_bsdf_out.throughput * transmission_mix_out.throughput;
            BSDF thin_film_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            thin_film_layer_out.response = mix(specular_layer_out.response, metal_bsdf_out.response, metalness);
            thin_film_layer_out.throughput = mix(specular_layer_out.throughput, metal_bsdf_out.throughput, metalness);
            vec3 thin_film_layer_attenuated_out_in2_clamped = clamp(coat_attenuation_out, 0.0, 1.0);
            BSDF thin_film_layer_attenuated_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            thin_film_layer_attenuated_out.response = thin_film_layer_out.response * thin_film_layer_attenuated_out_in2_clamped;
            thin_film_layer_attenuated_out.throughput = thin_film_layer_out.throughput * thin_film_layer_attenuated_out_in2_clamped;
            BSDF coat_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            coat_layer_out.response = coat_bsdf_out.response + thin_film_layer_attenuated_out.response * coat_bsdf_out.throughput;
            coat_layer_out.throughput = coat_bsdf_out.throughput * thin_film_layer_attenuated_out.throughput;

            // Accumulate the light's contribution
            shader_constructor_out.color += lightShader.intensity * coat_layer_out.response;
        }

        // Ambient occlusion
        occlusion = 1.0;

        // Add environment contribution
        {
            BSDF coat_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_dielectric_bsdf_indirect(V, coat, vec3(1.000000, 1.000000, 1.000000), coat_IOR, coat_roughness_vector_out, coat_normal, coat_tangent_out, 0, 0, coat_bsdf_out);
            BSDF metal_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            metal_bsdf_out.ior = thin_film_IOR;
            metal_bsdf_out.thickness = thin_film_thickness;
            mx_conductor_bsdf_indirect(V, 1.000000, artistic_ior_ior, artistic_ior_extinction, main_roughness_out, normal, main_tangent_out, 0, metal_bsdf_out);
            BSDF specular_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            specular_bsdf_out.ior = thin_film_IOR;
            specular_bsdf_out.thickness = thin_film_thickness;
            mx_dielectric_bsdf_indirect(V, specular, specular_color, specular_IOR, main_roughness_out, normal, main_tangent_out, 0, 0, specular_bsdf_out);
            BSDF transmission_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            BSDF sheen_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_sheen_bsdf_indirect(V, sheen, sheen_color, sheen_roughness, normal, sheen_bsdf_out);
            BSDF translucent_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_translucent_bsdf_indirect(V, 1.000000, coat_affected_subsurface_color_out, normal, translucent_bsdf_out);
            BSDF subsurface_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_subsurface_bsdf_indirect(V, 1.000000, coat_affected_subsurface_color_out, subsurface_radius_scaled_out, subsurface_anisotropy, normal, subsurface_bsdf_out);
            BSDF selected_subsurface_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            selected_subsurface_bsdf_out.response = mix(subsurface_bsdf_out.response, translucent_bsdf_out.response, subsurface_selector_out);
            selected_subsurface_bsdf_out.throughput = mix(subsurface_bsdf_out.throughput, translucent_bsdf_out.throughput, subsurface_selector_out);
            BSDF diffuse_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_oren_nayar_diffuse_bsdf_indirect(V, base, coat_affected_diffuse_color_out, diffuse_roughness, normal, diffuse_bsdf_out);
            BSDF subsurface_mix_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            subsurface_mix_out.response = mix(diffuse_bsdf_out.response, selected_subsurface_bsdf_out.response, subsurface);
            subsurface_mix_out.throughput = mix(diffuse_bsdf_out.throughput, selected_subsurface_bsdf_out.throughput, subsurface);
            BSDF sheen_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            sheen_layer_out.response = sheen_bsdf_out.response + subsurface_mix_out.response * sheen_bsdf_out.throughput;
            sheen_layer_out.throughput = sheen_bsdf_out.throughput * subsurface_mix_out.throughput;
            BSDF transmission_mix_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            transmission_mix_out.response = mix(sheen_layer_out.response, transmission_bsdf_out.response, transmission);
            transmission_mix_out.throughput = mix(sheen_layer_out.throughput, transmission_bsdf_out.throughput, transmission);
            BSDF specular_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            specular_layer_out.response = specular_bsdf_out.response + transmission_mix_out.response * specular_bsdf_out.throughput;
            specular_layer_out.throughput = specular_bsdf_out.throughput * transmission_mix_out.throughput;
            BSDF thin_film_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            thin_film_layer_out.response = mix(specular_layer_out.response, metal_bsdf_out.response, metalness);
            thin_film_layer_out.throughput = mix(specular_layer_out.throughput, metal_bsdf_out.throughput, metalness);
            vec3 thin_film_layer_attenuated_out_in2_clamped = clamp(coat_attenuation_out, 0.0, 1.0);
            BSDF thin_film_layer_attenuated_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            thin_film_layer_attenuated_out.response = thin_film_layer_out.response * thin_film_layer_attenuated_out_in2_clamped;
            thin_film_layer_attenuated_out.throughput = thin_film_layer_out.throughput * thin_film_layer_attenuated_out_in2_clamped;
            BSDF coat_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            coat_layer_out.response = coat_bsdf_out.response + thin_film_layer_attenuated_out.response * coat_bsdf_out.throughput;
            coat_layer_out.throughput = coat_bsdf_out.throughput * thin_film_layer_attenuated_out.throughput;

            shader_constructor_out.color += occlusion * coat_layer_out.response;
        }

        // Add surface emission
        {
            EDF emission_edf_out = EDF(0.0);
            mx_uniform_edf(N, V, emission_weight_out, emission_edf_out);
            EDF coat_tinted_emission_edf_out = emission_edf_out * coat_color;
            EDF coat_emission_edf_out = EDF(0.0);
            mx_generalized_schlick_edf(N, V, vec3(one_minus_coat_ior_to_F0_out, one_minus_coat_ior_to_F0_out, one_minus_coat_ior_to_F0_out), vec3(0.000000, 0.000000, 0.000000), 5.000000, coat_tinted_emission_edf_out, coat_emission_edf_out);
            // Omitted node 'emission_edf'. Function already called in this scope.
            EDF blended_coat_emission_edf_out = mix(emission_edf_out, coat_emission_edf_out, coat);
            shader_constructor_out.color += blended_coat_emission_edf_out;
        }

        // Calculate the BSDF transmission for viewing direction
        {
            BSDF coat_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_dielectric_bsdf_transmission(V, coat, vec3(1.000000, 1.000000, 1.000000), coat_IOR, coat_roughness_vector_out, coat_normal, coat_tangent_out, 0, 0, coat_bsdf_out);
            BSDF metal_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            BSDF specular_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            specular_bsdf_out.ior = thin_film_IOR;
            specular_bsdf_out.thickness = thin_film_thickness;
            mx_dielectric_bsdf_transmission(V, specular, specular_color, specular_IOR, main_roughness_out, normal, main_tangent_out, 0, 0, specular_bsdf_out);
            BSDF transmission_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            mx_dielectric_bsdf_transmission(V, 1.000000, transmission_color, specular_IOR, transmission_roughness_out, normal, main_tangent_out, 0, 1, transmission_bsdf_out);
            BSDF sheen_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            BSDF translucent_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            BSDF subsurface_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            BSDF selected_subsurface_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            selected_subsurface_bsdf_out.response = mix(subsurface_bsdf_out.response, translucent_bsdf_out.response, subsurface_selector_out);
            selected_subsurface_bsdf_out.throughput = mix(subsurface_bsdf_out.throughput, translucent_bsdf_out.throughput, subsurface_selector_out);
            BSDF diffuse_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            BSDF subsurface_mix_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            subsurface_mix_out.response = mix(diffuse_bsdf_out.response, selected_subsurface_bsdf_out.response, subsurface);
            subsurface_mix_out.throughput = mix(diffuse_bsdf_out.throughput, selected_subsurface_bsdf_out.throughput, subsurface);
            BSDF sheen_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            sheen_layer_out.response = sheen_bsdf_out.response + subsurface_mix_out.response * sheen_bsdf_out.throughput;
            sheen_layer_out.throughput = sheen_bsdf_out.throughput * subsurface_mix_out.throughput;
            BSDF transmission_mix_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            transmission_mix_out.response = mix(sheen_layer_out.response, transmission_bsdf_out.response, transmission);
            transmission_mix_out.throughput = mix(sheen_layer_out.throughput, transmission_bsdf_out.throughput, transmission);
            BSDF specular_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            specular_layer_out.response = specular_bsdf_out.response + transmission_mix_out.response * specular_bsdf_out.throughput;
            specular_layer_out.throughput = specular_bsdf_out.throughput * transmission_mix_out.throughput;
            BSDF thin_film_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            thin_film_layer_out.response = mix(specular_layer_out.response, metal_bsdf_out.response, metalness);
            thin_film_layer_out.throughput = mix(specular_layer_out.throughput, metal_bsdf_out.throughput, metalness);
            vec3 thin_film_layer_attenuated_out_in2_clamped = clamp(coat_attenuation_out, 0.0, 1.0);
            BSDF thin_film_layer_attenuated_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            thin_film_layer_attenuated_out.response = thin_film_layer_out.response * thin_film_layer_attenuated_out_in2_clamped;
            thin_film_layer_attenuated_out.throughput = thin_film_layer_out.throughput * thin_film_layer_attenuated_out_in2_clamped;
            BSDF coat_layer_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
            coat_layer_out.response = coat_bsdf_out.response + thin_film_layer_attenuated_out.response * coat_bsdf_out.throughput;
            coat_layer_out.throughput = coat_bsdf_out.throughput * thin_film_layer_attenuated_out.throughput;
            shader_constructor_out.color += coat_layer_out.response;
        }

        // Compute and apply surface opacity
        {
            shader_constructor_out.color *= surfaceOpacity;
            shader_constructor_out.transparency = mix(vec3(1.0), shader_constructor_out.transparency, surfaceOpacity);
        }
    }

    out1 = shader_constructor_out;
}

void main()
{
    vec3 geomprop_Nworld_out1 = normalize(vd.normalWorld);
    vec3 geomprop_Tworld_out1 = normalize(vd.tangentWorld);
    vec2 geomprop_UV0_out1 = vd.texcoord_0.xy;
    float mtlximage21_out = 0.0;
    mx_image_float(mtlximage21_file, mtlximage21_layer, mtlximage21_default, geomprop_UV0_out1, mtlximage21_uaddressmode, mtlximage21_vaddressmode, mtlximage21_filtertype, mtlximage21_framerange, mtlximage21_frameoffset, mtlximage21_frameendaction, mtlximage21_uv_scale, mtlximage21_uv_offset, mtlximage21_out);
    vec3 mtlximage20_out = vec3(0.0);
    mx_image_vector3(mtlximage20_file, mtlximage20_layer, mtlximage20_default, geomprop_UV0_out1, mtlximage20_uaddressmode, mtlximage20_vaddressmode, mtlximage20_filtertype, mtlximage20_framerange, mtlximage20_frameoffset, mtlximage20_frameendaction, mtlximage20_uv_scale, mtlximage20_uv_offset, mtlximage20_out);
    vec3 mtlxnormalmap15_out = vec3(0.0);
    mx_normalmap_vector2(mtlximage20_out, mtlxnormalmap15_space, mtlxnormalmap15_scale, geomprop_Nworld_out1, geomprop_Tworld_out1, mtlxnormalmap15_out);
    surfaceshader Pawn_Top_W_out = surfaceshader(vec3(0.0),vec3(0.0));
    NG_standard_surface_surfaceshader_100(Pawn_Top_W_base, Pawn_Top_W_base_color, Pawn_Top_W_diffuse_roughness, Pawn_Top_W_metalness, Pawn_Top_W_specular, Pawn_Top_W_specular_color, mtlximage21_out, Pawn_Top_W_specular_IOR, Pawn_Top_W_specular_anisotropy, Pawn_Top_W_specular_rotation, Pawn_Top_W_transmission, Pawn_Top_W_transmission_color, Pawn_Top_W_transmission_depth, Pawn_Top_W_transmission_scatter, Pawn_Top_W_transmission_scatter_anisotropy, Pawn_Top_W_transmission_dispersion, Pawn_Top_W_transmission_extra_roughness, Pawn_Top_W_subsurface, Pawn_Top_W_subsurface_color, Pawn_Top_W_subsurface_radius, Pawn_Top_W_subsurface_scale, Pawn_Top_W_subsurface_anisotropy, Pawn_Top_W_sheen, Pawn_Top_W_sheen_color, Pawn_Top_W_sheen_roughness, Pawn_Top_W_coat, Pawn_Top_W_coat_color, Pawn_Top_W_coat_roughness, Pawn_Top_W_coat_anisotropy, Pawn_Top_W_coat_rotation, Pawn_Top_W_coat_IOR, geomprop_Nworld_out1, Pawn_Top_W_coat_affect_color, Pawn_Top_W_coat_affect_roughness, Pawn_Top_W_thin_film_thickness, Pawn_Top_W_thin_film_IOR, Pawn_Top_W_emission, Pawn_Top_W_emission_color, Pawn_Top_W_opacity, Pawn_Top_W_thin_walled, mtlxnormalmap15_out, geomprop_Tworld_out1, Pawn_Top_W_out);
    material M_Pawn_Top_W_out = Pawn_Top_W_out;
    out1 = vec4(M_Pawn_Top_W_out.color, 1.0);
}

