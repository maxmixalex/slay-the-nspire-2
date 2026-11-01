"""Source-bounded temporal color segments; no runtime format or frame removal.

Each pixel keeps one color for the longest consecutive source interval whose
R/G/B error boxes still intersect. Static pixels stay exact. The two ends of a
loop share a legal color when possible; otherwise each endpoint is source-exact.
There is no shared spatial palette, rescaling, added green quantization, or new
calculator state. The existing copy-patch and retained-group encoders follow.
"""
from __future__ import annotations
import hashlib
from pathlib import Path
import numpy as np

POLICY = 'longest-legal-color-segment-source-seam-r1-g2-b1-v2'


def held_colors(frames):
    if not frames or len(frames) > 4096:
        raise ValueError('Ancient hold needs a bounded nonempty source loop')
    shape = frames[0].shape
    if (len(shape) != 2 or not 0 < shape[0] <= 240 or not 0 < shape[1] <= 320 or
            any(frame.shape != shape or frame.dtype != np.dtype('<u2') for frame in frames)):
        raise ValueError('Ancient hold requires equal native RGB565 frames')
    size, count = len(frames), frames[0].size
    limit = np.array([1, 2, 1], dtype=np.int16)

    def channels(raw):
        raw = raw.reshape(-1)
        return np.stack(((raw >> 11) & 31, (raw >> 5) & 63, raw & 31), axis=1).astype(np.int16)

    def encode(value):
        return ((value[:, 0].astype(np.uint16) << 11) |
                (value[:, 1].astype(np.uint16) << 5) | value[:, 2].astype(np.uint16)).astype('<u2')

    first, last = channels(frames[0]), channels(frames[-1])
    low, high = np.maximum(first-limit, last-limit), np.minimum(first+limit, last+limit)
    separate = np.any(low > high, axis=1)
    left = np.clip(first, low, high)
    left[separate] = first[separate]
    right = left.copy()
    right[separate] = last[separate]
    left565, right565 = encode(left), encode(right)
    prefix = np.full(count, size, dtype=np.int32)
    suffix = np.zeros(count, dtype=np.int32)
    for i, frame in enumerate(frames):
        bad = np.any(np.abs(channels(frame)-left) > limit, axis=1)
        prefix[(prefix == size) & bad] = i
    for i in range(size-1, -1, -1):
        bad = np.any(np.abs(channels(frames[i])-right) > limit, axis=1)
        suffix[(suffix == 0) & bad] = i+1

    # Host-only end markers avoid a Python object per pixel/segment. Expanding
    # backwards restores a color to exactly its original33ms frame interval.
    marks = np.zeros((size, count), dtype=bool)
    ends = np.zeros((size, count), dtype='<u2')
    low = np.zeros((count, 3), dtype=np.int16)
    high, anchor = low.copy(), low.copy()
    segments = 0
    for i, frame in enumerate(frames):
        current = channels(frame)
        active = (i >= prefix) & (i < suffix)
        begin = active & (prefix == i)
        low[begin], high[begin], anchor[begin] = current[begin]-limit, current[begin]+limit, current[begin]
        newlow, newhigh = np.maximum(low, current-limit), np.minimum(high, current+limit)
        close = active & ~begin & np.any(newlow > newhigh, axis=1)
        if np.any(close):
            marks[i-1, close] = True
            ends[i-1, close] = encode(np.clip(anchor[close], low[close], high[close]))
            segments += int(close.sum())
            newlow[close], newhigh[close], anchor[close] = current[close]-limit, current[close]+limit, current[close]
        low[active], high[active] = newlow[active], newhigh[active]
        finish = active & (suffix == i+1)
        if np.any(finish):
            marks[i, finish] = True
            ends[i, finish] = encode(np.clip(anchor[finish], low[finish], high[finish]))
            segments += int(finish.sum())

    previous = np.zeros(count, dtype='<u2')
    result = [None]*size
    maximum = np.zeros(3, dtype=int)
    static = np.ones(count, dtype=bool)
    for i in range(size-1, -1, -1):
        previous[marks[i]] = ends[i, marks[i]]
        active = (i >= prefix) & (i < suffix)
        output = np.where(active, previous, np.where(i < prefix, left565, right565)).reshape(shape).astype('<u2')
        result[i] = output
        maximum = np.maximum(maximum, np.abs(channels(frames[i])-channels(output)).max(axis=0))
        static &= frames[i].reshape(-1) == frames[0].reshape(-1)
    if (np.any(maximum > limit) or
            not np.array_equal(result[0].reshape(-1)[~separate], result[-1].reshape(-1)[~separate]) or
            not np.array_equal(result[0].reshape(-1)[separate], frames[0].reshape(-1)[separate]) or
            not np.array_equal(result[-1].reshape(-1)[separate], frames[-1].reshape(-1)[separate]) or
            any(not np.array_equal(out.reshape(-1)[static], src.reshape(-1)[static])
                for out, src in zip(result, frames))):
        raise ValueError('Ancient temporal hold violated source error/static/wrap bounds')
    return result, dict(policy=POLICY, frames=size, middle_segments=segments,
        source_changes=sum(int(np.count_nonzero(frames[i] != frames[i-1])) for i in range(size)),
        candidate_pixel_updates=sum(int(np.count_nonzero(result[i] != result[i-1])) for i in range(size)),
        source_static_pixels_exact=int(static.sum()), source_exact_wrap_endpoint_pixels=int(separate.sum()),
        maximum_rgb565_channel_error=maximum.tolist(), cyclic_seam_verified=True,
        source_first_pose_exact=bool(np.array_equal(result[0], frames[0])),
        policy_module_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
