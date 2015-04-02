static void
rayadd(vec3_t pos, vec3_t r, block_t block, int before)
{
	int3_t p;
	p.x = (int)floorf(pos.x);
	p.y = (int)floorf(pos.y);
	p.z = (int)floorf(pos.z);

	vec3_t b;
	b.x = pos.x - floorf(pos.x);
	b.y = pos.y - floorf(pos.y);
	b.z = pos.z - floorf(pos.z);

	int posx = r.x > 0 ? 1 : 0;
	int posy = r.y > 0 ? 1 : 0;
	int posz = r.z > 0 ? 1 : 0;

	float dirx = posx ? 1 : -1;
	float diry = posy ? 1 : -1;
	float dirz = posz ? 1 : -1;

	vec3_t rt;
	rt.y = r.y / r.x;
	rt.z = r.z / r.x;
	float deltax = sqrtf(1 + rt.y*rt.y + rt.z*rt.z);
	rt.x = r.x / r.y;
	rt.z = r.z / r.y;
	float deltay = sqrtf(rt.x*rt.x + 1 + rt.z*rt.z);
	rt.x = r.x / r.z;
	rt.y = r.y / r.z;
	float deltaz = sqrtf(rt.x*rt.x + rt.y*rt.y + 1);

	float maxx = deltax * (posx ? (1 - b.x) : b.x);
	float maxy = deltay * (posy ? (1 - b.y) : b.y);
	float maxz = deltaz * (posz ? (1 - b.z) : b.z);

	int i;
	for(i=0; i<1000; i++)
	{
		if(maxx < maxy && maxx < maxz)
		{
			maxx += deltax;
			p.x += dirx;
			if(block_issolid(world_getblock(p.x,p.y,p.z,0)))
			{
				if(before)
					p.x -= dirx;
				break;
			}
		}
		else if(maxy < maxx && maxy < maxz)
		{
			maxy += deltay;
			p.y += diry;
			if(block_issolid(world_getblock(p.x,p.y,p.z,0)))
			{
				if(before)
					p.y -= diry;
				break;
			}
		}
		else if(maxz < maxx && maxz < maxy)
		{
			maxz += deltaz;
			p.z += dirz;
			if(block_issolid(world_getblock(p.x,p.y,p.z,0)))
			{
				if(before)
					p.z -= dirz;
				break;
			}
		}
	}
	world_setblock(p.x, p.y, p.z, block, 0);
}

inline static void
raydel(vec3_t pos, vec3_t ray)
{
	block_t b;
	b.id = 0;
	rayadd(pos, ray, b, 0);
}
