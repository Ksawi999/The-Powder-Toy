#include "simulation/ElementCommon.h"

int Element_FIRE_update(UPDATE_FUNC_ARGS);
static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);
static int colourToWavelength(int cr, int cg, int cb, int &life);

void Element::Element_PHOT()
{
	Identifier = "DEFAULT_PT_PHOT";
	Name = "PHOT";
	Colour = PIXPACK(0xFFFFFF);
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = -0.99f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = -1;

	DefaultProperties.temp = R_TEMP + 900.0f + 273.15f;
	HeatConduct = 251;
	Description = "Photons. Refracts through glass, scattered by quartz, and color-changed by different elements. Ignites flammable materials.";

	Properties = TYPE_ENERGY|PROP_LIFE_DEC|PROP_LIFE_KILL_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.life = 680;
	DefaultProperties.ctype = 0x3FFFFFFF;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static int update(UPDATE_FUNC_ARGS)
{
	// process dcolour into ctype
	if (parts[i].dcolour != parts[i].tmp)
	{
		int cr, cg, cb, x;
		for (x=0; x<12; x++) {
			cr = (parts[i].ctype >> (x+18)) & 1;
			cg = (parts[i].ctype >> (x+9))  & 1;
			cb = (parts[i].ctype >>  x)     & 1;
		}
		x = std::min(parts[i].life, 680) * 624/(cr+cg+cb+1) / 680;
		cr *= x;
		cg *= x;
		cb *= x;
		int da = (parts[i].dcolour>>24)&0xFF;
		int dr = (parts[i].dcolour>>16)&0xFF;
		int dg = (parts[i].dcolour>>8)&0xFF;
		int db = parts[i].dcolour&0xFF;
		cr = (da*dr + (256-da)*cr) >> 8;
		cg = (da*dg + (256-da)*cg) >> 8;
		cb = (da*db + (256-da)*cb) >> 8;
		parts[i].ctype = colourToWavelength(cr, cg, cb, parts[i].life);
		for (x=0; x<12; x++) {
			cr = (parts[i].ctype >> (x+18)) & 1;
			cg = (parts[i].ctype >> (x+9))  & 1;
			cb = (parts[i].ctype >>  x)     & 1;
		}
		x = std::min(parts[i].life, 680) * 624/(cr+cg+cb+1) / 680;
		cr *= x;
		cg *= x;
		cb *= x;
		parts[i].tmp = parts[i].dcolour = 0xFF000000|(cr << 16)|(cg << 8)|cb;
	}

	int r, rx, ry;
	float rr, rrr;
	if (!(parts[i].ctype&0x3FFFFFFF)) {
		sim->kill_part(i);
		return 1;
	}
	if (parts[i].temp > 506)
		if (RNG::Ref().chance(1, 10))
			Element_FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK) {
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_ISOZ || TYP(r)==PT_ISZS)
				{
					if (RNG::Ref().chance(1, 400))
					{
						parts[i].vx *= 0.90f;
						parts[i].vy *= 0.90f;
						sim->create_part(ID(r), x+rx, y+ry, PT_PHOT);
						rrr = RNG::Ref().between(0, 359) * 3.14159f / 180.0f;
						if (TYP(r) == PT_ISOZ)
							rr = RNG::Ref().between(128, 255) / 127.0f;
						else
							rr = RNG::Ref().between(128, 355) / 127.0f;
						parts[ID(r)].vx = rr*cosf(rrr);
						parts[ID(r)].vy = rr*sinf(rrr);
						sim->pv[y/CELL][x/CELL] -= 15.0f * CFDS;
					}
				}
				else if((TYP(r) == PT_QRTZ || TYP(r) == PT_PQRT) && !ry && !rx)//if on QRTZ
				{
					float a = RNG::Ref().between(0, 359) * 3.14159f / 180.0f;
					parts[i].vx = 3.0f*cosf(a);
					parts[i].vy = 3.0f*sinf(a);
					if(parts[i].ctype == 0x3FFFFFFF)
						parts[i].ctype = 0x1F << RNG::Ref().between(0, 25);
					if (parts[i].life)
						parts[i].life++; //Delay death
				}
				else if(TYP(r) == PT_BGLA && !ry && !rx)//if on BGLA
				{
					float a = RNG::Ref().between(-50, 50) * 0.001f;
					float rx = cosf(a), ry = sinf(a), vx, vy;
					vx = rx * parts[i].vx + ry * parts[i].vy;
					vy = rx * parts[i].vy - ry * parts[i].vx;
					parts[i].vx = vx;
					parts[i].vy = vy;
				}
				else if (TYP(r) == PT_FILT && parts[ID(r)].tmp==9)
				{
					parts[i].vx += ((float)RNG::Ref().between(-500, 500))/1000.0f;
					parts[i].vy += ((float)RNG::Ref().between(-500, 500))/1000.0f;
				}
			}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = (cpart->dcolour>>16)&0xFF;
	*colg = (cpart->dcolour>>8)&0xFF;
	*colb = cpart->dcolour&0xFF;

	bool tozero = false;
	if (cpart->life <= 0)
	{
		tozero = true;
		cpart->life = 680;
	}

	*firea = 100 * std::min(cpart->life, 680) / 680;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	if (tozero)
		cpart->life = 0;

	*pixel_mode &= ~PMODE_FLAT;
	*pixel_mode |= FIRE_ADD | PMODE_ADD | NO_DECO;
	if (cpart->flags & FLAG_PHOTDECO)
	{
		*pixel_mode &= ~NO_DECO;
	}
	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	float a = RNG::Ref().between(0, 7) * 0.78540f;
	sim->parts[i].vx = 3.0f * cosf(a);
	sim->parts[i].vy = 3.0f * sinf(a);
	int Element_FILT_interactWavelengths(Particle* cpart, int origWl);
	if (TYP(sim->pmap[y][x]) == PT_FILT)
		sim->parts[i].ctype = Element_FILT_interactWavelengths(&sim->parts[ID(sim->pmap[y][x])], sim->parts[i].ctype);
}

static int colourToWavelength(int cr, int cg, int cb, int &life)
{
	float vl = std::max(std::max(cr, cg), cb);
	if (vl == 0.0f)
		vl = 1.0f;
	int mt = 5;
	int best = 1000;
	int bestmt = mt;
	int vr, vg, vb;
	for (; mt < 13; mt++)
	{
		vr = (int)(cr / vl * mt + 0.5f);
		vg = (int)(cg / vl * mt + 0.5f);
		vb = (int)(cb / vl * mt + 0.5f);
		if ((mt < 7 || vr + vb >= mt - 6) && (mt < 10 || vg >= std::max(vr - 9, 0) + std::max(vb - 9, 0)))
		{
			int diff = std::abs(cr - vr * vl / mt) + std::abs(cg - vg * vl / mt) + std::abs(cb - vb * vl / mt);
			if (diff <= best)
			{
				best = diff;
				bestmt = mt;
			}
		}
	}
	mt = bestmt;
	vr = (int)(cr / vl * mt + 0.5f);
	vg = (int)(cg / vl * mt + 0.5f);
	vb = (int)(cb / vl * mt + 0.5f);
	int shg = 0;
	if (vg > 6)
	{
		shg = std::min(std::max(std::max(std::min(vr - vb, vg - 6), 6 - vg), -3), 3);
		vr -= std::max(shg, 0);
		vb += std::min(shg, 0);
	}
	else
	{
		if (vb > 9)
			vg -= vb - 9;
		if (vr > 9)
			vg -= vr - 9;
	}
	unsigned int mask = ((1 << vr) - 1) << (30 - vr);
	mask |= ((1 << vg) - 1) << (12 + shg);
	mask |= ((1 << vb) - 1);
	mask &= 0x3FFFFFFF;
	life = vl * 680 / 255;
	if (life < 2)
		life = 2;
	return mask;
}
