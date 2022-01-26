#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>

#define magic "farbfeld"

typedef int32_t pixel[3];

pixel pal[128] = {
	{0, 0, 0},
	{UINT16_MAX, UINT16_MAX, UINT16_MAX},
	{UINT16_MAX, 0, 0},
	{0, UINT16_MAX, 0},
	{0, 0, UINT16_MAX}
};
size_t pal_len;

float
red_mean(pixel a, pixel b) {
	float mean_r =    (a[0] + b[0])/2,
		 d_r = abs(a[0] - b[0]),
		 d_g = abs(a[1] - b[1]),
		 d_b = abs(a[2] - b[2]);

	float c_1 = (512+mean_r/(float)UINT16_MAX)*d_r*d_r;
	float c_2 = 1024*d_g*d_g;
	float c_3 = (512+ (UINT16_MAX-1-mean_r)/(float)UINT16_MAX)*d_b*d_b;

	float c = sqrt(c_1 + c_2 + c_3);
	return c;
}

static
void
closest_color(pixel in, pixel *out){
	size_t  best_index = 0;
	int32_t best_delta = INT32_MAX;
	for(size_t i = 0; i < pal_len; ++i) {
		float delta = red_mean(in, pal[i]);
		if(delta < best_delta) {
			best_delta = delta;
			best_index = i;
		}
	}
	memcpy(out, &pal[best_index], sizeof(*out));
}

void
set_pal() {
	FILE *fp = fopen("pal", "r");
	if (fp != NULL) {
		size_t color_i = 0;
		int32_t input[3];
		while(fscanf(fp, "%2x%2x%2x", &input[0], &input[1], &input[2]) == 3) {
			for(size_t i = 0; i < 3; ++i)
				pal[color_i][i] = input[i] * 256;
			++color_i;
		}
		pal_len = color_i;
		return;
	}

	// get length
	size_t i = 0;
	pixel now, next;
	while(1){
		memcpy(&now,  pal[i],   sizeof(pixel));
		memcpy(&next, pal[i+1], sizeof(pixel));
		if(now[0] + now[1] + now[2] == 0 && next[0] + next[1] + next[2] == 0) {
			pal_len = i;
			return;
		}
		++i;
	}
}

int
main(int argc, char* argv[]){
	if(argc != 1){
		fputs("usage: jd <in.ff >out.ff\n", stderr);
		return 1;
	}

	// set to 2 for B/W
	set_pal();

	size_t len_magic = strlen(magic);
	uint8_t header[len_magic];
	fread(header, sizeof(*header), sizeof(header)/sizeof(*header), stdin);
	if(memcmp(magic, header, len_magic)){
		fputs("wrong header\n", stderr);
	}

	uint32_t in[2];
	fread(in, sizeof(*in), sizeof(in)/sizeof(*in), stdin);
	uint32_t w = ntohl(in[0]),
		 h = ntohl(in[1]);

	// read src img into memory
	uint16_t *out = malloc(sizeof(*out)*w*h*4); // finished image
	int32_t  *tmp = malloc(sizeof(*tmp)*w*h*3); // work-in-progress image
	for(size_t y = 0; y < h; ++y){
		for(size_t x = 0; x < w; ++x){
			// read next pixel
			uint16_t v[4];
			fread(v, sizeof(*v), sizeof(v)/sizeof(*v), stdin);

			size_t idx_out = 4 * (y * w + x);

			// fix endianness
			for(size_t i = 0; i < 4; ++i)
				v[i] = ntohs(v[i]);

			// copy alpha directly
			out[idx_out+3] = v[3];

			// tmp has only 3 channels
			size_t idx_tmp = 3 * (y * w + x);

			// copy src to tmp
			for(size_t i = 0; i < 3; ++i)
				tmp[idx_tmp+i]   = v[i];
		}
	}


	// generate dithered image
	for(size_t y = 0; y < h; ++y){
		for(size_t x = 0; x < w; ++x){
			size_t i_out = 4*(y*w+x);
			size_t i_r = 3*(y*w+x);
			size_t i_g = i_r + 1;
			size_t i_b = i_r + 2;

			// find output color
			pixel p_in = {tmp[i_r], tmp[i_g], tmp[i_b]};
			pixel p_out;
			closest_color(p_in, &p_out);

			// copy found color to output
			for(size_t o = 0; o < 3; ++o)
				out[i_out+o] = p_out[0+o];

			// find difference
			pixel d;
			for(size_t i = 0; i < 3; ++i)
				d[i] = p_in[i] - p_out[i];

			// correct future pixels
			if(x < w-1) {
				size_t offset = i_r+3;
				for(size_t i = 0; i < 3; ++i)
					tmp[offset + i] += 7*d[i] / 16;
			}
			if(y < h-1) {
				if(x < w-1) {
					size_t offset = i_r+3 * (w+1);
					for(size_t i = 0; i < 3; ++i)
						tmp[offset + i] += d[i] / 16;
				}

				if(x > 0) {
					size_t offset = i_r+3 * (w-1);
					for(size_t i = 0; i < 3; ++i)
						tmp[offset + i] += 3*d[i] / 16;
				}

				size_t offset = i_r+3 * w;
				for(size_t i = 0; i < 3; ++i)
					tmp[offset + i] += 5*d[i] / 16;
			}
		}
	}


	for(size_t i = 0; i < h*w*4; ++i)
		out[i] = htons(out[i]);

	fwrite(header, sizeof(*header), sizeof(header)/sizeof(*header), stdout);
	fwrite(in, sizeof(*in), sizeof(in)/sizeof(*in), stdout);
	fwrite(out, sizeof(*out), sizeof(*out)*w*h*4, stdout);

	return 0;
}

