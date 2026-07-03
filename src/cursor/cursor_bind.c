#include "tm/cursor.h"
#include "tm/catalog.h"
#include <string.h>

/*
 * Cursor binding and validation.
 * Manages the relationship between cursors and their bound segments,
 * including generation tracking and catalog-mediated validation.
 */

tm_status_t tm_cursor_bind_catalog_entry(tm_cursor_t *cursor,
                                            tm_catalog_entry_t *entry)
{
    if (!cursor || !cursor->active || !entry || !entry->valid)
        return TM_ERR_BAD_STATE;

    cursor->bound_catalog_entry = entry;
    cursor->bound_segment = (tm_segment_t *)entry->segment_ref;
    cursor->bound_segment_id = entry->segment_id;
    cursor->bind_generation = entry->generation;
    cursor->position = 0;
    cursor->bound = true;
    return TM_OK;
}

bool tm_cursor_binding_valid(const tm_cursor_t *cursor, const tm_catalog_t *cat)
{
    if (!cursor || !cursor->bound || !cat) return false;

    /* Check if catalog entry still exists and matches generation */
    tm_catalog_entry_t *entry = tm_catalog_lookup(
        (tm_catalog_t *)cat, cursor->bound_segment_id);
    if (!entry || !entry->valid) return false;
    if (entry->generation != cursor->bind_generation) return false;

    return true;
}

tm_status_t tm_cursor_rebind(tm_cursor_t *cursor, tm_catalog_t *cat)
{
    if (!cursor || !cursor->active || !cat) return TM_ERR_BAD_STATE;

    tm_catalog_entry_t *entry = tm_catalog_lookup(cat, cursor->bound_segment_id);
    if (!entry || !entry->valid || !entry->segment_ref)
        return TM_ERR_NOT_FOUND;

    cursor->bound_segment = (tm_segment_t *)entry->segment_ref;
    cursor->bind_generation = entry->generation;
    cursor->bound_catalog_entry = entry;

    /* Clamp position to new segment bounds */
    tm_segment_t *seg = cursor->bound_segment;
    if (seg && cursor->position > seg->data_len)
        cursor->position = (uint32_t)seg->data_len;

    return TM_OK;
}

tm_status_t tm_cursor_invalidate(tm_cursor_t *cursor)
{
    if (!cursor) return TM_ERR_BAD_STATE;
    cursor->bound = false;
    cursor->bound_segment = NULL;
    cursor->bound_catalog_entry = NULL;
    cursor->position = 0;
    return TM_OK;
}
