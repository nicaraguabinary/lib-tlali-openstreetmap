//
//  tlali-osm.h
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

#ifndef tlali_tlali_osm_h
#define tlali_tlali_osm_h

#include <stdio.h>		//FILE*

#ifdef __cplusplus
extern "C" {
#endif
	
//---------------------------------
//---------------------------------
//-- Configurable
//---------------------------------
//---------------------------------
	
//
// You can customize tlali's memory management
// by defining this MACROS before
// "tlali-osm.c" get included or compiled.
//
//#define TLA_MALLOC(POINTER_DEST, POINTER_TYPE, SIZE_BYTES)
//#define TLA_FREE(POINTER)


//---------------------------------
//---------------------------------
//-- Data types
//---------------------------------
//---------------------------------
typedef unsigned char 		TlaBOOL;	//TlaBOOL, Unsigned 8-bit integer value
typedef unsigned char 		TlaBYTE;	//TlaBYTE, Unsigned 8-bit integer value
typedef char 				TlaSI8;		//TlaSI8, Signed 8-bit integer value
typedef	short int 			TlaSI16;	//TlaSI16, Signed 16-bit integer value
typedef	int 				TlaSI32;	//TlaSI32, Signed 32-bit integer value
typedef	long long 			TlaSI64;	//TlaSI64, Signed 64-bit integer value
typedef unsigned char 		TlaUI8;		//TlaUI8, Unsigned 8-bit integer value
typedef	unsigned short int 	TlaUI16;	//TlaUI16, Unsigned 16-bit integer value
typedef	unsigned int 		TlaUI32;	//TlaUI32, Unsigned 32-bit integer value
typedef	unsigned long long	TlaUI64;	//TlaUI64[n], Unsigned 64-bit array—n is the number of array elements
typedef float				TlaFLOAT;	//float
typedef double				TlaDOUBLE;	//double
	
#define TLA_FALSE			0
#define TLA_TRUE			1

//tlali-osm database
typedef struct STTlaOsm_ {
	void* opaqueData;
} STTlaOsm;

//Tag
typedef struct STTlaTag_ {
	const char*	name;
	const char*	value;
	TlaSI32		opaquePtr;
} STTlaTag;

//Node
typedef struct STTlaNode_ {
	TlaSI64		id;
	TlaDOUBLE	lat;
	TlaDOUBLE	lon;
	TlaSI32		opaquePtr;
} STTlaNode;

//Way
typedef struct STTlaWay_ {
	TlaSI64		id;
	TlaSI32		opaquePtr;
} STTlaWay;

//Relation
typedef struct STTlaRel_ {
	TlaSI64		id;
	TlaSI32		opaquePtr;
} STTlaRel;

//Relation's member
typedef struct STTlaRelMember_ {
	const char*	type;
	//
	TlaSI64		ref;
	const char*	role;
	TlaSI32		opaquePtr;
} STTlaRelMember;

//Factory
void		osmInit(STTlaOsm* obj);
void		osmRelease(STTlaOsm* obj);

//Read (secuential)
TlaBOOL		osmGetNextNode(STTlaOsm* obj, STTlaNode* dst, const STTlaNode* afterThis);
TlaBOOL		osmGetNextNodeTag(STTlaOsm* obj, const STTlaNode* node, STTlaTag* dst, const STTlaTag* afterThis);
TlaBOOL		osmGetNextWay(STTlaOsm* obj, STTlaWay* dst, const STTlaWay* afterThis);
TlaBOOL		osmGetWayNodes(STTlaOsm* obj, const STTlaWay* way, const TlaSI64** dstArr, TlaSI32* dstSize);
TlaBOOL		osmGetNextWayTag(STTlaOsm* obj, const STTlaWay* way, STTlaTag* dst, const STTlaTag* afterThis);
TlaBOOL		osmGetNextRel(STTlaOsm* obj, STTlaRel* dst, const STTlaRel* afterThis);
TlaBOOL		osmGetNextRelMember(STTlaOsm* obj, const STTlaRel* rel, STTlaRelMember* dst, const STTlaRelMember* afterThis);
TlaBOOL		osmGetNextRelTag(STTlaOsm* obj, const STTlaRel* rel, STTlaTag* dst, const STTlaTag* afterThis);

//Read (by id)
TlaBOOL		osmGetNodeById(STTlaOsm* obj, STTlaNode* dst, const TlaSI64 refId);
TlaBOOL		osmGetWayById(STTlaOsm* obj, STTlaWay* dst, const TlaSI64 refId);
TlaBOOL		osmGetRelById(STTlaOsm* obj, STTlaRel* dst, const TlaSI64 refId);

//Load
TlaBOOL		osmLoadFromFileXml(STTlaOsm* obj, FILE* fileStream);
TlaBOOL		osmInitFromFileBinary(STTlaOsm* obj, FILE* fileStream);
	
//Save
TlaBOOL		osmSaveToFileAsBinary(STTlaOsm* obj, FILE* fileStream);
	
#ifdef __cplusplus
} //extern "C" {
#endif

#endif
