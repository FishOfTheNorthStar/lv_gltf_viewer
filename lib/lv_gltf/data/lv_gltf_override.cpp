#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <lvgl.h>
#include <drivers/glfw/lv_opengles_debug.h> /* GL_CALL */

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include "lv_gltf_override.h"
#include "lv_gltf_data_internal.hpp"

void lv_gltf_data_set_override_data1(lv_gltf_override_t * override, float new_value) {
    if (override->data1 != new_value) {
        override->data1 = new_value;
        override->dirty = true;
    }
}

void lv_gltf_data_set_override_data2(lv_gltf_override_t * override, float new_value) {
    if (override->data2 != new_value) {
        override->data2 = new_value;
        override->dirty = true;
    }
}

void lv_gltf_data_set_override_data3(lv_gltf_override_t * override, float new_value) {
    if (override->data3 != new_value) {
        override->data3 = new_value;
        override->dirty = true;
    }
}

void lv_gltf_data_set_override_data4(lv_gltf_override_t * override, float new_value) {
    if (override->data4 != new_value) {
        override->data4 = new_value;
        override->dirty = true;
    }
}

void lv_gltf_data_clean_override(lv_gltf_override_t * override) {
    override->dirty = false;
}

lv_gltf_override_t * add_by_node(lv_gltf_data_t * gltf_data, _NODE node, OverrideProp which_prop, uint32_t data_mask, bool read_only)
{
    if(node == nullptr) {
        return nullptr;
    }

    const auto _node = node;

    // Create a new override
    lv_gltf_override_t _newOverride;
    _newOverride.prop = which_prop;
    _newOverride.data_mask = data_mask;
    _newOverride.data1 =
    _newOverride.data2 =
    _newOverride.data3 = which_prop == OP_SCALE ? 1.f : 0.f;
    _newOverride.data4 = 0.f;
    _newOverride.read_only = read_only;
    _newOverride.dirty = true;
    _newOverride.next_override = nullptr; // Initialize next_override to null

    // Check if an override already exists for this node
    if(gltf_data->overrides->find(_node) != gltf_data->overrides->end()) {

        // Get the existing override
        lv_gltf_override_t * existingOverride = (*gltf_data->overrides)[_node];

        // Traverse to the end of the linked list of overrides
        while(existingOverride->next_override != nullptr) existingOverride = existingOverride->next_override;

        gltf_data->all_override_count += 1;
        (*gltf_data->all_overrides)[gltf_data->all_override_count - 1] = _newOverride;
        existingOverride->next_override = &(*gltf_data->all_overrides)[gltf_data->all_override_count - 1];
        return existingOverride->next_override;
    }
    else {
        // No existing override, insert the new one
        gltf_data->all_override_count += 1;
        (*gltf_data->all_overrides)[gltf_data->all_override_count - 1] = _newOverride;
        (*gltf_data->overrides)[_node] = &(*gltf_data->all_overrides)[gltf_data->all_override_count - 1];
        return (*gltf_data->overrides)[_node];
    }
    return nullptr;
}

lv_gltf_override_t * lv_gltf_data_override_add_by_index(lv_gltf_data_t * gltf_data, uint64_t nodeIndex,
                                                        OverrideProp which_prop, uint32_t data_mask)
{
    return (nodeIndex < gltf_data->node_by_index->size()) ? nullptr : add_by_node(gltf_data, (*gltf_data->node_by_index)[nodeIndex],
                                                                              which_prop, data_mask, false);
}

lv_gltf_override_t * lv_gltf_data_override_add_by_ip(lv_gltf_data_t * gltf_data, const char * nodeIp,
                                                     OverrideProp which_prop, uint32_t data_mask)
{
    std::string sNodeIp = std::string(nodeIp);
    return ((*gltf_data->node_by_ip).find(sNodeIp) == (*gltf_data->node_by_ip).end()) ? nullptr : add_by_node(gltf_data,
                                                                                                      (*gltf_data->node_by_ip)[sNodeIp], which_prop, data_mask, false);
}

lv_gltf_override_t * lv_gltf_data_override_add_by_id(lv_gltf_data_t * gltf_data, const char * nodeId,
                                                     OverrideProp which_prop, uint32_t data_mask)
{
    std::string sNodeId = std::string(nodeId);
    return ((*gltf_data->node_by_path).find(sNodeId) == (*gltf_data->node_by_path).end()) ? nullptr : add_by_node(gltf_data,
                                                                                                          (*gltf_data->node_by_path)[sNodeId], which_prop, data_mask, false);
}


lv_gltf_override_t * lv_gltf_data_readonly_add_by_index(lv_gltf_data_t * gltf_data, uint64_t nodeIndex,
                                                        OverrideProp which_prop)
{
    return (nodeIndex < gltf_data->node_by_index->size()) ? nullptr : add_by_node(gltf_data, (*gltf_data->node_by_index)[nodeIndex],
                                                                              which_prop, 0, true);
}

lv_gltf_override_t * lv_gltf_data_readonly_add_by_ip(lv_gltf_data_t * gltf_data, const char * nodeIp,
                                                     OverrideProp which_prop)
{
    std::string sNodeIp = std::string(nodeIp);
    return ((*gltf_data->node_by_ip).find(sNodeIp) == (*gltf_data->node_by_ip).end()) ? nullptr : add_by_node(gltf_data,
                                                                                                      (*gltf_data->node_by_ip)[sNodeIp], which_prop, 0, true);
}

lv_gltf_override_t * lv_gltf_data_readonly_add_by_id(lv_gltf_data_t * gltf_data, const char * nodeId,
                                                     OverrideProp which_prop)
{
    std::string sNodeId = std::string(nodeId);
    return ((*gltf_data->node_by_path).find(sNodeId) == (*gltf_data->node_by_path).end()) ? nullptr : add_by_node(gltf_data,
                                                                                                          (*gltf_data->node_by_path)[sNodeId], which_prop, 0, true);
}


// Custom comparison function to compare structs
bool compareOverrides(const lv_gltf_override_t & a, const lv_gltf_override_t & b)
{
    return (a.prop == b.prop)
           && (a.data_mask == b.data_mask)
           && (a.data1 == b.data1)
           && (a.data2 == b.data2)
           && (a.data3 == b.data3)
           && (a.data4 == b.data4)
           && (a.next_override == b.next_override);
}

bool lv_gltf_data_override_remove(lv_gltf_data_t * gltf_data, lv_gltf_override_t * overrideToRemove)
{
    for(auto pair : *gltf_data->overrides) {
        lv_gltf_override_t * currentOverride = pair.second;
        lv_gltf_override_t * previousOverride = nullptr;

        while(currentOverride != nullptr) {
            if(currentOverride == overrideToRemove) {
                // Found the override to remove
                if(previousOverride != nullptr) {
                    // Link the previous override to the next one
                    previousOverride->next_override = currentOverride->next_override;
                }
                else {
                    (*gltf_data->overrides)[pair.first] = currentOverride->next_override;
                }
                gltf_data->all_overrides->erase(
                    std::remove_if(gltf_data->all_overrides->begin(), gltf_data->all_overrides->end(),
                [&overrideToRemove](const lv_gltf_override_t & item) {
                    return compareOverrides(item, *overrideToRemove);
                }),
                gltf_data->all_overrides->end());
                return true; // Successfully removed
            }
            previousOverride = currentOverride;
            if(currentOverride != nullptr) {
                if(currentOverride->next_override != nullptr) {
                    currentOverride = currentOverride->next_override;
                }
                else {
                    currentOverride = nullptr;
                }
            }
        }
    }
    return false; // Override not found
}

/*
void lv_gltf_data_cleanup_overrides(lv_gltf_data_t * gltf_data) {
    for (auto& pair : *gltf_data->overrides) {
        lv_gltf_override_struct* currentOverride = pair.second.next_override; // Start with the first override
        lv_gltf_override_struct* toDelete = nullptr;

        // Delete the first override if it exists
        if (currentOverride != nullptr) {
            toDelete = &pair.second; // Start with the top-most override
            while (toDelete != nullptr) {
                lv_gltf_override_struct* next_override = toDelete->next_override; // Store the next override
                delete toDelete; // Delete the current override
                toDelete = next_override; // Move to the next override
            }
        }
    }

    // Clear the overrides map
    gltf_data->overrides->clear();
}*/
