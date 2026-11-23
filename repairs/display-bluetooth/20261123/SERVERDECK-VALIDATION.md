# ServerDeck framebuffer refresh fix — 2026-11-23

This is a userspace fix tested on the installed 6.17.0-sm8150 #8 kernel. No boot partition write or kernel replacement was required.

The previous display agent wrote directly through a GEM mmap. In this kernel msm_fbdev_mmap uses drm_gem_prime_mmap rather than fb_deferred_io_mmap, so these stores bypass fbdev damage tracking. By comparison the standard fb_write callback reports modified ranges to DRM. The previous wake path also drew before unblanking the panel.

0003-serverdeck-fbdev-refresh.patch changes the agent to submit complete stride-padded frames with pwrite, handles short writes, validates the framebuffer geometry and image size, and powers the panel before drawing while the backlight is still off. It then restores the normal brightness. The installed layout remains1080×2340×32 with stride4352; no framebuffer pitch change is needed.

Validation:
- Python syntax check passed.
- User confirmed the exclusive standard-write RGB bars, straight grid and text were correct.
- Six screen-on/off cycles completed, checking brightness70/0 each time; the service remained active.
- No new dsi_err message during those six cycles. Seven encoder-disabled IRQ warnings remained.
- User then confirmed the normal ServerDeck status interface was clear without white blocks or garbled text.
- The original agent was backed up privately. The test image was removed, normal timeout restored, and the temporary rollback timer cancelled after validation.

Limits: this validates the userspace refresh change and those cycles only. It does not establish that all intermittent white artifacts, boot handoff problems, GPU issues or DSI/IRQ warnings are fixed. A reboot, longer use and compositor-driven desktop still need validation. No private screenshots, framebuffer dumps, network identifiers or owner boot images are included.

The patch applies to the existing ServerDeck agent, not a stock postmarketOS component. Normalize the original file to LF before applying if necessary. Before-source SHA256 (original CRLF bytes):cc7d9dad81d7b84df0912c332df054a06bb37fcfbe728edbe977c395b67afa3d. Installed after-source SHA256 (LF):784b10f60b5b06836d727c7feac6b500330c3dec3df4c1521bb367b883a33601.

Reference: https://github.com/torvalds/linux/blob/v6.17/drivers/gpu/drm/msm/msm_fbdev.c and https://github.com/torvalds/linux/blob/v6.17/include/linux/fb.h .
