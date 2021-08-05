/*
 * XBR filter extracted from FFmpeg into a separate library.
 *
 *
 * Copyright (c) 2011, 2012 Hyllian/Jararaca <sergiogdb@gmail.com>
 * Copyright (c) 2014 Arwa Arif <arwaarif1994@gmail.com>
 * Copyright (c) 2015 Treeki <treeki@gmail.com>
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * XBR Filter is used for depixelization of image.
 * This is based on Hyllian's xBR shader.
 *
 * @see http://www.libretro.com/forums/viewtopic.php?f=6&t=134
 * @see https://github.com/yoyofr/iFBA/blob/master/fba_src/src/intf/video/scalers/xbr.cpp
 */

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint32_t rgbtoyuv[1<<24];
} xbr_data;

typedef struct {
    const uint8_t *input;
    uint8_t *output;
    int inWidth, inHeight;
    int inPitch, outPitch;
    const xbr_data *data;
} xbr_params;

#ifdef _MSC_VER
	#define XBR_INLINE __inline
#else
	#define XBR_INLINE inline
#endif

#define LB_MASK       0x00FEFEFE
#define RED_BLUE_MASK 0x00FF00FF
#define GREEN_MASK    0x0000FF00
#define PART_MASK     0x00FF00FF

static uint32_t pixel_diff(uint32_t x, uint32_t y, const uint32_t *r2y)
{
#define YMASK 0xff0000
#define UMASK 0x00ff00
#define VMASK 0x0000ff

    uint32_t yuv1 = r2y[x & 0xffffff];
    uint32_t yuv2 = r2y[y & 0xffffff];

    return (abs((x >> 24) - (y >> 24))) +
           (abs((yuv1 & YMASK) - (yuv2 & YMASK)) >> 16) +
           (abs((yuv1 & UMASK) - (yuv2 & UMASK)) >>  8) +
           abs((yuv1 & VMASK) - (yuv2 & VMASK));
}

#define ALPHA_BLEND_BASE(a, b, m, s) (  (PART_MASK & (((a) & PART_MASK) + (((((b) & PART_MASK) - ((a) & PART_MASK)) * (m)) >> (s)))) \
                                      | ((PART_MASK & ((((a) >> 8) & PART_MASK) + ((((((b) >> 8) & PART_MASK) - (((a) >> 8) & PART_MASK)) * (m)) >> (s)))) << 8))

#define ALPHA_BLEND_32_W(a, b)  ALPHA_BLEND_BASE(a, b, 1, 3)
#define ALPHA_BLEND_64_W(a, b)  ALPHA_BLEND_BASE(a, b, 1, 2)
#define ALPHA_BLEND_128_W(a, b) ALPHA_BLEND_BASE(a, b, 1, 1)
#define ALPHA_BLEND_192_W(a, b) ALPHA_BLEND_BASE(a, b, 3, 2)
#define ALPHA_BLEND_224_W(a, b) ALPHA_BLEND_BASE(a, b, 7, 3)



#define df(A, B) pixel_diff(A, B, r2y)
#define eq(A, B) (df(A, B) < 155)


#define FILT3(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1,   \
              N0, N1, N2, N3, N4, N5, N6, N7, N8) do {                                              \
    if (PE != PH && PE != PF) {                                                                     \
        const unsigned e = df(PE,PC) + df(PE,PG) + df(PI,H5) + df(PI,F4) + (df(PH,PF)<<2);          \
        const unsigned i = df(PH,PD) + df(PH,I5) + df(PF,I4) + df(PF,PB) + (df(PE,PI)<<2);          \
        if (e <= i) {                                                                               \
            const unsigned px = df(PE,PF) <= df(PE,PH) ? PF : PH;                                   \
            if (e < i && (!eq(PF,PB) && !eq(PF,PC) || !eq(PH,PD) && !eq(PH,PG) || eq(PE,PI)         \
                          && (!eq(PF,F4) && !eq(PF,I4) || !eq(PH,H5) && !eq(PH,I5))                 \
                          || eq(PE,PG) || eq(PE,PC))) {                                             \
                const unsigned ke = df(PF,PG);                                                      \
                const unsigned ki = df(PH,PC);                                                      \
                const int left    = ke<<1 <= ki && PE != PG && PD != PG;                            \
                const int up      = ke >= ki<<1 && PE != PC && PB != PC;                            \
                if (left && up) {                                                                   \
                    E[N7] = ALPHA_BLEND_192_W(E[N7], px);                                           \
                    E[N6] = ALPHA_BLEND_64_W( E[N6], px);                                           \
                    E[N5] = E[N7];                                                                  \
                    E[N2] = E[N6];                                                                  \
                    E[N8] = px;                                                                     \
                } else if (left) {                                                                  \
                    E[N7] = ALPHA_BLEND_192_W(E[N7], px);                                           \
                    E[N5] = ALPHA_BLEND_64_W( E[N5], px);                                           \
                    E[N6] = ALPHA_BLEND_64_W( E[N6], px);                                           \
                    E[N8] = px;                                                                     \
                } else if (up) {                                                                    \
                    E[N5] = ALPHA_BLEND_192_W(E[N5], px);                                           \
                    E[N7] = ALPHA_BLEND_64_W( E[N7], px);                                           \
                    E[N2] = ALPHA_BLEND_64_W( E[N2], px);                                           \
                    E[N8] = px;                                                                     \
                } else { /* diagonal */                                                             \
                    E[N8] = ALPHA_BLEND_224_W(E[N8], px);                                           \
                    E[N5] = ALPHA_BLEND_32_W( E[N5], px);                                           \
                    E[N7] = ALPHA_BLEND_32_W( E[N7], px);                                           \
                }                                                                                   \
            } else {                                                                                \
                E[N8] = ALPHA_BLEND_128_W(E[N8], px);                                               \
            }                                                                                       \
        }                                                                                           \
    }                                                                                               \
} while (0)

static XBR_INLINE void xbr_filter(const xbr_params *params)
{
    int x, y;
    const uint32_t *r2y = params->data->rgbtoyuv;
    const int nl = params->outPitch >> 2;
    const int nl1 = nl + nl;
    const int nl2 = nl1 + nl;

    for (y = 0; y < params->inHeight; y++) {

        uint32_t *E = (uint32_t *)(params->output + y * params->outPitch * 3);
        const uint32_t *sa2 = (uint32_t *)(params->input + y * params->inPitch - 8); /* center */
        const uint32_t *sa1 = sa2 - (params->inPitch>>2); /* up x1 */
        const uint32_t *sa0 = sa1 - (params->inPitch>>2); /* up x2 */
        const uint32_t *sa3 = sa2 + (params->inPitch>>2); /* down x1 */
        const uint32_t *sa4 = sa3 + (params->inPitch>>2); /* down x2 */

        if (y <= 1) {
            sa0 = sa1;
            if (y == 0) {
                sa0 = sa1 = sa2;
            }
        }

        if (y >= params->inHeight - 2) {
            sa4 = sa3;
            if (y == params->inHeight - 1) {
                sa4 = sa3 = sa2;
            }
        }

        for (x = 0; x < params->inWidth; x++) {
            const uint32_t B1 = sa0[2];
            const uint32_t PB = sa1[2];
            const uint32_t PE = sa2[2];
            const uint32_t PH = sa3[2];
            const uint32_t H5 = sa4[2];

            const int pprev = 2 - (x > 0);
            const uint32_t A1 = sa0[pprev];
            const uint32_t PA = sa1[pprev];
            const uint32_t PD = sa2[pprev];
            const uint32_t PG = sa3[pprev];
            const uint32_t G5 = sa4[pprev];

            const int pprev2 = pprev - (x > 1);
            const uint32_t A0 = sa1[pprev2];
            const uint32_t D0 = sa2[pprev2];
            const uint32_t G0 = sa3[pprev2];

            const int pnext = 3 - (x == params->inWidth - 1);
            const uint32_t C1 = sa0[pnext];
            const uint32_t PC = sa1[pnext];
            const uint32_t PF = sa2[pnext];
            const uint32_t PI = sa3[pnext];
            const uint32_t I5 = sa4[pnext];

            const int pnext2 = pnext + 1 - (x >= params->inWidth - 2);
            const uint32_t C4 = sa1[pnext2];
            const uint32_t F4 = sa2[pnext2];
            const uint32_t I4 = sa3[pnext2];

            E[0]   = E[1]     = E[2]     =     // 0, 1, 2
            E[nl]  = E[nl+1]  = E[nl+2]  =     // 3, 4, 5
            E[nl1] = E[nl1+1] = E[nl1+2] = PE; // 6, 7, 8

            FILT3(PE, PI, PH, PF, PG, PC, PD, PB, PA, G5, C4, G0, D0, C1, B1, F4, I4, H5, I5, A0, A1, 0, 1, 2, nl, nl+1, nl+2, nl1, nl1+1, nl1+2);
            FILT3(PE, PC, PF, PB, PI, PA, PH, PD, PG, I4, A1, I5, H5, A0, D0, B1, C1, F4, C4, G5, G0, nl1, nl, 0, nl1+1, nl+1, 1, nl1+2, nl+2, 2);
            FILT3(PE, PA, PB, PD, PC, PG, PF, PH, PI, C1, G0, C4, F4, G5, H5, D0, A0, B1, A1, I4, I5, nl1+2, nl1+1, nl1, nl+2, nl+1, nl, 2, 1, 0);
            FILT3(PE, PG, PD, PH, PA, PI, PB, PF, PC, A0, I5, A1, B1, I4, F4, H5, G5, D0, G0, C1, C4, 2, nl+2, nl1+2, 1, nl+1, nl1+1, 0, nl, nl1);

            sa0 += 1;
            sa1 += 1;
            sa2 += 1;
            sa3 += 1;
            sa4 += 1;

            E += 3;
        }
    }
}


static XBR_INLINE int _max(int a, int b)
{
	return (a > b) ? a : b;
}

static XBR_INLINE int _min(int a, int b)
{
	return (a < b) ? a : b;
}


void xbr_init_data(xbr_data *data)
{
    uint32_t c;
    int bg, rg, g;

    for (bg = -255; bg < 256; bg++) {
        for (rg = -255; rg < 256; rg++) {
            const uint32_t u = (uint32_t)((-169*rg + 500*bg)/1000) + 128;
            const uint32_t v = (uint32_t)(( 500*rg -  81*bg)/1000) + 128;
            int startg = _max(-bg, _max(-rg, 0));
            int endg = _min(255-bg, _min(255-rg, 255));
            uint32_t y = (uint32_t)(( 299*rg + 1000*startg + 114*bg)/1000);
            c = bg + (rg<<16) + 0x010101 * startg;
            for (g = startg; g <= endg; g++) {
                data->rgbtoyuv[c] = ((y++) << 16) + (u << 8) + v;
                c+= 0x010101;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////
//   START OF BAD ASS HACKERS
////////////////////////////////////////////////////////////////////////////

xbr_data* xbrdata = 0;
void Init()
{
	if(xbrdata != 0)
		return;

	xbrdata = (xbr_data*)malloc(sizeof(xbr_data));
	xbr_init_data(xbrdata);
}

void Shutdown()
{
	if(xbrdata == 0)
		return;

	free(xbrdata);
	xbrdata = 0;
}

void Process(void* input, int inWidth, int inHeight, int inStride, void* output, int outStride)
{
	xbr_params par;
	par.data = xbrdata;
	par.input = (uint8_t*)input;
	par.inWidth = inWidth;
	par.inHeight = inHeight;
	par.inPitch = inStride;
	par.output = (uint8_t*)output;
	par.outPitch = outStride;
	xbr_filter(&par);
}