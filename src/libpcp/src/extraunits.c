/*
 * Data-driven implementation of "extra" units for metadata.
 *
 * Copyright (c) 2026 Ken McDonell.  All Rights Reserved.
 * 
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 */

#include "pmapi.h"
#include "libpcp.h"
#include "internal.h"

typedef struct {
    int		ident;		/* one of the PM_<unit>_* values */
    char	*text;		/* per-scale text to be inserted in unit description */
    int		TODO;
} scale_map_t;

typedef struct {
    int			type;		/* one of the PM_UNIT_* values */
    char		*pre;		/* string up to scale text for unit description */
    char		*post;		/* string after scale text for unit description */
    int			nscale;		/* number of scales */
    const scale_map_t	*scale;		/* scales map */
} unit_t;

const scale_map_t	temperature_scale[] = {
    { PM_TEMPERATURE_C,	"C" },
    { PM_TEMPERATURE_F,	"F" },
    { PM_TEMPERATURE_K,	"K" },
};

const scale_map_t	voltage_scale[] = {
    { PM_VOLTAGE_V,	"V" },
    { PM_VOLTAGE_mV,	"mV" },
    { PM_VOLTAGE_uV,	"uV" },
};

const scale_map_t	current_scale[] = {
    { PM_CURRENT_A,	"A" },
    { PM_CURRENT_mA,	"mA" },
    { PM_CURRENT_uA,	"uA" },
};

const scale_map_t	power_scale[] = {
    { PM_POWER_kW,	"kW" },
    { PM_POWER_W,	"W" },
    { PM_POWER_mW,	"mW" },
    { PM_POWER_uW,	"uW" },
};

static const unit_t extra[] = {
    { PM_UNITS_TEMPERATURE, "temperature (", ")", sizeof(temperature_scale)/sizeof(temperature_scale[0]), temperature_scale },
    { PM_UNITS_VOLTAGE, "voltage (", ")", sizeof(voltage_scale)/sizeof(voltage_scale[0]), voltage_scale },
    { PM_UNITS_CURRENT, "current (", ")", sizeof(current_scale)/sizeof(current_scale[0]), current_scale },
    { PM_UNITS_POWER, "power (", ")", sizeof(power_scale)/sizeof(power_scale[0]), power_scale },
};

static const int nextra = sizeof(extra)/sizeof(extra[0]);

/*
 * extraUnit + extraScale => string for printing
 *
 * On entry, pu->extraUnits != 0
 */
void
__pmExtraUnitsStr(const pmUnits *pu, char *buf, size_t buflen)
{
    int		u;	/* index into extra[] */
    int		s;	/* scale selector */

    for (u = 0; u < nextra; u++) {
	if (pu->extraUnit == extra[u].type)
	    break;
    }
    if (u == nextra) {
	/* unknown extraUnit */
	pmsprintf(buf, buflen, "extra-%d", pu->extraUnit);
	return;
    }
    for (s = 0; s < extra[u].nscale; s++) {
	if (pu->extraScale == extra[u].scale[s].ident) {
	    pmsprintf(buf, buflen, "%s%s%s", extra[u].pre, extra[u].scale[s].text, extra[u].post);
	    return;
	}
    }
    /* unknown extraScale */
    pmsprintf(buf, buflen, "%sscale-%d%s", extra[u].pre, pu->extraScale, extra[u].post);
    return;
}

/*
 * parse units string to match extraUnit + extrScale encoding from
 * __pmExtraUnitsStr()
 *
 * return pointer updated to past end of parse in case of a match
 */
const char *
__pmParseExtraUnits(const char *buf, __pmUnits *pu)
{
    const char	*ptr = buf;
    int		u;	/* index into extra[] */
    int		s;	/* scale selector */

    for (u = 0; u < nextra; u++) {
	if (extra[u].pre == NULL || strncasecmp(ptr, extra[u].pre, strlen(extra[u].pre)) == 0) {
	    /* matched prefix */
	    if (ptr[strlen(extra[u].pre)] == '\0')
		continue;
	    ptr += strlen(extra[u].pre);
	    for (s = 0; s < extra[u].nscale; s++) {
fprintf(stderr, "%s %s %d\n", extra[u].scale[s].text, ptr, (int)strncasecmp(ptr, extra[u].scale[s].text, strlen(extra[u].scale[s].text)));
		if (strncasecmp(ptr, extra[u].scale[s].text, strlen(extra[u].scale[s].text)) == 0) {
		    /* matched scale */
		    if (ptr[strlen(extra[u].scale[s].text)] == '\0')
			continue;
		    ptr += strlen(extra[u].scale[s].text);
fprintf(stderr, "%s %s %d\n", extra[u].post, ptr, (int)strncasecmp(ptr, extra[u].post, strlen(extra[u].post)));
		    if (extra[u].post == NULL || strncasecmp(ptr, extra[u].post, strlen(extra[u].post)) == 0) {
			/* matched postfix */
			ptr += strlen(extra[u].post);
			pu->extraUnit = extra[u].type;
			pu->extraScale = extra[u].scale[s].ident;
fprintf(stderr, "%s\n", ptr);
			return ptr;
		    }
		    else {
			ptr -= strlen(extra[u].scale[s].text);
		    }
		}
	    }
	}
    }

    return buf;
}
