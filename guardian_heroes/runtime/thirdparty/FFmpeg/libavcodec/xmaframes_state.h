/* Project XMAFRAMES frame-boundary state. Same codec build and float ABI only. */
#ifndef AVCODEC_XMAFRAMES_STATE_H
#define AVCODEC_XMAFRAMES_STATE_H

#include <stddef.h>
#include <stdint.h>

struct AVCodecContext;

/* XMA has at most two channels and 512 samples per frame. Keep the complete
 * used XMA output workspace, including its IMDCT overlap, without host pointers.
 * Bit readers, transform workspaces and scale factors are rebuilt per frame. */
typedef struct FFXmaFramesState {
    uint32_t version;
    int32_t sample_rate;
    int32_t channels;
    uint32_t frame_num;
    int32_t skip_frame;
    int32_t packet_loss;
    int32_t prev_block_len[2];
    float output[2][512 + 512 / 2];
} FFXmaFramesState;

int ff_xmaframes_save_state(const struct AVCodecContext *avctx,
                            FFXmaFramesState *state);
int ff_xmaframes_restore_state(struct AVCodecContext *avctx,
                               const FFXmaFramesState *state);

#endif
