#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265
#endif

#ifdef LPCOMPACT
typedef float lpfloat_t;
#endif

#ifndef LPCOMPACT
typedef double lpfloat_t;
#endif

typedef lpfloat_t* (*generator)(int);

typedef struct Pulsar {
    lpfloat_t **wts;  // Wavetable stack
    lpfloat_t **wins; // Window stack
    lpfloat_t *mod;   // Pulsewidth modulation table
    lpfloat_t *morph; // Morph table
    int* burst;    // Burst table
    int numwts;    // Number of wts in stack
    int numwins;   // Number of wins in stack
    int tablesize; // All tables should be this size
    lpfloat_t samplerate;
    int boundry;
    int morphboundry;
    int burstboundry;
    int burstphase;
    lpfloat_t phase;
    lpfloat_t modphase;
    lpfloat_t morphphase;
    lpfloat_t freq;
    lpfloat_t modfreq;
    lpfloat_t morphfreq;
    lpfloat_t inc;
} Pulsar;



/* Utilities
 *
 * Small collection of utility functions
 */
int imax(int a, int b) {
    if(a > b) {
        return a;
    } else {
        return b;
    }
}

lpfloat_t interpolate(lpfloat_t* wt, int boundry, lpfloat_t phase) {
    lpfloat_t frac = phase - (int)phase;
    int i = (int)phase;
    lpfloat_t a, b;

    if (i >= boundry) return 0;

    a = wt[i];
    b = wt[i+1];

    return (1.0 - frac) * a + (frac * b);
}

int paramcount(char* str) {
    int count = 1;
    int i = 0;

    while(str[i] != '\0') {
        char c = str[i];
        i += 1;
        if(c == ',') {
            count += 1;
        }
    }

    return count;
}


/* Wavetable generators
 * 
 * All these functions return a table of values 
 * of the given length with values between -1 and 1
 */
lpfloat_t* make_sine(int length) {
    lpfloat_t* out = (lpfloat_t*)malloc(sizeof(lpfloat_t) * length);
    for(int i=0; i < length; i++) {
        out[i] = sin((i/(lpfloat_t)length) * PI * 2.0);         
    }
    return out;
}

lpfloat_t* make_square(int length) {
    lpfloat_t* out = (lpfloat_t*)malloc(sizeof(lpfloat_t) * length);
    for(int i=0; i < length; i++) {
        if(i < (length/2.0)) {
            out[i] = 0.9999;
        } else {
            out[i] = -0.9999;
        }
    }
    return out;
}

lpfloat_t* make_tri(int length) {
    lpfloat_t* out = (lpfloat_t*)malloc(sizeof(lpfloat_t) * length);
    for(int i=0; i < length; i++) {
        out[i] = fabs((i/(lpfloat_t)length) * 2.0 - 1.0) * 2.0 - 1.0;      
    }
    return out;
}



/* Window generators
 *
 * All these functions return a table of values 
 * of the given length with values between 0 and 1
 */
lpfloat_t* make_phasor(int length) {
    lpfloat_t* out = (lpfloat_t*)malloc(sizeof(lpfloat_t) * length);
    for(int i=0; i < length; i++) {
        out[i] = i/(lpfloat_t)length;      
    }
    return out;
}

lpfloat_t* make_tri_win(int length) {
    lpfloat_t* out = (lpfloat_t*)malloc(sizeof(lpfloat_t) * length);
    for(int i=0; i < length; i++) {
        out[i] = fabs((i/(lpfloat_t)length) * 2.0 - 1.0);      
    }
    return out;
}

lpfloat_t* make_sine_win(int length) {
    lpfloat_t* out = (lpfloat_t*)malloc(sizeof(lpfloat_t) * length);
    for(int i=0; i < length; i++) {
        out[i] = sin((i/(lpfloat_t)length) * PI);         
    }
    return out;
}



/* Param parsers
 */
lpfloat_t** parsewts(char* str, int numwts, int tablesize) {
    char sep[] = ",";
    char* token = strtok(str, sep);
    lpfloat_t** wts = (lpfloat_t**)malloc(sizeof(lpfloat_t*) * numwts);
    int i = 0;
    while(token != NULL) {
        if (strcmp(token, "sine") == 0) {
            wts[i] = make_sine(tablesize);            
        } else if (strcmp(token, "tri") == 0) {
            wts[i] = make_tri(tablesize);            
        } else if (strcmp(token, "square") == 0) {
            wts[i] = make_square(tablesize);            
        } else {
            wts[i] = make_sine(tablesize);            
        }

        token = strtok(NULL, sep);
        i += 1;
    }
    return wts;
}

lpfloat_t** parsewins(char* str, int numwins, int tablesize) {
    char sep[] = ",";
    char* token = strtok(str, sep);
    lpfloat_t** wins = (lpfloat_t**)malloc(sizeof(lpfloat_t*) * numwins);
    int i = 0;
    while(token != NULL) {
        if (strcmp(token, "sine") == 0) {
            wins[i] = make_sine_win(tablesize);            
        } else if (strcmp(token, "tri") == 0) {
            wins[i] = make_tri_win(tablesize);            
        } else if (strcmp(token, "phasor") == 0) {
            wins[i] = make_phasor(tablesize);            
        } else {
            wins[i] = make_sine_win(tablesize);            
        }

        token = strtok(NULL, sep);
        i += 1;
    }
    return wins;
}

int* parseburst(char* str, int numbursts) {
    char sep[] = ",";
    char* token = strtok(str, sep);
    int* burst = (int*)malloc(sizeof(int) * numbursts);
    int i = 0;
    burst[i] = atoi(token);
    while(token != NULL) {
        token = strtok(NULL, sep);
        if(token != NULL) burst[i] = atoi(token);
        i += 1;
    }
    return burst;
}



/* Pulsar lifecycle functions
 *
 * init -> process -> cleanup
 */
Pulsar* lpinit(
    int tablesize, 
    lpfloat_t freq, 
    lpfloat_t modfreq, 
    lpfloat_t morphfreq, 
    char* wts, 
    char* wins, 
    char* burst,
    lpfloat_t samplerate
) {
    int numwts = paramcount(wts);
    int numwins = paramcount(wins);
    int numbursts = paramcount(burst);

    Pulsar* p = (Pulsar*)malloc(sizeof(Pulsar));

    p->wts = parsewts(wts, numwts, tablesize);
    p->wins = parsewins(wins, numwins, tablesize);
    p->burst = parseburst(burst, numbursts);

    p->numwts = numwts;
    p->numwins = numwins;
    p->samplerate = samplerate;

    p->boundry = tablesize - 1;
    p->morphboundry = numwts - 1;
    p->burstboundry = numbursts - 1;
    if(p->burstboundry <= 1) burst = NULL; // Disable burst for single value tables

    p->mod = make_sine_win(tablesize);
    p->morph = make_sine_win(tablesize);

    p->burstphase = 0;
    p->phase = 0;
    p->modphase = 0;

    p->freq = freq;
    p->modfreq = modfreq;
    p->morphfreq = morphfreq;

    p->inc = (1.0/samplerate) * p->boundry;

    return p;
}

lpfloat_t lpprocess(Pulsar* p) {
    // Get the pulsewidth and inverse pulsewidth if the pulsewidth 
    // is zero, skip everything except phase incrementing and return 
    // a zero down the line.
    lpfloat_t pw = interpolate(p->mod, p->boundry, p->modphase);
    lpfloat_t ipw = 0;
    if(pw > 0) ipw = 1.0/pw;

    lpfloat_t sample = 0;
    lpfloat_t mod = 0;
    lpfloat_t burst = 1;

    if(p->burst != NULL) {
        burst = p->burst[p->burstphase];
    }

    if(ipw > 0 && burst > 0) {
        lpfloat_t morphpos = interpolate(p->morph, p->boundry, p->morphphase);

        assert(p->numwts >= 1);
        if(p->numwts == 1) {
            // If there is just a single wavetable in the stack, get the current value
            sample = interpolate(p->wts[0], p->boundry, p->phase * ipw);
        } else {
            // If there are multiple wavetables in the stack, get their values 
            // and then interpolate the value at the morph position between them.
            lpfloat_t wtmorphpos = morphpos * imax(1, p->numwts-1);
            int wtmorphidx = (int)wtmorphpos;
            lpfloat_t wtmorphfrac = wtmorphpos - wtmorphidx;
            lpfloat_t a = interpolate(p->wts[wtmorphidx], p->boundry, p->phase * ipw);
            lpfloat_t b = interpolate(p->wts[wtmorphidx+1], p->boundry, p->phase * ipw);
            sample = (1.0 - wtmorphfrac) * a + (wtmorphfrac * b);
        }

        assert(p->numwins >= 1);
        if(p->numwins == 1) {
            // If there is just a single window in the stack, get the current value
            mod = interpolate(p->wins[0], p->boundry, p->phase * ipw);
        } else {
            // If there are multiple wavetables in the stack, get their values 
            // and then interpolate the value at the morph position between them.
            lpfloat_t winmorphpos = morphpos * imax(1, p->numwins-1);
            int winmorphidx = (int)winmorphpos;
            lpfloat_t winmorphfrac = winmorphpos - winmorphidx;
            lpfloat_t a = interpolate(p->wins[winmorphidx], p->boundry, p->phase * ipw);
            lpfloat_t b = interpolate(p->wins[winmorphidx+1], p->boundry, p->phase * ipw);
            mod = (1.0 - winmorphfrac) * a + (winmorphfrac * b);
        }
    }

    // Increment the wavetable/window phase, pulsewidth/mod phase & the morph phase
    p->phase += p->inc * p->freq;
    p->modphase += p->inc * p->modfreq;
    p->morphphase += p->inc * p->morphfreq;

    // Increment the burst phase on pulse boundries
    if(p->phase >= p->boundry) {
        p->burstphase += 1;
    }

    // Prevent phase overflow by subtracting the boundries if they have been passed
    if(p->phase >= p->boundry) p->phase -= p->boundry;
    if(p->modphase >= p->boundry) p->modphase -= p->boundry;
    if(p->morphphase >= p->boundry) p->morphphase -= p->boundry;
    if(p->burstphase >= p->burstboundry) p->burstphase -= p->burstboundry;

    // Multiply the wavetable value by the window value
    return sample * mod;
}

void cleanup(Pulsar* p) {
    for(int i=0; i < p->numwts; i++) {
        free(p->wts[i]);
    }

    for(int i=0; i < p->numwins; i++) {
        free(p->wins[i]);
    }

    free(p->wts);
    free(p->wins);
    free(p->mod);
    free(p->morph);
    free(p->burst);
    free(p);
}


