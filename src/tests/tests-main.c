//
//  main.cpp
//  tests-tlali-osm-osx
//
//  Created by Marcos Ortega on 7/3/16.
//
//

#include "tlali-osm.h"
#include "../../src/tlali-osm.c"
#include <stdio.h>	//FILE*
#include <time.h>	//clock_t, clock()

#ifdef _WIN32
#	define CPU_CICLES_TYPE				TlaUI64
#	define CPU_CICLES_CUR_THREAD(DST)	{ LARGE_INTEGER t; QueryPerformanceCounter(&t); DST = t.QuadPart; } //ADVERTENCIA: el uso de clock() relantiza al hilo/proceso. Es mejor no utilizar llamadas a clock o limitarlas en ambitos generales.
#	define CPU_CICLES_PER_SEC(DST)		{ LARGE_INTEGER t; QueryPerformanceFrequency(&t); DST = t.QuadPart; }
#else
#	define CPU_CICLES_TYPE				clock_t
#	define CPU_CICLES_CUR_THREAD(DST)	DST = clock(); //ADVERTENCIA: el uso de clock() relantiza al hilo/proceso. Es mejor no utilizar llamadas a clock o limitarlas en ambitos generales.
#	define CPU_CICLES_PER_SEC(DST)		DST = CLOCKS_PER_SEC;
#endif

TlaBOOL testLoadOsmFromFileXml(STTlaOsm* osmData, const char* strPathLoadXml);
TlaBOOL testSaveAndLoadOsmFromFileBinary(STTlaOsm* osmData, STTlaOsm* loadCopy, const char* strPathSaveBin);
TlaBOOL testPrintOsmData(STTlaOsm* osmData);

//Read from file-stream function
TlaSI32 myReadFromFileFunc(void* dstBuffer, const TlaSI32 sizeOfBlock, const TlaSI32 maxBlocksToRead, void* userData){
	return (TlaSI32)fread(dstBuffer, sizeOfBlock, maxBlocksToRead, (FILE*) userData);
}

//Write to file-stream function
TlaSI32 myWriteToFileFunc(const void* srcBuffer, const TlaSI32 sizeOfBlock, const TlaSI32 maxBlocksToRead, void* userData){
	return (TlaSI32)fwrite(srcBuffer, sizeOfBlock, maxBlocksToRead, (FILE*) userData);
}


int main(int argc, const char * argv[]) {
	STTlaOsm osmFromXml;
	osmInit(&osmFromXml);
	//
	//Test: load osm data from file
	if(!testLoadOsmFromFileXml(&osmFromXml, "./rutasManagua.xml")){
		TLA_ASSERT(0);
	} else {
		//testPrintOsmData(&osmData);
		STTlaOsm osmFromBin;
		if(!testSaveAndLoadOsmFromFileBinary(&osmFromXml, &osmFromBin, "./rutasManagua.bin")){
			TLA_ASSERT(0);
		} else {
			//testPrintOsmData(&osmFromBin);
			osmRelease(&osmFromBin);
		}
	}
	//
	osmRelease(&osmFromXml);
	//
	return 0;
}

TlaBOOL testLoadOsmFromFileXml(STTlaOsm* osmData, const char* strPathLoadXml){
	TlaBOOL r = TLA_FALSE;
	//---------------------------
	//-- Loading using lib-tlali-osm
	//-- (C style, and not depending on any other library)
	//---------------------------
	{
		
		FILE* stream = fopen(strPathLoadXml, "rb");
		if(stream == NULL){
			TLA_PRINTF_ERROR("File could not be found: '%s'.\n", strPathLoadXml);
		} else {
			CPU_CICLES_TYPE timeStart, timeEnd, timeTicksPerSec;
			CPU_CICLES_CUR_THREAD(timeStart)
			if(!osmLoadFromXmlStream(osmData, myReadFromFileFunc, stream)){
				TLA_PRINTF_ERROR("File found but could not be parsed: '%s'.\n", strPathLoadXml);
			} else {
				CPU_CICLES_CUR_THREAD(timeEnd)
				CPU_CICLES_PER_SEC(timeTicksPerSec)
				TLA_PRINTF_INFO("File parsed from XML in %d ms.\n", (TlaSI32)((timeEnd - timeStart) * 1000 / timeTicksPerSec));
				r = TLA_TRUE;
			}
			fclose(stream);
		}
	}
	return r;
}

TlaBOOL testSaveAndLoadOsmFromFileBinary(STTlaOsm* osmData, STTlaOsm* loadCopy, const char* strPathSaveBin){
	TlaBOOL r = TLA_FALSE;
	//Save to data
	{
		FILE* stream = fopen(strPathSaveBin, "wb");
		if(stream == NULL){
			TLA_PRINTF_ERROR("Binary file could not be writted: '%s'.\n", strPathSaveBin);
			TLA_ASSERT(0);
		} else {
			CPU_CICLES_TYPE timeStart, timeEnd, timeTicksPerSec;
			CPU_CICLES_CUR_THREAD(timeStart)
			if(!osmSaveToBinaryStream(osmData, myWriteToFileFunc, stream)){
				TLA_ASSERT(0);
			} else {
				CPU_CICLES_CUR_THREAD(timeEnd)
				CPU_CICLES_PER_SEC(timeTicksPerSec)
				TLA_PRINTF_INFO("File saved to BIN in %d ms.\n", (TlaSI32)((timeEnd - timeStart) * 1000 / timeTicksPerSec));
				r = TLA_TRUE;
			}
			fclose(stream);
		}
	}
	//Load saved data
	if(r){
		FILE* stream = fopen(strPathSaveBin, "rb");
		r = TLA_FALSE;
		if(stream == NULL){
			TLA_PRINTF_ERROR("Binary file could not be opened: '%s'.\n", strPathSaveBin);
			TLA_ASSERT(0);
		} else {
			CPU_CICLES_TYPE timeStart, timeEnd, timeTicksPerSec;
			CPU_CICLES_CUR_THREAD(timeStart)
			if(!osmInitFromBinaryStream(loadCopy, myReadFromFileFunc, stream)){
				TLA_ASSERT(0);
			} else {
				CPU_CICLES_CUR_THREAD(timeEnd)
				CPU_CICLES_PER_SEC(timeTicksPerSec)
				TLA_PRINTF_INFO("File parsed from BIN in %d ms.\n", (TlaSI32)((timeEnd - timeStart) * 1000 / timeTicksPerSec));
				r = TLA_TRUE;
			}
			fclose(stream);
		}
	}
	//
	return r;
}

TlaBOOL testPrintOsmData(STTlaOsm* osmData){
	CPU_CICLES_TYPE timeStart, timeEnd, timeTicksPerSec;
	CPU_CICLES_CUR_THREAD(timeStart)
	//Walk nodes
	{
		STTlaNode data; TlaSI32 count = 0;
		if(osmGetNextNode(osmData, &data, NULL)){
			do{
				count++;
				TLA_PRINTF_INFO("Node #%d: id(%lld) lat(%f) lon(%f).\n", count, data.id, data.lat, data.lon);
				//Read tags
				{
					STTlaTag tag; TlaSI32 count = 0;
					if(osmGetNextNodeTag(osmData, &data, &tag, NULL)){
						do {
							count++;
							TLA_PRINTF_INFO("  Tag #%d: '%s' = '%s'.\n", count, tag.name, tag.value);
						} while(osmGetNextNodeTag(osmData, &data, &tag, &tag));
					}
				}
			} while(osmGetNextNode(osmData, &data, &data));
		}
		
	}
	//Walk ways
	{
		STTlaWay data; TlaSI32 count = 0;
		if(osmGetNextWay(osmData, &data, NULL)){
			do{
				count++;
				TLA_PRINTF_INFO("Way #%d: id(%lld).\n", count, data.id);
				//Read nodes ref
				{
					const TlaSI64* arr; TlaSI32 count;
					if(osmGetWayNodes(osmData, &data, &arr, &count)){
						TlaSI32 i;
						for(i = 0; i < count; i++){
							TLA_PRINTF_INFO("  Nd #%d: ref(%lld).\n", (i + 1), arr[i]);
							//Verify if node exists
							{
								STTlaNode node;
								if(!osmGetNodeById(osmData, &node, arr[i])){
									TLA_PRINTF_INFO("  ERROR, on way(%lld), node(%lld) doesnt exists.\n", data.id, arr[i]);
								}
							}
						}
					}
				}
				//Read tags
				{
					STTlaTag tag; TlaSI32 count = 0;
					if(osmGetNextWayTag(osmData, &data, &tag, NULL)){
						do {
							count++;
							TLA_PRINTF_INFO("  Tag #%d: '%s' = '%s'.\n", count, tag.name, tag.value);
						} while(osmGetNextWayTag(osmData, &data, &tag, &tag));
					}
				}
			} while(osmGetNextWay(osmData, &data, &data));
		}
	}
	//Walk relations
	{
		STTlaRel data; TlaSI32 count = 0;
		if(osmGetNextRel(osmData, &data, NULL)){
			do{
				count++;
				TLA_PRINTF_INFO("Relation #%d: id(%lld).\n", count, data.id);
				//Read members
				{
					STTlaRelMember member; TlaSI32 count = 0;
					if(osmGetNextRelMember(osmData, &data, &member, NULL)){
						do {
							count++;
							TLA_PRINTF_INFO("  Member #%d: type('%s') ref(%lld) role('%s').\n", count, member.type, member.ref, member.role);
							//Verify if member exists
							{
								TlaBOOL processed = TLA_FALSE;
								//Verify as node
								if(!processed) if(member.type[0] == 'n') if(member.type[1] == 'o') if(member.type[2] == 'd') if(member.type[3] == 'e') if(member.type[4] == '\0'){
									STTlaNode node;
									if(!osmGetNodeById(osmData, &node, member.ref)){
										TLA_PRINTF_INFO("  ERROR, on relation(%lld), node(%lld) doesnt exists.\n", data.id, member.ref);
									}
									processed = TLA_TRUE;
								}
								//Verify as way
								if(!processed) if(member.type[0] == 'w') if(member.type[1] == 'a') if(member.type[2] == 'y') if(member.type[3] == '\0') {
									STTlaWay way;
									if(!osmGetWayById(osmData, &way, member.ref)){
										TLA_PRINTF_INFO("  ERROR, on relation(%lld), way(%lld) doesnt exists.\n", data.id, member.ref);
									}
									processed = TLA_TRUE;
								}
							}
						} while(osmGetNextRelMember(osmData, &data, &member, &member));
					}
				}
				//Read tags
				{
					STTlaTag tag; TlaSI32 count = 0;
					if(osmGetNextRelTag(osmData, &data, &tag, NULL)){
						do {
							count++;
							TLA_PRINTF_INFO("  Tag #%d: '%s' = '%s'.\n", count, tag.name, tag.value);
						} while(osmGetNextRelTag(osmData, &data, &tag, &tag));
					}
				}
			} while(osmGetNextRel(osmData, &data, &data));
		}
	}
	CPU_CICLES_CUR_THREAD(timeEnd)
	CPU_CICLES_PER_SEC(timeTicksPerSec)
	TLA_PRINTF_INFO("End-of-stuctures-walk in %d ms.\n", (TlaSI32)((timeEnd - timeStart) * 1000 / timeTicksPerSec));
	return TLA_TRUE;
}
