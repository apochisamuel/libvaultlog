#include "tm/tidemark.h"
#include <string.h>

tm_status_t tm_tidemark_init(tm_tidemark_t *tm)
{
    if (!tm) return TM_ERR_BAD_STATE;
    memset(tm, 0, sizeof(*tm));
    return TM_OK;
}

tm_status_t tm_tidemark_advance(tm_tidemark_t *tm, uint32_t new_epoch)
{
    if (!tm) return TM_ERR_BAD_STATE;
    if (new_epoch > tm->watermark_epoch) {
        tm->watermark_epoch = new_epoch;
        tm->advance_count++;
        tm->zone_count = new_epoch / 4;
    }
    return TM_OK;
}

bool tm_tidemark_zone_eligible(tm_tidemark_t *tm, uint32_t epoch)
{
    if (!tm) return false;
    return epoch < tm->watermark_epoch;
}

uint32_t tm_tidemark_watermark(const tm_tidemark_t *tm)
{
    return tm ? tm->watermark_epoch : 0;
}
