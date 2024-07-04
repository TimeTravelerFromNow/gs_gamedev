/*================================================================
    * Copyright: 2020 John Jackson
    * gfxt

    The purpose of this example is to demonstrate how to use the gs_gfxt
    util.


    Press `esc` to exit the application.
================================================================*/

#define GS_IMPL
#include <gs/gs.h>

#define GS_IMMEDIATE_DRAW_IMPL
#include <gs/util/gs_idraw.h>

#define GS_GFXT_IMPL
#include <gs/util/gs_gfxt.h>

#include "data.c"

void app_camera_update();

void app_init()
{
    app_t* app = gs_user_data(app_t);
    app->camera = gs_camera_perspective();

    app->cb = gs_command_buffer_new();
    app->gsi = gs_immediate_draw_new(gs_platform_main_window());
    app->asset_dir = gs_platform_dir_exists("./assets") ? "./assets" : "../assets";
    char TMP[256] = {0};

    // Load pipeline from resource file
    gs_snprintf(TMP, sizeof(TMP), "%s/%s", app->asset_dir, "pipelines/sky_sphere.sf");
    app->pip = gs_gfxt_pipeline_load_from_file(TMP);

    // Create material using this pipeline
    app->mat = gs_gfxt_material_create(&(gs_gfxt_material_desc_t){
        .pip_func.hndl = &app->pip
    });

    // Create mesh that uses the layout from the pipeline's requested mesh layout
    gs_snprintf(TMP, sizeof(TMP), "%s/%s", app->asset_dir, "meshes/sphere.gltf");
    app->mesh = gs_gfxt_mesh_load_from_file(TMP, &(gs_gfxt_mesh_import_options_t){
        .layout = app->pip.mesh_layout,
        .size = gs_dyn_array_size(app->pip.mesh_layout) * sizeof(gs_gfxt_mesh_layout_t),
        .index_buffer_element_size = app->pip.desc.raster.index_buffer_element_size
    });

    gs_snprintf(TMP, sizeof(TMP), "%s/%s", app->asset_dir, "textures/TCom_HDRSky045_2K_hdri_skies_tone.png");
    app->texture = gs_gfxt_texture_load_from_file(TMP, NULL, false, false);
}

void app_update()
{
    // Cache data for frame
    app_t* app = gs_user_data(app_t);
    gs_command_buffer_t* cb = &app->cb;
    gs_immediate_draw_t* gsi = &app->gsi;
    gs_gfxt_material_t* mat = &app->mat;
    gs_gfxt_mesh_t* mesh = &app->mesh;
    gs_gfxt_texture_t* tex = &app->texture;
    gs_camera_t* cam = &app->camera;

    const gs_vec2 fbs = gs_platform_framebuffer_sizev(gs_platform_main_window());
    const float _t = gs_platform_elapsed_time() * 0.001f;
    const float dt = gs_platform_delta_time();

    if (gs_platform_key_pressed(GS_KEYCODE_ESC))
    {
        gs_quit();
    }

    // Camera for scene
    if (gs_platform_mouse_down(GS_MOUSE_RBUTTON)) {
    gs_platform_lock_mouse(gs_platform_main_window(), true);
    app_camera_update();
    }
    else {
        gs_platform_lock_mouse(gs_platform_main_window(), false);
    }
    gs_mat4 model = gs_vqs_to_mat4(&(gs_vqs){
        .translation = gs_v3(0.f, 0.f, 0.f),
        .rotation = gs_quat_angle_axis(_t, GS_YAXIS),
        .scale = gs_v3s(10.1f)
    });

    gs_mat4 vp = gs_camera_get_sky_view_projection(cam, fbs.x, fbs.y);

    gs_mat4 mvp = gs_mat4_mul(vp, gs_mat4_scale(10, 10, 10) );

    // Apply material uniforms
    gs_gfxt_material_set_uniform(mat, "u_mvp", &mvp);
    gs_gfxt_material_set_uniform(mat, "u_tex", tex);

    // Rendering
    gs_graphics_clear_desc_t clear = {.actions = &(gs_graphics_clear_action_t){.color = {0.05f, 0.05, 0.05, 1.f}}};
    gs_graphics_renderpass_begin(cb, (gs_handle(gs_graphics_renderpass_t)){0});
    {
        // Set view port
        gs_graphics_set_viewport(cb,0,0,(int)fbs.x,(int)fbs.y);

        // Clear screen
        gs_graphics_clear(cb, &clear);

        // Bind material
        gs_gfxt_material_bind(cb, mat);

        // Bind material uniforms
        gs_gfxt_material_bind_uniforms(cb, mat);

        // Render mesh
        gs_gfxt_mesh_draw_material(cb, mesh, mat);
    }
    gs_graphics_renderpass_end(cb);

    //Submits to cb
    gs_graphics_command_buffer_submit(cb);
}

void app_shutdown()
{
    // free
    app_t* app = gs_user_data(app_t);
    gs_immediate_draw_free(&app->gsi);
    gs_command_buffer_free(&app->cb);
}

gs_app_desc_t gs_main(int32_t argc, char** argv)
{
    return (gs_app_desc_t) {
        .user_data = gs_malloc_init(app_t),
        .init = app_init,
        .update = app_update,
        .shutdown = app_shutdown,
        .window.width = 900,
        .window.height = 580
    };
}


#define SENSITIVITY 0.2f
static float pitch = 0.f;
static float speed = 2.f;
void app_camera_update()
{
    app_t* app = gs_user_data(app_t);
    gs_platform_t* platform = gs_subsystem(platform);
    gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), SENSITIVITY);
    const float mod = gs_platform_key_down(GS_KEYCODE_LEFT_SHIFT) ? 2.f : 1.f;
    float dt = platform->time.delta;
    float old_pitch = pitch;
    gs_camera_t* camera = &app->camera;

    // Keep track of previous amount to clamp the camera's orientation
    pitch = gs_clamp(old_pitch + dp.y, -90.f, 90.f);

    // Rotate camera
    gs_camera_offset_orientation(camera, -dp.x, old_pitch - pitch);

    gs_vec3 vel = {0};
    switch (camera->proj_type)
    {
        case GS_PROJECTION_TYPE_ORTHOGRAPHIC:
        {
            if (gs_platform_key_down(GS_KEYCODE_W)) vel = gs_vec3_add(vel, gs_camera_up(camera));
            if (gs_platform_key_down(GS_KEYCODE_S)) vel = gs_vec3_add(vel, gs_vec3_scale(gs_camera_up(camera), -1.f));
            if (gs_platform_key_down(GS_KEYCODE_A)) vel = gs_vec3_add(vel, gs_camera_left(camera));
            if (gs_platform_key_down(GS_KEYCODE_D)) vel = gs_vec3_add(vel, gs_camera_right(camera));

            // Ortho scale
            gs_vec2 wheel = gs_platform_mouse_wheelv();
            camera->ortho_scale -= wheel.y;
        } break;

        case  GS_PROJECTION_TYPE_PERSPECTIVE:
        {
            if (gs_platform_key_down(GS_KEYCODE_W)) vel = gs_vec3_add(vel, gs_camera_forward(camera));
            if (gs_platform_key_down(GS_KEYCODE_S)) vel = gs_vec3_add(vel, gs_camera_backward(camera));
            if (gs_platform_key_down(GS_KEYCODE_A)) vel = gs_vec3_add(vel, gs_camera_left(camera));
            if (gs_platform_key_down(GS_KEYCODE_D)) vel = gs_vec3_add(vel, gs_camera_right(camera));
            gs_vec2 wheel = gs_platform_mouse_wheelv();
            speed = gs_clamp(speed + wheel.y, 0.01f, 50.f);
        } break;
    }

    camera->transform.position = gs_vec3_add(camera->transform.position, gs_vec3_scale(gs_vec3_norm(vel), dt * speed * mod));
}
