#include "can_api.h"
#include "canmessage.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <curl/curl.h>

static volatile int g_run = 1;
static void on_sigint(int sig) { (void)sig; g_run = 0; }

static const CanFilter f_single = {.type = CAN_FILTER_MASK, .data.mask = {.id = 0x321, .mask = 0x7FF}};
static const CanConfig cfg 		= {.channel = 0, .bitrate = 500000, .samplePoint = 0.75f, .sjw = 1, .mode = CAN_MODE_NORMAL };

static void on_rx(const CanFrame* f, void* user) {
	(void)user;
	if(f->id == 0x321) {
		int index = f->data[2];
		CanFrame response = *f;
		response.id = BCAN_ID_DCU_SEAT_ORDER;
		for(int i = 0; i < 4; ++i) {
			response.data[i] = (index % 4) == i ? 0x00 : 0xFF;
		}
		if(can_send("can0", response, 0) == CAN_OK) {
			printf("response Sended : id=%3x data= %2x %2x %2x %2x\n", response.id, response.data[0], response.data[1], response.data[2], response.data[3]);
		}
	}
}
int subID = 0;
void can_set() {
	if(can_init(CAN_DEVICE_LINUX) != CAN_OK) {
		printf("can_init failed\n");
		return;
	}
	else printf("can_init completed\n");
	

	if(can_open("can0", cfg) != CAN_OK) {
		printf("can_open failed\n");
		return;
	}
	else printf("can_open completed\n");
	
	
	if(can_subscribe("can0", &subID, f_single, on_rx, NULL) != CAN_OK) {
		printf("can_subscibe failed\n");
		return;
	}
	else printf("can_subscribe completed\n");
	
	printf("CAN Started. Press Ctrl+C to stop.\n");
}

const char *url = "https://h5wg6h5iqo4h34tp6xkjkvohwe0fggtw.lambda-url.ap-northeast-2.on.aws/";
static size_t write_stdout_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)userdata;  // unused
    size_t total = size * nmemb;
    if (total > 0) {
        size_t written = fwrite(ptr, 1, total, stdout);
        fflush(stdout);
        return written;
    }
    return 0;
}

int main(int argc, char *argv[])
{
	signal(SIGINT, on_sigint);
	
	can_set();
	
	CURLcode global_init = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (global_init != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(global_init));
        return 0;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl_easy_init() failed\n");
        curl_global_cleanup();
        return 0;
    }

    // Reuse the easy handle for efficiency
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_stdout_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.x just-web-post");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	while(g_run) {
		CURLcode res = curl_easy_perform(curl);
			if (res != CURLE_OK) {
				fprintf(stderr, "Error: %s\n", curl_easy_strerror(res));
			}
			// Ensure each response ends with a newline for readability if not already
			printf("\n");
			fflush(stdout);
		sleep(1);
	}
	
	can_unsubscribe("can0", subID);
	can_close("can0");
	can_dispose();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}
