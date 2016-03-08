//
//  tlali-osm.c
//  tlali
//
//  Created by Marcos Ortega on 05/03/2016.
//  Copyright (c) 2016 NIBSA. All rights reserved.
//
//  This entire notice must be retained in this source code.
//  This source code is under LGLP v2.1 Licence.
//
//  This software is provided "as is", with absolutely no warranty expressed
//  or implied. Any use is at your own risk.
//
//  Latest fixes enhancements and documentation at https://github.com/nicaraguabinary/lib-tlali-openstreetmap
//

#include <stdio.h>		//NULL
#include <string.h>		//memcpy, memset
#include <assert.h>		//assert
#include "tlali-osm.h"

//
// You can custom memory management by defining this MACROS
// and CONSTANTS before this file get included or compiled.
//
// This are the default memory management MACROS and CONSTANTS:
#if !defined(TLA_MALLOC) || !defined(TLA_FREE)
#	include <stdlib.h>		//malloc, free
#	ifndef TLA_MALLOC
#		define TLA_MALLOC(POINTER_DEST, POINTER_TYPE, SIZE_BYTES)	POINTER_DEST = (POINTER_TYPE*)malloc(SIZE_BYTES);
#	endif
#	ifndef TLA_FREE
#	define TLA_FREE(POINTER)		 free(POINTER);
#	endif
#endif


//-------------------------------
//-- BASIC DEFINITIONS
//-------------------------------

#define TLA_ASSERT(EVAL)			assert(EVAL);
#define TLA_ASSERT_LOADFILE(EVAL)	assert(EVAL);

#if defined(__ANDROID__)
#	include <android/log.h>
//#	pragma message("COMPILANDO PARA ANDROID")
//Requiere #include <android/log.h> (en el encabezado precompilado)
#	define TLA_PRINTF_INFO(STR_FMT, ...)		__android_log_print(ANDROID_LOG_INFO, "tlali-osm", STR_FMT, ##__VA_ARGS__)
#	define TLA_PRINTF_ERROR(STR_FMT, ...)		__android_log_print(ANDROID_LOG_ERROR, "tlali-osm", "ERROR, "STR_FMT, ##__VA_ARGS__)
#	define TLA_PRINTF_WARNING(STR_FMT, ...)		__android_log_print(ANDROID_LOG_WARN, "tlali-osm", "WARNING, "STR_FMT, ##__VA_ARGS__)
#elif defined(__APPLE__) || (defined(__APPLE__) && defined(__MACH__))
//#pragma message("COMPILANDO PARA iOS/Mac")
#	define TLA_PRINTF_INFO(STR_FMT, ...)		printf("TlaOsm, " STR_FMT, ##__VA_ARGS__)
#	define TLA_PRINTF_ERROR(STR_FMT, ...)		printf("TlaOsm ERROR, " STR_FMT, ##__VA_ARGS__)
#	define TLA_PRINTF_WARNING(STR_FMT, ...)		printf("TlaOsm WARNING, " STR_FMT, ##__VA_ARGS__)
#else
//#pragma message("(SE ASUME) COMPILANDO PARA BLACKBERRY")
#	define TLA_PRINTF_INFO(STR_FMT, ...)		fprintf(stdout, "TlaOsm, " STR_FMT, ##__VA_ARGS__); fflush(stdout)
#	define TLA_PRINTF_ERROR(STR_FMT, ...)		fprintf(stderr, "TlaOsm ERROR, " STR_FMT, ##__VA_ARGS__); fflush(stderr)
#	define TLA_PRINTF_WARNING(STR_FMT, ...)		fprintf(stdout, "TlaOsm WARNING, " STR_FMT, ##__VA_ARGS__); fflush(stdout)
#endif


#define TLA_FILE_VERIF_START	987
#define TLA_FILE_VERIF_END		453

//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++
//++ HELPER CLASS - ARRAY
//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++

//STTLAArray
typedef struct STTLAArray_ {
	TlaSI32		use;			//Buffer use count (in item units)
	//
	TlaSI32		_buffGrowth;	//Growth of the array buffer size when the limit is reached
	TlaSI32		_buffSize;		//Buffer curr max size (in items units)
	TlaBYTE*	_buffData;		//Buffer data
	TlaSI32		_bytesPerItem;	//Size of each element, in bytes
} STTLAArray;

void TLAArray_init(STTLAArray* obj, const TlaSI32 bytesPerItem, const TlaSI32 initialSize, const TlaSI32 growthSize);
void TLAArray_release(STTLAArray* obj);
//Search
void* TLAArray_itemAtIndex(const STTLAArray* obj, const TlaSI32 index);
//Remove
void TLAArray_removeItemAtIndex(STTLAArray* obj, const TlaSI32 index);
void TLAArray_removeItemsAtIndex(STTLAArray* obj, const TlaSI32 index, const TlaSI32 itemsCount);
//Redim
void TLAArray_empty(STTLAArray* obj);
void TLAArray_grow(STTLAArray* obj, const TlaSI32 quant);
void* TLAArray_add(STTLAArray* obj, const void* data, const TlaSI32 itemSize);
void* TLAArray_addItems(STTLAArray* obj, const void* data, const TlaSI32 itemSize, const TlaSI32 itemsCount);
void* TLAArray_set(STTLAArray* obj, const TlaSI32 index, const void* data, const TlaSI32 itemSize);
//File
TlaBOOL TLAArray_writeToFile(const STTLAArray* obj, FILE* file, const TlaBOOL includeUnusedBuffer);
TlaBOOL TLAArray_initFromFile(STTLAArray* obj, FILE* file, TlaBYTE* optExternalBuffer);

//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++
//++ HELPER CLASS - SORTED ARRAY
//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++

//STTLAArray
typedef struct STTLAArraySorted_ {
	TlaSI32 bytesToCompare;			//Bytes to be compared to determine item position.
	STTLAArray		_array;
} STTLAArraySorted;

//Factory
void TLAArraySorted_init(STTLAArraySorted* obj, const TlaSI32 bytesPerItem, const TlaSI32 bytesComparePerItem, const TlaSI32 initialSize, const TlaSI32 growthSize);
void TLAArraySorted_release(STTLAArraySorted* obj);
//Search
void* TLAArraySorted_itemAtIndex(const STTLAArraySorted* obj, const TlaSI32 index);
TlaSI32 TLAArraySorted_indexOf(const STTLAArraySorted* obj, const void* dataToSearch, const TlaSI32* optPosHint);
TlaSI32 TLAArraySorted_indexForNew(const STTLAArraySorted* obj, const TlaBYTE* dataToInsert);
//Add
TlaSI32 TLAArraySorted_add(STTLAArraySorted* obj, const void* data, const TlaSI32 itemSize);
//Remove
void TLAArraySorted_removeItemAtIndex(STTLAArraySorted* obj, const TlaSI32 index);
//File
TlaBOOL TLAArraySorted_writeToFile(const STTLAArraySorted* obj, FILE* file, const TlaBOOL includeUnusedBuffer);
TlaBOOL TLAArraySorted_initFromFile(STTLAArraySorted* obj, FILE* file, TlaBYTE* optExternalBuffer);
//
TlaSI32 TLAArraySorted_bytesCompare(const void* base, const void* compare, const TlaSI32 sizeInBytes);

//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++
//++ HELPER CLASS - STRING
//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++

//STTLAString
typedef struct STTLAString {
	char* str;
	TlaSI32 lenght;
	//
	TlaSI32 _buffSize;
	TlaSI32 _buffGrowth;
} STTLAString;

void TLAString_init(struct STTLAString* string, const TlaUI32 sizeInitial, const TlaUI32 minimunIncrease);
void TLAString_release(struct STTLAString* string);

void TLAString_empty(struct STTLAString* string);
void TLAString_set(struct STTLAString* string, const char* value);
void TLAString_increaseBuffer(struct STTLAString* string, const TlaSI32 additionalMinimunReq);
void TLAString_concat(struct STTLAString* string, const char* stringToAdd);
void TLAString_concatByte(struct STTLAString* string, const char charAdd);
void TLAString_concatBytes(struct STTLAString* string, const char* stringToAdd, const TlaSI32 qntBytesConcat);
void TLAString_concatUI32(struct STTLAString* string, TlaUI32 number);
void TLAString_concatUI64(struct STTLAString* string, TlaUI64 number);
void TLAString_concatSI32(struct STTLAString* string, TlaSI32 number);
void TLAString_concatSI64(struct STTLAString* string, TlaSI64 number);
void TLAString_concatFloat(struct STTLAString* string, float number);
void TLAString_concatDouble(struct STTLAString* string, double number);
void TLAString_removeLastByte(struct STTLAString* string);
void TLAString_removeLastBytes(struct STTLAString* string, const TlaSI32 numBytes);
//
TlaSI32 TLAString_lenghtBytes(const char* string);
TlaSI32 TLAString_indexOf(const char* haystack, const char* needle, TlaSI32 posInitial);
TlaUI32 TLAString_stringsAreEquals(const char* string1, const char* string2);
TlaUI32 TLAString_stringIsLowerOrEqualTo(const char* stringCompare, const char* stringBase);
//
TlaUI8 TLAString_isInteger(const char* string);
TlaUI8 TLAString_isIntegerBytes(const char* string, const TlaSI32 bytesCount);
TlaUI8 TLAString_isDecimal(const char* string);
//
TlaSI32 TLAString_toTlaSI32(const char* string);
TlaSI32 TLAString_toTlaSI32Bytes(const char* string, const TlaSI32 bytesCount);
TlaSI64 TLAString_toSI64(const char* string);
TlaUI32 TLAString_toTlaUI32(const char* string);
TlaUI64 TLAString_toUI64(const char* string);
float TLAString_toFloat(const char* string);
double TLAString_toDouble(const char* string);
//
TlaSI32 TLAString_toTlaSI32IfValid(const char* string, const TlaSI32 valueDefault);
TlaSI64 TLAString_toSI64IfValid(const char* string, const TlaSI64 valueDefault);
TlaUI32 TLAString_toTlaUI32IfValid(const char* string, const TlaUI32 valueDefault);
TlaUI64 TLAString_toUI64IfValid(const char* string, const TlaUI64 valueDefault);
float TLAString_toFloatIfValid(const char* string, const float valueDefault);
double TLAString_toDoubleIfValid(const char* string, const double valueDefault);
TlaSI32 TLAString_toTlaSI32FromHexIfValid(const char* string, const TlaSI32 valueDefault);
TlaSI32 TLAString_toTlaSI32FromHexLenIfValid(const char* string, const TlaSI32 strLen, const TlaSI32 valueDefault);
//File
TlaBOOL TLAString_writeToFile(const STTLAString* obj, FILE* file, const TlaBOOL includeUnusedBuffer);
TlaBOOL TLAString_initFromFile(STTLAString* obj, FILE* file);



//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++
//++ Private Header
//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++

//------------------
//-- String index.
//------------------
//As an optimization, strings are stored into large blocks
//of multiple null-terminated-string instead of individual
//data blocks. This structure is a pointer to the start of one string.
typedef struct STTlaStrIndx_ {
	TlaSI32 iStringLib;		//Index of string on array.
	TlaSI32 iFisrtChar;		//Index of first char on string.
} STTlaStrIndx;

//------------------
//-- Tag's data (pair name/value).
//------------------
typedef struct STTlaTagPriv_ {
	STTlaStrIndx	idxName;
	STTlaStrIndx	idxValue;
} STTlaTagPriv;

void __osmSTTlaTagPriv_init(STTlaTagPriv* obj){
	obj->idxName.iStringLib = 0;
	obj->idxName.iFisrtChar = 0;
	obj->idxValue.iStringLib = 0;
	obj->idxValue.iFisrtChar = 0;
}

//------------------
//-- Node's data (private structure).
//------------------
typedef struct STTlaNodePriv_ {
	TlaSI64		id;			// "id" param.
	TlaDOUBLE	lat;		// "lat" param.
	TlaDOUBLE	lon;		// "lon" param.
	TlaBOOL		tagsInited;	// Tag arrays is inited? (not inited for nodes without tags values)
	STTLAArray	tags;		// Arrays of "STTlaStrIndx" (pairs name/value).
} STTlaNodePriv;

void __osmSTTlaNodePriv_init(STTlaNodePriv* obj){
	obj->id = 0;
	obj->lat = 0;
	obj->lon = 0;
	obj->tagsInited = TLA_FALSE;
	obj->tags.use = 0; //just in case
}

void __osmSTTlaNodePriv_release(STTlaNodePriv* obj){
	//Release tags
	if(obj->tagsInited){
		TLAArray_release(&obj->tags);
		obj->tagsInited = TLA_FALSE; //just in case
		obj->tags.use = 0; //just in case
	}
}

//------------------
//-- Way's data (private structure).
//------------------
typedef struct STTlaWayPriv_ {
	TlaSI64		id;			// "id" param.
	STTLAArray	nodes;		// Arrays of "SI64" (ids of nodes).
	TlaBOOL		tagsInited;	// Tag arrays is inited? (not inited for ways without tags values)
	STTLAArray	tags;		// Arrays of "STTlaStrIndx" (pairs name/value).
} STTlaWayPriv;

void __osmSTTlaWayPriv_init(STTlaWayPriv* obj){
	obj->id = 0;
	TLAArray_init(&obj->nodes, sizeof(TlaSI64), 0, 32);
	obj->tagsInited = TLA_FALSE;
	obj->tags.use = 0; //just in case
}

void __osmSTTlaWayPriv_release(STTlaWayPriv* obj){
	TLAArray_release(&obj->nodes);
	//Release tags
	if(obj->tagsInited){
		TLAArray_release(&obj->tags);
		obj->tagsInited = TLA_FALSE; //just in case
		obj->tags.use = 0; //just in case
	}
}

//------------------
//-- Relation member's data (private structure).
//------------------
typedef struct STTlaRelMemberPriv_ {
	STTlaStrIndx	type;
	//
	TlaSI64			ref;
	STTlaStrIndx	role;
} STTlaRelMemberPriv;

void __osmSTTlaRelMemberPriv_init(STTlaRelMemberPriv* obj){
	obj->type.iStringLib = 0;
	obj->type.iFisrtChar = 0;
	//
	obj->ref	= 0;
	obj->role.iStringLib = 0;
	obj->role.iFisrtChar = 0;
}

/*
TODO: Implement when there's somethign to release.
void __osmSTTlaRelMemberPriv_release(STTlaRelMemberPriv* obj){
	//
}
*/

//------------------
//-- Relation's data (private structure).
//------------------
typedef struct STTlaRelPriv_ {
	TlaSI64		id;			// "id" param.
	STTLAArray	members;	// Arrays of "STTlaRelMemberPriv" (member).
	TlaBOOL		tagsInited;	// Tag arrays is inited? (not inited for ways without tags values)
	STTLAArray	tags;		// Arrays of "STTlaStrIndx" (pairs name/value).
} STTlaRelPriv;

void __osmSTTlaRelPriv_init(STTlaRelPriv* obj){
	obj->id = 0;
	TLAArray_init(&obj->members, sizeof(STTlaRelMemberPriv), 0, 32);
	obj->tagsInited = TLA_FALSE;
	obj->tags.use = 0; //just in case
}

void __osmSTTlaRelPriv_release(STTlaRelPriv* obj){
	//Release members
	{
		/*
		TODO: Implement when '__osmSTTlaRelMemberPriv_release' is implemented.
		TlaSI32 i; const TlaSI32 count = obj->members.use;
		for(i = 0; i < count; i++){
			STTlaRelMemberPriv* member = (STTlaRelMemberPriv*)TLAArray_itemAtIndex(&obj->members, i);
			__osmSTTlaRelMemberPriv_release(member);
		}
		*/
		TLAArray_release(&obj->members);
	}
	//Release tags
	if(obj->tagsInited){
		TLAArray_release(&obj->tags);
		obj->tagsInited = TLA_FALSE; //just in case
		obj->tags.use = 0; //just in case
	}
}

// Search index
typedef struct STTlaIdxById_ {
	TlaSI64	id;		//Item id
	TlaSI32	index;	//Position index at array
} STTlaIdxById;

//------------------
//-- OSM main data structure (private structure).
//------------------
typedef struct STTlaDataPriv_ {
	STTLAString			version;		//Value of "osm:version" param.
	STTLAString			generator;		//Value of "osm:generator" param.
	STTLAString			note;			//Value of "osm/note" node.
	//
	TlaSI32				stringsLibMaxSz;//Maximun size per string library
	STTLAArray			stringsLibs;	//Array with strings libraries contaning all strings values
	STTLAArray			nodes;			//Array of "STTlaNodePriv"
	STTLAArray			ways;			//Array of "STTlaWayPriv"
	STTLAArray			relations;		//Array of "STTlaRelPriv"
	//Search indexes
	STTLAArraySorted	idxNodesById;	//Array of "STTlaIdxById"
	STTLAArraySorted	idxWaysById;	//Array of "STTlaIdxById"
	STTLAArraySorted	idxRelsById;	//Array of "STTlaIdxById"
} STTlaDataPriv;

void __osmSTTlaDataPriv_init(STTlaDataPriv* obj){
	TLAString_init(&obj->version, 64, 128);
	TLAString_init(&obj->generator, 64, 128);
	TLAString_init(&obj->note, 64, 128);
	//
	obj->stringsLibMaxSz = (1024 * 256);
	TLAArray_init(&obj->stringsLibs, sizeof(STTLAString), 1, 8);
	TLAArray_init(&obj->nodes, sizeof(STTlaNodePriv), 128, 512);
	TLAArray_init(&obj->ways, sizeof(STTlaWayPriv), 128, 512);
	TLAArray_init(&obj->relations, sizeof(STTlaRelPriv), 128, 512);
	//Sorted indexes
	{
		const STTlaIdxById anIdx;
		TLAArraySorted_init(&obj->idxNodesById, sizeof(STTlaIdxById), sizeof(anIdx.id), 128, 512);
		TLAArraySorted_init(&obj->idxWaysById, sizeof(STTlaIdxById), sizeof(anIdx.id), 128, 512);
		TLAArraySorted_init(&obj->idxRelsById, sizeof(STTlaIdxById), sizeof(anIdx.id), 128, 512);
	}
	//Add first strig to library
	{
		STTLAString strLib;
		TLAString_init(&strLib, (1024 * 128), 1024);
		TLAArray_add(&obj->stringsLibs, &strLib, sizeof(strLib));
	}
}

void __osmSTTlaDataPriv_empty(STTlaDataPriv* obj){
	//Empty indexes
	TLAArray_empty(&obj->idxNodesById._array);
	TLAArray_empty(&obj->idxWaysById._array);
	TLAArray_empty(&obj->idxRelsById._array);
	//Empty relations
	{
		TlaSI32 i; const TlaSI32 count = obj->relations.use;
		for(i = 0; i < count; i++){
			STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&obj->relations, i);
			__osmSTTlaRelPriv_release(itm);
		}
		TLAArray_empty(&obj->relations);
	}
	//Empty ways
	{
		TlaSI32 i; const TlaSI32 count = obj->ways.use;
		for(i = 0; i < count; i++){
			STTlaWayPriv* itm = (STTlaWayPriv*)TLAArray_itemAtIndex(&obj->ways, i);
			__osmSTTlaWayPriv_release(itm);
		}
		TLAArray_empty(&obj->ways);
	}
	//Empty nodes
	{
		TlaSI32 i; const TlaSI32 count = obj->nodes.use;
		for(i = 0; i < count; i++){
			STTlaNodePriv* itm = (STTlaNodePriv*)TLAArray_itemAtIndex(&obj->nodes, i);
			__osmSTTlaNodePriv_release(itm);
		}
		TLAArray_empty(&obj->nodes);
	}
	//Empty strings libs
	{
		TlaSI32 i;
		for(i = (obj->stringsLibs.use - 1); i >= 0; i--){
			STTLAString* itm = (STTLAString*)TLAArray_itemAtIndex(&obj->stringsLibs, i);
			if(i == 0){
				TLAString_empty(itm);
			} else {
				TLAString_release(itm);
				TLAArray_removeItemAtIndex(&obj->stringsLibs, i);
			}
		}
	}
	TLAString_empty(&obj->note);
	TLAString_empty(&obj->generator);
	TLAString_empty(&obj->version);
}

void __osmSTTlaDataPriv_release(STTlaDataPriv* obj){
	//empty data
	__osmSTTlaDataPriv_empty(obj);
	//Release indexes
	TLA_ASSERT(obj->idxNodesById._array.use == 0)
	TLAArraySorted_release(&obj->idxNodesById);
	TLA_ASSERT(obj->idxWaysById._array.use == 0)
	TLAArraySorted_release(&obj->idxWaysById);
	TLA_ASSERT(obj->idxRelsById._array.use == 0)
	TLAArraySorted_release(&obj->idxRelsById);
	//Release relations
	TLA_ASSERT(obj->relations.use == 0)
	TLAArray_release(&obj->relations);
	//Release ways
	TLA_ASSERT(obj->ways.use == 0)
	TLAArray_release(&obj->ways);
	//Release nodes
	TLA_ASSERT(obj->nodes.use == 0)
	TLAArray_release(&obj->nodes);
	//Release strings libs
	{
		TlaSI32 i; const TlaSI32 count = obj->stringsLibs.use;
		for(i = 0; i < count; i++){
			STTLAString* itm = (STTLAString*)TLAArray_itemAtIndex(&obj->stringsLibs, i);
			TLAString_release(itm);
		}
		TLAArray_release(&obj->stringsLibs);
	}
	//
	TLAString_release(&obj->note);
	TLAString_release(&obj->generator);
	TLAString_release(&obj->version);
}

// Core-strings are strings that are very common on the structure content, such as tag's names.
// All the core-strings are stored on the first stringLib.
STTlaStrIndx __osmSTTlaDataPriv_indexForCoreString(STTlaDataPriv* obj, const char* newStr){
	STTlaStrIndx r; STTLAString* firstLib = NULL;
	r.iStringLib = 0;
	r.iFisrtChar = -1;
	TLA_ASSERT(obj->stringsLibs.use > 0)
	firstLib = (STTLAString*)TLAArray_itemAtIndex(&obj->stringsLibs, 0);
	//Search for current string index
	{
		TlaSI32 i = 0; const TlaSI32 count = firstLib->lenght; const char* strBuff = firstLib->str;
		const TlaSI32 newStrLen = TLAString_lenghtBytes(newStr);
		while(i < count){
			const TlaSI32 iStartOfStr = i;
			//Look for this substring end
			while(strBuff[i] != '\0'){ i++; }
			//Compare strings
			if(newStrLen == (i - iStartOfStr)){
				if(TLAString_stringsAreEquals(&strBuff[iStartOfStr], newStr)){
					r.iFisrtChar = iStartOfStr;
					break;
				}
			}
			i++;
		}
	}
	//Add new string (if necesary)
	if(r.iFisrtChar == -1){
		r.iFisrtChar = (firstLib->lenght + 1);
		TLAString_concatByte(firstLib, '\0');
		TLAString_concat(firstLib, newStr);
	}
	TLA_ASSERT(r.iFisrtChar >= 0)
	return r;
}

STTlaStrIndx __osmSTTlaDataPriv_indexForHeapString(STTlaDataPriv* obj, const char* string){
	TlaSI32 iStrLib = -1;
	const TlaSI32 newStrLen = TLAString_lenghtBytes(string);
	STTlaStrIndx r;
	r.iStringLib = 0;
	r.iFisrtChar = 0;
	TLA_ASSERT(obj->stringsLibs.use > 0)
	//Look for a heap-string-lib with available space
	{
		TlaSI32 i;
		for(i = (obj->stringsLibs.use - 1); i >= 1; i--){
			STTLAString* strLib = (STTLAString*)TLAArray_itemAtIndex(&obj->stringsLibs, i);
			if((strLib->lenght + 1 + newStrLen) < obj->stringsLibMaxSz){
				iStrLib = i;
				break;
			}
		}
	}
	//Create next heap-string-lib (if necesary)
	if(iStrLib == -1){
		STTLAString strLib;
		TLAString_init(&strLib, obj->stringsLibMaxSz, 1024);
		TLAArray_add(&obj->stringsLibs, &strLib, sizeof(strLib));
		iStrLib = (obj->stringsLibs.use - 1);
	}
	if(newStrLen == 0){
		//Optimization, quick-add zero-lenght-strings
		r.iStringLib = iStrLib;
		r.iFisrtChar = 0;
	} else {
		//Add string to heap-string-lib
		STTLAString* strLib = (STTLAString*)TLAArray_itemAtIndex(&obj->stringsLibs, iStrLib);
		TLA_ASSERT((strLib->lenght + 1 + TLAString_lenghtBytes(string)) < obj->stringsLibMaxSz)
		r.iStringLib = iStrLib;
		r.iFisrtChar = (strLib->lenght + 1);
		TLAString_concatByte(strLib, '\0');
		TLAString_concat(strLib, string);
	}
	TLA_ASSERT(r.iStringLib >= 0)
	TLA_ASSERT(r.iFisrtChar >= 0)
	return r;
}

const char* __osmSTTlaDataPriv_stringAtIndex(STTlaDataPriv* obj, const STTlaStrIndx index){
	const char* r = NULL;
	if(index.iStringLib >= 0 && index.iStringLib < obj->stringsLibs.use){
		const STTLAString* strLib = (const STTLAString*)TLAArray_itemAtIndex(&obj->stringsLibs, index.iStringLib);
		if(index.iFisrtChar >= 0 && index.iFisrtChar < strLib->lenght){
			r = &strLib->str[index.iFisrtChar];
		}
	}
	return r;
}

//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++
//++ Code
//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++

//Factory

void osmInit(STTlaOsm* pObj){
	//Init opaque data
	STTlaDataPriv* obj; TLA_MALLOC(obj, STTlaDataPriv, sizeof(STTlaDataPriv));
	pObj->opaqueData = obj;
	__osmSTTlaDataPriv_init(obj);
}

void osmRelease(STTlaOsm* pObj){
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	__osmSTTlaDataPriv_release(obj);
	pObj->opaqueData = NULL;
}

//Read (secuential)

TlaBOOL osmGetNextNode(STTlaOsm* pObj, STTlaNode* dst, const STTlaNode* afterThis){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	TlaSI32 iR = 0; if(afterThis != NULL){ iR = (afterThis->opaquePtr + 1); }
	if(iR >=0 && iR < obj->nodes.use){
		STTlaNodePriv* data = (STTlaNodePriv*)TLAArray_itemAtIndex(&obj->nodes, iR);
		dst->id			= data->id;
		dst->lat		= data->lat;
		dst->lon		= data->lon;
		dst->opaquePtr	= iR;
		r = TLA_TRUE;
	}
	return r;
}

TlaBOOL osmGetNextNodeTag(STTlaOsm* pObj, const STTlaNode* node, STTlaTag* dst, const STTlaTag* afterThis){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	if(node->opaquePtr >= 0 && node->opaquePtr < obj->nodes.use){
		STTlaNodePriv* itm = (STTlaNodePriv*)TLAArray_itemAtIndex(&obj->nodes, node->opaquePtr);
		if(itm->tagsInited){
			TlaSI32 iR = 0; if(afterThis != NULL){ iR = (afterThis->opaquePtr + 1); }
			if(iR >=0 && iR < itm->tags.use){
				STTlaTagPriv* data = (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, iR);
				dst->name		= __osmSTTlaDataPriv_stringAtIndex(obj, data->idxName);
				dst->value		= __osmSTTlaDataPriv_stringAtIndex(obj, data->idxValue);
				dst->opaquePtr	= iR;
				r = TLA_TRUE;
			}
		}
	}
	return r;
}

TlaBOOL osmGetNextWay(STTlaOsm* pObj, STTlaWay* dst, const STTlaWay* afterThis){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	TlaSI32 iR = 0; if(afterThis != NULL){ iR = (afterThis->opaquePtr + 1); }
	if(iR >=0 && iR < obj->ways.use){
		STTlaWayPriv* data = (STTlaWayPriv*)TLAArray_itemAtIndex(&obj->ways, iR);
		dst->id			= data->id;
		dst->opaquePtr	= iR;
		r = TLA_TRUE;
	}
	return r;
}

TlaBOOL osmGetWayNodes(STTlaOsm* pObj, const STTlaWay* way, const TlaSI64** dstArr, TlaSI32* dstSize){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	if(way->opaquePtr >= 0 && way->opaquePtr < obj->ways.use){
		STTlaWayPriv* itm = (STTlaWayPriv*)TLAArray_itemAtIndex(&obj->ways, way->opaquePtr);
		*dstArr		= (const TlaSI64*)itm->nodes._buffData; TLA_ASSERT(itm->nodes._bytesPerItem == sizeof(TlaSI64))
		*dstSize	= itm->nodes.use;
		r = TLA_TRUE;
	}
	return r;
}


TlaBOOL osmGetNextWayTag(STTlaOsm* pObj, const STTlaWay* way, STTlaTag* dst, const STTlaTag* afterThis){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	if(way->opaquePtr >= 0 && way->opaquePtr < obj->ways.use){
		STTlaWayPriv* itm = (STTlaWayPriv*)TLAArray_itemAtIndex(&obj->ways, way->opaquePtr);
		if(itm->tagsInited){
			TlaSI32 iR = 0; if(afterThis != NULL){ iR = (afterThis->opaquePtr + 1); }
			if(iR >=0 && iR < itm->tags.use){
				STTlaTagPriv* data = (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, iR);
				dst->name		= __osmSTTlaDataPriv_stringAtIndex(obj, data->idxName);
				dst->value		= __osmSTTlaDataPriv_stringAtIndex(obj, data->idxValue);
				dst->opaquePtr	= iR;
				r = TLA_TRUE;
			}
		}
	}
	return r;
}

TlaBOOL osmGetNextRel(STTlaOsm* pObj, STTlaRel* dst, const STTlaRel* afterThis){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	TlaSI32 iR = 0; if(afterThis != NULL){ iR = (afterThis->opaquePtr + 1); }
	if(iR >=0 && iR < obj->relations.use){
		STTlaRelPriv* data = (STTlaRelPriv*)TLAArray_itemAtIndex(&obj->relations, iR);
		dst->id			= data->id;
		dst->opaquePtr	= iR;
		r = TLA_TRUE;
	}
	return r;
}

TlaBOOL osmGetNextRelMember(STTlaOsm* pObj, const STTlaRel* rel, STTlaRelMember* dst, const STTlaRelMember* afterThis){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	if(rel->opaquePtr >= 0 && rel->opaquePtr < obj->relations.use){
		STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&obj->relations, rel->opaquePtr);
		if(itm->tagsInited){
			TlaSI32 iR = 0; if(afterThis != NULL){ iR = (afterThis->opaquePtr + 1); }
			if(iR >=0 && iR < itm->members.use){
				STTlaRelMemberPriv* data = (STTlaRelMemberPriv*)TLAArray_itemAtIndex(&itm->members, iR);
				dst->type		= __osmSTTlaDataPriv_stringAtIndex(obj, data->type);
				dst->ref		= data->ref;
				dst->role		= __osmSTTlaDataPriv_stringAtIndex(obj, data->role);
				dst->opaquePtr	= iR;
				r = TLA_TRUE;
			}
		}
	}
	return r;
}

TlaBOOL osmGetNextRelTag(STTlaOsm* pObj, const STTlaRel* rel, STTlaTag* dst, const STTlaTag* afterThis){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	if(rel->opaquePtr >= 0 && rel->opaquePtr < obj->relations.use){
		STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&obj->relations, rel->opaquePtr);
		if(itm->tagsInited){
			TlaSI32 iR = 0; if(afterThis != NULL){ iR = (afterThis->opaquePtr + 1); }
			if(iR >=0 && iR < itm->tags.use){
				STTlaTagPriv* data = (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, iR);
				dst->name		= __osmSTTlaDataPriv_stringAtIndex(obj, data->idxName);
				dst->value		= __osmSTTlaDataPriv_stringAtIndex(obj, data->idxValue);
				dst->opaquePtr	= iR;
				r = TLA_TRUE;
			}
		}
	}
	return r;
}

//Read (by id)

TlaBOOL osmGetNodeById(STTlaOsm* pObj, STTlaNode* dst, const TlaSI64 refId){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	//Optimized search (sorted index)
	TlaSI32 iFound; STTlaIdxById idxSrch;
	idxSrch.id	= refId;
	iFound		= TLAArraySorted_indexOf(&obj->idxNodesById, &idxSrch, NULL);
	if(iFound != -1){
		const STTlaIdxById* idx = (STTlaIdxById*)TLAArray_itemAtIndex(&obj->idxNodesById._array, iFound);
		const STTlaNodePriv* data = (const STTlaNodePriv*)TLAArray_itemAtIndex(&obj->nodes, idx->index);
		TLA_ASSERT(data->id == refId)
		dst->id			= data->id;
		dst->lat		= data->lat;
		dst->lon		= data->lon;
		dst->opaquePtr	= idx->index;
		r = TLA_TRUE;
	}
	/*
	//Search without optimization
	TlaSI32 i; const TlaSI32 count = obj->nodes.use; STTlaNodePriv* arr = (STTlaNodePriv*) obj->nodes._buffData;
	for(i = 0; i < count; i ++){
		STTlaNodePriv* data = &arr[i];
		if(data->id == refId){
			dst->id			= data->id;
			dst->lat		= data->lat;
			dst->lon		= data->lon;
			dst->opaquePtr	= i;
			r = TLA_TRUE;
			break;
		}
	}
	*/
	return r;
}

TlaBOOL osmGetWayById(STTlaOsm* pObj, STTlaWay* dst, const TlaSI64 refId){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	//Optimized search (sorted index)
	TlaSI32 iFound; STTlaIdxById idxSrch;
	idxSrch.id	= refId;
	iFound		= TLAArraySorted_indexOf(&obj->idxWaysById, &idxSrch, NULL);
	if(iFound != -1){
		const STTlaIdxById* idx = (STTlaIdxById*)TLAArray_itemAtIndex(&obj->idxWaysById._array, iFound);
		const STTlaWayPriv* data = (const STTlaWayPriv*)TLAArray_itemAtIndex(&obj->ways, idx->index);
		TLA_ASSERT(data->id == refId)
		dst->id			= data->id;
		dst->opaquePtr	= idx->index;
		r = TLA_TRUE;
	}
	/*
	//Search without optimization
	TlaSI32 i; const TlaSI32 count = obj->ways.use; STTlaWayPriv* arr = (STTlaWayPriv*) obj->ways._buffData;
	for(i = 0; i < count; i ++){
		STTlaWayPriv* data = &arr[i];
		if(data->id == refId){
			dst->id			= data->id;
			dst->opaquePtr	= i;
			r = TLA_TRUE;
			break;
		}
	}
	*/
	return r;
}

TlaBOOL osmGetRelById(STTlaOsm* pObj, STTlaRel* dst, const TlaSI64 refId){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	//Optimized search (sorted index)
	TlaSI32 iFound; STTlaIdxById idxSrch;
	idxSrch.id	= refId;
	iFound		= TLAArraySorted_indexOf(&obj->idxWaysById, &idxSrch, NULL);
	if(iFound != -1){
		const STTlaIdxById* idx = (STTlaIdxById*)TLAArray_itemAtIndex(&obj->idxWaysById._array, iFound);
		const STTlaRelPriv* data = (const STTlaRelPriv*)TLAArray_itemAtIndex(&obj->relations, idx->index);
		TLA_ASSERT(data->id == refId)
		dst->id			= data->id;
		dst->opaquePtr	= idx->index;
		r = TLA_TRUE;
	}
	/*
	//Search without optimization
	TlaSI32 i; const TlaSI32 count = obj->relations.use; STTlaRelPriv* arr = (STTlaRelPriv*) obj->relations._buffData;
	for(i = 0; i < count; i ++){
		STTlaRelPriv* data = &arr[i];
		if(data->id == refId){
			dst->id			= data->id;
			dst->opaquePtr	= i;
			r = TLA_TRUE;
			break;
		}
	}
	*/
	return r;
}

//------------------
//-- OSM parsing state
//------------------

typedef enum ENMainNodeType_ {
	ENMainNodeType_None = 0,	//xml-root (no node opened yet)
	ENMainNodeType_Osm,			//<osm>
	ENMainNodeType_OsmNote,		//<osm/note>
	ENMainNodeType_OsmNode,		//<osm/node>
	ENMainNodeType_OsmNodeTag,	//<osm/node/tag>
	ENMainNodeType_OsmWay,		//<osm/way>
	ENMainNodeType_OsmWayNd,	//<osm/way/nd>
	ENMainNodeType_OsmWayTag,	//<osm/way/tag>
	ENMainNodeType_OsmRel,		//<osm/relation>
	ENMainNodeType_OsmRelMember,//<osm/relation/member>
	ENMainNodeType_OsmRelTag,	//<osm/relation/tag>
	ENMainNodeType_Count
} ENMainNodeType;

typedef struct STParsingState_ {
	STTlaDataPriv*	osm;
	//
	ENMainNodeType	curMainNodeType;	//Wich type of node are we reading?
} STParsingState;

void __osmSTParsingState_init(STParsingState* obj, STTlaDataPriv* osm){
	obj->osm = osm;
	obj->curMainNodeType = ENMainNodeType_None;
}

void __osmSTParsingState_release(STParsingState* obj){
	obj->osm = NULL; //Just in case
	obj->curMainNodeType = ENMainNodeType_None; //Just in case
}

//------------------
//-- OSM parsing code - General
//------------------

TlaBOOL __osmConsumeNodeOpening(STParsingState* state, const char* strNodeName){
	TlaBOOL r = TLA_TRUE;
	switch (state->curMainNodeType) {
		case ENMainNodeType_None:
			if(TLAString_stringsAreEquals(strNodeName, "osm")){
				state->curMainNodeType = ENMainNodeType_Osm;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-OPENING: /'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_Osm:
			if(TLAString_stringsAreEquals(strNodeName, "note")){
				state->curMainNodeType = ENMainNodeType_OsmNote;
			} else if(TLAString_stringsAreEquals(strNodeName, "node")){
				//Add node
				STTlaNodePriv itm; __osmSTTlaNodePriv_init(&itm);
				TLAArray_add(&state->osm->nodes, &itm, sizeof(itm));
				state->curMainNodeType = ENMainNodeType_OsmNode;
			} else if(TLAString_stringsAreEquals(strNodeName, "way")){
				//Add way
				STTlaWayPriv itm; __osmSTTlaWayPriv_init(&itm);
				TLAArray_add(&state->osm->ways, &itm, sizeof(itm));
				state->curMainNodeType = ENMainNodeType_OsmWay;
			} else if(TLAString_stringsAreEquals(strNodeName, "relation")){
				//Add relation
				STTlaRelPriv itm; __osmSTTlaRelPriv_init(&itm);
				TLAArray_add(&state->osm->relations, &itm, sizeof(itm));
				state->curMainNodeType = ENMainNodeType_OsmRel;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-OPENING: /osm/'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmNote:
			//TLA_PRINTF_WARNING("Ignoring node-OPENING: /note/'%s'.\n", strNodeName);
			break;
		case ENMainNodeType_OsmNode:
			if(TLAString_stringsAreEquals(strNodeName, "tag")){
				//Add node/tag
				STTlaNodePriv* itm = (STTlaNodePriv*)TLAArray_itemAtIndex(&state->osm->nodes, (state->osm->nodes.use - 1));
				STTlaTagPriv tag; __osmSTTlaTagPriv_init(&tag);
				if(!itm->tagsInited){
					TLAArray_init(&itm->tags, sizeof(STTlaTagPriv), 8, 32);
					itm->tagsInited = TLA_TRUE;
				}
				TLAArray_add(&itm->tags, &tag, sizeof(tag));
				state->curMainNodeType = ENMainNodeType_OsmNodeTag;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-OPENING: /node/'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmNodeTag:
			//TLA_PRINTF_WARNING("Ignoring node-OPENING: /node/tag/'%s'.\n", strNodeName);
			break;
		case ENMainNodeType_OsmWay:
			if(TLAString_stringsAreEquals(strNodeName, "nd")){
				state->curMainNodeType = ENMainNodeType_OsmWayNd;
			} else if(TLAString_stringsAreEquals(strNodeName, "tag")){
				//Add way/tag
				STTlaWayPriv* itm = (STTlaWayPriv*)TLAArray_itemAtIndex(&state->osm->ways, (state->osm->ways.use - 1));
				STTlaTagPriv tag; __osmSTTlaTagPriv_init(&tag);
				if(!itm->tagsInited){
					TLAArray_init(&itm->tags, sizeof(STTlaTagPriv), 8, 32);
					itm->tagsInited = TLA_TRUE;
				}
				TLAArray_add(&itm->tags, &tag, sizeof(tag));
				state->curMainNodeType = ENMainNodeType_OsmWayTag;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-OPENING: /way/'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmWayNd:
			//TLA_PRINTF_WARNING("Ignoring node-OPENING: /way/nd/'%s'.\n", strNodeName);
			break;
		case ENMainNodeType_OsmWayTag:
			//TLA_PRINTF_WARNING("Ignoring node-OPENING: /way/tag/'%s'.\n", strNodeName);
			break;
		case ENMainNodeType_OsmRel:
			if(TLAString_stringsAreEquals(strNodeName, "member")){
				//Add relation/member
				STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&state->osm->relations, (state->osm->relations.use - 1));
				STTlaRelMemberPriv mem; __osmSTTlaRelMemberPriv_init(&mem);
				TLAArray_add(&itm->members, &mem, sizeof(mem));
				state->curMainNodeType = ENMainNodeType_OsmRelMember;
			} else if(TLAString_stringsAreEquals(strNodeName, "tag")){
				//Add relation/tag
				STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&state->osm->relations, (state->osm->relations.use - 1));
				STTlaTagPriv tag; __osmSTTlaTagPriv_init(&tag);
				if(!itm->tagsInited){
					TLAArray_init(&itm->tags, sizeof(STTlaTagPriv), 8, 32);
					itm->tagsInited = TLA_TRUE;
				}
				TLAArray_add(&itm->tags, &tag, sizeof(tag));
				state->curMainNodeType = ENMainNodeType_OsmRelTag;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-OPENING: /osm/relation/'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmRelMember:
			//TLA_PRINTF_WARNING("Ignoring node-OPENING: /osm/relation/member/'%s'.\n", strNodeName);
			break;
		case ENMainNodeType_OsmRelTag:
			//TLA_PRINTF_WARNING("Ignoring node-OPENING: /osm/relation/tag/'%s'.\n", strNodeName);
			break;
		default:
			TLA_PRINTF_WARNING("Ignoring node-OPENING: /.../'%s'.\n", strNodeName);
			break;
	}
	return r;
}

TlaBOOL __osmConsumeNodeClosing(STParsingState* state, const char* strNodeName){
	TlaBOOL r = TLA_TRUE;
	switch (state->curMainNodeType) {
		case ENMainNodeType_Osm:
			if(TLAString_stringsAreEquals(strNodeName, "osm")){
				state->curMainNodeType = ENMainNodeType_None;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmNote:
			if(TLAString_stringsAreEquals(strNodeName, "note")){
				state->curMainNodeType = ENMainNodeType_Osm;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/note/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmNode:
			if(TLAString_stringsAreEquals(strNodeName, "node")){
				state->curMainNodeType = ENMainNodeType_Osm;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/node/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmNodeTag:
			if(TLAString_stringsAreEquals(strNodeName, "tag")){
				state->curMainNodeType = ENMainNodeType_OsmNode;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/node/tag/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmWay:
			if(TLAString_stringsAreEquals(strNodeName, "way")){
				state->curMainNodeType = ENMainNodeType_Osm;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/way/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmWayNd:
			if(TLAString_stringsAreEquals(strNodeName, "nd")){
				state->curMainNodeType = ENMainNodeType_OsmWay;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/way/nd/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmWayTag:
			if(TLAString_stringsAreEquals(strNodeName, "tag")){
				state->curMainNodeType = ENMainNodeType_OsmWay;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/way/tag/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmRel:
			if(TLAString_stringsAreEquals(strNodeName, "relation")){
				state->curMainNodeType = ENMainNodeType_Osm;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/relation/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmRelMember:
			if(TLAString_stringsAreEquals(strNodeName, "member")){
				state->curMainNodeType = ENMainNodeType_OsmRel;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/relation/member/../'%s'.\n", strNodeName);
			}
			break;
		case ENMainNodeType_OsmRelTag:
			if(TLAString_stringsAreEquals(strNodeName, "tag")){
				state->curMainNodeType = ENMainNodeType_OsmRel;
			} else {
				//TLA_PRINTF_WARNING("Ignoring node-CLOSING: /osm/relation/tag/../'%s'.\n", strNodeName);
			}
			break;
		default:
			TLA_PRINTF_WARNING("Ignoring node-CLOSING: '%s'.\n", strNodeName);
			break;
	}
	return r;
}

TlaBOOL __osmConsumeParam(STParsingState* state, const char* paramName, const char* paramValue){
	TlaBOOL r = TLA_TRUE;
	switch (state->curMainNodeType) {
		case ENMainNodeType_Osm:
			if(TLAString_stringsAreEquals(paramName, "version")){
				TLAString_set(&state->osm->version, paramValue);
			} else if(TLAString_stringsAreEquals(paramName, "generator")){
				TLAString_set(&state->osm->generator, paramValue);
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		case ENMainNodeType_OsmNote:
			//TLA_PRINTF_WARNING("Ignoring param: /osm/note/ '%s' = '%s'.\n", paramName, paramValue);
			break;
		case ENMainNodeType_OsmNode:
			if(TLAString_stringsAreEquals(paramName, "id")){
				//Set osm/node:id
				if(TLAString_isInteger(paramValue)){
					STTlaNodePriv* itm = (STTlaNodePriv*)TLAArray_itemAtIndex(&state->osm->nodes, (state->osm->nodes.use - 1)); TLA_ASSERT(itm != NULL)
					itm->id = TLAString_toSI64(paramValue);
				} else { r = TLA_FALSE; TLA_ASSERT(0); }
			} else if(TLAString_stringsAreEquals(paramName, "lat")){
				//Set osm/node:lat
				if(TLAString_isDecimal(paramValue)){
					STTlaNodePriv* itm = (STTlaNodePriv*)TLAArray_itemAtIndex(&state->osm->nodes, (state->osm->nodes.use - 1)); TLA_ASSERT(itm != NULL)
					itm->lat = TLAString_toDouble(paramValue);
				} else { r = TLA_FALSE; TLA_ASSERT(0); }
			} else if(TLAString_stringsAreEquals(paramName, "lon")){
				//Set osm/node:lon
				if(TLAString_isDecimal(paramValue)){
					STTlaNodePriv* itm = (STTlaNodePriv*)TLAArray_itemAtIndex(&state->osm->nodes, (state->osm->nodes.use - 1)); TLA_ASSERT(itm != NULL)
					itm->lon = TLAString_toDouble(paramValue);
				} else { r = TLA_FALSE; TLA_ASSERT(0); }
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/node/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		case ENMainNodeType_OsmNodeTag:
			if(TLAString_stringsAreEquals(paramName, "k")){
				//Set osm/node/tag:k
				STTlaNodePriv* itm	= (STTlaNodePriv*)TLAArray_itemAtIndex(&state->osm->nodes, (state->osm->nodes.use - 1));
				STTlaTagPriv* tag	= (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, (itm->tags.use - 1));
				tag->idxName	= __osmSTTlaDataPriv_indexForCoreString(state->osm, paramValue);
			} else if(TLAString_stringsAreEquals(paramName, "v")){
				//Set osm/node/tag:v
				STTlaNodePriv* itm	= (STTlaNodePriv*)TLAArray_itemAtIndex(&state->osm->nodes, (state->osm->nodes.use - 1));
				STTlaTagPriv* tag	= (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, (itm->tags.use - 1));
				tag->idxValue	= __osmSTTlaDataPriv_indexForHeapString(state->osm, paramValue);
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/node/tag/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		case ENMainNodeType_OsmWay:
			if(TLAString_stringsAreEquals(paramName, "id")){
				//Set osm/way:id
				if(TLAString_isInteger(paramValue)){
					STTlaWayPriv* itm = (STTlaWayPriv*)TLAArray_itemAtIndex(&state->osm->ways, (state->osm->ways.use - 1)); TLA_ASSERT(itm != NULL)
					itm->id = TLAString_toSI64(paramValue);
				} else { r = TLA_FALSE; TLA_ASSERT(0); }
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/way/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		case ENMainNodeType_OsmWayNd:
			if(TLAString_stringsAreEquals(paramName, "ref")){
				//Set osm/way/nd:ref
				if(TLAString_isInteger(paramValue)){
					const TlaSI64 ref = TLAString_toSI64(paramValue);
					STTlaWayPriv* itm = (STTlaWayPriv*)TLAArray_itemAtIndex(&state->osm->ways, (state->osm->ways.use - 1)); TLA_ASSERT(itm != NULL)
					TLAArray_add(&itm->nodes, &ref, sizeof(ref));
				} else { r = TLA_FALSE; TLA_ASSERT(0); }
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/way/nd/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		case ENMainNodeType_OsmWayTag:
			if(TLAString_stringsAreEquals(paramName, "k")){
				//Set osm/way/tag:k
				STTlaWayPriv* itm	= (STTlaWayPriv*)TLAArray_itemAtIndex(&state->osm->ways, (state->osm->ways.use - 1));
				STTlaTagPriv* tag	= (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, (itm->tags.use - 1));
				tag->idxName	= __osmSTTlaDataPriv_indexForCoreString(state->osm, paramValue);
			} else if(TLAString_stringsAreEquals(paramName, "v")){
				//Set osm/way/tag:v
				STTlaWayPriv* itm	= (STTlaWayPriv*)TLAArray_itemAtIndex(&state->osm->ways, (state->osm->ways.use - 1));
				STTlaTagPriv* tag	= (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, (itm->tags.use - 1));
				tag->idxValue	= __osmSTTlaDataPriv_indexForHeapString(state->osm, paramValue);
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/way/tag/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		case ENMainNodeType_OsmRel:
			if(TLAString_stringsAreEquals(paramName, "id")){
				//Set osm/relation:id
				if(TLAString_isInteger(paramValue)){
					STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&state->osm->relations, (state->osm->relations.use - 1));
					itm->id = TLAString_toSI64(paramValue);
				} else { r = TLA_FALSE; TLA_ASSERT(0); }
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/relation/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		case ENMainNodeType_OsmRelMember:
			if(TLAString_stringsAreEquals(paramName, "type")){
				//Set osm/relation/member:type
				STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&state->osm->relations, (state->osm->relations.use - 1));
				STTlaRelMemberPriv* mem = (STTlaRelMemberPriv*)TLAArray_itemAtIndex(&itm->members, (itm->members.use - 1));
				mem->type = __osmSTTlaDataPriv_indexForCoreString(state->osm, paramValue);
			} else if(TLAString_stringsAreEquals(paramName, "ref")){
				//Set osm/relation/member:ref
				if(TLAString_isInteger(paramValue)){
					STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&state->osm->relations, (state->osm->relations.use - 1));
					STTlaRelMemberPriv* mem = (STTlaRelMemberPriv*)TLAArray_itemAtIndex(&itm->members, (itm->members.use - 1));
					mem->ref = TLAString_toSI64(paramValue);
				} else { r = TLA_FALSE; TLA_ASSERT(0); }
			} else if(TLAString_stringsAreEquals(paramName, "role")){
				//Set osm/relation/member:role
				STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&state->osm->relations, (state->osm->relations.use - 1));
				STTlaRelMemberPriv* mem = (STTlaRelMemberPriv*)TLAArray_itemAtIndex(&itm->members, (itm->members.use - 1));
				mem->role = __osmSTTlaDataPriv_indexForCoreString(state->osm, paramValue);
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/relation/member/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		case ENMainNodeType_OsmRelTag:
			if(TLAString_stringsAreEquals(paramName, "k")){
				//Set osm/relation/tag:k
				STTlaRelPriv* itm	= (STTlaRelPriv*)TLAArray_itemAtIndex(&state->osm->relations, (state->osm->relations.use - 1));
				STTlaTagPriv* tag	= (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, (itm->tags.use - 1)); TLA_ASSERT(tag != NULL)
				tag->idxName	= __osmSTTlaDataPriv_indexForCoreString(state->osm, paramValue);
			} else if(TLAString_stringsAreEquals(paramName, "v")){
				//Set osm/relation/tag:v
				STTlaRelPriv* itm	= (STTlaRelPriv*)TLAArray_itemAtIndex(&state->osm->relations, (state->osm->relations.use - 1));
				STTlaTagPriv* tag	= (STTlaTagPriv*)TLAArray_itemAtIndex(&itm->tags, (itm->tags.use - 1)); TLA_ASSERT(tag != NULL)
				tag->idxValue	= __osmSTTlaDataPriv_indexForHeapString(state->osm, paramValue);
			} else {
				//TLA_PRINTF_WARNING("Ignoring param: /osm/relation/tag/ '%s' = '%s'.\n", paramName, paramValue);
			}
			break;
		default:
			TLA_PRINTF_WARNING("Ignoring param: '%s' = '%s'.\n", paramName, paramValue);
			break;
	}
	return r;
}

TlaBOOL __osmSTTlaDataPriv_rebuildIndexes(STTlaDataPriv* obj){
	TlaBOOL r = TLA_TRUE;
	//Fill nodes indexes
	TLAArray_empty(&obj->idxNodesById._array);
	{
		TlaSI32 i; const TlaSI32 count = obj->nodes.use;
		STTlaNodePriv* arr = (STTlaNodePriv*)obj->nodes._buffData;
		for(i = 0; i < count; i++){
			STTlaNodePriv* itm = &arr[i];
			STTlaIdxById idx;
			idx.id		= itm->id;
			idx.index	= i;
			TLAArraySorted_add(&obj->idxNodesById, &idx, sizeof(idx));
		}
	}
	//Fill ways indexes
	TLAArray_empty(&obj->idxWaysById._array);
	{
		TlaSI32 i; const TlaSI32 count = obj->ways.use;
		STTlaWayPriv* arr = (STTlaWayPriv*)obj->ways._buffData;
		for(i = 0; i < count; i++){
			STTlaWayPriv* itm = &arr[i];
			STTlaIdxById idx;
			idx.id		= itm->id;
			idx.index	= i;
			TLAArraySorted_add(&obj->idxWaysById, &idx, sizeof(idx));
		}
	}
	//Fill relations indexes
	TLAArray_empty(&obj->idxRelsById._array);
	{
		TlaSI32 i; const TlaSI32 count = obj->relations.use;
		STTlaRelPriv* arr = (STTlaRelPriv*)obj->relations._buffData;
		for(i = 0; i < count; i++){
			STTlaRelPriv* itm = &arr[i];
			STTlaIdxById idx;
			idx.id		= itm->id;
			idx.index	= i;
			TLAArraySorted_add(&obj->idxRelsById, &idx, sizeof(idx));
		}
	}
	//
	return r;
}

//------------------
//-- OSM parsing code - XML specific
//------------------

TlaBOOL osmLoadFromFileXml(STTlaOsm* pObj, FILE* fileStream){
	TlaBOOL r = TLA_FALSE;
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	TlaBOOL xmlLogicError = TLA_FALSE;
	STTLAArray xmlTagsStack; STTLAString xmlTagName, paramName, wordAcumulated;
	TLAArray_init(&xmlTagsStack, sizeof(STTLAString), 16, 16);
	TLAString_init(&xmlTagName, 64, 64);
	TLAString_init(&paramName, 64, 64);
	TLAString_init(&wordAcumulated, 256, 256);
	{
		STParsingState osmParsingState;
		TlaBOOL xmlTagOpened		= TLA_FALSE;	//Are we parsing inside '<' and '>'?
		TlaBOOL xmlTagIsClosing		= TLA_FALSE;	//Current tag is closing this node?
		TlaBOOL paramValOpened		= TLA_FALSE;	//Currently waiting for param value?
		TlaBOOL tagNameDefined		= TLA_FALSE;	//Current opened tag has name defined?
		char curLiteralChar			= '\0';			//Current opened literal, can be '\"' or '\'' (quote or double quote)
		TlaSI32 countBytesLoaded	= 0;
		char readBuff[10240];
		//Init osm-parsing state
		__osmSTTlaDataPriv_empty(obj);
		__osmSTParsingState_init(&osmParsingState, obj);
		//Add root node
		{
			STTLAString strCopy;
			TLAString_init(&strCopy, TLAString_lenghtBytes("root") + 1, 1);
			TLAString_concat(&strCopy, "root");
			TLAArray_add(&xmlTagsStack, &strCopy, sizeof(strCopy));
		}
		//Parse content
		do {
			TlaSI32 iByteDone = 0;
			countBytesLoaded = (TlaSI32)fread(readBuff, sizeof(char), 10240, fileStream);
			while(iByteDone < countBytesLoaded && !xmlLogicError){
				//Identificar el siguiente caracter especial
				TlaSI32 iNextSpecialChar = iByteDone; //'<', ' ', '\t', '\r', '\n', '=', '/', '>'
				char curChar = readBuff[iNextSpecialChar];
				TlaBOOL nodeSeparatorFound = (curChar == '<' || curChar == '>') ? TLA_TRUE : TLA_FALSE;
				TlaBOOL wordSeparatorFound = (curLiteralChar == '\0' && (curChar == ' ' || curChar == '\t' || curChar == '\r' || curChar == '\n' || curChar == '=' || curChar == '/')) ? TLA_TRUE : TLA_FALSE;
				while(iNextSpecialChar < countBytesLoaded && !(nodeSeparatorFound || (xmlTagOpened && wordSeparatorFound))){
					if(xmlTagOpened){
						if(curChar == '\"' || curChar == '\''){
							if(curLiteralChar == '\0'){
								curLiteralChar		= curChar;	//Opening a new literal value (="something" or ='something')
								wordSeparatorFound	= TLA_TRUE;
								break;
							} else if(curLiteralChar == curChar){
								curLiteralChar		= '\0';		//Closing a literal value
								wordSeparatorFound	= TLA_TRUE;
								break;
							} else {
								TLAString_concatByte(&wordAcumulated, curChar);
							}
						} else {
							TLAString_concatByte(&wordAcumulated, curChar);
						}
					}
					//Next char
					iNextSpecialChar++;
					if(iNextSpecialChar < countBytesLoaded){
						curChar = readBuff[iNextSpecialChar];
						nodeSeparatorFound = (curChar == '<' || curChar == '>') ? TLA_TRUE : TLA_FALSE;
						wordSeparatorFound = (curLiteralChar == '\0' && (curChar == ' ' || curChar == '\t' || curChar == '\r' || curChar == '\n' || curChar == '=' || curChar == '/')) ? TLA_TRUE : TLA_FALSE;
					}
				}
				//Acumular el contenido
				//TLA_PRINTF_INFO("XML curChar('%c') word acumulated (%d len): '%s'.\n", curChar, wordAcumulated->tamano(), wordAcumulated->str());
				TLA_ASSERT(iNextSpecialChar <= countBytesLoaded)
				TLA_ASSERT(curChar == readBuff[iNextSpecialChar] || iNextSpecialChar == countBytesLoaded)
				{
					STTLAString* curNodeName	= (STTLAString*)TLAArray_itemAtIndex(&xmlTagsStack, xmlTagsStack.use - 1); TLA_ASSERT(curNodeName != NULL)
					if(!xmlTagOpened){
						//TODO
						/*if(curNodeName->nodosHijos == NULL && iByteDone < iNextSpecialChar){
							//Acumulate as node value
							if(curNodeName->indiceStrValor == 0){
								this->_strCompartidaValores.agregar((char)'\0');
								curNodeName->indiceStrValor = this->_strCompartidaValores.tamano();
							}
							this->_strCompartidaValores.agregar(&readBuff[iByteDone], iNextSpecialChar - iByteDone);
						}*/
					} else { //(xmlTagOpened == true)
						//Process word (nodeName, paramName or paramValue)
						if(wordSeparatorFound || nodeSeparatorFound){
							//TLA_PRINTF_INFO("XML, word acumulated (%d len): '%s'\n", wordAcumulated->tamano(), wordAcumulated->str());
							if(wordAcumulated.lenght != 0){
								if(tagNameDefined){
									//Define as param or value (style "<name param=value>")
									if(!paramValOpened){
										STTLAString strTmp;
										//process pendim param (ussually params without explicit values)
										if(paramName.lenght != 0){
											__osmConsumeParam(&osmParsingState, paramName.str, "");
											TLAString_empty(&paramName);
										}
										//optimization: swap strings instead of copy data
										strTmp = wordAcumulated;
										wordAcumulated = paramName;
										paramName = strTmp;
										//TLA_PRINTF_INFO("XML, param-name: '%s'.\n", paramName.str);
									} else {
										//TODO: acumulate param value
										__osmConsumeParam(&osmParsingState, paramName.str, wordAcumulated.str);
										TLAString_empty(&paramName);
										paramValOpened = TLA_FALSE;
										//TLA_PRINTF_INFO("XML, param-value: '%s'.\n", wordAcumulated.str);
									}
								} else { //(tagNameDefined == false)
									//Define as name (style "<name> or </name>")
									if(wordAcumulated.lenght != 1 || wordAcumulated.str[0] != '/'){ //Ignore the first word "/" from the closing nodes "</name>"
										//optimization: swap strings instead of copy data
										STTLAString strTmp = wordAcumulated;
										wordAcumulated = xmlTagName;
										xmlTagName = strTmp;
										tagNameDefined = TLA_TRUE;
										//TLA_PRINTF_INFO("XML, tag name: '%s'.\n", xmlTagName.str);
										//Open new tag
										if(!xmlTagIsClosing){
											//TODO: open a new tag
											//xmlTagName->str()
											STTLAString strCopy;
											TLAString_init(&strCopy, xmlTagName.lenght + 1, 1);
											TLAString_concatBytes(&strCopy, xmlTagName.str, xmlTagName.lenght);
											TLAArray_add(&xmlTagsStack, &strCopy, sizeof(strCopy));
											//
											__osmConsumeNodeOpening(&osmParsingState, xmlTagName.str);
										}
									}
								}
							}
							switch(curChar){
								case '/':
									xmlTagIsClosing = TLA_TRUE;
									break;
								case '=':
									paramValOpened = TLA_TRUE;
									break;
								default:
									break;
							}
						}
					}
				}
				//Determinar siguiente paso
				if(iNextSpecialChar < countBytesLoaded){ //to avoid processing the byte after the end
					TLA_ASSERT(!nodeSeparatorFound || (curChar == '<' || curChar == '>'))
					switch (curChar) {
						case '<':
						{
							TLA_ASSERT(paramName.lenght == 0)
							if(xmlTagOpened){
								xmlLogicError = TLA_TRUE; //Existe una apertura, sin cierre del nodo anterior
								TLA_PRINTF_ERROR("XML parse error: additional opening '<' for: '%s'.\n", xmlTagName.str);
								TLA_ASSERT(0)
							}
							xmlTagOpened	= TLA_TRUE;
							xmlTagIsClosing	= TLA_FALSE;
							paramValOpened	= TLA_FALSE;
							tagNameDefined	= TLA_FALSE;
						}
							break;
						case '>':
						{
							if(!xmlTagOpened){
								xmlLogicError = TLA_TRUE; //Existe un cierre, sin apertura del nodo anterior
								TLA_PRINTF_ERROR("XML parse error: additional close '>' after: '%s'.\n", xmlTagName.str);
								TLA_ASSERT(0)
							}
							//process pending param (ussually params without explicit values)
							if(paramName.lenght != 0){
								__osmConsumeParam(&osmParsingState, paramName.str, "");
								TLAString_empty(&paramName);
							}
							//
							if(!xmlTagIsClosing){
								//TLA_PRINTF_INFO("Etiqueta abierta: '%s'\n", xmlTagName->str());
							} else { //(xmlTagIsClosing == true)
								STTLAString* curNodeName = (STTLAString*)TLAArray_itemAtIndex(&xmlTagsStack, xmlTagsStack.use - 1); TLA_ASSERT(curNodeName != NULL)
								//TLA_PRINTF_INFO("Etiqueta cerrada: '%s' con contenido '%s'\n", xmlTagName->str(), &this->_strCompartidaValores.str()[curNodeName->indiceStrValor]);
								if(!TLAString_stringsAreEquals(curNodeName->str, xmlTagName.str)){
									xmlLogicError = TLA_TRUE;
									TLA_PRINTF_ERROR("XML parse error: closing '</%s>' does not match with opening '<%s>'.\n", xmlTagName.str, curNodeName->str);
									TLA_ASSERT(0)
									//readBuff[countBytesLoaded-1] = '\0';
									//TLA_PRINTF_INFO("readBuffXML:\n%s\n", readBuff);
								} else {
									//TLA_PRINTF_INFO("Cierra %d: %s\n", xmlTagsStack->conteo, xmlTagName->str());
									__osmConsumeNodeClosing(&osmParsingState, xmlTagName.str);
									TLAString_release(curNodeName);
									TLAArray_removeItemAtIndex(&xmlTagsStack, xmlTagsStack.use - 1);
								}
							}
							//
							TLAString_empty(&xmlTagName);
							xmlTagOpened = TLA_FALSE;
							//paramValOpened = TLA_FALSE; //Commented, unecesary to set
							//tagNameDefined = TLA_FALSE; //Commented, unecesary to set
						}
							break;
						default:
							break;
					}
				}
				//
				TLA_ASSERT(curChar == readBuff[iNextSpecialChar] || iNextSpecialChar == countBytesLoaded)
				iByteDone = iNextSpecialChar;
				if(nodeSeparatorFound || wordSeparatorFound){
					TLAString_empty(&wordAcumulated);
					iByteDone++; //Jump separator character
				}
			}
		} while(countBytesLoaded != 0 && !xmlLogicError);
		//
		if(!xmlLogicError && xmlTagsStack.use != 1){
			TLA_PRINTF_WARNING("XML parse error: %d tags remained open at the end.\n", (xmlTagsStack.use - 1));
			TLA_ASSERT(0)
		}
		//Release all allocated names
		{
			TlaSI32 i;
			for(i = 0; i < xmlTagsStack.use; i++){
				STTLAString* tagName = (STTLAString*)TLAArray_itemAtIndex(&xmlTagsStack, i);
				TLAString_release(tagName);
			}
			TLAArray_empty(&xmlTagsStack);
		}
		//Release osm-parsing state
		TLA_ASSERT(osmParsingState.curMainNodeType == ENMainNodeType_None)
		__osmSTParsingState_release(&osmParsingState);
	}
	TLA_ASSERT(xmlTagsStack.use == 0) //All names were released?
	TLAString_release(&wordAcumulated);
	TLAString_release(&paramName);
	TLAString_release(&xmlTagName);
	TLAArray_release(&xmlTagsStack);
	//
	r = (!xmlLogicError ? TLA_TRUE : TLA_FALSE);
	if(!r){
		//Release all allocated data
		__osmSTTlaDataPriv_empty(obj);
	} else {
		//Rebuild data optimization indexes
		__osmSTTlaDataPriv_rebuildIndexes(obj);
	}
	//
	return r;
};

TlaBOOL osmInitFromFileBinary(STTlaOsm* pObj, FILE* fileStream){
	TlaSI32 verfiStart, verifEnd;
	//Init opaque data
	STTlaDataPriv* obj; TLA_MALLOC(obj, STTlaDataPriv, sizeof(STTlaDataPriv));
	pObj->opaqueData = obj;
	//
	verfiStart = 0; fread(&verfiStart, sizeof(verfiStart), 1, fileStream);
	if(verfiStart != TLA_FILE_VERIF_START){
		return TLA_FALSE;
	}
	//
	if(!TLAString_initFromFile(&obj->version, fileStream)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAString_initFromFile(&obj->generator, fileStream)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAString_initFromFile(&obj->note, fileStream)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	//
	fread(&obj->stringsLibMaxSz, sizeof(obj->stringsLibMaxSz), 1, fileStream);
	//Strings libs
	{
		TlaSI32 i; TlaSI32 count;
		fread(&count, sizeof(count), 1, fileStream);
		TLAArray_init(&obj->stringsLibs, sizeof(STTLAString), count, 8);
		for(i = 0; i < count; i++){
			STTLAString itm;
			if(!TLAString_initFromFile(&itm, fileStream)){
				TLA_ASSERT(0); return TLA_FALSE;
			} else {
				TLAArray_add(&obj->stringsLibs, &itm, sizeof(itm));
			}
		}
	}
	//Nodes
	{
		TlaSI32 i; TlaSI32 count;
		fread(&count, sizeof(count), 1, fileStream);
		TLAArray_init(&obj->nodes, sizeof(STTlaNodePriv), count, 512);
		for(i = 0; i < count; i++){
			STTlaNodePriv itm;
			fread(&itm, sizeof(STTlaNodePriv), 1, fileStream);
			if(itm.tagsInited){
				if(!TLAArray_initFromFile(&itm.tags, fileStream, NULL)){
					TLA_ASSERT(0); return TLA_FALSE;
				}
			}
			TLAArray_add(&obj->nodes, &itm, sizeof(itm));
		}
	}
	//Ways
	{
		TlaSI32 i; TlaSI32 count;
		fread(&count, sizeof(count), 1, fileStream);
		TLAArray_init(&obj->ways, sizeof(STTlaWayPriv), count, 512);
		for(i = 0; i < count; i++){
			STTlaWayPriv itm;
			fread(&itm, sizeof(STTlaWayPriv), 1, fileStream);
			if(!TLAArray_initFromFile(&itm.nodes, fileStream, NULL)){
				TLA_ASSERT(0); return TLA_FALSE;
			}
			if(itm.tagsInited){
				if(!TLAArray_initFromFile(&itm.tags, fileStream, NULL)){
					TLA_ASSERT(0); return TLA_FALSE;
				}
			}
			TLAArray_add(&obj->ways, &itm, sizeof(itm));
		}
	}
	//Relations
	{
		TlaSI32 i; TlaSI32 count;
		fread(&count, sizeof(count), 1, fileStream);
		TLAArray_init(&obj->relations, sizeof(STTlaRelPriv), count, 512);
		for(i = 0; i < count; i++){
			STTlaRelPriv itm;
			fread(&itm, sizeof(STTlaRelPriv), 1, fileStream);
			if(!TLAArray_initFromFile(&itm.members, fileStream, NULL)){
				TLA_ASSERT(0); return TLA_FALSE;
			}
			if(itm.tagsInited){
				if(!TLAArray_initFromFile(&itm.tags, fileStream, NULL)){
					TLA_ASSERT(0); return TLA_FALSE;
				}
			}
			TLAArray_add(&obj->relations, &itm, sizeof(itm));
		}
	}
	//Indexes
	if(!TLAArraySorted_initFromFile(&obj->idxNodesById, fileStream, NULL)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAArraySorted_initFromFile(&obj->idxWaysById, fileStream, NULL)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAArraySorted_initFromFile(&obj->idxRelsById, fileStream, NULL)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	//
	verifEnd = 0; fread(&verifEnd, sizeof(verifEnd), 1, fileStream);
	if(verifEnd != TLA_FILE_VERIF_END){
		return TLA_FALSE;
	}
	return TLA_TRUE;
}

//Save

TlaBOOL osmSaveToFileAsBinary(STTlaOsm* pObj, FILE* fileStream){
	STTlaDataPriv* obj = (STTlaDataPriv*)pObj->opaqueData;
	const TlaSI32 verfiStart = TLA_FILE_VERIF_START, verifEnd = TLA_FILE_VERIF_END;
	//
	fwrite(&verfiStart, sizeof(verfiStart), 1, fileStream);
	//
	if(!TLAString_writeToFile(&obj->version, fileStream, TLA_FALSE)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAString_writeToFile(&obj->generator, fileStream, TLA_FALSE)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAString_writeToFile(&obj->note, fileStream, TLA_FALSE)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	//
	fwrite(&obj->stringsLibMaxSz, sizeof(obj->stringsLibMaxSz), 1, fileStream);
	//Strings libs
	{
		TlaSI32 i; const TlaSI32 count = obj->stringsLibs.use;
		fwrite(&count, sizeof(count), 1, fileStream);
		for(i = 0; i < count; i++){
			STTLAString* itm = (STTLAString*)TLAArray_itemAtIndex(&obj->stringsLibs, i);
			if(!TLAString_writeToFile(itm, fileStream, TLA_FALSE)){
				TLA_ASSERT(0); return TLA_FALSE;
			}
		}
	}
	//Nodes
	{
		TlaSI32 i; const TlaSI32 count = obj->nodes.use;
		fwrite(&count, sizeof(count), 1, fileStream);
		for(i = 0; i < count; i++){
			STTlaNodePriv* itm = (STTlaNodePriv*)TLAArray_itemAtIndex(&obj->nodes, i);
			fwrite(itm, sizeof(STTlaNodePriv), 1, fileStream);
			if(itm->tagsInited){
				if(!TLAArray_writeToFile(&itm->tags, fileStream, TLA_FALSE)){
					TLA_ASSERT(0); return TLA_FALSE;
				}
			}
		}
	}
	//Ways
	{
		TlaSI32 i; const TlaSI32 count = obj->ways.use;
		fwrite(&count, sizeof(count), 1, fileStream);
		for(i = 0; i < count; i++){
			STTlaWayPriv* itm = (STTlaWayPriv*)TLAArray_itemAtIndex(&obj->ways, i);
			fwrite(itm, sizeof(STTlaWayPriv), 1, fileStream);
			if(!TLAArray_writeToFile(&itm->nodes, fileStream, TLA_FALSE)){
				TLA_ASSERT(0); return TLA_FALSE;
			}
			if(itm->tagsInited){
				if(!TLAArray_writeToFile(&itm->tags, fileStream, TLA_FALSE)){
					TLA_ASSERT(0); return TLA_FALSE;
				}
			}
		}
	}
	//Relations
	{
		TlaSI32 i; const TlaSI32 count = obj->relations.use;
		fwrite(&count, sizeof(count), 1, fileStream);
		for(i = 0; i < count; i++){
			STTlaRelPriv* itm = (STTlaRelPriv*)TLAArray_itemAtIndex(&obj->relations, i);
			fwrite(itm, sizeof(STTlaRelPriv), 1, fileStream);
			if(!TLAArray_writeToFile(&itm->members, fileStream, TLA_FALSE)){
				TLA_ASSERT(0); return TLA_FALSE;
			}
			if(itm->tagsInited){
				if(!TLAArray_writeToFile(&itm->tags, fileStream, TLA_FALSE)){
					TLA_ASSERT(0); return TLA_FALSE;
				}
			}
		}
	}
	//Indexes
	if(!TLAArraySorted_writeToFile(&obj->idxNodesById, fileStream, TLA_FALSE)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAArraySorted_writeToFile(&obj->idxWaysById, fileStream, TLA_FALSE)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAArraySorted_writeToFile(&obj->idxRelsById, fileStream, TLA_FALSE)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	//
	fwrite(&verifEnd, sizeof(verifEnd), 1, fileStream);
	//
	/*typedef struct STTlaDataPriv_ {
		STTLAString			version;		//Value of "osm:version" param.
		STTLAString			generator;		//Value of "osm:generator" param.
		STTLAString			note;			//Value of "osm/note" node.
		//
		TlaSI32				stringsLibMaxSz;//Maximun size per string library
		STTLAArray			stringsLibs;	//Array with strings libraries contaning all strings values
		STTLAArray			nodes;			//Array of "STTlaNodePriv"
		STTLAArray			ways;			//Array of "STTlaWayPriv"
		STTLAArray			relations;		//Array of "STTlaRelPriv"
		//Search indexes
		STTLAArraySorted	idxNodesById;	//Array of "STTlaIdxById"
		STTLAArraySorted	idxWaysById;	//Array of "STTlaIdxById"
		STTLAArraySorted	idxRelsById;	//Array of "STTlaIdxById"
	} STTlaDataPriv;*/
	return TLA_TRUE;
}























































//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++
//++ HELPER CLASS - ARRAY
//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++

void TLAArray_init(STTLAArray* obj, const TlaSI32 bytesPerItem, const TlaSI32 initialSize, const TlaSI32 growthSize){
	TLA_ASSERT(bytesPerItem > 0)
	TLA_ASSERT(initialSize >= 0)
	TLA_ASSERT(growthSize > 0)
	obj->use			= 0;
	obj->_buffGrowth	= growthSize; if(obj->_buffGrowth <= 0) obj->_buffGrowth = 1;
	obj->_bytesPerItem	= bytesPerItem;
	if(initialSize > 0){
		obj->_buffSize	= initialSize;
		TLA_MALLOC(obj->_buffData, TlaBYTE, (obj->_buffSize * obj->_bytesPerItem));
	} else {
		obj->_buffSize		= 0;
		obj->_buffData		= NULL;
	}
}

void TLAArray_release(STTLAArray* obj){
#	ifdef DR_USE_MEMORY_LEAK_DETECTION
	TLAArray_dbgUnregisterById(obj);
#	endif
	if(obj->_buffData != NULL){
		TLA_FREE(obj->_buffData);
		obj->_buffData = NULL;
	}
	obj->use		= 0;
	obj->_buffSize	= 0;
	obj->_bytesPerItem	= 0;
}

//Search
void* TLAArray_itemAtIndex(const STTLAArray* obj, const TlaSI32 index){
	TLA_ASSERT(index >= 0 && index < obj->use)
	if(index >= 0 && index < obj->use){
		return &obj->_buffData[index * obj->_bytesPerItem];
	}
	return NULL;
}

void TLAArray_removeItemsAtIndex(STTLAArray* obj, const TlaSI32 index, const TlaSI32 itemsCount){
	TlaSI32 i, iCount;
	TLA_ASSERT(index >= 0 && (index + itemsCount) <= obj->use)
	//reordenar el arreglo
	obj->use -= itemsCount;
	iCount = obj->use;
	for(i = index; i < iCount; i++){
		memcpy(&obj->_buffData[i * obj->_bytesPerItem], &obj->_buffData[(i + itemsCount) * obj->_bytesPerItem], obj->_bytesPerItem);
	}
}

void TLAArray_removeItemAtIndex(STTLAArray* obj, const TlaSI32 index){
	TLAArray_removeItemsAtIndex(obj, index, 1);
}

//Redim
void TLAArray_empty(STTLAArray* obj){
	obj->use = 0;
}

void TLAArray_grow(STTLAArray* obj, const TlaSI32 quant){
	if(quant > 0){
		TlaBYTE* newArr = NULL;
		obj->_buffSize += quant;
		TLA_MALLOC(newArr, TlaBYTE, (obj->_buffSize * obj->_bytesPerItem));
		if(obj->_buffData != NULL){
			memcpy(newArr, obj->_buffData, (obj->use * obj->_bytesPerItem));
			TLA_FREE(obj->_buffData);
		}
		obj->_buffData = newArr;
	}
}

void* TLAArray_add(STTLAArray* obj, const void* data, const TlaSI32 itemSize){
	void* r = NULL;
	TLA_ASSERT(itemSize == obj->_bytesPerItem)
	//Grow, if necesary
	TLA_ASSERT(obj->use <= obj->_buffSize)
	if(obj->use == obj->_buffSize){
		TLAArray_grow(obj, obj->_buffGrowth);
	}
	r = &obj->_buffData[obj->use * obj->_bytesPerItem];
	if(data != NULL){
		memcpy(r, data, obj->_bytesPerItem);
	}
	obj->use++;
	return r;
}

void* TLAArray_addItems(STTLAArray* obj, const void* data, const TlaSI32 itemSize, const TlaSI32 itemsCount){
	void* r = NULL;
	TLA_ASSERT(itemSize == obj->_bytesPerItem)
	//Grow, if necesary
	if((obj->use + itemsCount) > obj->_buffSize){
		const TlaSI32 growth = (obj->use + itemsCount) - obj->_buffSize;
		TLAArray_grow(obj, (growth < obj->_buffGrowth ? obj->_buffGrowth : growth));
	}
	r = &obj->_buffData[obj->use * obj->_bytesPerItem];
	if(data != NULL){
		memcpy(r, data, obj->_bytesPerItem * itemsCount);
	}
	obj->use += itemsCount;
	return r;
}

void* TLAArray_set(STTLAArray* obj, const TlaSI32 index, const void* data, const TlaSI32 itemSize){
	void* r = NULL;
	TLA_ASSERT(itemSize == obj->_bytesPerItem)
	if(index >= 0 && index < obj->use){
		r = &obj->_buffData[index * obj->_bytesPerItem];
		memcpy(r, data, obj->_bytesPerItem);
	}
	return r;
}

//File
TlaBOOL TLAArray_writeToFile(const STTLAArray* obj, FILE* file, const TlaBOOL includeUnusedBuffer){
	const TlaSI32 verfiStart		= TLA_FILE_VERIF_START, verifEnd = TLA_FILE_VERIF_END;
	const TlaSI32 incUnusedBuffer	= (includeUnusedBuffer != TLA_FALSE ? 1 : 0);
	const TlaSI32 itemsToWrite		= (includeUnusedBuffer != TLA_FALSE ? obj->_buffSize : obj->use);
	if(obj == NULL || file == NULL){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	TLA_ASSERT(obj->use == 0 || obj->_buffData != NULL)
	if(fwrite(&verfiStart, sizeof(verfiStart), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(fwrite(&obj->use, sizeof(obj->use), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(fwrite(&incUnusedBuffer, sizeof(incUnusedBuffer), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(incUnusedBuffer){
		if(fwrite(&obj->_buffSize, sizeof(obj->_buffSize), 1, file) != 1){
			TLA_ASSERT(0); return TLA_FALSE;
		}
	}
	if(fwrite(&obj->_bytesPerItem, sizeof(obj->_bytesPerItem), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(fwrite(&obj->_buffGrowth, sizeof(obj->_buffGrowth), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(itemsToWrite > 0){
		if(fwrite(obj->_buffData, 1, (obj->_bytesPerItem * itemsToWrite), file) != (obj->_bytesPerItem * itemsToWrite)){
			TLA_ASSERT(0); return TLA_FALSE;
		}
	}
	if(fwrite(&verifEnd, sizeof(verifEnd), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	return TLA_TRUE;
}

TlaBOOL TLAArray_initFromFile(STTLAArray* obj, FILE* file, TlaBYTE* optExternalBuffer){
	TlaSI32 verfiStart, verifEnd; TlaSI32 incUnusedBuffer; TlaSI32 sizeToInit = 0;
	if(obj == NULL || file == NULL){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&verfiStart, sizeof(verfiStart), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	} else if(verfiStart != TLA_FILE_VERIF_START){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&obj->use, sizeof(obj->use), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&incUnusedBuffer, sizeof(incUnusedBuffer), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(incUnusedBuffer){
		if(fread(&obj->_buffSize, sizeof(obj->_buffSize), 1, file) != 1){
			TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
		}
		sizeToInit = obj->_buffSize;
	} else {
		sizeToInit = obj->use;
	}
	if(fread(&obj->_bytesPerItem, sizeof(obj->_bytesPerItem), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&obj->_buffGrowth, sizeof(obj->_buffGrowth), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(obj->use < 0 || obj->_bytesPerItem <= 0 || obj->_buffGrowth <= 0){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	obj->_buffSize = sizeToInit;
	if(optExternalBuffer != NULL){
		obj->_buffData = optExternalBuffer;
	} else {
		obj->_buffData = NULL;
		if(obj->_buffSize > 0){
			TLA_MALLOC(obj->_buffData, TlaBYTE, obj->_bytesPerItem * obj->_buffSize);
			if(obj->_buffData == NULL){
				TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
			} else {
				if(fread(obj->_buffData, 1, (obj->_bytesPerItem * obj->_buffSize), file) != (obj->_bytesPerItem * obj->_buffSize)){
					TLA_FREE(obj->_buffData);
					TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
				}
			}
		}
	}
	if(fread(&verifEnd, sizeof(verifEnd), 1, file) != 1){
		if(obj->_buffData != NULL){ TLA_FREE(obj->_buffData); obj->_buffData = NULL; }
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	} else if(verifEnd != TLA_FILE_VERIF_END){
		if(obj->_buffData != NULL){ TLA_FREE(obj->_buffData); obj->_buffData = NULL; }
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	return TLA_TRUE;
}


//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++
//++ HELPER CLASS - SORTED ARRAY
//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++

#define TLAArraySORTED_COMPARE_FUNC(BASE, COMPARE, SIZE) TLAArraySorted_bytesCompare(BASE, COMPARE, SIZE)

//Factory
void TLAArraySorted_init(STTLAArraySorted* obj, const TlaSI32 bytesPerItem, const TlaSI32 bytesComparePerItem, const TlaSI32 initialSize, const TlaSI32 growthSize){
	TLAArray_init(&obj->_array, bytesPerItem, initialSize, growthSize);
	obj->bytesToCompare = bytesComparePerItem;
	TLA_ASSERT(obj->bytesToCompare > 0)
}

void TLAArraySorted_release(STTLAArraySorted* obj){
	obj->bytesToCompare = 0;
	TLAArray_release(&obj->_array);
}

//Search
void* TLAArraySorted_itemAtIndex(const STTLAArraySorted* obj, const TlaSI32 index){
	return TLAArray_itemAtIndex(&obj->_array, index);
}

TlaSI32 TLAArraySorted_indexOf(const STTLAArraySorted* obj, const void* dataToSearch, const TlaSI32* optPosHint){
	if(obj->_array.use > 0){
		TlaSI32 posEnd		= (obj->_array.use - 1);
		if(optPosHint != NULL){
			//Optimization: search first on the position-hint.
			//This optimize the search when looking for items in order.
			if(*optPosHint >= 0 && *optPosHint < obj->_array.use){
				if(TLAArraySORTED_COMPARE_FUNC(dataToSearch, (TlaBYTE*)TLAArray_itemAtIndex(&obj->_array, *optPosHint), obj->bytesToCompare) == 0){
					return *optPosHint;
				}
			}
		} else {
			//Optimization: only search when the searched item is last item is lower  than the last item in the array.
			//This optimize the search for new items to add at the end (by example: when reading and ordered table).
			const TlaSI32 searchingVsLastItem = TLAArraySORTED_COMPARE_FUNC(dataToSearch, (TlaBYTE*)TLAArray_itemAtIndex(&obj->_array, posEnd), obj->bytesToCompare);
			if(searchingVsLastItem == 0){
				return posEnd;
			} else if(searchingVsLastItem > 0){ //Optimization: only search when the searched-item is last item is lower than the last-item in the array.
				return -1;
			}
		}
		//Binary Search
		{
			TlaSI32 posStart	= 0;
			TlaSI32 posMidd;
			const TlaBYTE* dataMidd = NULL;
			TLA_ASSERT(obj->bytesToCompare > 0)
			while(posStart <= posEnd){
				posMidd		= posStart + ((posEnd - posStart)/2);
				dataMidd	= (TlaBYTE*)TLAArray_itemAtIndex(&obj->_array, posMidd);
				if(TLAArraySORTED_COMPARE_FUNC(dataMidd, dataToSearch, obj->bytesToCompare) == 0){
					return posMidd;
				} else {
					if(TLAArraySORTED_COMPARE_FUNC(dataToSearch, dataMidd, obj->bytesToCompare) < 0){
						posEnd		= posMidd - 1;
					} else {
						posStart	= posMidd + 1;
					}
				}
			}
		}
	}
	return -1;
}

TlaSI32 TLAArraySorted_indexForNew(const STTLAArraySorted* obj, const TlaBYTE* dataToInsert){
	//Buscar nueva posicion (busqueda binaria)
	TlaSI32 index = 0;
	TLA_ASSERT(obj->bytesToCompare > 0)
	if(obj->_array.use > 0){
		TlaSI32 posStart			= 0;
		TlaSI32 posEnd				= (obj->_array.use - 1);
		do {
			if(TLAArraySORTED_COMPARE_FUNC((TlaBYTE*)TLAArray_itemAtIndex(&obj->_array, posEnd), dataToInsert, obj->bytesToCompare) <= 0){ //<=
				index			= posEnd + 1;
				break;
			} else if(TLAArraySORTED_COMPARE_FUNC((TlaBYTE*)TLAArray_itemAtIndex(&obj->_array, posStart), dataToInsert, obj->bytesToCompare) >= 0){ //>=
				index			= posStart;
				break;
			} else {
				const TlaUI32 posMidd = (posStart + posEnd) / 2;
				if(TLAArraySORTED_COMPARE_FUNC((TlaBYTE*)TLAArray_itemAtIndex(&obj->_array, posMidd), dataToInsert, obj->bytesToCompare) <= 0){
					posStart	= posMidd + 1;
				} else {
					posEnd		= posMidd;
				}
			}
		} while(1);
	}
	return index;
}

//Add
TlaSI32 TLAArraySorted_add(STTLAArraySorted* obj, const void* data, const TlaSI32 itemSize){
	const TlaSI32 dstIndex = TLAArraySorted_indexForNew(obj, (TlaBYTE*)data);
	TLA_ASSERT(dstIndex >= 0 && dstIndex <= obj->_array.use)
	TLA_ASSERT(itemSize == obj->_array._bytesPerItem)
	//PRINTF_INFO("Adding to index %d of %d.\n", dstIndex, obj->_array.use);
	//Make room for new item
	TLAArray_add(&obj->_array, NULL, obj->_array._bytesPerItem);
	//Make room for the new record
	if(dstIndex < (obj->_array.use - 1)){
		TlaSI32 i;
		for(i = (obj->_array.use - 2); i >= dstIndex; i--){
			memcpy(TLAArray_itemAtIndex(&obj->_array, i + 1), TLAArray_itemAtIndex(&obj->_array, i), obj->_array._bytesPerItem);
		}
	}
	TLAArray_set(&obj->_array, dstIndex, data, itemSize);
	/*#ifdef DR_CONFIG_INCLUDE_ASSERTS
	 TLAArraySorted_dbgValidate(obj);
	 #endif*/
	return dstIndex;
}

//Remove
void TLAArraySorted_removeItemAtIndex(STTLAArraySorted* obj, const TlaSI32 index){
	TLAArray_removeItemAtIndex(&obj->_array, index);
}

//Debug
/*
#ifdef DR_CONFIG_INCLUDE_ASSERTS
void TLAArraySorted_dbgValidate(STTLAArraySorted* obj){
	TLA_ASSERT(obj->bytesToCompare > 0)
	if(obj->_array.use> 1){
		TlaSI32 i;
		TlaBYTE* dataBefore = (TlaBYTE*)TLAArraySorted_itemAtIndex(obj, 0);
		for(i = 1; i < obj->_array.use; i++){
			TlaBYTE* data = (TlaBYTE*)TLAArraySorted_itemAtIndex(obj, i);
			TLA_ASSERT(TLAArraySORTED_COMPARE_FUNC(dataBefore, data, obj->bytesToCompare) <= 0)
			dataBefore = data;
		}
	}
}
#endif
*/

//File

TlaBOOL TLAArraySorted_writeToFile(const STTLAArraySorted* obj, FILE* file, const TlaBOOL includeUnusedBuffer){
	const TlaSI32 verfiStart = TLA_FILE_VERIF_START, verifEnd = TLA_FILE_VERIF_END;
	if(obj == NULL || file == NULL){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(fwrite(&verfiStart, sizeof(verfiStart), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(fwrite(&obj->bytesToCompare, sizeof(obj->bytesToCompare), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(!TLAArray_writeToFile(&obj->_array, file, includeUnusedBuffer)){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(fwrite(&verifEnd, sizeof(verifEnd), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	return TLA_TRUE;
}

TlaBOOL TLAArraySorted_initFromFile(STTLAArraySorted* obj, FILE* file, TlaBYTE* optExternalBuffer){
	TlaSI32 verfiStart, verifEnd;
	if(obj == NULL || file == NULL){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&verfiStart, sizeof(verfiStart), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	} else if(verfiStart != TLA_FILE_VERIF_START){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&obj->bytesToCompare, sizeof(obj->bytesToCompare), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(obj->bytesToCompare <= 0){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(!TLAArray_initFromFile(&obj->_array, file, optExternalBuffer)){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&verifEnd, sizeof(verifEnd), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	} else if(verifEnd != TLA_FILE_VERIF_END){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	return TLA_TRUE;
}

//Compare

TlaSI32 TLAArraySorted_bytesCompare(const void* base, const void* compare, const TlaSI32 sizeInBytes){
	TlaSI32 i, words32 = (sizeInBytes / 4); TlaSI32 bytes8 = (sizeInBytes % 4);
	const TlaUI32 *w32Base = (TlaUI32*)base, *w32Compare = (TlaUI32*)compare;
	const TlaBYTE *bytesBase = (TlaBYTE*)&w32Base[words32], *bytesCompare = (TlaBYTE*)&w32Compare[words32];
	for(i = 0; i < words32; i++){
		if(w32Base[i] < w32Compare[i]){
			return -1;
		} else if(w32Base[i] > w32Compare[i]){
			return 1;
		}
	}
	for(i = 0; i < bytes8; i++){
		if(bytesBase[i] < bytesCompare[i]){
			return -1;
		} else if(bytesBase[i] > bytesCompare[i]){
			return 1;
		}
	}
	return 0;
}

//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++
//++ HELPER CLASS - STRING
//+++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++

void TLAString_init(struct STTLAString* obj, const TlaUI32 sizeInitial, const TlaUI32 minimunIncrease){
	obj->_buffSize		= (sizeInitial < 1 ? 1 : sizeInitial);
	obj->_buffGrowth	= minimunIncrease;
	TLA_MALLOC(obj->str, char, (sizeof(char) * obj->_buffSize));
	//
	obj->lenght			= 0;
	obj->str[0]			= '\0';
}

void TLAString_release(struct STTLAString* obj){
#	ifdef DR_USE_MEMORY_LEAK_DETECTION
	TLAString_dbgUnregisterById(obj);
#	endif
	TLA_FREE(obj->str);
	obj->str		= NULL;
	obj->lenght	= 0;
	obj->_buffSize = 0;
	obj->_buffGrowth = 0;
}

void TLAString_empty(struct STTLAString* string){
	string->lenght		= 0;
	string->str[0]		= '\0';
}

void TLAString_set(struct STTLAString* string, const char* value){
	string->lenght		= 0;
	string->str[0]		= '\0';
	TLAString_concat(string, value);
}

TlaSI32 TLAString_lenghtBytes(const char* string){
	TlaSI32 r = 0; while(string[r]!='\0') r++;
	return r;
}

TlaSI32 TLAString_indexOf(const char* haystack, const char* needle, TlaSI32 posInitial){
	//validacion de cadena vacia
	if(needle[0]!='\0'){
		//busqueda de la aguja en el pajar
		int positionFound = -1;
		int posInHayStack = (posInitial<0?0:posInitial);
		int posInNeedle = 0;
		while(haystack[posInHayStack]!='\0'){
			if(haystack[posInHayStack]!=needle[posInNeedle]) posInNeedle = 0;
			if(haystack[posInHayStack]==needle[posInNeedle]){
				if(posInNeedle==0) positionFound = posInHayStack;
				posInNeedle++;
				if(needle[posInNeedle]=='\0') return positionFound;
			}
			posInHayStack++;
		}
	}
	return -1;
}

TlaUI32 TLAString_stringsAreEquals(const char* string1, const char* string2){
	TlaUI32 result = 1;
	if(string1 == NULL || string2 == NULL) return 0; //Validar nulos
	//TLA_PRINTF_INFO("Comparando cadenas: '%s' vs '%s'\n", cadena1, cadena2);
	//comparas caracteres
	//comparas caracteres
	while(result && (*string1) != 0 && (*string2) != 0){
		if((*string1) != (*string2)){
			result = 0;
		}
		string1++;	//mover puntero al siguiente caracter
		string2++;	//mover puntero al siguiente caracter
	}
	//validar si no se realizo la compracion de todos los caracteres
	//(una cadena es mas corta que la otra)
	if(result){
		result = ((*string1)==(*string2)); //la condicional es falso si uno es '\0' y el otro es diferente
	}
	return result;
}

TlaUI32 TLAString_stringIsLowerOrEqualTo(const char* string1 /*stringCompare*/, const char* string2/*stringBase*/){
	if(string1==NULL || string2==NULL) return 0; //Validar nulos
	if(*string1==0 || *string2==0) return 0; //Validar cadenas vacias
	//
	while((*string1)!=0 && (*string2)!=0){
		if((*string1)!=(*string2))return 0;
		string1++;	//mover puntero al siguiente caracter
		string2++;	//mover puntero al siguiente caracter
	}
	//
	return 1; //si la cadena a comparar no termina, entonces es diferente en algun punto
}

void TLAString_increaseBuffer(struct STTLAString* string, const TlaSI32 additionalMinimunReq){
	char* newBuffer;
	string->_buffSize += (string->_buffGrowth < additionalMinimunReq ? additionalMinimunReq : string->_buffGrowth);
	TLA_MALLOC(newBuffer, char, sizeof(char) * string->_buffSize);
	if(string->str != NULL){
		memcpy(newBuffer, string->str, string->lenght + 1);
		TLA_FREE(string->str);
	}
	string->str = newBuffer;
}

/*
#ifdef DR_STRING_USE_UTILS
void TLAString_concatDateTimeCurrent(struct STTLAString* string){
	TLAString_concatDateTime(string, time(NULL));
}
#endif
*/

/*
#ifdef DR_STRING_USE_UTILS
void TLAString_concatDateTimeCompactCurrent(struct STTLAString* string){
	TLAString_concatDateTimeCompact(string, time(NULL));
}
#endif
*/

/*
#ifdef DR_STRING_USE_UTILS
void TLAString_concatDateTime(struct STTLAString* string, const time_t dateTime){
	/ *struct tm{
 int     tm_sec;         / * Seconds: 0-59 (K&R says 0-61?) * /
 int     tm_min;         / * Minutes: 0-59 * /
 int     tm_hour;        / * Hours since midnight: 0-23 * /
 int     tm_mday;        / * Day of the month: 1-31 * /
 int     tm_mon;         / * Months *since* january: 0-11 * /
 int     tm_year;        / * Years since 1900 * /
 int     tm_wday;        / * Days since Sunday (0-6) * /
 int     tm_yday;        / * Days since Jan. 1: 0-365 * /
	}* /
	SYSTEMTIME st = *localtime(&dateTime);
	TLAString_concatSI32(string, 1900+st.tm_year);
	TLAString_concatByte(string, '-');
	if(st.tm_mon<9) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, st.tm_mon+1);
	TLAString_concatByte(string, '-');
	if(st.tm_mday<10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, st.tm_mday);
	TLAString_concatByte(string, ' ');
	if(st.tm_hour<10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, st.tm_hour);
	TLAString_concatByte(string, ':');
	if(st.tm_min<10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, st.tm_min);
	TLAString_concatByte(string, ':');
	if(st.tm_sec<10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, st.tm_sec);
}
#endif
*/

#ifdef DR_STRING_USE_UTILS
void TLAString_concatDateTimeCompact(struct STTLAString* string, const STDRDateTime dateTime){
	TLAString_concatSI32(string, dateTime.year);
	if(dateTime.month < 10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, dateTime.month);
	if(dateTime.day < 10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, dateTime.day);
	TLAString_concatByte(string, ' ');
	if(dateTime.hour<10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, dateTime.hour);
	if(dateTime.min<10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, dateTime.min);
	if(dateTime.sec<10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, dateTime.sec);
}
#endif

#ifdef DR_STRING_USE_UTILS
void TLAString_concatSQLTime(struct STTLAString* string, const STDRDateTime* time){
	TLAString_concatSI32(string, time->year);
	TLAString_concatByte(string, '-');
	if(time->month < 10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, time->month);
	TLAString_concatByte(string, '-');
	if(time->day < 10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, time->day);
	TLAString_concatByte(string, ' ');
	if(time->hour < 10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, time->hour);
	TLAString_concatByte(string, ':');
	if(time->min < 10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, time->min);
	TLAString_concatByte(string, ':');
	if(time->sec < 10) TLAString_concatByte(string, '0'); TLAString_concatSI32(string, time->sec);
	TLAString_concatByte(string, '.');
	TLAString_concatSI32(string, time->milisec);
}
#endif

void TLAString_concat(struct STTLAString* string, const char* stringToAdd){
	if(stringToAdd!=NULL){
		if(stringToAdd[0]!='\0'){
			TlaSI32 i, use, sizeConcat; char* buffer;
			//Asegurar el tamano del buffer
			sizeConcat = 0; while(stringToAdd[sizeConcat]!='\0') sizeConcat++;
			if((string->lenght + sizeConcat + 1) > (string->_buffSize)){
				TLAString_increaseBuffer(string, sizeConcat);
			}
			//
			TLA_ASSERT(string->lenght >= 0); //Por definicion una buffer debe tener por lo menos el caracter nulo
			use = string->lenght; buffer = string->str;
			for(i=0; i<sizeConcat; i++) buffer[use++] = stringToAdd[i];
			buffer[use++] = '\0';
			string->lenght = (use - 1);
		}
	}
}

void TLAString_concatByte(struct STTLAString* string, const char charAdd){
	//Asegurar el tamano del buffer
	if((string->lenght + 1 + 1) > string->_buffSize){
		TLAString_increaseBuffer(string, 1);
	}
	TLA_ASSERT(string->lenght >= 0); //Por definicion una buffer debe tener por lo menos el caracter nulo
	string->str[string->lenght++] = charAdd;
	string->str[string->lenght] = '\0';
}

void TLAString_removeLastByte(struct STTLAString* string) {
	TLA_ASSERT(string->lenght >= 0); //Por definicion una buffer debe tener por lo menos el caracter nulo
	if(string->lenght > 0){
		string->str[string->lenght - 1] = '\0';
		string->lenght--;
	}
}

void TLAString_removeLastBytes(struct STTLAString* string, const TlaSI32 numBytes){
	TLA_ASSERT(string->lenght >= 0); //Por definicion una buffer debe tener por lo menos el caracter nulo
	if(string->lenght >= numBytes){
		string->str[string->lenght - numBytes] = '\0';
		string->lenght -= numBytes;
	}
}

void TLAString_concatBytes(struct STTLAString* string, const char* stringToAdd, const TlaSI32 qntBytesConcat){
	TlaSI32 i, use; char* buffer;
	//Asegurar el tamano del buffer
	if((string->lenght + qntBytesConcat + 1) > (string->_buffSize)){
		TLAString_increaseBuffer(string, qntBytesConcat);
	}
	//
	TLA_ASSERT(string->lenght >= 0); //Por definicion una buffer debe tener por lo menos el caracter nulo
	use = string->lenght; buffer = string->str;
	for(i = 0; i < qntBytesConcat; i++){
		buffer[use++] = stringToAdd[i];
	}
	buffer[use++] = '\0';
	string->lenght = (use - 1);
}

#define CONCAT_INTEGER_U(TYPENUMERIC, numberToConvert, SIZEBUFFER) \
	/*buffer donde se almacenan los digitos (enteros y decimales por separado)*/ \
	TlaUI32 position; char dstIntegers[SIZEBUFFER]; \
	/*procesar parte entera*/ \
	dstIntegers[SIZEBUFFER-1]		= '\0'; \
	position						= SIZEBUFFER-1; \
	{ \
		TYPENUMERIC copyNumber	= (TYPENUMERIC)numberToConvert; \
		do { \
			TLA_ASSERT((copyNumber % (TYPENUMERIC)10) >= 0) \
			dstIntegers[--position] = (char)((TYPENUMERIC)'0' + (copyNumber % (TYPENUMERIC)10)); \
			copyNumber				/= (TYPENUMERIC)10; \
		} while(copyNumber!=0); \
	} \
	/*copiar resultado a destino*/ \
	TLAString_concat(string, &(dstIntegers[position]));

#define CONCAT_INTEGER(TYPENUMERIC, numberToConvert, SIZEBUFFER) \
	/*buffer donde se almacenan los digitos (enteros y decimales por separado)*/ \
	char sign, dstIntegers[SIZEBUFFER]; TlaUI32 position; TYPENUMERIC valueZero; \
	/*definir signo y asegurar positivo*/ \
	valueZero = 0; dstIntegers[SIZEBUFFER-1] = '\0'; \
	if(numberToConvert < valueZero){ \
		sign = '-'; numberToConvert = -numberToConvert;			/*convertir a positivo (hay un bug cuando es negativo, causado por el bit de signo cuya posicion no esta definida en cada sistema)*/ \
		/*Nota: cuando el valor es el limite negativo (pe: -2147483648 en 32 bits, este no se puede convertir a positivo, porque el maximo positivo es una unidad menor, ej: 2147483647 en 32 bits)*/ \
	} else { \
		sign = '+'; \
	} \
	TLA_ASSERT(numberToConvert >= valueZero) \
	/*procesar parte entera*/ \
	position						= SIZEBUFFER-1; \
	{ \
		TYPENUMERIC copyNumber	= (TYPENUMERIC)numberToConvert; \
		do { \
			TLA_ASSERT((copyNumber % (TYPENUMERIC)10) >= 0) \
			dstIntegers[--position] = (char)((TYPENUMERIC)'0' + (copyNumber % (TYPENUMERIC)10)); \
			copyNumber				/= (TYPENUMERIC)10; \
		} while(copyNumber!=0); \
		/*sign*/ \
		if(sign=='-') dstIntegers[--position] = sign; \
	} \
	/*copiar resultado a destino*/ \
	TLAString_concat(string, &(dstIntegers[position]));


#define CONCAT_DECIMAL(TYPENUMERIC, TYPEINTEGER, numberToConvert, SIZEBUFFER, posDecimals) \
	/*buffer donde se almacenan los digitos (enteros y decimales por separado)*/ \
	char sign, dstIntegers[SIZEBUFFER], dstDecimals[SIZEBUFFER]; TlaUI32 digitsAtRightNotZero, position; TYPENUMERIC valueZero; \
	/*definir signo y asegurar positivo*/ \
	valueZero = 0; dstIntegers[SIZEBUFFER-1] = dstDecimals[0] = '\0'; \
	if(numberToConvert<valueZero){ \
		sign = '-'; numberToConvert = -numberToConvert;			/*convertir a positivo (hay un bug cuando es negativo, causado por el bit de signo cuya posicion no esta definida en cada sistema)*/ \
	} else { \
		sign = '+'; \
	} \
	TLA_ASSERT(numberToConvert >= valueZero) \
	/*procesar parte decimal*/ \
	digitsAtRightNotZero	= 0; \
	if(posDecimals>0){ \
		TlaUI32 added				= 0; \
		TYPENUMERIC copyNumber	= numberToConvert; \
		while(added<posDecimals){ \
			copyNumber				= copyNumber - (TYPENUMERIC)((TYPEINTEGER)copyNumber);	/*Quitar la parte entera*/ \
			copyNumber				*= (TYPENUMERIC)10;										/*Mover el punto decimal a la derecha*/ \
			TLA_ASSERT((TYPEINTEGER)copyNumber >= 0) \
			dstDecimals[added++]	= (char)((TYPEINTEGER)'0' + (TYPEINTEGER)copyNumber);	/*Guardar digito (despues de posicion ASCII del cero)*/ \
			if((TYPEINTEGER)copyNumber!=0) digitsAtRightNotZero = added; \
		} \
		dstDecimals[digitsAtRightNotZero]		= '\0'; \
	} \
	/*procesar parte entera*/ \
	position						= SIZEBUFFER-1; \
	{ \
		TYPEINTEGER copyNumber		= (TYPEINTEGER)numberToConvert; \
		do { \
			TLA_ASSERT((copyNumber % (TYPEINTEGER)10) >= 0) \
			dstIntegers[--position] = (char)((TYPEINTEGER)'0' + (copyNumber % (TYPEINTEGER)10)); \
			copyNumber				/= (TYPEINTEGER)10; \
		} while(copyNumber!=0); \
		/*sign*/ \
		if(sign=='-') dstIntegers[--position] = sign; \
	} \
	/*copiar resultado a destino*/ \
	TLAString_concat(string, &(dstIntegers[position])); \
	if(digitsAtRightNotZero>0){ \
		TLAString_concat(string, "."); \
		TLAString_concat(string, dstDecimals); \
	}

void TLAString_concatUI32(struct STTLAString* string, TlaUI32 number){
	CONCAT_INTEGER_U(TlaUI32, number, 64)
}

void TLAString_concatUI64(struct STTLAString* string, TlaUI64 number){
	CONCAT_INTEGER_U(TlaUI64, number, 64)
}

void TLAString_concatSI32(struct STTLAString* string, TlaSI32 number){
	//2015-11-03, Marcos Ortega.
	//Nota: cuando el valor es "-2147483648" el sistema nolo puede convertir a positivo.
	//Por eso es mejor procesarlo en el rango siguiente de valores (SI64)
	TlaSI64 number64 = number;
	CONCAT_INTEGER(TlaSI64, number64, 64)
	
}

void TLAString_concatSI64(struct STTLAString* string, TlaSI64 number){
	CONCAT_INTEGER(TlaSI64, number, 64)
}

void TLAString_concatFloat(struct STTLAString* string, float number){
	CONCAT_DECIMAL(float, TlaSI64, number, 64, 5)
}

void TLAString_concatDouble(struct STTLAString* string, double number){
	CONCAT_DECIMAL(double, TlaSI64, number, 64, 5)
}

//

TlaUI8	TLAString_isInteger(const char* string){
	if((*string)=='+' || (*string)=='-') string++;
	while((*string)!='\0'){
		if((*string)<48 || (*string)>57) return 0; //ASCII 48='0' ... 57='9'
		string++;
	}
	return 1;
}

TlaUI8 TLAString_isIntegerBytes(const char* string, const TlaSI32 bytesCount){
	const char* afterLast = (string + bytesCount);
	if((*string)=='+' || (*string)=='-') string++;
	while(string < afterLast){
		if((*string)<48 || (*string)>57) return 0; //ASCII 48='0' ... 57='9'
		string++;
	}
	return 1;
}

TlaUI8	TLAString_isDecimal(const char* string){
	if((*string)=='+' || (*string)=='-') string++;
	while((*string)!='\0'){
		if(((*string)<48 || (*string)>57) && (*string)!='.') return 0; //ASCII 48='0' ... 57='9'
		string++;
	}
	return 1;
}

TlaSI32 TLAString_toTlaSI32(const char* string){
	TlaSI32 value = 0;
	const char sign = (*string); if(sign=='-' || sign=='+') string++;
	while((*string)!='\0'){
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	if(sign=='-') value = -value; //negativo
	return value;
}

TlaSI32 TLAString_toTlaSI32Bytes(const char* string, const TlaSI32 bytesCount){
	const char* afterLast = (string + bytesCount);
	TlaSI32 value = 0;
	const char sign = (*string); if(sign=='-' || sign=='+') string++;
	while(string < afterLast){
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	if(sign=='-') value = -value; //negativo
	return value;
}

TlaSI64 TLAString_toSI64(const char* string){
	TlaSI64 value = 0;
	const char sign = (*string); if(sign=='-' || sign=='+') string++;
	while((*string)!='\0'){
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	if(sign=='-') value = -value; //negativo
	return value;
}

TlaUI32 TLAString_toTlaUI32(const char* string){
	TlaUI32 value = 0;
	const char sign = (*string); if(sign=='-' || sign=='+') string++;
	while((*string)!='\0'){
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	return value;
}

TlaUI64 TLAString_toUI64(const char* string){
	TlaUI64 value = 0;
	const char sign = (*string);
	if(sign=='-' || sign=='+') string++;
	while((*string)!='\0'){
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	return value;
}

float TLAString_toFloat(const char* string){
	const char sign= (*string);
	float value	= 0;
	if(sign=='-' || sign=='+') string++;
	//Parte entera
	while((*string)!='\0' && (*string)!='.'){
		value = (value * 10.0f) + ((*string) - 48.0f); //ASCII 48='0' ... 57='9'
		string++;
	}
	//Parte decimal
	if((*string)=='.'){
		float factorDivisor = 10.0f;
		string++;
		while((*string)!='\0'){
			value += (((*string) - 48.0f) / factorDivisor);
			factorDivisor *= 10.0f;
			string++;
		}
	}
	//Signo
	if(sign=='-') value = -value;
	//
	return value;
}

double TLAString_toDouble(const char* string){
	const char sign= (*string);
	double value	= 0;
	if(sign=='-' || sign=='+') string++;
	//Parte entera
	while((*string)!='\0' && (*string)!='.'){
		value = (value * 10.0f) + ((*string) - 48.0f); //ASCII 48='0' ... 57='9'
		string++;
	}
	//Parte decimal
	if((*string)=='.'){
		float factorDivisor = 10.0f;
		string++;
		while((*string)!='\0'){
			value += (((*string) - 48.0f) / factorDivisor);
			factorDivisor *= 10.0f;
			string++;
		}
	}
	//Signo
	if(sign=='-') value = -value;
	//
	return value;
}

TlaSI32 TLAString_toTlaSI32FromHex(const char* string){
	TlaSI32 v = 0; char c;
	while((*string) != '\0'){
		c = (*string); string++;
		if(c > 47 && c < 58) v = (v * 16) + (c - 48); //ASCII 48='0' ... 57='9'
		else if(c > 64 && c < 71) v = (v * 16) + (10 + (c - 65)); //ASCII 65 = 'A' ... 70 = 'F'
		else if(c > 96 && c < 103) v = (v * 16) + (10 + (c - 97)); //ASCII 97 = 'a' ... 102 = 'f'
	}
	return v;
}

TlaSI32 TLAString_toTlaSI32FromHexLen(const char* string, const TlaSI32 strLen){
	TlaSI32 v = 0; TlaSI32 pos = 0; char c;
	while(pos < strLen){
		c = string[pos++];
		if(c > 47 && c < 58) v = (v * 16) + (c - 48); //ASCII 48='0' ... 57='9'
		else if(c > 64 && c < 71) v = (v * 16) + (10 + (c - 65)); //ASCII 65 = 'A' ... 70 = 'F'
		else if(c > 96 && c < 103) v = (v * 16) + (10 + (c - 97)); //ASCII 97 = 'a' ... 102 = 'f'
	}
	return v;
}

TlaSI32 TLAString_toTlaSI32IfValid(const char* string, const TlaSI32 valueDefault){
	TlaSI32 value = 0;
	const char sign = (*string);
	if((*string)=='\0') return valueDefault;
	if(sign=='-' || sign=='+') string++;
	while((*string)!='\0'){
		if((*string)<48 || (*string)>57) return valueDefault;
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	if(sign=='-') value = -value; //negativo
	return value;
}

TlaSI64 TLAString_toSI64IfValid(const char* string, const TlaSI64 valueDefault){
	TlaSI64 value = 0;
	const char sign = (*string);
	if((*string)=='\0') return valueDefault;
	if(sign=='-' || sign=='+') string++;
	while((*string)!='\0'){
		if((*string)<48 || (*string)>57) return valueDefault;
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	if(sign=='-') value = -value; //negativo
	return value;
}

TlaUI32 TLAString_toTlaUI32IfValid(const char* string, const TlaUI32 valueDefault){
	TlaUI32 value = 0; const char sign = (*string);
	if((*string)=='\0') return valueDefault;
	if(sign=='-' || sign=='+') string++;
	while((*string)!='\0'){
		if((*string)<48 || (*string)>57) return valueDefault;
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	return value;
}

TlaUI64 TLAString_toUI64IfValid(const char* string, const TlaUI64 valueDefault){
	TlaUI64 value = 0;
	const char sign = (*string);
	if((*string)=='\0') return valueDefault;
	if(sign=='-' || sign=='+') string++;
	while((*string)!='\0'){
		if((*string)<48 || (*string)>57) return valueDefault;
		value = (value * 10) + ((*string) - 48); //ASCII 48='0' ... 57='9'
		string++;
	}
	return value;
}

float TLAString_toFloatIfValid(const char* string, const float valueDefault){
	const char sign= (*string);
	float value	= 0;
	if((*string)=='\0') return valueDefault;
	if(sign=='-' || sign=='+') string++;
	//Parte entera
	while((*string)!='\0' && (*string)!='.'){
		if(((*string)<48 || (*string)>57) && (*string)!='.') return valueDefault;
		value = (value * 10.0f) + ((*string) - 48.0f); //ASCII 48='0' ... 57='9'
		string++;
	}
	//Parte decimal
	if((*string)=='.'){
		float factorDivisor = 10.0f;
		string++;
		while((*string)!='\0'){
			if(((*string)<48 || (*string)>57) && (*string)!='.') return valueDefault;
			value += (((*string) - 48.0f) / factorDivisor);
			factorDivisor *= 10.0f;
			string++;
		}
	}
	//Signo
	if(sign=='-') value = -value;
	//
	return value;
}

double TLAString_toDoubleIfValid(const char* string, const double valueDefault){
	const char sign= (*string);
	double value	= 0;
	if((*string)=='\0') return valueDefault;
	if(sign=='-' || sign=='+') string++;
	//Parte entera
	while((*string)!='\0' && (*string)!='.'){
		if(((*string)<48 || (*string)>57) && (*string)!='.') return valueDefault;
		value = (value * 10.0f) + ((*string) - 48.0f); //ASCII 48='0' ... 57='9'
		string++;
	}
	//Parte decimal
	if((*string)=='.'){
		float factorDivisor = 10.0f;
		string++;
		while((*string)!='\0'){
			if(((*string)<48 || (*string)>57) && (*string)!='.') return valueDefault;
			value += (((*string) - 48.0f) / factorDivisor);
			factorDivisor *= 10.0f;
			string++;
		}
	}
	//Signo
	if(sign=='-') value = -value;
	//
	return value;
}

TlaSI32 TLAString_toTlaSI32FromHexIfValid(const char* string, const TlaSI32 valueDefault){
	TlaSI32 v = 0; char c;
	if((*string)=='\0') return valueDefault;
	while((*string) != '\0'){
		c = (*string); string++;
		if(c > 47 && c < 58) v = (v * 16) + (c - 48); //ASCII 48='0' ... 57='9'
		else if(c > 64 && c < 71) v = (v * 16) + (10 + (c - 65)); //ASCII 65 = 'A' ... 70 = 'F'
		else if(c > 96 && c < 103) v = (v * 16) + (10 + (c - 97)); //ASCII 97 = 'a' ... 102 = 'f'
		else return valueDefault;
	}
	return v;
}

TlaSI32 TLAString_toTlaSI32FromHexLenIfValid(const char* string, const TlaSI32 strLen, const TlaSI32 valueDefault){
	TlaSI32 v = 0; TlaSI32 pos = 0; char c;
	if(string[0]=='\0') return valueDefault;
	while(pos < strLen){
		c = string[pos++];
		if(c > 47 && c < 58) v = (v * 16) + (c - 48); //ASCII 48='0' ... 57='9'
		else if(c > 64 && c < 71) v = (v * 16) + (10 + (c - 65)); //ASCII 65 = 'A' ... 70 = 'F'
		else if(c > 96 && c < 103) v = (v * 16) + (10 + (c - 97)); //ASCII 97 = 'a' ... 102 = 'f'
		else return valueDefault;
	}
	return v;
}

//File

TlaBOOL TLAString_writeToFile(const STTLAString* obj, FILE* file, const TlaBOOL includeUnusedBuffer){
	const TlaSI32 verfiStart		= TLA_FILE_VERIF_START, verifEnd = TLA_FILE_VERIF_END;
	const TlaSI32 incUnusedBuff		= (includeUnusedBuffer != TLA_FALSE ? 1 : 0);
	const TlaSI32 bytesToWrite		= (includeUnusedBuffer != TLA_FALSE ? obj->_buffSize : (obj->lenght + 1));
	if(obj == NULL || file == NULL){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	TLA_ASSERT(obj->lenght == 0 || obj->str != NULL)
	if(fwrite(&verfiStart, sizeof(verfiStart), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(fwrite(&obj->lenght, sizeof(obj->lenght), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(fwrite(&incUnusedBuff, sizeof(incUnusedBuff), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(incUnusedBuff){
		if(fwrite(&obj->_buffSize, sizeof(obj->_buffSize), 1, file) != 1){
			TLA_ASSERT(0); return TLA_FALSE;
		}
	}
	if(fwrite(&obj->_buffGrowth, sizeof(obj->_buffGrowth), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	if(bytesToWrite > 0){
		if(fwrite(obj->str, sizeof(TlaBYTE), bytesToWrite, file) != bytesToWrite){
			TLA_ASSERT(0); return TLA_FALSE;
		}
	}
	if(fwrite(&verifEnd, sizeof(verifEnd), 1, file) != 1){
		TLA_ASSERT(0); return TLA_FALSE;
	}
	return TLA_TRUE;
}

TlaBOOL TLAString_initFromFile(STTLAString* obj, FILE* file){
	TlaSI32 verfiStart, verifEnd, includesUnusedBuffer = 0; TlaUI32 bytesToLoad = 0;
	//
	if(obj == NULL || file == NULL){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&verfiStart, sizeof(verfiStart), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	} else if(verfiStart != TLA_FILE_VERIF_START){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&obj->lenght, sizeof(obj->lenght), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(fread(&includesUnusedBuffer, sizeof(includesUnusedBuffer), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(includesUnusedBuffer){
		if(fread(&obj->_buffSize, sizeof(obj->_buffSize), 1, file) != 1){
			TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
		}
		bytesToLoad = obj->_buffSize;
	} else {
		bytesToLoad = obj->lenght + 1;
	}
	if(fread(&obj->_buffGrowth, sizeof(obj->_buffGrowth), 1, file) != 1){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(obj->_buffGrowth <= 0){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	TLA_ASSERT(bytesToLoad > 0)
	obj->_buffSize = bytesToLoad;
	TLA_MALLOC(obj->str, char, obj->_buffSize);
	if(obj->str == NULL){
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	if(bytesToLoad > 0){
		if(fread(obj->str, sizeof(char), bytesToLoad, file) != bytesToLoad){
			TLA_FREE(obj->str);
			TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
		}
	}
	obj->str[obj->lenght] = '\0';
	if(fread(&verifEnd, sizeof(verifEnd), 1, file) != 1){
		TLA_FREE(obj->str);
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	} else if(verifEnd != TLA_FILE_VERIF_END){
		TLA_FREE(obj->str);
		TLA_ASSERT_LOADFILE(0); return TLA_FALSE;
	}
	return TLA_TRUE;
}


