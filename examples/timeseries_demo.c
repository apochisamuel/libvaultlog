#include "tm/session.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    printf("libvaultlog time-series demo\n");
    printf("============================\n\n");

    tm_session_t *sess = tm_session_create(1);
    if (!sess) { fprintf(stderr, "Session create failed\n"); return 1; }
    sess->active = true;

    /* Create segments for different time ranges */
    printf("Creating segments...\n");
    for (int i = 0; i < 4; i++) {
        tm_segment_t *seg = tm_segment_create(&sess->segments,
                                                 sess->clock.current_epoch,
                                                 &sess->slab, NULL);
        if (seg) {
            char data[64];
            int len = snprintf(data, sizeof(data), "ts_data_range_%d", i);
            tm_segment_append(seg, (uint8_t *)data, (size_t)len);
            tm_catalog_register(&sess->catalog, seg->segment_id,
                                  seg, sess->clock.current_epoch);
            printf("  Segment %u: %zu bytes\n", seg->segment_id, seg->data_len);
        }
    }

    /* Create index entries */
    printf("\nCreating index entries...\n");
    for (uint64_t ts = 1000; ts <= 4000; ts += 1000) {
        uint32_t seg_id = (uint32_t)(ts / 1000);
        tm_index_insert(&sess->index, ts, seg_id, 0, NULL, NULL);
        printf("  Index: ts=%lu -> seg %u\n", (unsigned long)ts, seg_id);
    }

    /* Capture snapshot */
    printf("\nCapturing snapshot...\n");
    tm_snapshot_t *snap = tm_snapshot_capture(1, sess->clock.current_epoch,
                                                &sess->segments);
    if (snap) {
        printf("  Snapshot %u: %u segments captured\n",
               snap->snapshot_id, snap->segment_count);
    }

    /* Advance epochs */
    for (int i = 0; i < 3; i++) {
        tm_clock_advance_epoch(&sess->clock);
    }
    printf("\nEpoch advanced to %u\n", sess->clock.current_epoch);

    /* Set retention policy */
    tm_retention_set_policy(&sess->retention, 1, 2);
    printf("Retention policy set (max_age=2)\n");

    /* Cleanup */
    if (snap) tm_snapshot_release(snap);
    tm_session_destroy(sess);
    printf("\nDemo complete.\n");
    return 0;
}
