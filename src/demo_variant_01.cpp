#include "demo.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "rhombo_flex_kit.h"
#include "lvgl/src/stdlib/lv_sprintf.h"
#include "lib/lv_gltf/data/lv_gltf_data_internal.h"
#include "lib/lv_gltf/data/lv_gltf_override.h"
#include "lib/lv_gltf/data/lv_gltf_data.h"
#include "lib/lv_gltf/view/lv_gltf_view.h"
#include "lib/lv_gltf/view/lv_gltf_view_internal.hpp"

#define SYSTEM_ASSETS_FILENAME  "./gltfs/support_assets.glb"

float TIME_SCALE = 1.0f;
unsigned int _current_tab = 0;
lv_obj_t * tab_pages[MAX_TABS];

float spin_counter_degrees = 0.f;
float spin_rate = 0.f;
float anim_rate = 1.f;
int camera = -1;
int anim = -1;
bool use_scenecam = false;
bool anim_enabled = true;
bool animate_spin = true;
bool show_grid = true;
bool needs_system_gltfdata = false;
bool frame_grab_ui = false;
bool enable_intro_zoom = true;
bool reapply_layout_flag = true;
bool stub_mode = false;
uint32_t cycle_frames = 1;
extern bool requires_file_name;

float tri_spacing_scale = 4.0f;
float inv_tri_spacing_scale = 1.f / tri_spacing_scale;

float rhombo_height = 4.f;
float rhombo_width = 4.f;
float rhombo_depth = 4.f;
float rhombo_tri_scale = 2.f;

float tri_pipe_length  = -1.f;
float height_pipe_length = -1.f;
float width_pipe_length = -1.f;
float depth_pipe_length = -1.f;

float rhombo_total_height = -1.f;
float rhombo_total_width = -1.f;
float rhombo_total_depth = -1.f;

lv_indev_t * mouse;
lv_glfw_window_t * window;
lv_display_t * display_texture;
lv_glfw_texture_t * window_texture;

lv_obj_t * gltfview_3dtex;
lv_gltf_view_t * demo_gltfview;
lv_opengl_shader_cache_t * shader_cache = NULL;
lv_gltf_data_t * system_gltfdata = NULL;
lv_gltf_data_t * demo_gltfdata = NULL;
lv_gltf_override_t * ov_cursor;
lv_gltf_override_t * ov_cursor_scale;
lv_gltf_override_t * ov_ground_scale;

// Temporarily defined but not used
lv_gltf_override_t * ov_boom = NULL;
lv_gltf_override_t * ov_stick = NULL;
lv_gltf_override_t * ov_bucket = NULL;
lv_gltf_override_t * ov_swing = NULL;

lv_obj_t * rhombo_summary;

const uint32_t NUM_PIPES_PER_AXIS = 8;
const uint32_t NUM_PIPES_PER_TRI = 3;
const uint32_t NUM_TRI_CORNERS_PER_HALF = 4;
const uint32_t NUM_TRI_PIPES_TOTAL = ( NUM_TRI_CORNERS_PER_HALF * NUM_PIPES_PER_TRI ) * 2;  // 24
const uint32_t NUM_QUAD_CORNERS_PER_HALF = 12;

lv_gltf_override_t * ov_vpipes[NUM_PIPES_PER_AXIS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
lv_gltf_override_t * ov_wpipes[NUM_PIPES_PER_AXIS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
lv_gltf_override_t * ov_dpipes[NUM_PIPES_PER_AXIS] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

lv_gltf_override_t * ov_top = NULL;
lv_gltf_override_t * ov_bottom = NULL;
lv_gltf_override_t * ov_top_tri_corners_pos[NUM_TRI_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL};
lv_gltf_override_t * ov_bottom_tri_corners_pos[NUM_TRI_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL};

lv_gltf_override_t * ov_top_tri_corners_scale[NUM_TRI_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL};
lv_gltf_override_t * ov_bottom_tri_corners_scale[NUM_TRI_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL};

lv_gltf_override_t * ov_top_tri_corners_visible_scale[NUM_TRI_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL};
lv_gltf_override_t * ov_bottom_tri_corners_visible_scale[NUM_TRI_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL};

lv_gltf_override_t * ov_top_quad_corners_scale[NUM_QUAD_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
lv_gltf_override_t * ov_bottom_quad_corners_scale[NUM_QUAD_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

lv_gltf_override_t * ovro_top_quad_corners_pos[NUM_QUAD_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
lv_gltf_override_t * ovro_bottom_quad_corners_pos[NUM_QUAD_CORNERS_PER_HALF] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

lv_gltf_override_t * ov_all_tri_pipes[NUM_TRI_PIPES_TOTAL] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

const float QUAD_PIPE_MARGIN = 0.31f * 2.f;


void update_rhombo_summary(void) {

    float corner_width = (-ovro_top_quad_corners_pos[0]->data1) - (-ovro_top_quad_corners_pos[1]->data1);
    float rhombo_total_floorspace = rhombo_total_depth * rhombo_total_width;
    rhombo_total_floorspace -= ((corner_width * corner_width) * 2.f);

    float total_pipe_length = tri_pipe_length * NUM_TRI_PIPES_TOTAL;
    total_pipe_length += height_pipe_length * NUM_PIPES_PER_AXIS;
    total_pipe_length += width_pipe_length * NUM_PIPES_PER_AXIS;
    total_pipe_length += depth_pipe_length * NUM_PIPES_PER_AXIS;
    char buffer[4096];
    lv_snprintf(buffer, 4095, R"(
Dimensions:
Width: %.2f
Depth: %.2f
Height: %.2f
Floorspace: %.2f sqr
Pipes:
Tri pipe length = %.2f
Height pipe length = %.2f
Width pipe length = %.2f
Depth pipe length = %.2f
Total pipe length = %.2f
)", rhombo_total_width, rhombo_total_depth, rhombo_total_height, rhombo_total_floorspace, tri_pipe_length, height_pipe_length, width_pipe_length, depth_pipe_length, total_pipe_length);
    buffer[4095] = '\0';
    lv_label_set_text(rhombo_summary, buffer);
}


void set_rhombo_height(float new_height) {
    new_height = new_height > 0.f ? new_height : 0.f;
    //float pipe_length = new_height + 0.2f;

    rhombo_height = new_height;

    new_height += 1.8f;

    lv_gltf_data_set_override_data2(ov_top, new_height / 2.f);
    lv_gltf_data_set_override_data2(ov_bottom, -new_height / 2.f);
    
    lv_gltf_view_recache_all_transforms(demo_gltfview, demo_gltfdata);
    //    for (uint32_t i = 0; i < NUM_QUAD_CORNERS_PER_HALF; i++) printf("Top Corner #%d World_Pos = %.2f, %.2f, %.2f\n", (i+1), ovro_top_quad_corners_pos[i]->data1, ovro_top_quad_corners_pos[i]->data2, ovro_top_quad_corners_pos[i]->data3);
    float pipe_length = ovro_top_quad_corners_pos[0]->data2 * 2.f ;
    rhombo_total_height = ovro_top_quad_corners_pos[2]->data2 * 2.f;
    pipe_length = pipe_length > 0.f ? pipe_length : -pipe_length;
    pipe_length = pipe_length - QUAD_PIPE_MARGIN > 0.f ? pipe_length - QUAD_PIPE_MARGIN : 0.f;

    height_pipe_length = pipe_length;
    //printf("height pipe length = %.2f for new_height of %.2f\n", pipe_length, new_height);

    const float base_scale = 1.f;//inv_tri_spacing_scale;
    const float radius_scale =  1.f;//inv_tri_spacing_scale;
    for (int i = 0; i < NUM_PIPES_PER_AXIS; i++) {
        lv_gltf_data_set_override_data1(ov_vpipes[i], radius_scale);
        lv_gltf_data_set_override_data2(ov_vpipes[i], base_scale * pipe_length);
        lv_gltf_data_set_override_data3(ov_vpipes[i], radius_scale);
        //ov_vpipes[i]->data1 = radius_scale;
        //ov_vpipes[i]->data2 = base_scale * pipe_length;
        //ov_vpipes[i]->data3 = radius_scale;
    }

}

void set_rhombo_width(float new_width) {
    new_width = new_width > 0.f ? new_width : 0.f;
    rhombo_width = new_width;
    new_width += 1.8f;

    float rside = new_width / 2.f;
    lv_gltf_data_set_override_data1(ov_bottom_tri_corners_pos[1], rside);
    lv_gltf_data_set_override_data1(ov_bottom_tri_corners_pos[3], rside);
    lv_gltf_data_set_override_data1(ov_top_tri_corners_pos[1], rside);
    lv_gltf_data_set_override_data1(ov_top_tri_corners_pos[3], rside);

    lv_gltf_data_set_override_data1(ov_bottom_tri_corners_pos[0], -rside);
    lv_gltf_data_set_override_data1(ov_bottom_tri_corners_pos[2], -rside);
    lv_gltf_data_set_override_data1(ov_top_tri_corners_pos[0], -rside);
    lv_gltf_data_set_override_data1(ov_top_tri_corners_pos[2], -rside);

    lv_gltf_view_recache_all_transforms(demo_gltfview, demo_gltfdata);
    //    for (uint32_t i = 0; i < NUM_QUAD_CORNERS_PER_HALF; i++) printf("Top Corner #%d World_Pos = %.2f, %.2f, %.2f\n", (i+1), ovro_top_quad_corners_pos[i]->data1, ovro_top_quad_corners_pos[i]->data2, ovro_top_quad_corners_pos[i]->data3);
    float pipe_length = ovro_top_quad_corners_pos[1]->data1 * -2.f ;
    rhombo_total_width = ovro_top_quad_corners_pos[0]->data1 * -2.f;
    //ipe_length = pipe_length > 0.f ? pipe_length : -pipe_length;
    pipe_length = pipe_length - QUAD_PIPE_MARGIN > 0.f ? pipe_length - QUAD_PIPE_MARGIN : 0.f;
    width_pipe_length = pipe_length;

    //printf("width pipe length = %.2f for new_width of %.2f\n", pipe_length, new_width);

    const float base_scale = 1.f;//inv_tri_spacing_scale;
    const float radius_scale =  1.f;//inv_tri_spacing_scale;
    for (int i = 0; i < NUM_PIPES_PER_AXIS; i++) {
        lv_gltf_data_set_override_data1(ov_wpipes[i], radius_scale);
        lv_gltf_data_set_override_data2(ov_wpipes[i], base_scale * pipe_length);
        lv_gltf_data_set_override_data3(ov_wpipes[i], radius_scale);
    }
}

void set_rhombo_depth(float new_depth) {
    new_depth = new_depth > 0.f ? new_depth : 0.f;
    rhombo_depth = new_depth;
    new_depth += 1.8f;

    float fside = new_depth / 2.f;
    lv_gltf_data_set_override_data3(ov_bottom_tri_corners_pos[2], fside);
    lv_gltf_data_set_override_data3(ov_bottom_tri_corners_pos[3], fside);
    lv_gltf_data_set_override_data3(ov_top_tri_corners_pos[2], fside);
    lv_gltf_data_set_override_data3(ov_top_tri_corners_pos[3], fside);

    lv_gltf_data_set_override_data3(ov_bottom_tri_corners_pos[0], -fside);
    lv_gltf_data_set_override_data3(ov_bottom_tri_corners_pos[1], -fside);
    lv_gltf_data_set_override_data3(ov_top_tri_corners_pos[0], -fside);
    lv_gltf_data_set_override_data3(ov_top_tri_corners_pos[1], -fside);

    lv_gltf_view_recache_all_transforms(demo_gltfview, demo_gltfdata);
    //    for (uint32_t i = 0; i < NUM_QUAD_CORNERS_PER_HALF; i++) printf("Top Corner #%d World_Pos = %.2f, %.2f, %.2f\n", (i+1), ovro_top_quad_corners_pos[i]->data1, ovro_top_quad_corners_pos[i]->data2, ovro_top_quad_corners_pos[i]->data3);
    float pipe_length = ovro_top_quad_corners_pos[0]->data3 * -2.f;
    rhombo_total_depth = ovro_top_quad_corners_pos[1]->data3 * -2.f;
    //pipe_length = pipe_length > 0.f ? pipe_length : -pipe_length;
    pipe_length = pipe_length - QUAD_PIPE_MARGIN > 0.f ? pipe_length - QUAD_PIPE_MARGIN : 0.f;
    //printf("depth pipe length = %.2f for new_depth of %.2f\n", pipe_length, new_depth);
    depth_pipe_length = pipe_length;

    const float base_scale = 1.f;//inv_tri_spacing_scale;
    const float radius_scale =  1.f;//inv_tri_spacing_scale;
    for (int i = 0; i < NUM_PIPES_PER_AXIS; i++) {
        lv_gltf_data_set_override_data1(ov_dpipes[i], radius_scale);
        lv_gltf_data_set_override_data2(ov_dpipes[i], base_scale * pipe_length);
        lv_gltf_data_set_override_data3(ov_dpipes[i], radius_scale);
    }

}

void set_rhombo_tri_scale(float new_tri_scale) {

    /* Set safe defaults for the scale values */

    if (new_tri_scale < 0.0001f) {
        new_tri_scale = 0.0001f;
    }

    rhombo_tri_scale = new_tri_scale;

    tri_spacing_scale = new_tri_scale;
    inv_tri_spacing_scale = 1.f / new_tri_scale;
    for (uint32_t i = 0; i < NUM_TRI_CORNERS_PER_HALF; i++) {
        lv_gltf_data_set_override_data1(ov_top_tri_corners_scale[i], tri_spacing_scale);
        lv_gltf_data_set_override_data2(ov_top_tri_corners_scale[i], tri_spacing_scale);
        lv_gltf_data_set_override_data3(ov_top_tri_corners_scale[i], tri_spacing_scale);
        lv_gltf_data_set_override_data1(ov_bottom_tri_corners_scale[i], tri_spacing_scale);
        lv_gltf_data_set_override_data2(ov_bottom_tri_corners_scale[i], tri_spacing_scale);
        lv_gltf_data_set_override_data3(ov_bottom_tri_corners_scale[i], tri_spacing_scale);

        lv_gltf_data_set_override_data1(ov_top_tri_corners_visible_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data2(ov_top_tri_corners_visible_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data3(ov_top_tri_corners_visible_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data1(ov_bottom_tri_corners_visible_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data2(ov_bottom_tri_corners_visible_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data3(ov_bottom_tri_corners_visible_scale[i], inv_tri_spacing_scale);

    }

    for (uint32_t i = 0; i < NUM_QUAD_CORNERS_PER_HALF; i++) {
        lv_gltf_data_set_override_data1(ov_top_quad_corners_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data2(ov_top_quad_corners_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data3(ov_top_quad_corners_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data1(ov_bottom_quad_corners_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data2(ov_bottom_quad_corners_scale[i], inv_tri_spacing_scale);
        lv_gltf_data_set_override_data3(ov_bottom_quad_corners_scale[i], inv_tri_spacing_scale);
    }

    tri_pipe_length = tri_spacing_scale * 0.5f - 0.25f;
    for (uint32_t i = 0; i < NUM_TRI_PIPES_TOTAL; i++)
        lv_gltf_data_set_override_data2(ov_all_tri_pipes[i], tri_pipe_length);

    // After changing the tri-scale, the height/width/depth need updated as well
    set_rhombo_height(rhombo_height);
    set_rhombo_width(rhombo_width);
    set_rhombo_depth(rhombo_depth);

    update_rhombo_summary();
}

void demo_set_overrides_rhombo( void ) {
    ov_top = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

    ov_top_tri_corners_pos[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_pos[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_pos[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_pos[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_pos[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_pos[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_pos[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_pos[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04", OP_POSITION, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

    ov_top_tri_corners_scale[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_scale[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_scale[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_scale[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_scale[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_scale[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_scale[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_scale[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

    ov_top_tri_corners_visible_scale[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/tetroid_fitting", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_visible_scale[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/tetroid_fitting", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_visible_scale[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/tetroid_fitting", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_tri_corners_visible_scale[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/tetroid_fitting", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_visible_scale[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/tetroid_fitting", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_visible_scale[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/tetroid_fitting", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_visible_scale[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/tetroid_fitting", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_tri_corners_visible_scale[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/tetroid_fitting", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

    ov_top_quad_corners_scale[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[4] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[5] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[6] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[7] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[8] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[9] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[10] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_top_quad_corners_scale[11] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

    ov_bottom_quad_corners_scale[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[4] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[5] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[6] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[7] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[8] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[9] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[10] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_bottom_quad_corners_scale[11] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);


    uint32_t tp = 0;
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_01_front/pipe_target_01", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_02_left/pipe_target_02", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_03_top/pipe_target_03", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_01_front/pipe_target_01", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_02_left/pipe_target_02", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_03_top/pipe_target_03", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_01_front/pipe_target_01", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_02_left/pipe_target_02", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_03_top/pipe_target_03", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_01_front/pipe_target_01", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_02_left/pipe_target_02", OP_WORLD_POSITION);
    ovro_top_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_03_top/pipe_target_03", OP_WORLD_POSITION);
    tp = 0;
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_01_front/pipe_target_01", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_02_left/pipe_target_02", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_03_top/pipe_target_03", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_01_front/pipe_target_01", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_02_left/pipe_target_02", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_03_top/pipe_target_03", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_01_front/pipe_target_01", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_02_left/pipe_target_02", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_03_top/pipe_target_03", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_01_front/pipe_target_01", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_02_left/pipe_target_02", OP_WORLD_POSITION);
    ovro_bottom_quad_corners_pos[tp++] = lv_gltf_data_readonly_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_03_top/pipe_target_03", OP_WORLD_POSITION);


    ov_vpipes[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_02_left/quad_pipe_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_vpipes[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_02_left/quad_pipe_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_vpipes[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_02_left/quad_pipe_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_vpipes[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_02_left/quad_pipe_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_vpipes[4] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_02_left/quad_pipe_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_vpipes[5] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_02_left/quad_pipe_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_vpipes[6] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_02_left/quad_pipe_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_vpipes[7] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_02_left/quad_pipe_02_left", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

    ov_wpipes[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_03_top/quad_pipe_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_wpipes[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_01_front/quad_pipe_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_wpipes[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_01_front/quad_pipe_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_wpipes[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_03_top/quad_pipe_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_wpipes[4] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_03_top/quad_pipe_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_wpipes[5] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_01_front/quad_pipe_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_wpipes[6] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_01_front/quad_pipe_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_wpipes[7] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_03_top/quad_pipe_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

    ov_dpipes[0] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/quad_fitting_01_front/quad_pipe_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_dpipes[1] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/quad_fitting_03_top/quad_pipe_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_dpipes[2] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/quad_fitting_03_top/quad_pipe_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_dpipes[3] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/quad_fitting_01_front/quad_pipe_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_dpipes[4] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/quad_fitting_01_front/quad_pipe_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_dpipes[5] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/quad_fitting_03_top/quad_pipe_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_dpipes[6] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/quad_fitting_03_top/quad_pipe_03_top", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
    ov_dpipes[7] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/quad_fitting_01_front/quad_pipe_01_front", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

    uint32_t tripipe_chans = OMC_CHAN2;
    tp = 0;
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/tetroid_fitting/tri_pipe_01_front", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/tetroid_fitting/tri_pipe_02_left", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_01/root/rescale/tetroid_fitting/tri_pipe_03_top", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/tetroid_fitting/tri_pipe_01_front", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/tetroid_fitting/tri_pipe_02_left", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_02/root/rescale/tetroid_fitting/tri_pipe_03_top", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/tetroid_fitting/tri_pipe_01_front", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/tetroid_fitting/tri_pipe_02_left", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_03/root/rescale/tetroid_fitting/tri_pipe_03_top", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/tetroid_fitting/tri_pipe_01_front", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/tetroid_fitting/tri_pipe_02_left", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/top/corner_04/root/rescale/tetroid_fitting/tri_pipe_03_top", OP_SCALE, tripipe_chans);

    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/tetroid_fitting/tri_pipe_01_front", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/tetroid_fitting/tri_pipe_02_left", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_01/root/rescale/tetroid_fitting/tri_pipe_03_top", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/tetroid_fitting/tri_pipe_01_front", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/tetroid_fitting/tri_pipe_02_left", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_02/root/rescale/tetroid_fitting/tri_pipe_03_top", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/tetroid_fitting/tri_pipe_01_front", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/tetroid_fitting/tri_pipe_02_left", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_03/root/rescale/tetroid_fitting/tri_pipe_03_top", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/tetroid_fitting/tri_pipe_01_front", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/tetroid_fitting/tri_pipe_02_left", OP_SCALE, tripipe_chans);
    ov_all_tri_pipes[tp++] = lv_gltf_data_override_add_by_id(demo_gltfdata, "/bottom/corner_04/root/rescale/tetroid_fitting/tri_pipe_03_top", OP_SCALE, tripipe_chans);

    /* Set safe defaults for the scale values */
    set_rhombo_tri_scale(tri_spacing_scale);

    /* For grid and cursor support, call the original demo_set_overrides(), too. */
    demo_set_overrides();
}

static void ov_triscale_slider_event_cb(lv_event_t * e)
{
    const float MAX_VAL = 0.1f;
    const float MIN_VAL = 8.f;
    lv_obj_t * slider = lv_event_get_target_obj(e);
    float normval = ((float)lv_slider_get_value(slider) / 10000.0f) ;
    //rhombo_tri_scale = (normval * (MAX_VAL - MIN_VAL)) + MIN_VAL;
    set_rhombo_tri_scale((normval * (MAX_VAL - MIN_VAL)) + MIN_VAL);
    cancel_dragnav_frames = 1;
}

static void ov_height_slider_event_cb(lv_event_t * e)
{
    const float MAX_VAL = 0.f;
    const float MIN_VAL = 8.f;
    lv_obj_t * slider = lv_event_get_target_obj(e);
    float normval = ((float)lv_slider_get_value(slider) / 10000.0f) ;
    set_rhombo_height((normval * (MAX_VAL - MIN_VAL)) + MIN_VAL);
    cancel_dragnav_frames = 1;
}

static void ov_width_slider_event_cb(lv_event_t * e)
{
    const float MAX_VAL = 0.f;
    const float MIN_VAL = 8.f;
    lv_obj_t * slider = lv_event_get_target_obj(e);
    float normval = ((float)lv_slider_get_value(slider) / 10000.0f) ;
    set_rhombo_width((normval * (MAX_VAL - MIN_VAL)) + MIN_VAL);
    cancel_dragnav_frames = 1;
}

static void ov_depth_slider_event_cb(lv_event_t * e)
{
    const float MAX_VAL = 0.f;
    const float MIN_VAL = 8.f;
    lv_obj_t * slider = lv_event_get_target_obj(e);
    float normval = ((float)lv_slider_get_value(slider) / 10000.0f) ;
    set_rhombo_depth((normval * (MAX_VAL - MIN_VAL)) + MIN_VAL);
    cancel_dragnav_frames = 1;
}

void make_rhombo_sliders(lv_obj_t * _cont) {
    {
        lv_obj_t * slider = lv_slider_create(_cont);
        lv_obj_set_size(slider, 24, ((ui_get_window_height() - 90) / 2));
        lv_obj_align(slider, LV_ALIGN_BOTTOM_LEFT, 105, -70);
        lv_obj_add_event_cb(slider, ov_triscale_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_set_style_anim(slider, 0, LV_ANIM_OFF);
        lv_obj_set_style_anim_duration(slider, 0, 0);
        lv_obj_set_style_bg_opa(slider, LV_OPA_40, LV_PART_INDICATOR);
        lv_slider_set_range(slider, 0, 10000);
        lv_slider_set_value(slider, 0.f, 0);
        //lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
        //lv_obj_add_style(slider, &style_bg_vert, LV_PART_MAIN);
    }
    {
        lv_obj_t * slider = lv_slider_create(_cont);
        lv_obj_set_size(slider, 24, ((ui_get_window_height() - 90) / 2));
        lv_obj_align(slider, LV_ALIGN_BOTTOM_LEFT, 140, -70);
        lv_obj_add_event_cb(slider, ov_height_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_set_style_anim(slider, 0, LV_ANIM_OFF);
        lv_obj_set_style_anim_duration(slider, 0, 0);
        lv_obj_set_style_bg_opa(slider, LV_OPA_40, LV_PART_INDICATOR);
        lv_slider_set_range(slider, 0, 10000);
        lv_slider_set_value(slider, 0.f, 0);
        //lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
        //lv_obj_add_style(slider, &style_bg_vert, LV_PART_MAIN);
    }
    {
        lv_obj_t * slider = lv_slider_create(_cont);
        lv_obj_set_size(slider, 24, ((ui_get_window_height() - 90) / 2));
        lv_obj_align(slider, LV_ALIGN_BOTTOM_LEFT, 175, -70);
        lv_obj_add_event_cb(slider, ov_width_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_set_style_anim(slider, 0, LV_ANIM_OFF);
        lv_obj_set_style_anim_duration(slider, 0, 0);
        lv_obj_set_style_bg_opa(slider, LV_OPA_40, LV_PART_INDICATOR);
        lv_slider_set_range(slider, 0, 10000);
        lv_slider_set_value(slider, 0.f, 0);
        //lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
        //lv_obj_add_style(slider, &style_bg_vert, LV_PART_MAIN);
    }
    {
        lv_obj_t * slider = lv_slider_create(_cont);
        lv_obj_set_size(slider, 24, ((ui_get_window_height() - 90) / 2));
        lv_obj_align(slider, LV_ALIGN_BOTTOM_LEFT, 210, -70);
        lv_obj_add_event_cb(slider, ov_depth_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_flag(slider, LV_OBJ_FLAG_ADV_HITTEST);
        lv_obj_set_style_anim(slider, 0, LV_ANIM_OFF);
        lv_obj_set_style_anim_duration(slider, 0, 0);
        lv_obj_set_style_bg_opa(slider, LV_OPA_40, LV_PART_INDICATOR);
        lv_slider_set_range(slider, 0, 10000);
        lv_slider_set_value(slider, 0.f, 0);
        //lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
        //lv_obj_add_style(slider, &style_bg_vert, LV_PART_MAIN);
    }
}

void make_rhombo_summary(lv_obj_t * _cont) {
    rhombo_summary = lv_label_create(_cont);
    lv_obj_align(rhombo_summary, LV_ALIGN_TOP_RIGHT, -60,35);
    lv_label_set_text(rhombo_summary, "test123");
    lv_obj_set_style_text_color(rhombo_summary, lv_color_hex(0x55FF55u), 0);\
    //update_rhombo_summary();
}
void reload(char * _filename, const char * _hdr_filename) {

    lv_obj_clear_flag(grp_loading, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(gltfview_3dtex, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(grp_loading, LV_OBJ_FLAG_HIDDEN);

    demo_gltfdata = (lv_gltf_data_t*)lv_malloc(lv_gltf_data_get_struct_size() );

    if (shader_cache == NULL) setup_shadercache(_hdr_filename, 1800);
    lv_gltf_data_load_bytes(rhombo_flex_kit, rhombo_flex_kit_len, demo_gltfdata, shader_cache);

    if (lv_gltf_view_get_probe(demo_gltfdata)->cameraCount == 0) {
        use_scenecam = false;
        camera = -1;
    } else {
        use_scenecam = true;
        if (camera > (int32_t)lv_gltf_view_get_probe(demo_gltfdata)->cameraCount ) {
            camera = (int32_t)lv_gltf_view_get_probe(demo_gltfdata)->cameraCount;
        }
    }
    if (lv_gltf_view_get_probe(demo_gltfdata)->animationCount == 0) {
        anim_enabled = false;
        //anim = 0;
    } else {
        anim_enabled = true;
        anim = 0;
    }
    needs_system_gltfdata = show_grid;
    #ifdef EXPERIMENTAL_GROUNDCAST
    needs_system_gltfdata = true;
    #endif

    if (needs_system_gltfdata) {
        system_gltfdata = (lv_gltf_data_t*) lv_malloc(lv_gltf_data_get_struct_size() );
        lv_gltf_data_load_file(SYSTEM_ASSETS_FILENAME, system_gltfdata, shader_cache);
        lv_gltf_data_copy_bounds_info(system_gltfdata, demo_gltfdata);
        float newradius = lv_gltf_data_get_int_radiusX1000(demo_gltfdata) / 1000.f;
        ov_ground_scale = lv_gltf_data_override_add_by_id(system_gltfdata, "/grid", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);

        float unitscale = newradius * ((1.f / 2.f) * 3.f);
        float tscale = unitscale;
        if (!show_grid) {
            tscale = 0.f;
        }
        lv_gltf_data_set_override_data1(ov_ground_scale, tscale);
        lv_gltf_data_set_override_data2(ov_ground_scale, tscale);
        lv_gltf_data_set_override_data3(ov_ground_scale, tscale);

        tscale = unitscale / 8.f;
        ov_cursor_scale = lv_gltf_data_override_add_by_id(system_gltfdata, "/cursor/visible", OP_SCALE, OMC_CHAN1 | OMC_CHAN2 | OMC_CHAN3);
        #ifndef EXPERIMENTAL_GROUNDCAST
        tscale = 0.f;
        #endif
        lv_gltf_data_set_override_data1(ov_cursor_scale, tscale);
        lv_gltf_data_set_override_data2(ov_cursor_scale, tscale);
        lv_gltf_data_set_override_data3(ov_cursor_scale, tscale);
    }
    lv_obj_add_flag(grp_loading, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(gltfview_3dtex, LV_OBJ_FLAG_HIDDEN);
    demo_os_integrate_window_standard_title(_filename);
    demo_file_load_dialog_set_directory_from_filepath(_filename);
    demo_ui_set_tab(TAB_VIEW);
}

int main(int argc, char *argv[]) {
    demo_gltfview = (lv_gltf_view_t * ) lv_malloc(get_viewer_datasize() );
    init_viewer_struct(demo_gltfview);
    char gltfFilePath[MAX_PATH_LENGTH] = "";
    char hdrFilePath[MAX_PATH_LENGTH] = "assets/hdr/directional.jpg";
    int lastMouseX = 0, lastMouseY = 0;
    int frameCount = 0;
    bool softwareOnly = false;
    bool startMaximized = false;
    anim_rate = 1.0f;
    camera = 0;
    use_scenecam = true;
    requires_file_name = false;
    
    if ( demo_cli_apply_commandline_options(demo_gltfview, gltfFilePath, hdrFilePath, &frameCount, &softwareOnly, &startMaximized, &stub_mode, &anim_rate, argc, argv) ) {

        if (softwareOnly) setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        lv_init();
        glfwInit();

        uint32_t max_window_width;
        uint32_t max_window_height;
        int maxFrames = frameCount;
        if (!demo_os_integrate_get_maximum_window_framebuffer_size(&max_window_width, &max_window_height)) startMaximized = false;

        if (startMaximized) {
            lv_gltf_view_set_width(demo_gltfview, max_window_width - (INNER_BG_CROP_LEFT + INNER_BG_CROP_RIGHT));
            lv_gltf_view_set_height(demo_gltfview, max_window_height - (INNER_BG_CROP_TOP + INNER_BG_CROP_BOTTOM));
        } else {
            lv_gltf_view_set_width(demo_gltfview, (int)(max_window_width * 0.6f) - (INNER_BG_CROP_LEFT + INNER_BG_CROP_RIGHT));
            lv_gltf_view_set_height(demo_gltfview, (int)(max_window_height * 0.8f) - (INNER_BG_CROP_TOP + INNER_BG_CROP_BOTTOM));
        }

        /* create a window and initialize OpenGL */
        window = lv_glfw_window_create_ex( max_window_width, max_window_height, true, false, false, MY_WINDOW_TITLE, "gltf-view-new");
        glfwHideWindow((GLFWwindow *)lv_glfw_window_get_glfw_window(window));

        /* create a display that flushes to a texture */
        display_texture = lv_opengles_texture_create(max_window_width, max_window_height);
        lv_display_set_default(display_texture);

        demo_os_integrate_setup_glfw_window(window, false, startMaximized);

        /* add the texture to the window */
        window_texture = lv_glfw_window_add_texture(window, lv_opengles_texture_get_texture_id(display_texture), max_window_width, max_window_height);

        /* get the mouse indev of the window texture */
        mouse = lv_glfw_texture_get_mouse_indev(window_texture);

        demo_ui_make_underlayer();

        gltfview_3dtex = lv_3dtexture_create(tab_pages[TAB_VIEW]);
        lv_3dtexture_set_src(gltfview_3dtex, demo_make_small_clear_texture());
        lv_obj_set_size(gltfview_3dtex, max_window_width - (INNER_BG_CROP_LEFT + INNER_BG_CROP_RIGHT), max_window_height - (INNER_BG_CROP_TOP + INNER_BG_CROP_BOTTOM));
        lv_obj_add_flag(gltfview_3dtex, LV_OBJ_FLAG_HIDDEN);
        lv_obj_align(gltfview_3dtex, LV_ALIGN_TOP_LEFT, INNER_BG_CROP_LEFT, INNER_BG_CROP_TOP);
        lv_obj_clear_flag(gltfview_3dtex, LV_OBJ_FLAG_CLICKABLE  );
        lv_3dtexture_set_flip(gltfview_3dtex, false, false);
        demo_ui_make_overlayer();
        make_rhombo_sliders(tab_pages[TAB_VIEW]);
        make_rhombo_summary(tab_pages[TAB_VIEW]);
        lv_refr_now(NULL);

        glfwShowWindow(glfw_window);
        if (startMaximized) glfwMaximizeWindow(glfw_window);

        reload(gltfFilePath, hdrFilePath);
        demo_set_overrides_rhombo();
        set_rhombo_tri_scale(rhombo_tri_scale);
//        set_rhombo_height(rhombo_height);
//        set_rhombo_width(rhombo_width);
//        set_rhombo_depth(rhombo_depth);

        if (lv_gltf_view_get_probe(demo_gltfdata)->animationCount > 0) anim = 0;
        if (lv_gltf_view_get_probe(demo_gltfdata)->cameraCount == 0) {
            use_scenecam = false;
            camera = -1;
        }

        demo_refocus(demo_gltfview, true);
        demo_os_integrate_window_standard_title(gltfFilePath);

        lv_obj_clear_flag(gltfview_3dtex, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(gltfview_3dtex);

        struct timeval start;
        float ROLLING_FPS = -1.0f;
        long unsigned int frames_this_second = 0;
        long unsigned int frames_rendered_this_second = 0;
        unsigned long int usec_span = 0;
        unsigned long int usec_per_frame_optimal = 0;
        float seconds_this_second = 0.f;
        float total_seconds = 0.f;
        float goal_fps = 15.0f;
        float goal_fps_span = 1.0f / goal_fps;
        time_t last_poll = time(0);
        #ifdef EXPERIMENTAL_GROUNDCAST 
        float _groundpos[3] = {0.f, 0.f, 0.f};
        #endif /* EXPERIMENTAL_GROUNDCAST */
        float cycle_seconds = fabs(spin_rate) > 0 ? 360.f /  fabs(spin_rate) : 0.f;
        if (lv_gltf_view_get_probe(demo_gltfdata)->animationCount > 0) {
            if (anim_enabled && (anim < (int32_t)lv_gltf_view_get_probe(demo_gltfdata)->animationCount)){
                printf("USING ANIMATION FOR CYCLE TIMING\n");
                float anim_total_time = lv_gltf_animation_get_total_time(demo_gltfdata, anim);
                cycle_seconds = anim_total_time / anim_rate;
            }
        }
        cycle_frames = ui_max(1, (uint32_t)(cycle_seconds * goal_fps));
        bool _timing_break_flag = false;
        while (!_timing_break_flag) {
            time_t this_poll = time(0);
            _timing_break_flag = true;
            if (this_poll != last_poll) { _timing_break_flag = true; usleep(1000); }
            lv_refr_now(NULL);
            last_poll = this_poll;
        }

        demo_ui_apply_camera_button_visibility(demo_gltfdata);
        demo_ui_apply_anim_button_visibility(demo_gltfdata);

        gettimeofday(&start, NULL);
        uint32_t totalframenum = 0;
        lv_point_t _mousepoint;
        lv_indev_get_point(mouse, &_mousepoint);
        struct pollfd fds[1];
        fds[0].fd = -1;     // No file descriptors to monitor
        fds[0].events = 0;  // No events to monitor
        while(!demo_os_integrate_window_should_close()) {

            poll(fds, 0, lv_timer_handler());
            lv_task_handler();
            float sec_span;

            if (frameCount > 0) {
                sec_span = 1.f / 30.f;
            } else {
                struct timeval stop;
                gettimeofday(&stop, NULL);
                usec_span = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
                start = stop;
                sec_span = (float)usec_span / 1000000.0f;
            }
            seconds_this_second += sec_span;
            total_seconds += sec_span;

            //float windowed_seconds = cycle_seconds != 0.f ? total_seconds - ((int)(total_seconds / cycle_seconds) * cycle_seconds) : 0.f;
            uint32_t framenum = totalframenum % cycle_frames;

            demo_nav_gradual_to_goals( );

            if (animate_spin) {
                spin_counter_degrees += (spin_rate * sec_span);
                lv_gltf_view_set_spin_degree_offset(demo_gltfview, spin_counter_degrees);
            }
            lv_gltf_view_set_timestep(demo_gltfview, anim_enabled ? anim_rate * sec_span : 0.f );

            lv_indev_get_point(mouse, &_mousepoint);
            int mouse_delta_x = (_mousepoint.x - lastMouseX); mouse_delta_x *= mouse_delta_x;
            int mouse_delta_y = (_mousepoint.y - lastMouseY); mouse_delta_y *= mouse_delta_y;
            const int TOUCH_MOUSEJUMP_THRESH = 50*50;

            if (mouse_delta_x + mouse_delta_y > TOUCH_MOUSEJUMP_THRESH ) {
                // Mouse point jumped drastically indicating a touch screen mouse driver
                // just updated the last touch position, so this update should be skipped
                // to avoid big weird jumps in movement

                /* Do nothing here, let lastMouseX/Y update below and next time through it will be valid */
            } else {
                int WINDOW_WIDTH_MINUS_MARGIN = ui_get_window_width()-WINDOW_CONTROL_MARGIN;
                int WINDOW_HEIGHT_MINUS_MARGIN = ui_get_window_height()-WINDOW_CONTROL_MARGIN;
                bool mouse_in_window = ((_mousepoint.x >= WINDOW_CONTROL_MARGIN) && (_mousepoint.x <= (WINDOW_WIDTH_MINUS_MARGIN)) && (_mousepoint.y >= WINDOW_CONTROL_MARGIN) && (_mousepoint.y <= (WINDOW_HEIGHT_MINUS_MARGIN)) );
                if (mouse_in_window) {
                    lv_indev_state_t mouse_state = lv_indev_get_state(mouse);
                    double subjectRadius = lv_gltf_data_get_int_radiusX1000(demo_gltfdata) / 1000.f;
                    double movePow = d_min(subjectRadius, pow(subjectRadius, 0.5));
                    if ((mouse_state & 0x0F) == LV_INDEV_STATE_PR) demo_nav_process_drag(movePow, (mouse_state & 0xF0), _mousepoint.x, _mousepoint.y, lastMouseX, lastMouseY);
                }
                #ifdef EXPERIMENTAL_GROUNDCAST
                if ((lastMouseX != _mousepoint.x) || (lastMouseY != _mousepoint.y)) lv_gltf_view_mark_dirty(demo_gltfview);
                if (mouse_in_window) {
                    bool _res = lv_gltf_view_raycast_ground_position(demo_gltfview, _mousepoint.x - INNER_BG_CROP_LEFT, _mousepoint.y - INNER_BG_CROP_TOP, ui_get_window_width() - (INNER_BG_CROP_LEFT + INNER_BG_CROP_RIGHT), ui_get_window_height() - (INNER_BG_CROP_TOP + INNER_BG_CROP_BOTTOM),  0.0, _groundpos);
                    if (_res && (ov_cursor != NULL)) {
                        
                        lv_gltf_data_set_override_data1(ov_cursor, _groundpos[0]);
                        lv_gltf_data_set_override_data2(ov_cursor, _groundpos[1]);
                        lv_gltf_data_set_override_data3(ov_cursor, _groundpos[2]);
                    }
                }

                #endif /* EXPERIMENTAL_GROUNDCAST */
            }
            lastMouseX = _mousepoint.x;
            lastMouseY = _mousepoint.y;

            lv_gltf_view_set_camera(demo_gltfview, use_scenecam ? camera : -1);
            lv_gltf_view_set_anim(demo_gltfview, anim_enabled ? anim : -1);

            time_t this_poll = time(0);
            frames_this_second += 1;
            bool seconds_changed = difftime(this_poll, last_poll) > 0;
            last_poll = this_poll;

            if (seconds_changed) {
                ROLLING_FPS = (ROLLING_FPS > 0) ? ((ROLLING_FPS * 3.0f) + ((float)frames_this_second / (float)seconds_this_second) ) / 4.0f : ((float)frames_this_second / (float)seconds_this_second);
                seconds_this_second = 0.f;
                if (frames_rendered_this_second > 0) {
                    #ifndef NDEBUG
                    printf("[DEBUG BUILD] ");
                    #endif
                    printf("Frames Drawn: %ld | Average FPS: %2.1f\n", frames_rendered_this_second, ROLLING_FPS);
                }
                frames_this_second = 0;
                frames_rendered_this_second = 0;
                usec_per_frame_optimal = (int)(1000000.f / ROLLING_FPS);
            }

            lv_3dtexture_id_t gltf_texture = 0;
            uint32_t fps_lock_delay_msec = lv_gltf_view_get_fps_goal_delay(demo_gltfview, 60.0);
            if (fps_lock_delay_msec > 0) {
                //printf("Delaying for %d msec to hit fps goal of %.1f fps\n", fps_lock_delay_msec, 30.0f);
                poll(fds, 0, fps_lock_delay_msec);
            }
            //float preserved_scale = rhombo_tri_scale;
            //set_rhombo_tri_scale((rhombo_tri_scale * 0.75f) + (sin(total_seconds) * (rhombo_tri_scale * 0.25f)));
            //rhombo_tri_scale = preserved_scale;

            /*  Only draw the grid (and other extras) if they are enabled 
                and the view is not currently from a scene defined camera */
            if (needs_system_gltfdata && (use_scenecam == false)) {
                gltf_texture = lv_gltf_view_get_desc(demo_gltfview)->render_func(shader_cache, demo_gltfview, demo_gltfdata, system_gltfdata, NULL);
            } else {
                gltf_texture = lv_gltf_view_get_desc(demo_gltfview)->render_func(shader_cache, demo_gltfview, demo_gltfdata, NULL);
            }

            if (reapply_layout_flag) demo_ui_reposition_all();
            if (!lv_gltf_view_check_frame_was_cached(demo_gltfview)) {
                frames_rendered_this_second += 1;
                lv_3dtexture_set_src(gltfview_3dtex, gltf_texture);
                lv_obj_invalidate(gltfview_3dtex);
                lv_refr_now(NULL);
                glfwPollEvents();
                bool file_alpha = lv_gltf_view_get_bg_mode(demo_gltfview) != BG_ENVIRONMENT;
            } else {
                glfwPollEvents();
                usleep(33000);
            }
            totalframenum += 1;
            if (frameCount > 0) {
                frameCount -= 1;
                if (frameCount == 0) demo_os_integrate_signal_window_close();
            }
        }
        if (needs_system_gltfdata) lv_gltf_data_destroy(system_gltfdata);
        lv_gltf_data_destroy(demo_gltfdata);
        lv_gltf_view_destroy(demo_gltfview);
        lv_opengl_shader_cache_destroy(shader_cache);
    }
    lv_free(demo_gltfview);
    #ifndef NDEBUG
    printf("Note: Because this is a debug build, it is normal for there to be a significant delay when closing the application.  To avoid this in the future, rebuild in release mode by running the switch_release.sh script in the project's /ex folder.\n");
    #endif
    exit(0);
}
