/*
 * basic_ingest.c — demonstrates creating a session, appending
 * time-series data, and running compaction.
 */
#include "tm/session.h"
#include <stdio.h>

int main(void)
{
    tm_session_t *sess = tm_session_create(1);
    if (!sess) {
        fprintf(stderr, "Failed to create session\n");
        return 1;
    }

    printf("Session created (id=%u)\n", sess->session_id);
    printf("  Clock epoch: %u\n", tm_clock_epoch(&sess->clock));
    printf("  Segment count: %zu\n", sess->segments.count);

    /* Manual operations for demonstration */
    sess->active = true;

    tm_segment_t *seg = tm_segment_create(&sess->segments,
                                             sess->clock.current_epoch,
                                             &sess->slab, NULL);
    if (seg) {
        printf("  Created segment id=%u\n", seg->segment_id);

        uint8_t data[] = "Hello VaultLog time-series storage!";
        tm_segment_append(seg, data, sizeof(data) - 1);
        printf("  Appended %zu bytes\n", seg->data_len);

        tm_catalog_register(&sess->catalog, seg->segment_id,
                              seg, sess->clock.current_epoch);
        printf("  Registered in catalog\n");
    }

    tm_session_destroy(sess);
    printf("Session destroyed.\n");
    return 0;
}
