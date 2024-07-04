#ifdef GS_PLATFORM_WEB
    #define GS_VERSION_STR "#version 300 es\n"
#else
    #define GS_VERSION_STR "#version 330 core\n"
#endif
/*================================================================
    * Copyright: 2020 John Jackson
================================================================*/

typedef struct {
    gs_command_buffer_t cb;
    gs_immediate_draw_t gsi;
    gs_gfxt_pipeline_t pip;
    gs_gfxt_material_t mat;
    gs_gfxt_mesh_t mesh;
    gs_gfxt_texture_t texture;
    const char* asset_dir;
    gs_camera_t camera;
} app_t;
