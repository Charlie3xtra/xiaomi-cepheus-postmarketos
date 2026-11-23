/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace libcamera {

/* Scale the four CFA planes independently, preserving their Bayer order.
 * Input and output coordinates include the complete active sensor rectangle.
 * Extra columns supply interpolation neighbours, not a crop of the image.
 */
class Raw10FullFrame
{
public:
	struct Axis {
		unsigned int lo, hi, weight;
	};
	unsigned int width = 0, height = 0, stride = 0;
	unsigned int imageX = 0, imageY = 0, imageWidth = 0, imageHeight = 0;
	static constexpr unsigned int border = 4;

	bool configure(unsigned int inWidth, unsigned int inHeight, unsigned int inStride,
		       unsigned int outWidth, unsigned int outHeight)
	{
		if (inWidth < 4 || inWidth % 4 || inHeight < 2 || inHeight % 2 ||
		    inStride < inWidth / 4 * 5 || outWidth < 4 || outWidth % 4 ||
		    outHeight < 2 || outHeight % 2)
			return false;
		inputWidth_ = inWidth;
		inputStride_ = inStride;
		width = outWidth;
		height = outHeight;
		stride = (width + 2 * border) / 4 * 5;
		if (uint64_t(width) * inHeight <= uint64_t(height) * inWidth) {
			imageWidth = width;
			imageHeight = (uint64_t(width) * inHeight / inWidth) & ~1U;
		} else {
			imageHeight = height;
			imageWidth = (uint64_t(height) * inWidth / inHeight) & ~3U;
		}
		if (!imageWidth || !imageHeight)
			return false;
		imageX = ((width - imageWidth) / 2) & ~3U;
		imageY = ((height - imageHeight) / 2) & ~1U;
		x_ = axis(inWidth / 2, imageWidth / 2);
		y_ = axis(inHeight / 2, imageHeight / 2);
		return true;
	}

	/* yBegin/yEnd are non-overlapping, even row boundaries for the workers. */
	void rows(const uint8_t *input, uint8_t *output, unsigned int yBegin,
		  unsigned int yEnd, std::vector<uint8_t> &scratch) const
	{
		const unsigned int rowBytes = inputWidth_ / 4 * 5;
		scratch.resize(4 * rowBytes);
		for (unsigned int y = yBegin; y < yEnd; y += 2) {
			uint8_t *dest[2] = { output + size_t(y) * stride,
					    output + size_t(y + 1) * stride };
			std::memset(dest[0], 0, 2 * stride);
			if (y < imageY || y >= imageY + imageHeight)
				continue;
			const Axis &ay = y_[(y - imageY) / 2];
			for (unsigned int p = 0; p < 2; ++p) {
				std::memcpy(scratch.data() + (2 * p) * rowBytes,
					    input + size_t(2 * ay.lo + p) * inputStride_, rowBytes);
				std::memcpy(scratch.data() + (2 * p + 1) * rowBytes,
					    input + size_t(2 * ay.hi + p) * inputStride_, rowBytes);
			}
			for (unsigned int p = 0; p < 2; ++p) {
				const uint8_t *top = scratch.data() + (2 * p) * rowBytes;
				const uint8_t *bottom = top + rowBytes;
				for (unsigned int x = 0; x < imageWidth; x += 4) {
					uint16_t v[4];
					for (unsigned int k = 0; k < 4; ++k) {
						const Axis &ax = x_[(x + k) / 2];
						const unsigned int phase = k & 1;
						unsigned int a = blend(sample(top, 2 * ax.lo + phase),
								       sample(top, 2 * ax.hi + phase), ax.weight);
						unsigned int b = blend(sample(bottom, 2 * ax.lo + phase),
								       sample(bottom, 2 * ax.hi + phase), ax.weight);
						v[k] = blend(a, b, ay.weight);
					}
					pack(dest[p] + (border + imageX + x) / 4 * 5, v);
				}
				/* Replicate a complete CFA group at horizontal boundaries. */
				std::memcpy(dest[p], dest[p] + 5, 5);
				std::memcpy(dest[p] + stride - 5, dest[p] + stride - 10, 5);
			}
		}
	}

	static uint16_t sample(const uint8_t *row, unsigned int x)
	{
		const uint8_t *p = row + (x / 4) * 5;
		return (uint16_t(p[x % 4]) << 2) | ((p[4] >> (2 * (x % 4))) & 3);
	}

	static void pack(uint8_t *p, const uint16_t v[4])
	{
		p[4] = 0;
		for (unsigned int k = 0; k < 4; ++k) {
			p[k] = v[k] >> 2;
			p[4] |= (v[k] & 3) << (2 * k);
		}
	}

private:
	static unsigned int blend(unsigned int a, unsigned int b, unsigned int weight)
	{
		return (a * (65536 - weight) + b * weight + 32768) >> 16;
	}
	static std::vector<Axis> axis(unsigned int input, unsigned int output)
	{
		std::vector<Axis> map(output);
		for (unsigned int i = 0; i < output; ++i) {
			/* Align plane endpoints so every edge of the sensor is retained. */
			uint64_t at = output > 1 ? uint64_t(i) * (input - 1) * 65536 / (output - 1) : 0;
			map[i] = { unsigned(at >> 16), std::min(unsigned(at >> 16) + 1, input - 1),
				   unsigned(at & 65535) };
		}
		return map;
	}
	unsigned int inputWidth_ = 0, inputStride_ = 0;
	std::vector<Axis> x_, y_;
};

} /* namespace libcamera */
