/***************************************************************************
*   Copyright (C) 2016 by iCatButler                                      *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
***************************************************************************/

/**************************************************************************
*	pgxp_gpu.c
*	PGXP - Parallel/Precision Geometry Xform Pipeline
*
*	Created on: 25 Mar 2016
*      Author: iCatButler
***************************************************************************/

#include "pgxp_gpu.h"
#include "stdafx.h"
#include "externals.h"

#include <math.h>

typedef struct
{
	float	x;
	float	y;
	float	z;
	union
	{
		unsigned int	flags;
		unsigned char	compFlags[4];
	};
	unsigned int	count;
	unsigned int	value;
	unsigned int	mFlags;
} PGXP_vertex;

#define NONE	 0
#define ALL		 0xFFFFFFFF
#define VALID	 1
#define VALID_0  (VALID << 0)
#define VALID_1  (VALID << 8)
#define VALID_2  (VALID << 16)
#define VALID_3  (VALID << 24)
#define VALID_01  (VALID_0 | VALID_1)
#define VALID_012  (VALID_0 | VALID_1 | VALID_2)
#define VALID_ALL  (VALID_0 | VALID_1 | VALID_2 | VALID_3)
#define INV_VALID_ALL  (ALL ^ VALID_ALL)

enum PGXP_source
{
	SRC_2D = 0,
	SRC_PGXP,
	SRC_PGXP_NO_W,
	SRC_NATIVE,
	SRC_CACHE,
	SRC_CACHE_AMBIGUOUS,
	SRC_UNKNOWN
};


unsigned int PGXP_tDebug = 0;
/////////////////////////////////
//// Blade_Arma's Vertex Cache (CatBlade?)
/////////////////////////////////
const unsigned int mode_init = 0;
const unsigned int mode_write = 1;
const unsigned int mode_read = 2;
const unsigned int mode_fail = 3;

PGXP_vertex vertexCache[0x800 * 2][0x800 * 2];

unsigned int baseID = 0;
unsigned int lastID = 0;
unsigned int cacheMode = 0;

unsigned int IsSessionID(unsigned int vertID)
{
	// No wrapping
	if (lastID >= baseID)
		return (vertID >= baseID);

	// If vertID is >= baseID it is pre-wrap and in session
	if (vertID >= baseID)
		return 1;

	// vertID is < baseID, If it is <= lastID it is post-wrap and in session
	if (vertID <= lastID)
		return 1;

	return 0;
}

unsigned int GetSessionIndex(unsigned int vertID)
{
	if (!IsSessionID(vertID))
		return 0;

	// No wrapping
	if (lastID >= baseID)
		return (vertID - baseID);

	// If vertID is >= baseID it is pre-wrap and in session
	if (vertID >= baseID)
		return (vertID - baseID);

	// vertID is < baseID, If it is <= lastID it is post-wrap and in session
	if (vertID <= lastID)
		return vertID + (0xFFFFFFFF - baseID);

	return 0;
}

void CALLBACK GPUpgxpCacheVertex(short sx, short sy, const unsigned char* _pVertex)
{
	const PGXP_vertex*	pNewVertex = (const PGXP_vertex*)_pVertex;
	PGXP_vertex*		pOldVertex = NULL;

	if (!pNewVertex)
	{
		cacheMode = mode_fail;
		return;
	}

	//if (bGteAccuracy)
	{
		if (cacheMode != mode_write)
		{
			// Initialise cache on first use
			if (cacheMode == mode_init)
				memset(vertexCache, 0x00, sizeof(vertexCache));

			// First vertex of write session (frame?)
			cacheMode = mode_write;
			baseID = pNewVertex->count;
		}

		lastID = pNewVertex->count;

		if (sx >= -0x800 && sx <= 0x7ff &&
			sy >= -0x800 && sy <= 0x7ff)
		{
			pOldVertex = &vertexCache[sy + 0x800][sx + 0x800];

			// To avoid ambiguity there can only be one valid entry per-session
			if (IsSessionID(pOldVertex->count) && (pOldVertex->value == pNewVertex->value))
			{
				// check to ensure this isn't identical
				if ((fabsf(pOldVertex->x - pNewVertex->x) > 0.1f) ||
					(fabsf(pOldVertex->y - pNewVertex->y) > 0.1f) ||
					(fabsf(pOldVertex->z - pNewVertex->z) > 0.1f))
				{
					pOldVertex->mFlags = 5;
					return;
				}
			}

			// Write vertex into cache
			*pOldVertex = *pNewVertex;
			pOldVertex->mFlags = 1;
		}
	}
}

PGXP_vertex* PGXP_GetCachedVertex(short sx, short sy)
{
	//if (bGteAccuracy)
	{
		if (cacheMode != mode_read)
		{
			if (cacheMode == mode_fail)
				return NULL;

			// Initialise cache on first use
			if (cacheMode == mode_init)
				memset(vertexCache, 0x00, sizeof(vertexCache));

			// First vertex of read session (frame?)
			cacheMode = mode_read;
		}

		if (sx >= -0x800 && sx <= 0x7ff &&
			sy >= -0x800 && sy <= 0x7ff)
		{
			// Return pointer to cache entry
			return &vertexCache[sy + 0x800][sx + 0x800];
		}
	}

	return NULL;
}


/////////////////////////////////
//// PGXP Implementation
/////////////////////////////////

const unsigned char primSizeTable[256] =
{
    // 00
    0,0,3,0,0,0,0,0,
    // 08
    0,0,0,0,0,0,0,0,
    // 10
    0,0,0,0,0,0,0,0,
    // 18
    0,0,0,0,0,0,0,0,
    // 20
    4,4,4,4,7,7,7,7,
    // 28
    5,5,5,5,9,9,9,9,
    // 30
    6,6,6,6,9,9,9,9,
    // 38
    8,8,8,8,12,12,12,12,
    // 40
    3,3,3,3,0,0,0,0,
    // 48
//    5,5,5,5,6,6,6,6,      //FLINE
    254,254,254,254,254,254,254,254,
    // 50
    4,4,4,4,0,0,0,0,
    // 58
//    7,7,7,7,9,9,9,9,    //    LINEG3    LINEG4
    255,255,255,255,255,255,255,255,
    // 60
    3,3,3,3,4,4,4,4,    //    TILE    SPRT
    // 68
    2,2,2,2,3,3,3,3,    //    TILE1
    // 70
    2,2,2,2,3,3,3,3,
    // 78
    2,2,2,2,3,3,3,3,
    // 80
    4,0,0,0,0,0,0,0,
    // 88
    0,0,0,0,0,0,0,0,
    // 90
    0,0,0,0,0,0,0,0,
    // 98
    0,0,0,0,0,0,0,0,
    // a0
    3,0,0,0,0,0,0,0,
    // a8
    0,0,0,0,0,0,0,0,
    // b0
    0,0,0,0,0,0,0,0,
    // b8
    0,0,0,0,0,0,0,0,
    // c0
    3,0,0,0,0,0,0,0,
    // c8
    0,0,0,0,0,0,0,0,
    // d0
    0,0,0,0,0,0,0,0,
    // d8
    0,0,0,0,0,0,0,0,
    // e0
    0,1,1,1,1,1,1,0,
    // e8
    0,0,0,0,0,0,0,0,
    // f0
    0,0,0,0,0,0,0,0,
    // f8
    0,0,0,0,0,0,0,0
};

const unsigned int primStrideTable[] = { 1, 2, 1, 2, 2, 3, 2, 3, 0 };
const unsigned int primCountTable[] = { 3, 3, 4, 4, 3, 3, 4, 4, 0 };

PGXP_vertex*	PGXP_Mem = NULL;	// pointer to parallel memory
unsigned int	currentAddr = 0;	// address of current DMA
uint32_t*		pDMABlock = NULL;
int				blockSize = 0;

unsigned int	currentDepth = 0;
static float minZ = 0xffffffff, maxZ = 0.f;

unsigned int	numVertices = 0;	// iCB: Used for glVertex3fv fix
unsigned int	vertexIdx = 0;

// Set current DMA address and pointer to parallel memory
void CALLBACK GPUpgxpMemory(unsigned int addr, unsigned char* pVRAM)
{
	PGXP_Mem = (PGXP_vertex*)(pVRAM);
	currentAddr = addr;
}

// Set current DMA address
void PGXP_SetAddress(unsigned int addr, uint32_t *baseAddrL, int size)
{
	currentAddr = addr;
	pDMABlock = baseAddrL;
	blockSize = size;
}

void PGXP_SetDepth(unsigned int addr)
{
	currentDepth = addr/* & 0xFFFF*/;
	maxZ = (currentDepth > maxZ) ? currentDepth : maxZ;
}

void PGXP_SetMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
{
	GLfloat m[16];
	for (unsigned int i = 0; i < 16; ++i)
		m[i] = 0.f;

	//if ((right-left) != 0)
	//{
	//	m[0] = 2 / (right - left);
	//	m[12] = -((right + left) / (right - left));
	//}
	//if ((top-bottom) != 0)
	//{
	//	m[5] = 2 / (top - bottom);
	//	m[13] = -((top + bottom) / (top - bottom));
	//}
	//m[10] = -2 / (zFar - zNear);
	//m[14] = -((zFar + zNear) / (zFar - zNear));
	//m[15] = 1;

	if ((right-left) != 0)
	{
		m[0] = 2 / (right - left);
		m[8] = -((right + left) / (right - left));
	}
	if ((top-bottom) != 0)
	{
		m[5] = 2 / (top - bottom);
		m[9] = -((top + bottom) / (top - bottom));
	}
	m[10] = -2 / (zFar - zNear);
	m[14] = -((zFar + zNear) / (zFar - zNear));
	m[11] = 1;

	glLoadMatrixf(m);
	//glOrtho(left, right, bottom, top, zNear, zFar);
}

// Wrap glVertex3fv/glVertex4fv
void PGXP_glVertexfv(GLfloat* pV)
{
	// If there are PGXP vertices expected
	if (1)//(vertexIdx < numVertices)
	{
	float temp[4];
	memcpy(temp, pV, sizeof(float) * 4);

		//pre-multiply each element by w (to negate perspective divide)
		for (unsigned int i = 0; i < 3; i++)
			temp[i] *= temp[3];

		//pass complete vertex to OpenGL
		glVertex4fv(temp);
		vertexIdx++;

		//pV[3] = 1.f;
	}
	else
	{
		glVertex3fv(pV);
	}
}

// Get parallel vertex values
int PGXP_GetVertices(unsigned int* addr, void* pOutput, int xOffs, int yOffs)
{
	unsigned int	primCmd		= ((*addr >> 24) & 0xff);			// primitive command
	unsigned int	primIdx		= min((primCmd - 0x20) >> 2, 8);	// index to primitive lookup
	OGLVertex*		pVertex		= (OGLVertex*)pOutput;				// pointer to output vertices
	unsigned int	stride		= primStrideTable[primIdx];			// stride between vertices
	unsigned int	count		= primCountTable[primIdx];			// number of vertices
	PGXP_vertex*	primStart	= NULL;								// pointer to first vertex
	char			invalidVert	= 0;								// Number of vertices without valid PGXP values

	short*			pPrimData	= ((short*)addr) + 2;				// primitive data for cache lookups
	PGXP_vertex*	pCacheVert	= NULL;

	// calculate offset to actual data
	int offset = 0;

	/* Dirty hack */
	if (!pDMABlock)
		return 0;

	while ((pDMABlock[offset] != *addr) && (offset < blockSize))
	{
		unsigned char command = (unsigned char)((pDMABlock[offset] >> 24) & 0xff);
		unsigned int primSize = primSizeTable[command];

		if (primSize == 0)
		{
			offset++;
			continue;
		}
		else if (primSize > 128)
		{
			while (((pDMABlock[offset] & 0xF000F000) != 0x50005000) && (offset < blockSize))
				++offset;
		}
		else
			offset += primSize;
	}

	// Reset vertex count
	numVertices = count;
	vertexIdx = 0;

	// if PGXP is enabled
	if (PGXP_Mem != NULL)
	{
		// Offset to start of primitive
		primStart = &PGXP_Mem[currentAddr + offset + 1];

		// Find any invalid vertices
		for (unsigned i = 0; i < count; ++i)
		{
			if (!((primStart[stride * i].flags & VALID_012) == VALID_012))
				invalidVert++;
		}
	}
	else
		invalidVert = count;

	for (unsigned i = 0; i < count; ++i)
	{
		if (primStart && ((primStart[stride * i].flags & VALID_01) == VALID_01) && (primStart[stride * i].value == *(unsigned int*)(&pPrimData[stride * i * 2])))
		{
			// clear upper 4 bits
			float x = primStart[stride * i].x *(1 << 16);
			float y = primStart[stride * i].y *(1 << 16);
			x = (float)(((int)x << 4) >> 4);
			y = (float)(((int)y << 4) >> 4);
			x /= (1 << 16);
			y /= (1 << 16);

			pVertex[i].x = x + xOffs;
			pVertex[i].y = y + yOffs;
			pVertex[i].z = 0.95f;
			pVertex[i].w = primStart[stride * i].z;
			pVertex[i].PGXP_flag = SRC_PGXP;
			pVertex[i].Vert_ID = primStart[stride * i].count;

			if ((primStart[stride * i].flags & VALID_2) != VALID_2)
			{
				pVertex[i].PGXP_flag = SRC_PGXP_NO_W;
		//		__Log("GPPV No W: v:%x (%d, %d) pgxp(%f, %f)|\n", (currentAddr + 1 + (i * stride)) * 4, pPrimData[stride * i * 2], pPrimData[(stride * i * 2) + 1], primStart[stride * i].x, primStart[stride * i].y);
			}

			// Log incorrect vertices
			//if (PGXP_tDebug &&
			//	(fabs((float)pPrimData[stride * i * 2] - primStart[stride * i].x) > debug_tolerance) ||
			//	(fabs((float)pPrimData[(stride * i * 2) + 1] - primStart[stride * i].y) > debug_tolerance))
			//	__Log("GPPV: v:%x (%d, %d) pgxp(%f, %f)|\n", (currentAddr + offset + 1 + (i * stride)) * 4, pPrimData[stride * i * 2], pPrimData[(stride * i * 2) + 1], primStart[stride * i].x, primStart[stride * i].y);
		}
		else
		{
			// Default to low precision vertex data
			//if (primStart  && ((primStart[stride * i].flags & VALID_01) == VALID_01) && primStart[stride * i].value != *(unsigned int*)(&pPrimData[stride * i * 2]))
			//	pVertex[i].PGXP_flag = 6;
			//else
				pVertex[i].PGXP_flag = SRC_NATIVE;

			// Look in cache for valid vertex
			pCacheVert = PGXP_GetCachedVertex(pPrimData[stride * i * 2], pPrimData[(stride * i * 2) + 1]);
			if (pCacheVert)
			{
				if (IsSessionID(pCacheVert->count))
				{
					if (pCacheVert->mFlags == 1)
					{
						pVertex[i].x = (pCacheVert->x + xOffs);
						pVertex[i].y = (pCacheVert->y + yOffs);
						pVertex[i].z = 0.95f;
						pVertex[i].w = pCacheVert->z;
						pVertex[i].PGXP_flag = SRC_CACHE;
						pVertex[i].Vert_ID = pCacheVert->count;
						// reduce number of invalid vertices
						invalidVert--;
					}
					else if(pCacheVert->mFlags > 1)
						pVertex[i].PGXP_flag = SRC_CACHE_AMBIGUOUS;
				}
			}

			// Log unprocessed vertices
			//if(PGXP_tDebug)
			//	__Log("GPPV: v:%x (%d, %d)|\n", (currentAddr + offset + 1 + (i * stride))*4, pPrimData[stride * i * 2], pPrimData[(stride * i * 2) + 1]);
		}
	}

	// If there are any invalid vertices set all w values to 1
	// iCB: Could use plane equation to find w for single invalid vertex in a quad
	if (invalidVert > 0)
		for (unsigned i = 0; i < count; ++i)
			pVertex[i].w = 1;

	//if(PGXP_vDebug == 5)
	//	for (unsigned i = 0; i < count; ++i)
	//		pVertex[i].PGXP_flag = primIdx + 10;

	return 1;
}

/////////////////////////////////
//// Visual Debugging Functions
/////////////////////////////////
unsigned int		PGXP_vDebug = 0;

enum PGXP_vDebugMode
{
	vDEBUG_NONE = 0,
	vDEBUG_SOURCE,
	vDEBUG_W,
	vDEBUG_OTZ,
	vDEBUG_COLOUR,
	vDEBUG_TEXTURE,
	vDEBUG_PRIMTYPE,

	vDEBUG_MAX,

	vDEBUG_TEXCOORD,
	vDEBUG_ID,
};

const char red[4]		= { 255, 0, 0, 255 };
const char blue[4]		= { 0, 0, 255, 255 };
const char green[4]		= { 0, 255, 0, 255 };

const char yellow[4]	= { 255, 255, 0, 255 };
const char magenta[4]	= { 255, 0, 255, 255 };
const char cyan[4]		= { 0, 255, 255, 255 };

const char orange[4]	= { 255, 128 ,0 ,255 };

const char black[4]		= { 0, 0, 0, 255 };
const char mid_grey[4]	= { 128, 128, 128, 255 };


//void CALLBACK GPUtoggleDebug(void)
//{
//	PGXP_tDebug = (PGXP_tDebug) ? 0 : 1;
//}

void CALLBACK GPUtoggleDebug(void)
{
	PGXP_vDebug++;

	if (PGXP_vDebug == vDEBUG_MAX)
		PGXP_vDebug = vDEBUG_NONE;
}

void ColourFromRange(float val, float min, float max, GLubyte alpha, int wrap)
{
	float r=0.f, g=0.f, b=0.f;

	// normalise input
	val = val - min;
	val /= (max - min);
	val *= 4.f;

	if (wrap)
		val = fmod(val, 1);

	if (0 <= val && val<= 1.f / 8.f)
	{
		r = 0;
		g = 0;
		b = 4 * val + .5; // .5 - 1 // b = 1/2
	}
	else if (1.f / 8.f < val && val <= 3.f / 8.f)
	{
		r = 0;
		g = 4 * val - .5; // 0 - 1 // b = - 1/2
		b = 1; // small fix
	}
	else if (3.f / 8.f < val && val <= 5.f / 8.f)
	{
		r = 4 * val - 1.5; // 0 - 1 // b = - 3/2
		g = 1;
		b = -4 * val + 2.5; // 1 - 0 // b = 5/2
	}
	else if (5.f / 8.f < val && val <= 7.f / 8.f)
	{
		r = 1;
		g = -4 * val + 3.5; // 1 - 0 // b = 7/2
		b = 0;
	}
	else if (7.f / 8.f < val && val <= 1.f)
	{
		r = -4 * val + 4.5; // 1 - .5 // b = 9/2
		g = 0;
		b = 0;
	}
	else
	{    // should never happen - value > 1
		r = .5;
		g = 0;
		b = 0;
	}

	glColor4f(r, g, b, (float)alpha/255.f);
}

void PGXP_colour(OGLVertex* vertex, GLubyte alpha, int prim, int isTextured, int colourMode, unsigned char* flatColour)
{
	const char* pColour;
	float fDepth;
	static float minW = 0xffffffff, maxW = 0.f;


	PGXP_vertex* pVal = NULL;


	switch (PGXP_vDebug)
	{
	case vDEBUG_SOURCE:
		// Vertex source mode
		switch (vertex->PGXP_flag)
		{
		case SRC_2D:
			pColour = yellow;
			break;
		case SRC_PGXP:
			pColour = blue;
			break;
		case SRC_PGXP_NO_W:
			pColour = cyan;
			break;
		case SRC_NATIVE:
			pColour = red;
			break;
		case SRC_CACHE:
			pColour = green;
			break;
		case SRC_CACHE_AMBIGUOUS:
			pColour = magenta;
			break;
		default:
			pColour = mid_grey;
			break;
		}
		glColor4ub(pColour[0], pColour[1], pColour[2], alpha);
		break;
	case vDEBUG_W:
		// W component visualisation
		ColourFromRange(vertex->w, 0, 0xFFFF, alpha, 0);
		break;
	case vDEBUG_OTZ:
		// order table position visualisation
		ColourFromRange(maxZ - currentDepth, 0, maxZ * 5, alpha, 0);// 1024 * 16);
		break;
	case vDEBUG_COLOUR:
		// Vertex colour only
		switch (colourMode)
		{
			// Flat shaded primitives have their colour set earlier so we'll just leave it.
		//case COLOUR_NONE:
		//	glColor4ub(255, 255, 255, 255);
		//	break;
		case COLOUR_FLAT:
			glColor4ubv(flatColour);
			break;
		case COLOUR_SMOOTH:
			glColor4ubv(vertex->c.col);
			break;
		}

		break;
	case vDEBUG_TEXTURE:
		// Texture only
		glColor4ub(255, 255, 255, 255);
		break;
	case vDEBUG_PRIMTYPE:
		// Primitive type
		glColor4ub((prim+1) * 64, (isTextured) * 255, colourMode * 64, alpha);
		break;
	case vDEBUG_TEXCOORD:
		// texture coordinates
		glColor4f(vertex->sow, vertex->tow, isTextured, alpha);
		break;
	case vDEBUG_ID:
		// Vertex ID
		ColourFromRange(GetSessionIndex(vertex->Vert_ID), 0, GetSessionIndex(lastID-1), alpha, 1);
	}
}

#define DRAW_QUAD		0
#define DRAW_TRI		1
#define DRAW_TRIQUAD	2

int DrawDebugPrim(int prim, OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4, int colourMode, int isTextured)
{
	GLboolean	bTexture = glIsEnabled(GL_TEXTURE_2D);
	GLboolean	bBlend = glIsEnabled(GL_BLEND);
	GLfloat		fColour[4];
	GLint		iShadeModel;
	GLubyte		alpha = 255;

	//if ((vertex1->PGXP_flag == 0) ||
	//	(vertex2->PGXP_flag == 0) ||
	//	(vertex3->PGXP_flag == 0) ||
	//	(vertex4->PGXP_flag == 0))
	//	return 0;

	// Quit if PGXP_flag == ignore
	if ((vertex1->PGXP_flag >= SRC_UNKNOWN) ||
		(vertex2->PGXP_flag >= SRC_UNKNOWN) ||
		(vertex3->PGXP_flag >= SRC_UNKNOWN))
		return 1;

	if (bBlend == GL_TRUE)
	{
		//		alpha = 128;
		glDisable(GL_BLEND);
	}

	if(DrawSemiTrans)
	{
		glPolygonMode(GL_FRONT, GL_LINE);
		glPolygonMode(GL_BACK, GL_LINE);
		glLineWidth(5.f);
	//	return 1;
	}

	glGetIntegerv(GL_SHADE_MODEL, &iShadeModel);
	glGetFloatv(GL_CURRENT_COLOR, fColour);

	if(PGXP_vDebug != vDEBUG_TEXTURE)
		glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	switch (prim)
	{
	case DRAW_QUAD:
		glBegin(GL_QUADS);
		break;
	case DRAW_TRI:
		glBegin(GL_TRIANGLES);
		break;
	case DRAW_TRIQUAD:
		glBegin(GL_TRIANGLE_STRIP);
		break;
	}

	PGXP_colour(vertex1, alpha, prim, isTextured, colourMode, vertex1->c.col);
	glTexCoord2fv(&vertex1->sow);
	PGXP_glVertexfv(&vertex1->x);

	PGXP_colour(vertex2, alpha, prim, isTextured, colourMode, vertex1->c.col);
	glTexCoord2fv(&vertex2->sow);
	PGXP_glVertexfv(&vertex2->x);

	PGXP_colour(vertex3, alpha, prim, isTextured, colourMode, vertex1->c.col);
	glTexCoord2fv(&vertex3->sow);
	PGXP_glVertexfv(&vertex3->x);

	if (prim != DRAW_TRI)
	{
		PGXP_colour(vertex4, alpha, prim, isTextured, colourMode, vertex1->c.col);
		glTexCoord2fv(&vertex4->sow);
		PGXP_glVertexfv(&vertex4->x);
	}

	glEnd();


//	if (bBlend == GL_TRUE)
//		glDisable(GL_BLEND);

	glLineWidth(1.f);
	glPolygonMode(GL_FRONT, GL_LINE);
	glPolygonMode(GL_BACK, GL_LINE);

	switch (prim)
	{
	case DRAW_QUAD:
		glBegin(GL_QUADS);
		break;
	case DRAW_TRI:
		glBegin(GL_TRIANGLES);
		break;
	case DRAW_TRIQUAD:
		glBegin(GL_TRIANGLE_STRIP);
		break;
	}

	glColor4ubv(black);
	PGXP_glVertexfv(&vertex1->x);
	PGXP_glVertexfv(&vertex2->x);
	PGXP_glVertexfv(&vertex3->x);
	if (prim != DRAW_TRI)
		PGXP_glVertexfv(&vertex4->x);

	glColor4fv(fColour);

	glEnd();

	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	if(bTexture == GL_TRUE)
		glEnable(GL_TEXTURE_2D);

	if (bBlend == GL_TRUE)
		glEnable(GL_BLEND);

	glShadeModel(iShadeModel);

	return 1;
}

int PGXP_DrawDebugTri(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, int colourMode, int isTextured)
{
	return DrawDebugPrim(DRAW_TRI, vertex1, vertex2, vertex3, NULL, colourMode, isTextured);
}

int PGXP_DrawDebugQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4, int colourMode, int isTextured)
{
	return DrawDebugPrim(DRAW_QUAD, vertex1, vertex2, vertex3, vertex4, colourMode, isTextured);
}

int PGXP_DrawDebugTriQuad(OGLVertex* vertex1, OGLVertex* vertex2, OGLVertex* vertex3, OGLVertex* vertex4, int colourMode, int isTextured)
{
	return DrawDebugPrim(DRAW_TRIQUAD, vertex1, vertex2, vertex3, vertex4, colourMode, isTextured);
}
