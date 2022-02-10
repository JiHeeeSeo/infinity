/**
 * Examples - Read/Write/Send Operations
 *
 * (c) 2018 Claude Barthels, ETH Zurich
 * Contact: claudeb@inf.ethz.ch
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <cassert>

#include <infinity/core/Context.h>
#include <infinity/queues/QueuePairFactory.h>
#include <infinity/queues/QueuePair.h>
#include <infinity/memory/Buffer.h>
#include <infinity/memory/RegionToken.h>
#include <infinity/requests/RequestToken.h>

#define PORT_NUMBER 33333
#define SERVER_IP "192.168.0.105"

// Usage: ./progam -s for server and ./program for client component
int main(int argc, char **argv) {

	bool isServer = false;

	while (argc > 1) {
		if (argv[1][0] == '-') {
			switch (argv[1][1]) {

				case 's': {
					isServer = true;
					break;
				}

			}
		}
		++argv;
		--argc;
	}

	// Create mew Context
	infinity::core::Context *context = new infinity::core::Context();
	// Create a queue pair
	infinity::queues::QueuePairFactory *qpFactory = new  infinity::queues::QueuePairFactory(context);
	infinity::queues::QueuePair *qp;

	if(isServer) {

		usleep(2000000);
		printf("Creating buffers to read from and write to\n");
		// Create and register a buffer with the network
		infinity::memory::Buffer *bufferToReadWrite = new infinity::memory::Buffer(context, 128 * sizeof(char));
		// Get information from a remote buffer
		infinity::memory::RegionToken *bufferToken = bufferToReadWrite->createRegionToken();

		usleep(2000000);
		printf("Creating buffers to receive a message\n");
		infinity::memory::Buffer *bufferToReceive = new infinity::memory::Buffer(context, 128 * sizeof(char));
		context->postReceiveBuffer(bufferToReceive);

		usleep(2000000);
		printf("Setting up connection (blocking)\n");
		qpFactory->bindToPort(PORT_NUMBER);
		qp = qpFactory->acceptIncomingConnection(bufferToken, sizeof(infinity::memory::RegionToken));

		usleep(2000000);
		printf("Waiting for message (blocking)\n");
		infinity::core::receive_element_t receiveElement;
		while(!context->receive(&receiveElement));

		usleep(2000000);
		printf("Message received\n");
		delete bufferToReadWrite;
		delete bufferToReceive;

	} else {
		usleep(2000000);
		printf("Connecting to remote node\n");
		qp = qpFactory->connectToRemoteHost(SERVER_IP, PORT_NUMBER);
		// Get information from a remote buffer
		infinity::memory::RegionToken *remoteBufferToken = (infinity::memory::RegionToken *) qp->getUserData();


		usleep(2000000);
		printf("Creating buffers\n");
		// Create and register a buffer with the network
		infinity::memory::Buffer *buffer1Sided = new infinity::memory::Buffer(context, 128 * sizeof(char));
		infinity::memory::Buffer *buffer2Sided = new infinity::memory::Buffer(context, 128 * sizeof(char));

		usleep(2000000);
		printf("Reading content from remote buffer\n");
		// Read (one-sided) from a remote buffer and wait for completion
		infinity::requests::RequestToken requestToken(context);
		qp->read(buffer1Sided, remoteBufferToken, &requestToken);
		requestToken.waitUntilCompleted();

		usleep(2000000);
		printf("Writing content to remote buffer\n");
		// Write (one-sided) content of a local buffer to a remote buffer and wait for completion
		qp->write(buffer1Sided, remoteBufferToken, &requestToken);
		requestToken.waitUntilCompleted();

		usleep(2000000);
		printf("Sending message to remote host\n");
		// Send (two-sided) content of a local buffer over the queue pair and wait for completion
		qp->send(buffer2Sided, &requestToken);
		requestToken.waitUntilCompleted();

		// close connetion
		delete buffer1Sided;
		delete buffer2Sided;

	}

	delete qp;
	delete qpFactory;
	delete context;

	return 0;

}
