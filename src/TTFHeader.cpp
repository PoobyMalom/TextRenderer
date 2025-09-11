#include "TTFHeader.h"
#include "Helpers.h"
#include <stdexcept>
#include <iostream>

/*
Notably most of this subtable is not used but might be implemented later if rewritten in c
In c without a custom class for the header you can use the searchrange, entryselector, and range shift
to perform a binary search for a specific table
*/
TTFHeader::TTFHeader(uint32_t scalarType, uint16_t numTables, uint16_t searchRange, uint16_t entrySelector, uint16_t rangeShift)
    : scalarType(scalarType), numTables(numTables), searchRange(searchRange), entrySelector(entrySelector), rangeShift(rangeShift) {
        /*
        scalarType: Scalar Type is used to determine which scalar to use for the font (how to extract glyph data from the font)
                    the values 'true' (0x74727565) and 0x00010000 are recognized as referring to ttf file types
                    the value 'typ1' (0x74797931) is recognized as referring to the old style of PostScript fonts housed in a sfnt wrapper
                    the value 'OTTO' (0x4F54544F) indicates an OpenType font with PostScript outlines (that is, a 'CFF' table instead of 'GLYF')
                    other values not currently supported
        numTables: Number of tables listed in the file
        searchRange:  the largest power of two less than or equal to the number of items in the table, 
                      i.e. the largest number of items that can be easily searched.
        rangeShift: rangeShift is the number of items minus searchRange; i.e. the number of items that will not get looked at if you only look at searchRange items.
                    Before the search loop starts, compare the target item to the item with number rangeShift. If the target item is less than rangeShift, s
                    earch from the beginning of the table. If it is greater, search starting at the item with number rangeShift.
        entrySelector: entrySelector is log2(searchRange). It tells how many iterations of the search loop are needed. (i.e. how many times to cut the range in half)
                       Note that the searchRange, the entrySelector and the rangeShift are all multiplied by 16 which represents the size of a directory entry.
        */
    }

TTFHeader TTFHeader::parse(const std::vector<char>& data) {
    int offset = 0;
    uint32_t scalarType = read4Bytes(data, offset); 
    uint16_t numTables = read2Bytes(data, offset); 
    uint16_t searchRange = read2Bytes(data, offset);
    uint16_t entrySelector = read2Bytes(data, offset);
    uint16_t rangeShift = read2Bytes(data, offset);

    if (scalarType == 0x74727565 || scalarType == 0x00010000) {
        cout << "ttf format detected\n";
        return TTFHeader(scalarType, numTables, searchRange, entrySelector, rangeShift);
    } else if (scalarType == 0x74797931) {
        cerr << "typ1 format detected, file format not yet supported\n";
        return TTFHeader(scalarType, numTables, searchRange, entrySelector, rangeShift);
    } else if (scalarType == 0x4F54544F) {
        cerr << "OTTO format detected, file format not yet supported\n";
        return TTFHeader(scalarType, numTables, searchRange, entrySelector, rangeShift);
    } else {
        cerr << "Unrecognized file format\n";
        return TTFHeader(scalarType, numTables, searchRange, entrySelector, rangeShift);
    } 
}

TableMap buildTableMap(const vector<TTFTable*>& tables) {
    TableMap map;
    map.reserve(tables.size());
    for (TTFTable* t : tables) {
        map.emplace(t->getTag(), t);
    }

    return map;
}

void TTFHeader::parseTables(const std::vector<char>& data) {
    tablesList = TTFTable::parseTableDirectory(data, numTables);
    tables = buildTableMap(tablesList);
}


uint32_t TTFHeader::getScalarType() const { return scalarType; }
uint16_t TTFHeader::getNumTables() const { return numTables; }
uint16_t TTFHeader::getSearchRange() const { return searchRange; }
uint16_t TTFHeader::getEntrySelector() const { return entrySelector; }
uint16_t TTFHeader::getRangeShift() const { return rangeShift; }
TableMap TTFHeader::getTables() const { return tables; }

std::ostream& operator<<(std::ostream& os, const TTFHeader& obj) {
    os << "Offset Subtable (ScalarType: " << obj.scalarType << 
    ", NumTables: " << obj.numTables << 
    ", SearchRange: " << obj.searchRange << 
    ", EntrySelector: " << obj.entrySelector << 
    ", RangeShift: " << obj.rangeShift << ")" << endl;
    return os;
}