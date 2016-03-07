//
//  main.cpp
//  tests-tlali-osm-osx
//
//  Created by Marcos Ortega on 7/3/16.
//
//

#include "tlali-osm.h"
#include "../../src/tlali-osm.c"

int testLoadOsmFromFileXml(const char* strPath);

int main(int argc, const char * argv[]) {
	//Test: load osm data from file
	testLoadOsmFromFileXml("./rutasManagua.xml");
	//
	return 0;
}

int testLoadOsmFromFileXml(const char* strPath){
	int r = 0;
	//---------------------------
	//-- Loading using lib-tlali-osm
	//-- (C style, and not depending on any other library)
	//---------------------------
	{
		FILE* stream = fopen(strPath, "rb");
		if(stream == NULL){
			TLA_PRINTF_ERROR("File could not be found: '%s'.\n", strPath);
		} else {
			STTlaOsm osmData;
			osmInit(&osmData);
			if(!osmLoadFromFileXml(&osmData, stream)){
				TLA_PRINTF_ERROR("File found but could not be parsed: '%s'.\n", strPath);
			} else {
				TLA_PRINTF_INFO("File parsed from XML.\n");
				//Walk nodes
				{
					STTlaNode data; TlaSI32 count = 0;
					if(osmGetNextNode(&osmData, &data, NULL)){
						do{
							count++;
							TLA_PRINTF_INFO("Node #%d: id(%lld) lat(%f) lon(%f).\n", count, data.id, data.lat, data.lon);
							//Read tags
							{
								STTlaTag tag; TlaSI32 count = 0;
								if(osmGetNextNodeTag(&osmData, &data, &tag, NULL)){
									do {
										count++;
										TLA_PRINTF_INFO("  Tag #%d: '%s' = '%s'.\n", count, tag.name, tag.value);
									} while(osmGetNextNodeTag(&osmData, &data, &tag, &tag));
								}
							}
						} while(osmGetNextNode(&osmData, &data, &data));
					}
					
				}
				//Walk ways
				{
					STTlaWay data; TlaSI32 count = 0;
					if(osmGetNextWay(&osmData, &data, NULL)){
						do{
							count++;
							TLA_PRINTF_INFO("Way #%d: id(%lld).\n", count, data.id);
							//Read nodes ref
							{
								const TlaSI64* arr; TlaSI32 count;
								if(osmGetWayNodes(&osmData, &data, &arr, &count)){
									TlaSI32 i;
									for(i = 0; i < count; i++){
										TLA_PRINTF_INFO("  Nd #%d: ref(%lld).\n", (i + 1), arr[i]);
										//Verify if node exists
										{
											STTlaNode node;
											if(!osmGetNodeById(&osmData, &node, arr[i])){
												TLA_PRINTF_INFO("  ERROR, on way(%lld), node(%lld) doent exists.\n", data.id, arr[i]);
											}
										}
									}
								}
							}
							//Read tags
							{
								STTlaTag tag; TlaSI32 count = 0;
								if(osmGetNextWayTag(&osmData, &data, &tag, NULL)){
									do {
										count++;
										TLA_PRINTF_INFO("  Tag #%d: '%s' = '%s'.\n", count, tag.name, tag.value);
									} while(osmGetNextWayTag(&osmData, &data, &tag, &tag));
								}
							}
						} while(osmGetNextWay(&osmData, &data, &data));
					}
				}
				//Walk relations
				{
					STTlaRel data; TlaSI32 count = 0;
					if(osmGetNextRel(&osmData, &data, NULL)){
						do{
							count++;
							TLA_PRINTF_INFO("Relation #%d: id(%lld).\n", count, data.id);
							//Read members
							{
								STTlaRelMember member; TlaSI32 count = 0;
								if(osmGetNextRelMember(&osmData, &data, &member, NULL)){
									do {
										count++;
										TLA_PRINTF_INFO("  Member #%d: type('%s') ref(%lld) role('%s').\n", count, member.type, member.ref, member.role);
										//Verify if member exists
										{
											TlaBOOL processed = TLA_FALSE;
											//Verify as node
											if(!processed) if(member.type[0] == 'n') if(member.type[1] == 'o') if(member.type[2] == 'd') if(member.type[3] == 'e') if(member.type[4] == '\0'){
												STTlaNode node;
												if(!osmGetNodeById(&osmData, &node, member.ref)){
													TLA_PRINTF_INFO("  ERROR, on relation(%lld), node(%lld) doent exists.\n", data.id, member.ref);
												}
												processed = TLA_TRUE;
											}
											//Verify as way
											if(!processed) if(member.type[0] == 'w') if(member.type[1] == 'a') if(member.type[2] == 'y') if(member.type[3] == '\0') {
												STTlaWay way;
												if(!osmGetWayById(&osmData, &way, member.ref)){
													TLA_PRINTF_INFO("  ERROR, on relation(%lld), way(%lld) doent exists.\n", data.id, member.ref);
												}
												processed = TLA_TRUE;
											}
										}
									} while(osmGetNextRelMember(&osmData, &data, &member, &member));
								}
							}
							//Read tags
							{
								STTlaTag tag; TlaSI32 count = 0;
								if(osmGetNextRelTag(&osmData, &data, &tag, NULL)){
									do {
										count++;
										TLA_PRINTF_INFO("  Tag #%d: '%s' = '%s'.\n", count, tag.name, tag.value);
									} while(osmGetNextRelTag(&osmData, &data, &tag, &tag));
								}
							}
						} while(osmGetNextRel(&osmData, &data, &data));
					}
				}
				TLA_PRINTF_INFO("En of stuctures walk.\n");
				r = 1;
			}
			osmRelease(&osmData);
			fclose(stream);
		}
	}
	return r;
}
