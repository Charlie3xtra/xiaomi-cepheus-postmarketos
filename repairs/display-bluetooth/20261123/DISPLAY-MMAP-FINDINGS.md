# Display write versus mmap semantics in final6.17 source

The coordinating agent reports the user physically confirmed that the exclusively written, brightness300 color bars/grid/text are correct. This is a successful functional display-path observation, not merely framebuffer readback. Prior dark/overwritten patterns were confounded by ServerDeck's periodic redraw and its stop-finally backlight0 behavior. The daily mmap writer remains to be tested against the write path under otherwise identical conditions.

## Confirmed source behavior

- drivers/gpu/drm/msm/msm_fbdev.c: msm_fbdev_mmap delegates directly to **drm_gem_prime_mmap**. The FB operations use that custom mapper, not fb_deferred_io_mmap.
- drivers/gpu/drm/drm_prime.c: drm_gem_prime_mmap installs the GEM object's vm_ops and vm_private_data.
- drivers/gpu/drm/msm/msm_gem.c: object mmap sets PFNMAP/DONTEXPAND/DONTDUMP and page protections. Its vm_ops contain fault/open/close, **no fbdev page_mkwrite dirty-tracking callback**. Neither normal mapped stores nor the shown GEM fault path calls drm_fb_helper_damage_range.
- drivers/video/fbdev/core/fb_defio.c: fb_deferred_io_mmap installs a different VM operations table for deferred-I/O dirty-page handling. This mapper is not used by the MSM fbdev path above. Simply setting FBINFO_VIRTFB does not replace vm_ops or add damage tracking.
- include/linux/fb.h: FB_GEN_DEFAULT_DEFERRED_SYSMEM_OPS generates msm_fbdev_defio_write, which records the original offset, calls fb_sys_write, and on a positive return calls drm_fb_helper_damage_range(info, offset, bytes_written). Thus ordinary file write/pwrite marks the successfully written range dirty; a zero-byte write does not.
- drm_fb_helper_damage_range converts offset/length using fix.line_length, schedules damage_work, then invokes msm_fbdev_fb_dirty. This invokes the framebuffer dirty hook for a nonempty clip.
- drivers/gpu/drm/msm/msm_fb.c: msm_framebuffer_dirtyfb calls drm_atomic_helper_dirtyfb when the framebuffer is used by a display requiring dirty updates. drivers/gpu/drm/msm/disp/dpu1/dpu_crtc.c: dpu_crtc_needs_dirtyfb returns true for command-mode DSI and self-refresh. This provides the explicit commit/refresh path that raw mapped writes lack.

Conclusion: **this exact mmap path bypasses fbdev dirty-page tracking; write triggers damage.** On a command-mode display that is a concrete reason memory can look correct while changes do not update the panel until another commit. It is a strong software-side candidate, supported by the successful write test. It does not establish that every historical white block shares this cause, nor rule out all DSI errors.

## Recommended low-impact application test

Keep the known-good #8 kernel first. Render into a private staging buffer and submit by os.pwrite (or lseek+write under a single-writer lock). Preserve stride4352, BGRX order, native orientation and valid padding. Loop over short writes/EINTR; only count a frame submitted once all requested bytes were accepted. Full padded frames are simplest for the first comparison; dirty row ranges can be an optimization after correctness. Do not rely on mmap.flush(), fsync, or zero-byte write to request DRM damage; their framebuffer semantics do not provide that path here.

Use one owner for drawing/unblank/backlight. After a wake/unblank, submit a complete frame, then restore the intended brightness, with any required settling/retry governed by observed state. Ensure shutdown/cleanup setting brightness0 is deliberate and cannot race a resumed writer. Repeat cold start and blank/unblank with physical feedback. Log actual write errors and timeouts, not framebuffer reads alone. The coordinator owns this application change and device testing.

Do not replace .fb_mmap with fb_deferred_io_mmap as a one-line kernel patch: that would require deferred-I/O setup, page access/lifetime and GEM mapping compatibility work. Keep the #9 FBINFO/DSI-diagnostic kernel separate until evidence requires it. No new display kernel changes or phone operations were made for this investigation.
