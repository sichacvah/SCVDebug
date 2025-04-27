#ifndef SCV_FONT
#define SCV_FONT

u16
scvFontGetU16(SCVSlice bytes)
{
  u8 *base = (u8 *)bytes.base;
  scvAssert(bytes.len > 2);

  return ((u16)base[0] << 8 | (u16)base[1]);
}

i16
scvFontGetI16(SCVSlice bytes)
{
  return (i16)(scvFontGetU16(slice));
}

f32
scvFontGetF2_14(SCVSlice bytes)
{
  return ((f32)scvFontGetI16(bytes) * (1.0f / (f32)(1 << 14)));
}

u32
scvFontGetU32(SCVSlice bytes)
{
  u8 *base = (u8 *)bytes.base;
  scvAssert(bytes.len > 4);

  return (((u32)base[0] << 24) | ((u32)base[1] << 16) | ((u32)base[2] << 8) | (u32)base[3]);
}

i16
scvFontIndexToLocFormat(SCVSlice bytes)
{
  return scvFontGetI16(scvSliceLeft(bytes, 50));
}

u16
scvFontUnitsPerEm(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 16));
}

u16
scvFontNumGlyphs(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 4));
}

u32
scvFontGetOff(SCVSlice bytes, glyphIx: u16, fmt: i16)
{
  return fmt != 0 ? scvFontGetU32(scvSliceLeft(bytes, glyphIx * 4)) : scvFontGetU32(scvSliceLeft(bytes, glyphIx * 2)) * 2;
}

i16
scvFontAscent(SCVSlice bytes)
{
  return scvFontGetI16(scvSliceLeft(bytes, 4));
}

i16
scvFontDescent(SCVSlice bytes)
{
  return scvFontGetI16(scvSliceLeft(bytes, 6));
}

i16
scvFontLineGap(SCVSlice bytes)
{
  return scvFontGetI16(scvSliceLeft(bytes, 8));
}

u16
scvFontNumOfLongHorMetrics(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 34));
}

typedef struct SCVFontHMetrics SCVFontHMetrics;
struct SCVFontHMetrics {
  u16 advanceWidth;
  i16 leftSideBearing;
};

SCVFontHMetrics
scvFontGetHMetrics(SCVSlice bytes, u16 glyphID, u16 numOfLongHMetrics)
{
  SCVFontHMetrics metrics = {0};
  if (glyphID < numOfLongHMetrics) {
    metrics.advanceWidth = scvFontGetU16(scvSliceLeft(bytes, 4 * (u64)glyphID));
    metrics.leftSideBearing = scvFontGetI16(scvSliceLeft(bytes, 4 * (u64)glyphID + 2));
  } else {
    metrics.advanceWidth = scvFontGetU16(scvSliceLeft(bytes, 4 * (u64)(numOfLongHMetrics - 1)));
    metrics.leftSideBearing = scvFontGetI16(scvSliceLeft(bytes,
          4 * (u64)numOfLongHMetrics + 2 * ((u64)glyphID - (u64)numOfLongHMetrics)));
    ));
  }

  return metrics;
}

u16
scvFontGetPlatformID(SCVSlice bytes)
{
  return scvFontGetU16(bytes);
}

u16
scvFontGetEndcodingID(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 2));
}

u32
scvFontGetOffset(SCVSlice bytes)
{
  return scvFontGetU32(scvSliceLeft(bytes, 4));
}

void
scvPrintEncodingRecord(SCVSlice bytes)
{
  scvPrintCString("EncodingRecord {");
  scvPrint("\tplatformID = ");
  scvPrintU64((u64)scvFontGetPlatformID(bytes));
  scvPrint("\tencodingID = ");
  scvPrintU64((u64)scvFontGetEndcodingID(bytes));
  scvPrint("\toffset = ");
  scvPrintU64((u64)scvFontGetOffset(bytes));
  scvPrintCString("}"); 
}

u16
scvFontGetFormat(SCVSlice bytes)
{
  return scvFontGetU16(bytes);
}

u16
scvFontGetLength(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 2));
}

u16
scvFontGetLanguage(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 4));
}

void
scvPrintEncodig(SCVSlice bytes)
{
  scvPrintCString("Encoding {");
  scvPrint("\tformat = ");
  scvPrintU64((u64)scvFontGetFormat(bytes));
  scvPrint("\tlength = ");
  scvPrintU64((u64)scvFontGetLength(bytes));
  scvPrint("\tlanguage = ");
  scvPrintU64((u64)scvFontGetLanguage(bytes));
  scvPrintCString("}");
}

u16
scvFontGetSegCountX2(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 6));
}

u16
scvFontGetSegCount(SCVSlice bytes)
{
  return scvFontGetSegCountX2(bytes) / 2;
}

u16
scvFontGetSearchRange(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 8));
}

typedef struct SCV16Iterator SCV16Iterator;
struct SCV16Iterator {
  SCVSlice slice;
  u64      offset;
};

SCV16Iterator
scv16Iterator(SCVSlice slice)
{
  SCV16Iterator iterator;
  iterator.slice = slice;
  iterator.offset = 0;
  return iterator;
}

bool
scv16IteratorHasNext(SCV16Iterator *iterator) 
{
  return (iterator->slice.len - 1) > iterator->offset;
}

i16
scv16IteratorGetNext(SCV16Iterator *iterator)
{
  u64 index = iterator->offset;
  scvAssert(scv16IteratorHasNext(iterator));
  iterator->offset += 2;

  return scvFontGetI16(scvSliceLeft(iterator->slice, index));
}


u16
scv16IteratorGetNextU(SCV16Iterator *iterator)
{
  u64 index = iterator->offset;
  scvAssert(scv16IteratorHasNext(iterator));
  iterator->offset += 2;

  return scvFontGetU16(scvSliceLeft(iterator->slice, index));
}

#define SCV_FONT_ENDCOUNTPOSITION 14


u16
scvFontGetEntrySelector(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 10));
}

u16
scvFontGetRangeShift(SCVSlice bytes)
{
  return scvFontGetU16(scvSliceLeft(bytes, 12));
}

SCV16Iterator
scvFontGetEndCounts(SCVSlice bytes)
{
  u64 segCount = (u64)scvFontGetSegCount(bytes);
  return scv16Iterator(scvSliceRight(scvSliceLeft(bytes, SCV_FONT_ENDCOUNTPOSITION), segCount));
}

u16
scvFontGetStartCountPosition(SCVSlice bytes, u16 segCount)
{
  return (SCV_FONT_ENDCOUNTPOSITION + 2 + 2 * segCount);
}

SCV16Iterator
scvFontGetStartCounts(SCVSlice bytes)
{
  
  u16 segCount = scvFontGetSegCount(bytes);

  return scv16Iterator(scvSliceRight(scvSliceLeft(bytes, (u64)scvFontGetStartCountPosition(bytes, segCount)), (u64)segCount));
}

u16
scvFontGetIDDeltasPosition(SCVSlice bytes, u16 segCount)
{
  return scvFontGetStartCountPosition(bytes, segCount) + 2 * segCount;
}

SCV16Iterator
scvFontGetIDDeltas(SCVSlice bytes)
{
  u16 segCount = scvFontGetSegCount(bytes);

  return scv16Iterator(scvSliceRight(scvSliceLeft(bytes, (u64)scvFontGetIDDeltasPosition(bytes, segCount)), (u64)segCount));
}



#endif
