#version 430 core

layout (location = 0) out vec4 gbuffer_c0; // base_color / reflection_color (rgba8)
layout (location = 1) out vec4 gbuffer_c1; // normal (rgb10)
layout (location = 2) out vec4 gbuffer_c2; // emissive (rgb8) and something else
layout (location = 3) out vec4 gbuffer_c3; // occlusion (r8), roughness (g8), metallic (b8) and something else

in shader_shared
{
    vec3 shared_vertex_position;
    vec2 shared_texcoord;
    vec3 shared_normal;
    vec3 shared_tangent;
    vec3 shared_bitangent;
    flat bool calculate_normals;
    flat bool calculate_tangents;
} fs_in;

layout (location = 1, binding = 0) uniform sampler2D t_base_color;
layout (location = 2, binding = 1) uniform sampler2D t_roughness_metallic;
layout (location = 3, binding = 2) uniform sampler2D t_occlusion;
layout (location = 4, binding = 3) uniform sampler2D t_normal;
layout (location = 5, binding = 4) uniform sampler2D t_emissive_color;

layout(binding = 0, std140) uniform scene_vertex_uniforms
{
    mat4 u_model_matrix;
    mat3 u_normal_matrix;
    bool u_has_normals;
    bool u_has_tangents;
};

layout(binding = 1, std140) uniform scene_material_uniforms
{
    vec4  base_color;
    vec4  emissive_color; // this is a vec3, but there are annoying bugs with some drivers.
    float metallic;
    float roughness;

    bool base_color_texture;
    bool roughness_metallic_texture;
    bool occlusion_texture;
    bool packed_occlusion;
    bool normal_texture;
    bool emissive_color_texture;

    int alpha_mode;
    float alpha_cutoff;
};

vec4 get_base_color()
{
    vec4 color = base_color_texture ? texture(t_base_color, fs_in.shared_texcoord) : base_color;
    if(alpha_mode == 1 && color.a <= alpha_cutoff)
        discard;
    return color;
}

vec3 get_emissive()
{
    return emissive_color_texture ? texture(t_emissive_color, fs_in.shared_texcoord).rgb : emissive_color.rgb;
}

vec3 get_occlusion_roughness_metallic()
{
    vec3 o_r_m = roughness_metallic_texture ? texture(t_roughness_metallic, fs_in.shared_texcoord).rgb : vec3(1.0, roughness, metallic);
    if(packed_occlusion)
        return o_r_m;

    float occlusion = occlusion_texture ? texture(t_occlusion, fs_in.shared_texcoord).r : 1.0;
    o_r_m.r = occlusion;

    return o_r_m;
}

vec3 get_normal()
{
    vec3 normal = normalize(fs_in.shared_normal);
    vec3 dfdx = dFdx(fs_in.shared_vertex_position);
    vec3 dfdy = dFdy(fs_in.shared_vertex_position);
    if(!u_has_normals)
        normal = normalize(cross(dfdx, dfdy)); // approximation
    if(normal_texture)
    {
        vec3 tangent   = fs_in.shared_tangent;
        vec3 bitangent = fs_in.shared_bitangent;
        if(!u_has_tangents)
        {
            vec3 uv_dx = dFdx(vec3(fs_in.shared_texcoord, 0.0));
            vec3 uv_dy = dFdy(vec3(fs_in.shared_texcoord, 0.0));
            vec3 t_    = (uv_dy.y * dfdx - uv_dx.y * dfdy) / (uv_dx.x * uv_dy.y - uv_dy.x * uv_dx.y);
            tangent    = normalize(t_ - normal * dot(normal, t_));
            bitangent  = cross(normal, tangent);
        }

        mat3 tbn = mat3(normalize(tangent), normalize(bitangent), normal);
        vec3 mapped_normal = normalize(texture(t_normal, fs_in.shared_texcoord).rgb * 2.0 - 1.0);
        normal = normalize(tbn * mapped_normal.rgb);
    }
    return normal * 0.5 + 0.5;
}

void main()
{
    gbuffer_c0 = vec4(get_base_color());
    gbuffer_c1 = vec4(get_normal(), 0.0);
    gbuffer_c2 = vec4(get_emissive(), 0.0);
    gbuffer_c3 = vec4(get_occlusion_roughness_metallic(), 0.0);
}
