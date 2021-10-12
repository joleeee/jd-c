#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdlib.h>

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

int
main(int argc, char* argv[]){
	if(argc != 1){
		fputs("usage: jd <in.ff >out.ff\n", stderr);
		return 1;
	}

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
	int32_t *o = malloc(sizeof(*o)*w*h);
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

			int32_t avg = v[0]/10*3 + v[1]/10*6 + v[2]/10;
			o[y*w+x] = avg;
		}
	}


	for(size_t y = 0; y < h; ++y){
		for(size_t x = 0; x < w; ++x){
			size_t i = y*w*4+x*4;

			int32_t v = o[y*w+x];
			int32_t s = o[y*w+x] > UINT16_MAX/2 ? UINT16_MAX : 0;

			int32_t d = (int32_t)v-(int32_t)s;
			if(x < w-1)
				o[y*w+x+1] += 7*d / 16;
			if(y < h-1){
				if(x < w-1)
					o[(y+1)*w+x+1] += d / 16;
				if(x > 0)
					o[(y+1)*w+x-1] += 3*d / 16;
				o[(y+1)*w+x] += 5*d / 16;
			}

			m[i] = s;
			m[i+1] = s;
			m[i+2] = s;
		}
	}


	for(size_t i = 0; i < h*w*4; ++i)
		m[i] = htons(m[i]);

	fwrite(header, sizeof(*header), sizeof(header)/sizeof(*header), stdout);
	fwrite(in, sizeof(*in), sizeof(in)/sizeof(*in), stdout);
	fwrite(m, sizeof(*m), sizeof(*m)*w*h*4, stdout);

	return 0;
}

