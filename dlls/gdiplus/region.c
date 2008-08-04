/*
 * Copyright (C) 2008 Google (Lei Zhang)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "objbase.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

/**********************************************************
 *
 * Data returned by GdipGetRegionData looks something like this:
 *
 * struct region_data_header
 * {
 *   DWORD size;     size in bytes of the data - 8.
 *   DWORD magic1;   probably a checksum.
 *   DWORD magic2;   always seems to be 0xdbc01001 - version?
 *   DWORD num_ops;  number of combining ops * 2
 * };
 *
 * Then follows a sequence of combining ops and region elements.
 *
 * A region element is either a RECTF or some path data.
 *
 * Combining ops are just stored as their CombineMode value.
 *
 * Each RECTF is preceded by the DWORD 0x10000000.  An empty rect is
 * stored as 0x10000002 (with no following RECTF) and an infinite rect
 * is stored as 0x10000003 (again with no following RECTF).
 *
 * Path data is preceded by the DWORD 0x10000001.  Then follows a
 * DWORD size and then size bytes of data.
 *
 * The combining ops are stored in the reverse order to the region
 * elements and in the reverse order to which the region was
 * constructed.
 *
 * When two or more complex regions (ie those with more than one
 * element) are combined, the combining op for the two regions comes
 * first, then the combining ops for the region elements in region 1,
 * followed by the region elements for region 1, then follows the
 * combining ops for region 2 and finally region 2's region elements.
 * Presumably you're supposed to use the 0x1000000x header to find the
 * end of the op list (the count of the elements in each region is not
 * stored).
 *
 * When a simple region (1 element) is combined, it's treated as if a
 * single rect/path is being combined.
 *
 */

typedef enum RegionType
{
    RegionDataRect          = 0x10000000,
    RegionDataPath          = 0x10000001,
    RegionDataEmptyRect     = 0x10000002,
    RegionDataInfiniteRect  = 0x10000003,
} RegionType;

#define FLAGS_NOFLAGS   0x0
#define FLAGS_INTPATH   0x4000

/* Header size as far as header->size is concerned. This doesn't include
 * header->size or header->checksum
 */
static const INT sizeheader_size = sizeof(DWORD) * 2;

typedef struct packed_point
{
    short X;
    short Y;
} packed_point;

/* Everything is measured in DWORDS; round up if there's a remainder */
static inline INT get_pathtypes_size(const GpPath* path)
{
    INT needed = path->pathdata.Count / sizeof(DWORD);

    if (path->pathdata.Count % sizeof(DWORD) > 0)
        needed++;

    return needed * sizeof(DWORD);
}

static inline INT get_element_size(const region_element* element)
{
    INT needed = sizeof(DWORD); /* DWORD for the type */
    switch(element->type)
    {
        case RegionDataRect:
            return needed + sizeof(GpRect);
        case RegionDataPath:
             needed += element->elementdata.pathdata.pathheader.size;
             needed += sizeof(DWORD); /* Extra DWORD for pathheader.size */
             return needed;
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            return needed;
        default:
            needed += get_element_size(element->elementdata.combine.left);
            needed += get_element_size(element->elementdata.combine.right);
            return needed;
    }

    return 0;
}

/* Does not check parameters, caller must do that */
static inline GpStatus init_region(GpRegion* region, const RegionType type)
{
    region->node.type       = type;
    region->header.checksum = 0xdeadbeef;
    region->header.magic    = VERSION_MAGIC;
    region->header.num_children  = 0;
    region->header.size     = sizeheader_size + get_element_size(&region->node);

    return Ok;
}

static inline void delete_element(region_element* element)
{
    switch(element->type)
    {
        case RegionDataRect:
            break;
        case RegionDataPath:
            GdipDeletePath(element->elementdata.pathdata.path);
            break;
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            break;
        default:
            delete_element(element->elementdata.combine.left);
            delete_element(element->elementdata.combine.right);
            GdipFree(element->elementdata.combine.left);
            GdipFree(element->elementdata.combine.right);
            break;
    }
}

/*****************************************************************************
 * GdipCloneRegion [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCloneRegion(GpRegion *region, GpRegion **clone)
{
    FIXME("(%p %p): stub\n", region, clone);

    *clone = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCombineRegionPath(GpRegion *region, GpPath *path, CombineMode mode)
{
    FIXME("(%p %p %d): stub\n", region, path, mode);
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCombineRegionRect(GpRegion *region, GDIPCONST GpRectF *rect,
                                          CombineMode mode)
{
    FIXME("(%p %p %d): stub\n", region, rect, mode);
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCombineRegionRectI(GpRegion *region, GDIPCONST GpRect *rect,
                                           CombineMode mode)
{
    FIXME("(%p %p %d): stub\n", region, rect, mode);
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCombineRegionRegion(GpRegion *region1, GpRegion *region2,
                                            CombineMode mode)
{
    FIXME("(%p %p %d): stub\n", region1, region2, mode);
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCreateRegion(GpRegion **region)
{
    if(!region)
        return InvalidParameter;

    TRACE("%p\n", region);

    *region = GdipAlloc(sizeof(GpRegion));
    if(!*region)
        return OutOfMemory;

    return init_region(*region, RegionDataInfiniteRect);
}

/*****************************************************************************
 * GdipCreateRegionPath [GDIPLUS.@]
 *
 * Creates a GpRegion from a GpPath
 *
 * PARAMS
 *  path    [I] path to base the region on
 *  region  [O] pointer to the newly allocated region
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter
 *
 * NOTES
 *  If a path has no floating point points, its points will be stored as shorts
 *  (INTPATH)
 *
 *  If a path is empty, it is considered to be an INTPATH
 */
GpStatus WINGDIPAPI GdipCreateRegionPath(GpPath *path, GpRegion **region)
{
    region_element* element;
    GpPoint  *pointsi;
    GpPointF *pointsf;

    GpStatus stat;
    DWORD flags = FLAGS_INTPATH;
    INT count, i;

    TRACE("%p, %p\n", path, region);

    if (!(path && region))
        return InvalidParameter;

    *region = GdipAlloc(sizeof(GpRegion));
    if(!*region)
        return OutOfMemory;
    stat = init_region(*region, RegionDataPath);
    if (stat != Ok)
    {
        GdipDeleteRegion(*region);
        return stat;
    }
    element = &(*region)->node;
    count = path->pathdata.Count;

    /* Test to see if the path is an Integer path */
    if (count)
    {
        pointsi = GdipAlloc(sizeof(GpPoint) * count);
        pointsf = GdipAlloc(sizeof(GpPointF) * count);
        if (!(pointsi && pointsf))
        {
            GdipFree(pointsi);
            GdipFree(pointsf);
            GdipDeleteRegion(*region);
            return OutOfMemory;
        }

        stat = GdipGetPathPointsI(path, pointsi, count);
        if (stat != Ok)
        {
            GdipDeleteRegion(*region);
            return stat;
        }
        stat = GdipGetPathPoints(path, pointsf, count);
        if (stat != Ok)
        {
            GdipDeleteRegion(*region);
            return stat;
        }

        for (i = 0; i < count; i++)
        {
            if (!(pointsi[i].X == pointsf[i].X &&
                  pointsi[i].Y == pointsf[i].Y ))
            {
                flags = FLAGS_NOFLAGS;
                break;
            }
        }
        GdipFree(pointsi);
        GdipFree(pointsf);
    }

    stat = GdipClonePath(path, &element->elementdata.pathdata.path);
    if (stat != Ok)
    {
        GdipDeleteRegion(*region);
        return stat;
    }

    /* 3 for headers, once again size doesn't count itself */
    element->elementdata.pathdata.pathheader.size = ((sizeof(DWORD) * 3));
    switch(flags)
    {
        /* Floats, sent out as floats */
        case FLAGS_NOFLAGS:
            element->elementdata.pathdata.pathheader.size +=
                (sizeof(DWORD) * count * 2);
            break;
        /* INTs, sent out as packed shorts */
        case FLAGS_INTPATH:
            element->elementdata.pathdata.pathheader.size +=
                (sizeof(DWORD) * count);
            break;
        default:
            FIXME("Unhandled flags (%08x). Expect wrong results.\n", flags);
    }
    element->elementdata.pathdata.pathheader.size += get_pathtypes_size(path);
    element->elementdata.pathdata.pathheader.magic = VERSION_MAGIC;
    element->elementdata.pathdata.pathheader.count = count;
    element->elementdata.pathdata.pathheader.flags = flags;
    (*region)->header.size = sizeheader_size + get_element_size(element);

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateRegionRect(GDIPCONST GpRectF *rect, GpRegion **region)
{
    FIXME("(%p, %p): stub\n", rect, region);

    *region = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCreateRegionRectI(GDIPCONST GpRect *rect, GpRegion **region)
{
    FIXME("(%p, %p): stub\n", rect, region);

    *region = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCreateRegionRgnData(GDIPCONST BYTE *data, INT size, GpRegion **region)
{
    FIXME("(%p, %d, %p): stub\n", data, size, region);

    *region = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipCreateRegionHrgn(HRGN hrgn, GpRegion **region)
{
    FIXME("(%p, %p): stub\n", hrgn, region);

    *region = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipDeleteRegion(GpRegion *region)
{
    TRACE("%p\n", region);

    if (!region)
        return InvalidParameter;

    delete_element(&region->node);
    GdipFree(region);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetRegionBounds(GpRegion *region, GpGraphics *graphics, GpRectF *rect)
{
    FIXME("(%p, %p, %p): stub\n", region, graphics, rect);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetRegionBoundsI(GpRegion *region, GpGraphics *graphics, GpRect *rect)
{
    FIXME("(%p, %p, %p): stub\n", region, graphics, rect);

    return NotImplemented;
}

static inline void write_dword(DWORD* location, INT* offset, const DWORD write)
{
    location[*offset] = write;
    (*offset)++;
}

static inline void write_float(DWORD* location, INT* offset, const FLOAT write)
{
    ((FLOAT*)location)[*offset] = write;
    (*offset)++;
}

static inline void write_packed_point(DWORD* location, INT* offset,
        const GpPointF* write)
{
    packed_point point;

    point.X = write->X;
    point.Y = write->Y;
    memcpy(location + *offset, &point, sizeof(packed_point));
    (*offset)++;
}

static inline void write_path_types(DWORD* location, INT* offset,
        const GpPath* path)
{
    memcpy(location + *offset, path->pathdata.Types, path->pathdata.Count);

    /* The unwritten parts of the DWORD (if any) must be cleared */
    if (path->pathdata.Count % sizeof(DWORD))
        ZeroMemory(((BYTE*)location) + (*offset * sizeof(DWORD)) +
                path->pathdata.Count,
                sizeof(DWORD) - path->pathdata.Count % sizeof(DWORD));
    *offset += (get_pathtypes_size(path) / sizeof(DWORD));
}

static void write_element(const region_element* element, DWORD *buffer,
        INT* filled)
{
    write_dword(buffer, filled, element->type);
    switch (element->type)
    {
        case CombineModeReplace:
        case CombineModeIntersect:
        case CombineModeUnion:
        case CombineModeXor:
        case CombineModeExclude:
        case CombineModeComplement:
            write_element(element->elementdata.combine.left, buffer, filled);
            write_element(element->elementdata.combine.right, buffer, filled);
            break;
        case RegionDataRect:
            write_float(buffer, filled, element->elementdata.rect.X);
            write_float(buffer, filled, element->elementdata.rect.Y);
            write_float(buffer, filled, element->elementdata.rect.Width);
            write_float(buffer, filled, element->elementdata.rect.Height);
            break;
        case RegionDataPath:
        {
            INT i;
            const GpPath* path = element->elementdata.pathdata.path;

            memcpy(buffer + *filled, &element->elementdata.pathdata.pathheader,
                    sizeof(element->elementdata.pathdata.pathheader));
            *filled += sizeof(element->elementdata.pathdata.pathheader) / sizeof(DWORD);
            switch (element->elementdata.pathdata.pathheader.flags)
            {
                case FLAGS_NOFLAGS:
                    for (i = 0; i < path->pathdata.Count; i++)
                    {
                        write_float(buffer, filled, path->pathdata.Points[i].X);
                        write_float(buffer, filled, path->pathdata.Points[i].Y);
                    }
                    break;
                case FLAGS_INTPATH:
                    for (i = 0; i < path->pathdata.Count; i++)
                    {
                        write_packed_point(buffer, filled,
                                &path->pathdata.Points[i]);
                    }
            }
            write_path_types(buffer, filled, path);
            break;
        }
        case RegionDataEmptyRect:
        case RegionDataInfiniteRect:
            break;
    }
}

/*****************************************************************************
 * GdipGetRegionData [GDIPLUS.@]
 *
 * Returns the header, followed by combining ops and region elements.
 *
 * PARAMS
 *  region  [I] region to retrieve from
 *  buffer  [O] buffer to hold the resulting data
 *  size    [I] size of the buffer
 *  needed  [O] (optional) how much data was written
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParamter
 *
 * NOTES
 *  The header contains the size, a checksum, a version string, and the number
 *  of children. The size does not count itself or the checksum.
 *  Version is always something like 0xdbc01001 or 0xdbc01002
 *
 *  An element is a RECT, or PATH; Combining ops are stored as their
 *  CombineMode value. Special regions (infinite, empty) emit just their
 *  op-code; GpRectFs emit their code followed by their points; GpPaths emit
 *  their code followed by a second header for the path followed by the actual
 *  path data. Followed by the flags for each point. The pathheader contains
 *  the size of the data to follow, a version number again, followed by a count
 *  of how many points, and any special flags which may apply. 0x4000 means its
 *  a path of shorts instead of FLOAT.
 *
 *  Combining Ops are stored in reverse order from when they were constructed;
 *  the output is a tree where the left side combining area is always taken
 *  first.
 */
GpStatus WINGDIPAPI GdipGetRegionData(GpRegion *region, BYTE *buffer, UINT size,
        UINT *needed)
{
    INT filled = 0;

    if (!(region && buffer && size))
        return InvalidParameter;

    TRACE("%p, %p, %d, %p\n", region, buffer, size, needed);
    memcpy(buffer, &region->header, sizeof(region->header));
    filled += sizeof(region->header) / sizeof(DWORD);
    /* With few exceptions, everything written is DWORD aligned,
     * so use that as our base */
    write_element(&region->node, (DWORD*)buffer, &filled);

    if (needed)
        *needed = filled * sizeof(DWORD);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetRegionDataSize(GpRegion *region, UINT *needed)
{
    if (!(region && needed))
        return InvalidParameter;

    TRACE("%p, %p\n", region, needed);

    /* header.size doesn't count header.size and header.checksum */
    *needed = region->header.size + sizeof(DWORD) * 2;

    return Ok;
}

/*****************************************************************************
 * GdipGetRegionHRgn [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetRegionHRgn(GpRegion *region, GpGraphics *graphics, HRGN *hrgn)
{
    FIXME("(%p, %p, %p): stub\n", region, graphics, hrgn);

    *hrgn = NULL;
    return NotImplemented;
}

GpStatus WINGDIPAPI GdipIsEmptyRegion(GpRegion *region, GpGraphics *graphics, BOOL *res)
{
    FIXME("(%p, %p, %p): stub\n", region, graphics, res);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipIsEqualRegion(GpRegion *region, GpRegion *region2, GpGraphics *graphics,
                                      BOOL *res)
{
    FIXME("(%p, %p, %p, %p): stub\n", region, region2, graphics, res);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipIsInfiniteRegion(GpRegion *region, GpGraphics *graphics, BOOL *res)
{
    FIXME("(%p, %p, %p): stub\n", region, graphics, res);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipSetEmpty(GpRegion *region)
{
    GpStatus stat;

    TRACE("%p\n", region);

    if (!region)
        return InvalidParameter;

    delete_element(&region->node);
    stat = init_region(region, RegionDataEmptyRect);

    return stat;
}

GpStatus WINGDIPAPI GdipSetInfinite(GpRegion *region)
{
    GpStatus stat;

    if (!region)
        return InvalidParameter;

    TRACE("%p\n", region);

    delete_element(&region->node);
    stat = init_region(region, RegionDataInfiniteRect);

    return stat;
}

GpStatus WINGDIPAPI GdipTransformRegion(GpRegion *region, GpMatrix *matrix)
{
    FIXME("(%p, %p): stub\n", region, matrix);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipTranslateRegion(GpRegion *region, REAL dx, REAL dy)
{
    FIXME("(%p, %f, %f): stub\n", region, dx, dy);

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipTranslateRegionI(GpRegion *region, INT dx, INT dy)
{
    FIXME("(%p, %d, %d): stub\n", region, dx, dy);

    return NotImplemented;
}
