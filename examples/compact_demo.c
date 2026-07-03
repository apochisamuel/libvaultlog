#include "tm/session.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    printf("libvaultlog compaction demo\n");
    printf("===========================\n\n");
    tm_session_t *sess = tm_session_create(1);
    if (!sess) return 1;
    sess->active = true;
    printf("Creating and filling segments...\n");
    for (int i = 0; i < 4; i++) {
        tm_segment_t *seg = tm_segment_create(&sess->segments,
                                                 sess->clock.current_epoch,
                                                 &sess->slab, NULL);
        if (seg) {
            uint8_t data[300];
            memset(data, (uint8_t)(0x10 + i), sizeof(data));
            tm_segment_append(seg, data, sizeof(data));
            seg->sealed = true;
            printf("  Segment %u: %zu bytes (sealed)\n",
                   seg->segment_id, seg->data_len);
        }
    }
    printf("\nActive segments before compact: ");
    uint32_t active = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++)
        if (sess->segments.segments[i] && sess->segments.segments[i]->active)
            active++;
    printf("%u\n", active);
    printf("Triggering compaction...\n");
    tm_compact_trigger(&sess->compact, &sess->segments, &sess->slab,
                          &sess->index, sess->clock.current_epoch);
    active = 0;
    for (size_t i = 0; i < TM_MAX_SEGMENTS; i++)
        if (sess->segments.segments[i] && sess->segments.segments[i]->active)
            active++;
    printf("Active segments after compact: %u\n", active);
    printf("Merges completed: %u\n", sess->compact.merges_completed);
    tm_session_destroy(sess);
    printf("\nDemo complete.\n");
    return 0;
}
