/**
 * @file main.cpp
 *
 */
/* Copyright (C) 2017-2018 by Arjan van Vught mailto:info@raspberrypi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "hardwarelinux.h"
#include "networklinux.h"

#include "osc.h"
#include "oscsend.h"
#include "oscmessage.h"
#include "oscparams.h"

#include "software_version.h"

int main(int argc, char **argv) {
	HardwareLinux hw;
	NetworkLinux nw;
	uint8_t nTextLength;
	OSCParams oscparms;
	uint8_t buffer[1600];
	uint32_t remote_ip;
	uint16_t incoming_port, outgoing_port, remote_port;

	if (argc < 2) {
		printf("Usage: %s ip_address|interface_name\n", argv[0]);
		return -1;
	}

	if (oscparms.Load()) {
		oscparms.Dump();
	}

	incoming_port = oscparms.GetIncomingPort();
	outgoing_port = oscparms.GetOutgoingPort();

	printf("[V%s] %s %s Compiled on %s at %s\n", SOFTWARE_VERSION, hw.GetSysName(nTextLength), hw.GetVersion(nTextLength), __DATE__, __TIME__);
	printf("OSC, Incoming port: %d, Outgoing port: %d\n", incoming_port, outgoing_port);

	if (nw.Init(argv[1]) < 0) {
		fprintf(stderr, "Not able to start the network\n");
		return -1;
	}

	nw.Begin(incoming_port);

	nw.Print();

	for (;;) {
		int bytes_received = nw.RecvFrom(buffer, sizeof buffer, &remote_ip, &remote_port);

		if (bytes_received > 0) {
			struct timeval tv;
			gettimeofday(&tv, NULL);
			struct tm ltm = *localtime(&tv.tv_sec);

			printf("%.2d-%.2d-%.4d %.2d:%.2d:%.2d.%.6ld ", ltm.tm_mday, ltm.tm_mon + 1, ltm.tm_year + 1900, ltm.tm_hour, ltm.tm_min, ltm.tm_sec, tv.tv_usec);

			if (OSC::isMatch((const char*) buffer, "/ping")) {
				OSCSend MsgSend(remote_ip, outgoing_port, "/pong", 0);
				printf("ping->pong\n");
			} else {
				OSCMessage Msg((char *) buffer, bytes_received);
				printf("path : %s\n", OSC::GetPath((char*) buffer, bytes_received));
				int argc = Msg.GetArgc();

				for (int i = 0; i < argc; i++) {
					int type = (int) Msg.GetType(i);

					printf("\targ %d, ", i);

					switch (type) {
					case OSC_INT32: {
						printf("int, %d", Msg.GetInt(i));
						break;
					}
					case OSC_FLOAT: {
						printf("float, %f", Msg.GetFloat(i));
						break;
					}
					case OSC_STRING: {
						printf("string, %s", Msg.GetString(i));
						break;
					}
					case OSC_BLOB: {
						OSCBlob blob = Msg.GetBlob(i);

						int size = (int) blob.GetDataSize();
						printf("blob, size %d, [", (int) size);

						for (int j = 0; j < size && j < 16; j++) {
							printf("%02x", blob.GetByte(j));
							if (j + 1 < size) {
								printf(" ");
							}
						}

						printf("]");
						break;
					}
					}

					const int result = Msg.GetResult();

					if (result) {
						printf(", result = %d\n", result);
					} else {
						puts("");
					}
				}
			}

			if (OSC::isMatch((const char*) buffer, "/2")) {
				unsigned char nLength;
				OSCSend MsgSendModel(remote_ip, outgoing_port, "/info/model", "s", Hardware::Get()->GetBoardName(nLength));
				OSCSend MsgSendInfo(remote_ip, outgoing_port, "/info/os", "s", Hardware::Get()->GetSysName(nLength));
				OSCSend MsgSendSoc(remote_ip, outgoing_port, "/info/soc", "s", Hardware::Get()->GetSocName(nLength));
			}
		}
	}

	return 0;
}
