/*****************************************************************************
 * @file  gl_srp_utils.c
 * @brief Provide srp related functions.
 *******************************************************************************
 Copyright 2022 GL-iNet. https://www.gl-inet.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ******************************************************************************/

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/srp_client_buffers.h>

#include "gl_srp_utils.h"
#include "gl_ot_api.h"

LOG_MODULE_REGISTER(srp_utils, CONFIG_GL_SRP_UTILS_LOG_LEVEL);

srp_srv_reg_cb srp_util_srv_reg_cb = NULL;

static void CopyString(char *aDest, uint16_t aDestSize, const char *aSource)
{
	size_t len = strlen(aSource);

	memcpy(aDest, aSource, len + 1);
}

int srp_utils_set_host_name(char *host_name)
{
	struct otInstance *ot = openthread_get_default_instance();
	char *buffer = NULL;
	uint16_t size;
	uint16_t len = strlen(host_name);

	buffer = otSrpClientBuffersGetHostNameString(ot, &size);

	if (otSrpClientSetHostName(ot, host_name) != OT_ERROR_NONE) {
		LOG_ERR("otSrpClientSetHostName failed.");
		return -1;
	}

	memcpy(buffer, host_name, len + 1);

	if (otSrpClientSetHostName(ot, buffer) != OT_ERROR_NONE) {
		LOG_ERR("otSrpClientSetHostName failed.");
		return -1;
	}

	LOG_INF("SRP Client Host Name: %s", host_name);

	return 0;
}

int srp_utils_set_host_address(char *host_address)
{
	struct otInstance *ot = openthread_get_default_instance();
	uint8_t arrayLength;
	otIp6Address address;
	otIp6Address *hostAddressArray;

	otIp6AddressFromString(host_address, &address);
	hostAddressArray = otSrpClientBuffersGetHostAddressesArray(ot, &arrayLength);

	if (otSrpClientSetHostAddresses(ot, &address, 1) != OT_ERROR_NONE) {
		LOG_ERR("otSrpClientSetHostAddresses failed.");
		return -1;
	}

	memcpy(hostAddressArray, &address, 1 * sizeof(hostAddressArray[0]));

	if (otSrpClientSetHostAddresses(ot, hostAddressArray, 1) != OT_ERROR_NONE) {
		LOG_ERR("otSrpClientSetHostAddresses failed.");
		return -1;
	}

	LOG_INF("SRP Client Host Address: %s", host_address);

	return 0;
}

int srp_utils_add_service(char *instance_name, char *service_name, uint16_t port)
{
	struct otInstance *ot = openthread_get_default_instance();
	otSrpClientBuffersServiceEntry *entry = NULL;
	char *str;
	char *label;
	uint16_t size;

	entry = otSrpClientBuffersAllocateService(ot);
	if (entry == NULL) {
		LOG_ERR("otSrpClientBuffersAllocateService failed.");
		return -1;
	}
	entry->mService.mPort = port;
	entry->mService.mPriority = 0;
	entry->mService.mWeight = 0;
	// entry->mService.mNumTxtEntries = 0;
	str = otSrpClientBuffersGetServiceEntryInstanceNameString(entry, &size);
	CopyString(str, size, instance_name);
	str = otSrpClientBuffersGetServiceEntryServiceNameString(entry, &size);
	CopyString(str, size, service_name);

	label = strchr(str, ',');
	if (label != NULL) {
		uint16_t arrayLength;
		const char **subTypeLabels =
			otSrpClientBuffersGetSubTypeLabelsArray(entry, &arrayLength);

		// Leave the last array element as `nullptr` to indicate end of array.
		for (uint16_t index = 0; index + 1 < arrayLength; index++) {
			*label++ = '\0';
			subTypeLabels[index] = label;

			label = strchr(label, ',');

			if (label == NULL) {
				break;
			}
		}
	}

	uint8_t *txtBuffer = otSrpClientBuffersGetServiceEntryTxtBuffer(entry, &size);
#if 1
	entry->mTxtEntry.mKey = NULL;
	
	size_t offsetPoint = 0;
	char tmpStr[64] = {0};

	/* Format: Len Value Len Value ...*/
	snprintf(tmpStr, 64, "eui64=%s", ot_get_eui64());
	txtBuffer[offsetPoint++] = strlen(tmpStr);
	snprintf((char*)&txtBuffer[offsetPoint], size - offsetPoint, "%s", tmpStr);
	entry->mTxtEntry.mValueLength = strlen((char*)txtBuffer);
	offsetPoint+=strlen(tmpStr);

	snprintf(tmpStr, 64, "sw=%s", CONFIG_SW_VERSION);
	txtBuffer[offsetPoint++] = strlen(tmpStr);
	snprintf((char*)&txtBuffer[offsetPoint], size - offsetPoint, "%s", tmpStr);
	entry->mTxtEntry.mValueLength = strlen((char*)txtBuffer);
	offsetPoint+=strlen(tmpStr);
#else
	entry->mTxtEntry.mKey = "eui64";
	entry->mTxtEntry.mValueLength = snprintf((char *)txtBuffer, size, "%s", ot_get_eui64());
#endif
	// LOG_INF("SRP Client TXT[%d]: %s", entry->mTxtEntry.mValueLength, (char*)txtBuffer);
	// LOG_INF("SRP Client mNumTxtEntries: %d", entry->mService.mNumTxtEntries);

	otSrpClientAddService(ot, &entry->mService);

	LOG_INF("SRP Client Instance Name: %s", instance_name);
	LOG_INF("SRP Client Service Name,Port: %s,%d", service_name, port);

	return 0;
}

typedef enum{
	SRP_SERV_RM_INIT = 0,
	SRP_SERV_RM_START = 1,
	SRP_SERV_RM_SUCCESS = 2,
	SRP_SERV_RM_FAILED = 3,
}srp_serv_rm_status;
static srp_serv_rm_status srp_serv_status = SRP_SERV_RM_INIT;

void srp_cb(otError aError, const otSrpClientHostInfo * aHostInfo,
             const otSrpClientService * aServices, const otSrpClientService * aRemovedServices,
             void * aContext)
{
	const char* errorStr = NULL;

	switch(aError)
	{
		case OT_ERROR_NONE:
		{
			LOG_INF("SRP update succeeded");
			if(aHostInfo)
			{
            	if (aHostInfo->mState == OT_SRP_CLIENT_ITEM_STATE_REMOVED)
				{
					LOG_INF("SRP remove services succeeded");
					srp_serv_status = SRP_SERV_RM_SUCCESS;
				}else if(aHostInfo->mState == OT_SRP_CLIENT_ITEM_STATE_REGISTERED){
					LOG_INF("SRP services registered");
					if(srp_util_srv_reg_cb)
					{
						srp_util_srv_reg_cb();
					}
				}else{
					LOG_INF("SRP state: %d", aHostInfo->mState);
				}
			}
			break;
		}
		case OT_ERROR_PARSE:
			errorStr = "parsing operation failed";
			break;
		case OT_ERROR_NOT_FOUND:
			errorStr = "domain name or RRset does not exist";
			break;
		case OT_ERROR_NOT_IMPLEMENTED:
			errorStr = "server does not support query type";
			break;
		case OT_ERROR_SECURITY:
			errorStr = "operation refused for security reasons";
			break;
		case OT_ERROR_DUPLICATED:
			errorStr = "domain name or RRset is duplicated";
			break;
		case OT_ERROR_RESPONSE_TIMEOUT:
			errorStr = "timed out waiting on server response";
			break;
		case OT_ERROR_INVALID_ARGS:
			errorStr = "invalid service structure detected";
			break;
		case OT_ERROR_NO_BUFS:
			errorStr = "insufficient buffer to handle message";
			break;
		case OT_ERROR_FAILED:
			errorStr = "internal server error";
			break;
		default:
			errorStr = "unknown error";
			break;
    }

    if (errorStr != NULL)
    {
        LOG_ERR("SRP update error: %s", errorStr);
		srp_serv_status = SRP_SERV_RM_FAILED;
    }
		
	return;
}


void srp_utils_autostart(srp_srv_reg_cb cb)
{
	if(cb != NULL)
	{
		srp_util_srv_reg_cb = cb;
	}

	struct otInstance *ot = openthread_get_default_instance();

	otSrpClientSetCallback(ot, srp_cb, NULL);

	otSrpClientEnableAutoStartMode(ot, NULL, NULL);
}

int srp_utils_host_remove(void)
{
	struct otInstance *ot = openthread_get_default_instance();

	otError err = otSrpClientRemoveHostAndServices(ot, false, true);
	if(err != OT_ERROR_NONE)
	{
		LOG_ERR("SRP Client Remove Services Failed: %d", err);
		return err;
	}
	LOG_INF("SRP remove services start ... ");

	srp_serv_status = SRP_SERV_RM_START;
	int time = 0;
	while((srp_serv_status == SRP_SERV_RM_START) && (time++ < 15))
	{
		k_sleep(K_MSEC(200));
	}

	if(srp_serv_status == SRP_SERV_RM_SUCCESS)
	{
		return 0;
	}else{
		srp_serv_status = SRP_SERV_RM_INIT;
		return -1;
	}
}