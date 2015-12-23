#include "StreamSerializer.hpp"
#include "PacketHeader.hpp"
#include "helpers.hpp"

#include <stdio.h>

using namespace ecg;

StreamSerializer::StreamSerializer(BitFifo **streams, int numStreams):
	streams(streams),
	numStreams(numStreams)
{
	
}

bool StreamSerializer::makePacket(char* buffer, int totalSize) {
	int totalAvailable = 0;
	int toStoreTotal = 0;
	int headerSize = sizeof(PacketHeader);
	int dataSize = totalSize - 2*numStreams - headerSize;
	
	//printf("dataSize: %d\n", dataSize);
	
	PacketHeader* header = (PacketHeader*) buffer;
	BigEndianInt16* sizes = (BigEndianInt16*) (buffer + headerSize);
	char* data = buffer + headerSize + 2*numStreams;
	
	for(int i = 0; i < numStreams; ++i)
		totalAvailable += streams[i]->getAvailableBytes();

	//printf("totalAvailable: %d\n", totalAvailable);

	if(totalAvailable == 0)
		return false;

	for(int i = 0; i < numStreams; ++i) {
		int toStore = dataSize * streams[i]->getAvailableBytes() / totalAvailable;
		sizes[i] = toStore;
		toStoreTotal += toStore;
	}

	
	if(toStoreTotal > totalAvailable)
		return false;
	
	int correction = dataSize - toStoreTotal;
	
	for(int i = 0; i < numStreams; ++i) {
		if(correction < 0 && sizes[i] > 0) {
			sizes[i] = sizes[i] - 1;
			correction++;
		}
		if(correction > 0 && sizes[i] < streams[i]->getAvailableBytes()) {
			sizes[i] = sizes[i] + 1;
			correction--;
		}
		//printf("popping %d bytes from stream %d\n", (int)sizes[i], i);		
		streams[i]->popBytes(data, (int)sizes[i]);
		//printf("%d bytes remained in stream %d\n", streams[i]->getAvailableBytes(), i);	
		data += sizes[i];
	}
	
	header->packetType = PacketHeader::STREAM_PACKET;
	
	return true;
}

bool StreamSerializer::readPacket(const char* buffer, int totalSize) {
	int totalAvailable = 0;
	int headerSize = sizeof(PacketHeader);
	int dataSize = totalSize - 2*numStreams - headerSize;
	
	PacketHeader* header = (PacketHeader*) buffer;
	BigEndianInt16* sizes = (BigEndianInt16*) (buffer + headerSize);
	const char* data = buffer + headerSize + 2*numStreams;
	
	if(header->packetType != PacketHeader::STREAM_PACKET)
		return false;
	
	for(int i = 0; i < numStreams; ++i) {
		streams[i]->pushBytes(data, sizes[i]);
		data += sizes[i];
	}
	
	return true;	
}
