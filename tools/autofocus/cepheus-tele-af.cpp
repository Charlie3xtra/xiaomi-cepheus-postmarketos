// SPDX-License-Identifier: GPL-2.0
/*
 * cepheus-tele-af: experimental one-shot autofocus for Xiaomi Mi 9.
 * Adapted from the local ZCode prototype; corrections verified 2026-11-07.
 * Captures XRGB8888 through libcamera's simple software ISP and scores
 * luminance gradients. Motor access is restricted to the telephoto
 * AK7374 on I2C bus 4 / address 0x0c, with a 10-bit DAC (code << 6).
 * Main-camera LC898217XC autofocus remains unresolved.
 * See README.md and use the wrapper; the inherited CLI defaults are
 * intentionally rejected by the motor-access guard.
 */
#include <cerrno>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fcntl.h>
#include <getopt.h>
#include <mutex>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include <fcntl.h>
#include <libcamera/libcamera.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

using namespace libcamera;

#define I2C_BUS  "/dev/i2c-2"
#define AF_ADDR  0x33
#define AF_REG   0x84
#define DAC_MAX  1900	/* stay below the 2047 hardware limit */
#define DAC_MIN  0

/* Runtime-configurable actuator/camera selection */
static int af_bus = 2;          /* /dev/i2c-N */
static int af_addr = 0x33;
static int af_reg = 0x84;       /* register for DAC (lc8982) */
static int af_max = 1900;
static int chip_ak7374 = 0;     /* 0 = lc8982 (reg + 16bit BE), 1 = ak7374 (reg00 10-bit code, left aligned) */
static const char *cam_needle = "ac4a000";	/* camera id substring */
static const char *cam_needle2 = "i2c-bus@1";

static int i2c_fd = -1;
static bool measure_only = false;

static int i2c_open_bus()
{
	char path[32];
	snprintf(path, sizeof(path), "/dev/i2c-%d", af_bus);
	i2c_fd = open(path, O_RDWR);
	if (i2c_fd < 0) {
		fprintf(stderr, "open %s: %s\n", path, strerror(errno));
		return -1;
	}
	if (ioctl(i2c_fd, I2C_SLAVE, af_addr) < 0) {
		fprintf(stderr, "I2C_SLAVE 0x%02x: %s\n", af_addr, strerror(errno));
		return -1;
	}
	return 0;
}

static int af_write_dac(uint16_t dac)
{
	uint8_t buf[3];
	if (chip_ak7374) {
		/* AK7374: 10-bit position << 6, big endian, registers 0x00/0x01. */
		buf[0] = 0x00;
		buf[1] = uint8_t((dac >> 2) & 0xff);
		buf[2] = uint8_t((dac & 0x03) << 6);
	} else {
		/* LC898217XC: reg 0x84, 16-bit BE value */
		buf[0] = uint8_t(af_reg);
		buf[1] = uint8_t(dac >> 8);
		buf[2] = uint8_t(dac & 0xff);
	}
	if (write(i2c_fd, buf, 3) != 3) {
		fprintf(stderr, "af write dac %u: %s\n", dac, strerror(errno));
		return -1;
	}
	return 0;
}

static int af_read_dac(uint16_t *dac)
{
	uint8_t reg = chip_ak7374 ? 0x00 : uint8_t(af_reg);
	uint8_t b[2];
	i2c_msg msgs[2] = {
		{ .addr = uint16_t(af_addr), .flags = 0, .len = 1, .buf = &reg },
		{ .addr = uint16_t(af_addr), .flags = I2C_M_RD, .len = 2, .buf = b },
	};
	i2c_rdwr_ioctl_data data = { .msgs = msgs, .nmsgs = 2 };
	if (ioctl(i2c_fd, I2C_RDWR, &data) < 0) {
		fprintf(stderr, "af read: %s\n", strerror(errno));
		return -1;
	}
	*dac = ((uint16_t(b[0]) << 8) | b[1]) >> (chip_ak7374 ? 6 : 0);
	return 0;
}

static double focus_score(const uint8_t *base, unsigned int stride,
			  unsigned int width, unsigned int height,
			  unsigned int bpp)
{
	unsigned int x0 = width / 4, x1 = width - width / 4;
	unsigned int y0 = height / 4, y1 = height - height / 4;
	uint64_t sum = 0;

	for (unsigned int row = y0; row < y1 - 1; row++) {
		const uint8_t *r = base + size_t(row) * stride;
		for (unsigned int x = x0; x < x1 - 1; x++) {
			auto luma = [](const uint8_t *p) { return (29 * p[0] + 150 * p[1] + 77 * p[2] + 128) >> 8; };
            int center = luma(r + x * bpp);
            unsigned int gx = abs(luma(r + (x + 1) * bpp) - center);
            unsigned int gy = abs(luma(r + x * bpp + stride) - center);
			sum += gx + gy;
		}
	}
	uint64_t n = uint64_t(x1 - x0 - 1) * (y1 - y0 - 1);
	return n ? double(sum) / double(n) : -1.0;
}

struct Grab {
	std::mutex m;
	std::condition_variable cv;
	std::atomic<uint64_t> frames{ 0 };	/* needs <atomic> */
	double last_score = -1.0;

	void push(double s)
	{
		std::lock_guard<std::mutex> lock(m);
		last_score = s;
		frames.fetch_add(1);
		cv.notify_all();
	}
};

static double score_frame(const FrameBuffer *fb, const StreamConfiguration &cfg)
{
	if (fb->planes().empty())
		return -1.0;
	const FrameBuffer::Plane &p = fb->planes()[0];
	if (cfg.pixelFormat != formats::XRGB8888 || cfg.size.width < 4 || cfg.size.height < 4 ||
        cfg.stride < cfg.size.width * 4 ||
        size_t(cfg.stride) * (cfg.size.height - 1) + cfg.size.width * 4 > p.length ||
        p.offset > SIZE_MAX - p.length)
        return -1.0;
    void *mem = mmap(nullptr, p.offset + p.length, PROT_READ, MAP_SHARED, p.fd.get(), 0);
	if (mem == MAP_FAILED)
		return -1.0;
	/* soft ISP outputs 32bpp XRGB8888 for viewfinder streams */
	double s = focus_score(static_cast<const uint8_t *>(mem) + p.offset,
			       cfg.stride, cfg.size.width, cfg.size.height, 4);
	munmap(mem, p.offset + p.length);
	return s;
}

class AfSession
{
public:
	int run(bool coarse_only)
	{
		cm_.start();
		camera_ = findMain();
		if (!camera_) {
			fprintf(stderr, "main camera (imx586) not found\n");
			return 1;
		}
		if (camera_->acquire()) {
			fprintf(stderr, "acquire failed\n");
			return 1;
		}

		cfg_ = camera_->generateConfiguration({ StreamRole::Viewfinder });
		StreamConfiguration &sc = cfg_->at(0);
		sc.pixelFormat = formats::XRGB8888;
		sc.size = Size(1440, 1080);	/* 4:3, fills 4000x3000 */
		cfg_->validate();
		printf("stream: %s\n", sc.toString().c_str());
		if (camera_->configure(cfg_.get()))
			return 1;

		allocator_ = new FrameBufferAllocator(camera_);
		allocator_->allocate(sc.stream());
		const auto &buffers = allocator_->buffers(sc.stream());
		for (unsigned int i = 0; i < 4 && i < buffers.size(); i++) {
			std::unique_ptr<Request> r = camera_->createRequest();
			r->addBuffer(sc.stream(), buffers[i].get());
			requests_.push_back(std::move(r));
		}

		camera_->requestCompleted.connect(this, &AfSession::onComplete);
		if (camera_->start()) {
			fprintf(stderr, "start failed\n");
			return 1;
		}
		for (auto &r : requests_)
			camera_->queueRequest(r.get());

        int result = 0;
        if (measure_only) {
            std::unique_lock<std::mutex> lock(grab_.m);
            const bool ready = grab_.cv.wait_for(lock, std::chrono::seconds(6), [&] {
                return grab_.frames.load() >= 30;
            });
            printf("MEASURE_ONLY frames=%llu score=%.3f (no actuator access)\n",
                   (unsigned long long)grab_.frames.load(), grab_.last_score);
            if (!ready || grab_.last_score < 0) result = 1;
        } else {
            {
                std::unique_lock<std::mutex> lock(grab_.m);
                if (!grab_.cv.wait_for(lock, std::chrono::seconds(6), [&] {
                    return grab_.frames.load() >= 30;
                })) result = 1;
            }
            if (!result) {
            uint16_t best = sweep(coarse_only);
            if (best == 0xffff || af_write_dac(best) < 0) result = 1;
            else printf("AF_RESULT best=%u\n", best);
            }
        }
		usleep(200000);

        { std::lock_guard<std::mutex> lock(queueMutex_); stopping_ = true; }
        camera_->stop();
		allocator_->free(sc.stream());
		delete allocator_;
		camera_->release();
		return result;
	}

private:
	CameraManager cm_;
	std::shared_ptr<CameraConfiguration> cfg_;
	std::shared_ptr<Camera> camera_;
	FrameBufferAllocator *allocator_ = nullptr;
	std::vector<std::unique_ptr<Request>> requests_;
	Grab grab_;
	std::mutex queueMutex_;
	bool stopping_ = false;

	std::shared_ptr<Camera> findMain()
	{
		for (auto const &cam : cm_.cameras())
			if (cam->id().find(cam_needle) != std::string::npos &&
			    cam->id().find(cam_needle2) != std::string::npos)
				return cam;
		return nullptr;
	}

	void onComplete(Request *request)
	{
        std::lock_guard<std::mutex> queueLock(queueMutex_);
        if (stopping_ || request->status() == Request::RequestCancelled) return;
        double s = -1.0;
        const Stream *stream = nullptr;
		FrameBuffer *fb = nullptr;
		for (auto &it : request->buffers()) {
			s = score_frame(it.second, it.first->configuration());
			stream = it.first;
			fb = it.second;
		}
		grab_.push(s);
		/* reset status (Complete -> Pending); reuse() drops buffers, re-add */
		request->reuse();
		if (stream && fb)
			request->addBuffer(stream, fb);
        int ret = camera_->queueRequest(request);
        if (ret) fprintf(stderr, "AF_QUEUE_ERROR=%d\n",ret);
	}

	double measure(uint16_t dac)
	{
		uint64_t before;
		if (af_write_dac(dac) < 0)
			return -1.0;
        usleep(120000); /* Start counting only after the mechanical settling delay. */
        before = grab_.frames.load();
		std::unique_lock<std::mutex> lock(grab_.m);
		bool ok = grab_.cv.wait_for(lock, std::chrono::milliseconds(2000),
					    [&] { return grab_.frames.load() >= before + requests_.size() + 2; });
		if (!ok)
			return -1.0;
		double s = grab_.last_score;
		lock.unlock();
		printf("dac %5u  score %9.1f\n", dac, s);
		fflush(stdout);
		return s;
	}

	uint16_t sweep(bool coarse_only)
	{
		uint16_t best = 0xffff;
		double best_score = -1.0;
		unsigned int step = af_max / 12 > 0 ? unsigned(af_max / 12) : 150;
		unsigned int lo = 0, hi = unsigned(af_max);

		for (int round = 0; round < (coarse_only ? 1 : 2); round++) {
			for (unsigned int dac = lo; dac <= hi; dac += step) {
				double s = measure(uint16_t(dac));
				if (s > best_score) {
					best_score = s;
					best = uint16_t(dac);
				}
			}
			if (coarse_only)
				break;
			if (best == 0xffff) return best;
			unsigned int span = step * 2;
			lo = best > span ? best - span : 0;
			hi = best + span < unsigned(af_max) ? best + span : unsigned(af_max);
			step = 25;
			if (round == 0) printf("-- fine sweep %u..%u step %u --\n", lo, hi, step);
		}
		return best;
	}
};

int main(int argc, char **argv)
{
	static struct option longopts[] = {
		{ "measure-only", no_argument, nullptr, 'n' },
		{ "set", required_argument, nullptr, 's' },
		{ "get", no_argument, nullptr, 'g' },
		{ "park", no_argument, nullptr, 'p' },
		{ "coarse-only", no_argument, nullptr, 'c' },
		{ "bus", required_argument, nullptr, 'B' },
		{ "addr", required_argument, nullptr, 'A' },
		{ "max", required_argument, nullptr, 'M' },
		{ "chip", required_argument, nullptr, 'C' },
		{ "camera", required_argument, nullptr, 'K' },
		{ nullptr, 0, nullptr, 0 },
	};
	int opt;
	uint16_t set_dac = 0xffff;
	bool do_get = false, do_park = false, coarse_only = false;
	while ((opt = getopt_long(argc, argv, "ns:gpcB:A:M:C:K:", longopts, nullptr)) != -1) {
		switch (opt) {
		case 'n': measure_only = true; break;
		case 's': set_dac = uint16_t(atoi(optarg)); break;
		case 'g': do_get = true; break;
		case 'p': do_park = true; break;
		case 'c': coarse_only = true; break;
		case 'B': af_bus = atoi(optarg); break;
		case 'A': af_addr = int(strtoul(optarg, nullptr, 0)); break;
		case 'M': af_max = atoi(optarg); break;
		case 'C': chip_ak7374 = strcmp(optarg, "ak7374") == 0; break;
		case 'K':
			if (strcmp(optarg, "tele") == 0) {
				cam_needle = "ac4b000";
				cam_needle2 = "camera@10";
			} else if (strcmp(optarg, "front") == 0) {
				cam_needle = "ac4b000";
				cam_needle2 = "camera@2d";
			} else if (strcmp(optarg, "wide") == 0) {
				cam_needle = "ac4a000";
				cam_needle2 = "i2c-bus@0";
			}
			break;
		default: return 2;
		}
	}
	printf("config: bus=%d addr=0x%02x max=%d chip=%s camera=%s/%s\n",
	       af_bus, af_addr, af_max, chip_ak7374 ? "ak7374" : "lc8982",
	       cam_needle, cam_needle2);

    if (measure_only) {
        if (do_get || do_park || set_dac != 0xffff) return 2;
        AfSession session;
        return session.run(true);
    }
    if (!chip_ak7374 || af_bus != 4 || af_addr != 0x0c ||
        strcmp(cam_needle2, "camera@10") || af_max < 0 || af_max > 1023) {
        fprintf(stderr, "Motor access restricted to verified tele bus4/0x0c, AK7374, max<=1023\n");
        return 2;
    }
    if (i2c_open_bus() < 0)
		return 1;

	if (do_get) {
		uint16_t dac;
		if (af_read_dac(&dac) == 0)
			printf("dac=%u\n", dac);
		return 0;
	}
    if (chip_ak7374) {
        const uint8_t active[] = { 0x02, 0x00 };
        if (write(i2c_fd, active, sizeof(active)) != sizeof(active)) {
            perror("AK7374 active mode"); return 1;
        }
        usleep(10000);
    }
	if (do_park)
		return af_write_dac(0) < 0;
	if (set_dac != 0xffff) {
		if (set_dac > af_max)
			set_dac = uint16_t(af_max);
		return af_write_dac(set_dac) < 0;
	}

	AfSession session;
	return session.run(coarse_only);
}
