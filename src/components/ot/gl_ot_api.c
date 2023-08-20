/*****************************************************************************
 * @file  gl_ot_api.c
 * @brief Provide openthread api related functions
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
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/net/openthread.h>
#include <openthread/thread.h>
#include <openthread/joiner.h>
#include <platform-zephyr.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <version.h>


#include "gl_ot_api.h"

LOG_MODULE_REGISTER(gl_ot_api, CONFIG_GL_OT_API_LOG_LEVEL);

char *ot_get_eui64(void)
{
	static char eui64[32] = { 0 };
	char *p = eui64;

	if (strlen(eui64) == 0) {
		struct otInstance *instance = openthread_get_default_instance();

		otExtAddress t_eui64;
		otLinkGetFactoryAssignedIeeeEui64(instance, &t_eui64);
		sprintf(eui64, "%02x%02x%02x%02x%02x%02x%02x%02x", t_eui64.m8[0], t_eui64.m8[1],
			t_eui64.m8[2], t_eui64.m8[3], t_eui64.m8[4], t_eui64.m8[5], t_eui64.m8[6],
			t_eui64.m8[7]);
	}

	return p;
}

char *ot_get_extaddr(void)
{
	static char extaddr[32] = { 0 };

	memset(extaddr, 0, sizeof(extaddr));

	struct otInstance *instance = openthread_get_default_instance();
	const otExtAddress *t_extaddr = otLinkGetExtendedAddress(instance);
	sprintf(extaddr, "%02x%02x%02x%02x%02x%02x%02x%02x", t_extaddr->m8[0], t_extaddr->m8[1],
		t_extaddr->m8[2], t_extaddr->m8[3], t_extaddr->m8[4], t_extaddr->m8[5],
		t_extaddr->m8[6], t_extaddr->m8[7]);

	return extaddr;
}

int ot_get_thread_version(void)
{
	return otThreadGetVersion();
}

const char *ot_get_version(void)
{
	return otGetVersionString();
}

int ot_get_rloc16(void)
{
	struct otInstance *instance = openthread_get_default_instance();
	return otThreadGetRloc16(instance);
}

char *ot_get_mleid(void)
{
	static char address[OT_IP6_ADDRESS_STRING_SIZE];

	memset(address, 0, sizeof(address));

	struct otInstance *instance = openthread_get_default_instance();
	const otIp6Address *mleid = otThreadGetMeshLocalEid(instance);

	otIp6AddressToString(mleid, address, sizeof(address));

	return address;
}

char *ot_get_slaac_addr(void)
{
	static char address[OT_IP6_ADDRESS_STRING_SIZE];

	memset(address, 0, sizeof(address));

	struct otInstance *instance = openthread_get_default_instance();

	const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(instance);
	for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext) {
		if (addr->mAddressOrigin == OT_ADDRESS_ORIGIN_SLAAC) {
			otIp6AddressToString(&addr->mAddress, address, sizeof(address));
			break;
		}
	}

	return address;
}

void ot_factoryreset(void)
{
	struct openthread_context *context = openthread_get_default_context();
	openthread_api_mutex_lock(context);
	openthread_stop(context);
	otInstanceFactoryReset(context->instance);
	openthread_api_mutex_unlock(context);
}

int ot_start(void)
{
	struct openthread_context *context = openthread_get_default_context();
	otLinkSetPanId(context->instance, 0xffff);
	return openthread_start(context);
}

void ot_print_network_info(void)
{
	struct otInstance *instance = openthread_get_default_instance();

	char eui64[32] = { 0 };
	otExtAddress extAddress1;
	otLinkGetFactoryAssignedIeeeEui64(instance, &extAddress1);
	sprintf(eui64, "%02x%02x%02x%02x%02x%02x%02x%02x", extAddress1.m8[0], extAddress1.m8[1],
		extAddress1.m8[2], extAddress1.m8[3], extAddress1.m8[4], extAddress1.m8[5],
		extAddress1.m8[6], extAddress1.m8[7]);

	char extaddr[32] = { 0 };
	otExtAddress *extAddress2 = otLinkGetExtendedAddress(instance);
	sprintf(extaddr, "%02x%02x%02x%02x%02x%02x%02x%02x", extAddress2->m8[0], extAddress2->m8[1],
		extAddress2->m8[2], extAddress2->m8[3], extAddress2->m8[4], extAddress2->m8[5],
		extAddress2->m8[6], extAddress2->m8[7]);

	char ipaddr[512] = { 0 };
	strcat(ipaddr, "[");
	const otNetifAddress *unicastAddrs = otIp6GetUnicastAddresses(instance);
	for (const otNetifAddress *addr = unicastAddrs; addr; addr = addr->mNext) {
		char string[OT_IP6_ADDRESS_STRING_SIZE] = { 0 };
		otIp6AddressToString(&addr->mAddress, string, sizeof(string));
		strcat(ipaddr, string);
		strcat(ipaddr, ",");
	}
	strcat(ipaddr, "]");

	otRouterInfo parentInfo;
	otThreadGetParentInfo(instance, &parentInfo);

	LOG_INF("txpower=%ddBm, networkname=%s, channel=%d, panid=0x%04x, eui64=%s, extaddr=%s, ipaddr=%s, parent.rloc16=0x%04x",
		ot_get_txpower(), otThreadGetNetworkName(instance), otLinkGetChannel(instance),
		otLinkGetPanId(instance), eui64, extaddr, ipaddr, parentInfo.mRloc16);
}

char *ot_get_mode(void)
{
	otLinkModeConfig linkMode;
	static char mode[5] = { 0 };
	struct otInstance *instance = openthread_get_default_instance();

	memset(&linkMode, 0, sizeof(otLinkModeConfig));

	linkMode = otThreadGetLinkMode(instance);

	if (linkMode.mRxOnWhenIdle) {
		strcat(mode, "r");
	}

	if (linkMode.mDeviceType) {
		strcat(mode, "d");
	}

	if (linkMode.mNetworkData) {
		strcat(mode, "n");
	}

	return mode;
}

int ot_get_txpower(void)
{
	struct otInstance *instance = openthread_get_default_instance();
	int8_t power = 0;

	otPlatRadioGetTransmitPower(instance, &power);

	return power;
}

static void ot_joiner_start_handler(otError error, void *context)
{
	struct openthread_context *ot_context = context;

	switch (error) {
	case OT_ERROR_NONE:
		LOG_INF("Join success");
		otThreadSetEnabled(ot_context->instance, true);
		break;
	default:
		LOG_INF("Join failed [%d]", error);
		break;
	}
}

void start_joiner(void)
{
	struct openthread_context *context = openthread_get_default_context();

	openthread_api_mutex_lock(context);

	otJoinerStart(context->instance, CONFIG_OPENTHREAD_JOINER_PSKD, NULL,
						"Zephyr", CONFIG_OPENTHREAD_PLATFORM_INFO,
						KERNEL_VERSION_STRING, NULL,
						&ot_joiner_start_handler, context);

	openthread_api_mutex_unlock(context);

}