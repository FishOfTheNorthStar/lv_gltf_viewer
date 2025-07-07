#ifndef LV_GLTFOVERRIDE_H
#define LV_GLTFOVERRIDE_H

#include "lv_gltf_data.h"
#include "lv_gltf_data_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum  {
    OP_VISIBILITY,
    OP_POSITION,
    OP_ROTATION,
    OP_SCALE,
    OP_BASE_COLOR,
    OP_ALPHA_FACTOR,
    OP_EMIS_COLOR,
    OP_WORLD_POSITION
} OverrideProp;

typedef enum  {
    OMC_CHAN1 = 0x01,
    OMC_CHAN2 = 0x02,
    OMC_CHAN3 = 0x04,
    OMC_CHAN4 = 0x08
} OverrideMaskChannels;

struct lv_gltf_override_struct {
    OverrideProp prop;
    uint32_t data_mask;
    float data1;
    float data2;
    float data3;
    float data4;
    bool read_only;
    bool dirty;
    struct lv_gltf_override_struct * next_override; // Pointer to the next override
} ;

/**
 * @brief Change the value in data channel 1 of a given override.
 *
 * @param override Pointer to the lv_gltf_override_t to change the data channel 1 value of.
 * @param new_value The new value.
 */
void lv_gltf_data_set_override_data1(lv_gltf_override_t * override, float new_value);

/**
 * @brief Change the value in data channel 2 of a given override.
 *
 * @param override Pointer to the lv_gltf_override_t to change the data channel 2 value of.
 * @param new_value The new value.
 */
void lv_gltf_data_set_override_data2(lv_gltf_override_t * override, float new_value);

/**
 * @brief Change the value in data channel 3 of a given override.
 *
 * @param override Pointer to the lv_gltf_override_t to change the data channel 3 value of.
 * @param new_value The new value.
 */
void lv_gltf_data_set_override_data3(lv_gltf_override_t * override, float new_value);

/**
 * @brief Change the value in data channel 4 of a given override.
 *
 * @param override Pointer to the lv_gltf_override_t to change the data channel 4 value of.
 * @param new_value The new value.
 */
void lv_gltf_data_set_override_data4(lv_gltf_override_t * override, float new_value);

/**
 * @brief Reset the dirty flag for a given override.
 *
 * @param override Pointer to the lv_gltf_override_t to reset the dirty flag for.
 */
void lv_gltf_data_clean_override(lv_gltf_override_t * override);

/**
 * @brief Add an override to a GLTF data object by node index.
 *
 * @param gltf_data Pointer to the lv_gltf_data_t object to which the override will be added.
 * @param nodeIndex The index of the node to override.
 * @param which_prop The property to override.
 * @param data_mask A mask indicating which data fields to override.
 * @return Pointer to the newly created lv_gltf_override_t object, or NULL if the operation failed.
 */
lv_gltf_override_t * lv_gltf_data_override_add_by_index(lv_gltf_data_t * gltf_data, uint64_t nodeIndex,
                                                        OverrideProp which_prop, uint32_t data_mask);

/**
 * @brief Add an override to a GLTF data object by node IP address.
 *
 * @param gltf_data Pointer to the lv_gltf_data_t object to which the override will be added.
 * @param nodeIp The IP address of the node to override.
 * @param which_prop The property to override.
 * @param data_mask A mask indicating which data fields to override.
 * @return Pointer to the newly created lv_gltf_override_t object, or NULL if the operation failed.
 */
lv_gltf_override_t * lv_gltf_data_override_add_by_ip(lv_gltf_data_t * gltf_data, const char * nodeIp,
                                                     OverrideProp which_prop, uint32_t data_mask);

/**
 * @brief Add an override to a GLTF data object by node ID.
 *
 * @param gltf_data Pointer to the lv_gltf_data_t object to which the override will be added.
 * @param nodeId The ID of the node to override.
 * @param which_prop The property to override.
 * @param data_mask A mask indicating which data fields to override.
 * @return Pointer to the newly created lv_gltf_override_t object, or NULL if the operation failed.
 */
lv_gltf_override_t * lv_gltf_data_override_add_by_id(lv_gltf_data_t * gltf_data, const char * nodeId,
                                                     OverrideProp which_prop, uint32_t data_mask);

                                                     
/**
 * @brief Add a read-only override (value monitor) to a GLTF data object by node index.
 *
 * @param gltf_data Pointer to the lv_gltf_data_t object to which the override will be added.
 * @param nodeIndex The index of the node to override.
 * @param which_prop The property to override.
 * @param data_mask A mask indicating which data fields to override.
 * @return Pointer to the newly created lv_gltf_override_t object, or NULL if the operation failed.
 */
lv_gltf_override_t * lv_gltf_data_readonly_add_by_index(lv_gltf_data_t * gltf_data, uint64_t nodeIndex,
                                                        OverrideProp which_prop, uint32_t data_mask);

/**
 * @brief  Add a read-only override (value monitor) to a GLTF data object by node IP address.
 *
 * @param gltf_data Pointer to the lv_gltf_data_t object to which the override will be added.
 * @param nodeIp The IP address of the node to override.
 * @param which_prop The property to override.
 * @return Pointer to the newly created lv_gltf_override_t object, or NULL if the operation failed.
 */
lv_gltf_override_t * lv_gltf_data_readonly_add_by_ip(lv_gltf_data_t * gltf_data, const char * nodeIp,
                                                     OverrideProp which_prop);

/**
 * @brief  Add a read-only override (value monitor) to a GLTF data object by node ID.
 *
 * @param gltf_data Pointer to the lv_gltf_data_t object to which the read-only will be added.
 * @param nodeId The ID of the node to read.
 * @param which_prop The property to read.
 * @return Pointer to the newly created lv_gltf_override_t object, or NULL if the operation failed.
 */
lv_gltf_override_t * lv_gltf_data_readonly_add_by_id(lv_gltf_data_t * gltf_data, const char * nodeId,
                                                     OverrideProp which_prop);

/**
 * @brief Remove an override from a GLTF data object.
 *
 * @param gltf_data Pointer to the lv_gltf_data_t object from which the override will be removed.
 * @param override The override to be removed.
 * @param which_prop The property to override.
 * @param data_mask A mask indicating which data fields to override.
 * @return True on success, False on failure.
 */
bool lv_gltf_data_override_remove(lv_gltf_data_t * gltf_data, lv_gltf_override_t * override);

/**
 * @brief Retrieve the probe information from the GLTF view.
 *
 * @param gltf_data Pointer to the lv_gltf_data_t object containing the model data.
 * @return Pointer to the gltf_probe_info structure containing the probe information.
 */
gltf_probe_info * lv_gltf_view_get_probe(lv_gltf_data_t * gltf_data);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_GLTFOVERRIDE_H*/
