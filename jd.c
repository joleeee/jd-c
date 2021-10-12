#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>

/*╔════════╤═════════════════════════════════════════════════════════╗*/
/*║ Bytes  │ Description                                             ║*/
/*╠════════╪═════════════════════════════════════════════════════════╣*/
/*║ 8      │ "farbfeld" magic value                                  ║*/
/*╟────────┼─────────────────────────────────────────────────────────╢*/
/*║ 4      │ 32-Bit BE unsigned integer (width)                      ║*/
/*╟────────┼─────────────────────────────────────────────────────────╢*/
/*║ 4      │ 32-Bit BE unsigned integer (height)                     ║*/
/*╟────────┼─────────────────────────────────────────────────────────╢*/
/*║ [2222] │ 4x16-Bit BE unsigned integers [RGBA] / pixel, row-major ║*/
/*╚════════╧═════════════════════════════════════════════════════════╝*/

#define magic "farbfeld"

struct pixel {
	int32_t r, g, b;
};

struct pixel pal[] = {
	{0, 0, 0},
	{UINT16_MAX, UINT16_MAX, UINT16_MAX},
	{UINT16_MAX, 0, 0},
	{0, UINT16_MAX, 0},
	{0, 0, UINT16_MAX}
};
size_t pal_len;

static
void
closest_color(struct pixel in, struct pixel *out){
	size_t bi = 0;
	int32_t bd = INT32_MAX;
	for(size_t i = 0; i < pal_len; ++i){
		float rmean = (in.r + pal[i].r)/2,
			dr = abs(in.r - pal[i].r),
			dg = abs(in.g - pal[i].g),
			db = abs(in.b - pal[i].b);

		float d = abs(in.r - pal[i].r);
			+ abs(in.g - pal[i].g);
			+ abs(in.b - pal[i].b);

		float c = sqrt((512+rmean/(float)UINT16_MAX)*dr*dr + 1024*dg*dg + (512+ (UINT16_MAX-1-rmean)/(float)UINT16_MAX)*db*db);
		if(c < bd){
			bd = c;
			bi = i;
		}
	}
	memcpy(out, &pal[bi], sizeof(*out));
}

int
main(int argc, char* argv[]){
	if(argc != 1){
		fputs("usage: jd <in.ff >out.ff\n", stderr);
		return 1;
	}
	// set to 2 for B/W
	pal_len = sizeof(pal)/sizeof(*pal);

	uint8_t header[8];
	fread(header, sizeof(*header), sizeof(header)/sizeof(*header), stdin);
	if(memcmp(magic, header, 8)){
		fputs("wrong header\n", stderr);
	}

	uint32_t in[2];
	fread(in, sizeof(*in), sizeof(in)/sizeof(*in), stdin);
	uint32_t w = ntohl(in[0])
		,h = ntohl(in[1]);

	uint16_t *m = malloc(sizeof(*m)*w*h*4);
	int32_t *o = malloc(sizeof(*o)*w*h*3);
	for(size_t y = 0; y < h; ++y){
		for(size_t x = 0; x < w; ++x){
			uint16_t v[4];
			fread(v, sizeof(*v), sizeof(v)/sizeof(*v), stdin);
			size_t i = y*w*4+x*4;

			v[0] = ntohs(v[0]);
			v[1] = ntohs(v[1]);
			v[2] = ntohs(v[2]);
			v[3] = ntohs(v[3]);
			m[i+3] = v[3];

			size_t i2 = y*w*3+x*3;

			o[i2] = v[0];
			o[i2+1] = v[1];
			o[i2+2] = v[2];
		}
	}


	for(size_t y = 0; y < h; ++y){
		for(size_t x = 0; x < w; ++x){
			size_t i = y*w*4+x*4;

			size_t i0 = y*w*3+x*3;
			size_t i1 = y*w*3+x*3+1;
			size_t i2 = y*w*3+x*3+2;

			struct pixel p_in = {o[i0], o[i1], o[i2]}, p_out;
			closest_color(p_in, &p_out);

			struct pixel d = {
				p_in.r - p_out.r,
				p_in.g - p_out.g,
				p_in.b - p_out.b
			};

			if(x < w-1){
				o[i0+3] += 7*d.r / 16;
				o[i1+3] += 7*d.g / 16;
				o[i2+3] += 7*d.b / 16;
			}
			if(y < h-1){
				if(x < w-1){
					o[i0+3*w+3] += d.r / 16;
					o[i1+3*w+3] += d.g / 16;
					o[i2+3*w+3] += d.b / 16;
				}
				if(x > 0){
					o[i0+3*w-3] += 3*d.r / 16;
					o[i1+3*w-3] += 3*d.g / 16;
					o[i2+3*w-3] += 3*d.b / 16;
				}
				o[i0+3*w] += 5*d.r / 16;
				o[i1+3*w] += 5*d.g / 16;
				o[i2+3*w] += 5*d.b / 16;
			}

			m[i] = p_out.r*1;
			m[i+1] = p_out.g*1;
			m[i+2] = p_out.b*1;
		}
	}


	for(size_t i = 0; i < h*w*4; ++i)
		m[i] = htons(m[i]);

	fwrite(header, sizeof(*header), sizeof(header)/sizeof(*header), stdout);
	fwrite(in, sizeof(*in), sizeof(in)/sizeof(*in), stdout);
	fwrite(m, sizeof(*m), sizeof(*m)*w*h*4, stdout);

	return 0;
}

